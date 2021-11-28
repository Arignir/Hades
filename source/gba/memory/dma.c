/******************************************************************************\
**
**  This file is part of the Hades GBA Emulator, and is made available under
**  the terms of the GNU General Public License version 2.
**
**  Copyright (C) 2021 - The Hades Authors
**
\******************************************************************************/

#include "hades.h"
#include "gba/gba.h"
#include "gba/scheduler.h"

static uint32_t src_mask[4]   = {0x07FFFFFF, 0x0FFFFFFF, 0x0FFFFFFF, 0x0FFFFFFF};
static uint32_t dst_mask[4]   = {0x07FFFFFF, 0x07FFFFFF, 0x07FFFFFF, 0x0FFFFFFF};
static uint32_t count_mask[4] = {0x3FFF,     0x3FFF,     0x3FFF,     0xFFFF};

void
mem_dma_load(
    struct dma_channel *channel,
    uint32_t channel_idx
) {
    channel->internal_src = channel->src.raw & (channel->control.unit_size ? ~3 : ~1); // TODO Investigate why the alignment is needed
    channel->internal_src &= src_mask[channel_idx];
    channel->internal_dst = channel->dst.raw & (channel->control.unit_size ? ~3 : ~1); // TODO Investigate why the alignment is needed
    channel->internal_dst &= dst_mask[channel_idx];
    channel->internal_count = channel->count.raw;
    channel->internal_count &= count_mask[channel_idx];
}

/*
** Go through all DMA channels and process them (if they are enabled).
*/
static
void
mem_do_dma_transfer(
    struct gba *gba,
    union event_data data
) {
    enum dma_timings timing;
    bool prefetch_state;
    bool dma_was_enabled;
    bool first;
    size_t i;

    /*
    ** Disable prefetchng during DMA.
    **
    ** According to Fleroviux (https://github.com/fleroviux/) this
    ** leads to better accuracy but the reasons why aren't well known yet.
    */
    prefetch_state = gba->memory.pbuffer.enabled;
    gba->memory.pbuffer.enabled = false;

    dma_was_enabled = gba->core.processing_dma;
    gba->core.processing_dma = true;
    first = !dma_was_enabled;

    timing = data.u32;
    for (i = 0; i < 4; ++i) {
        struct dma_channel *channel;
        enum access_type access;
        int32_t src_step;
        int32_t dst_step;
        int32_t unit_size;
        bool reload;

        channel = &gba->io.dma[i];

        // Skip channels that aren't enabled or that shouldn't happen at the given timing
        if (!channel->control.enable || channel->control.timing != timing) {
            continue;
        }

        // The first DMA take at least two internal cycles
        // (Supposedly to transition from CPU to DMA)
        if (first) {
            core_idle_for(gba, 2);
            first = false;
        }

        bool src_in_gamepak = (((channel->internal_src >> 24) & 0xF) >= CART_REGION_START && ((channel->internal_src >> 24) & 0xF) <= CART_REGION_END);
        bool dst_in_gamepak = (((channel->internal_dst >> 24) & 0xF) >= CART_REGION_START && ((channel->internal_dst >> 24) & 0xF) <= CART_REGION_END);

        if ((src_in_gamepak && dst_in_gamepak)) {
            core_idle_for(gba, 2);
        }

        reload = false;
        unit_size = channel->control.unit_size ? 4 : 2; // In  bytes

        switch (channel->control.dst_ctl) {
            case 0b00:      dst_step = unit_size; break;
            case 0b01:      dst_step = -unit_size; break;
            case 0b10:      dst_step = 0; break;
            case 0b11:      dst_step = unit_size; reload = true; break;
        }

        switch (channel->control.src_ctl) {
            case 0b00:      src_step = unit_size; break;
            case 0b01:      src_step = -unit_size; break;
            case 0b10:      src_step = 0; break;
            case 0b11:      src_step = 0; break;
        }

        // A count of 0 is treated as max length.
        if (channel->internal_count == 0) {
            channel->internal_count = count_mask[i] + 1;
        }

        logln(
            HS_DMA,
            "DMA transfer from 0x%08x%c to 0x%08x%c (len=%#08x, unit_size=%u, channel %zu)",
            channel->internal_src,
            src_step > 0 ? '+' : '-',
            channel->internal_dst,
            dst_step > 0 ? '+' : '-',
            channel->internal_count,
            unit_size,
            i
        );

        access = NON_SEQUENTIAL;
        while (channel->internal_count > 0) {
            if (unit_size == 4) {
                mem_write32(gba, channel->internal_dst, mem_read32(gba, channel->internal_src, access), access);
            } else { // unit_size == 2
                mem_write16(gba, channel->internal_dst, mem_read16(gba, channel->internal_src, access), access);
            }
            channel->internal_src += src_step;
            channel->internal_dst += dst_step;
            channel->internal_count -= 1;
            access = SEQUENTIAL;
        }

        if (channel->control.irq_end) {
            core_trigger_irq(gba, IRQ_DMA0 + i);
        }

        if (channel->control.repeat) {
            channel->internal_count = channel->count.raw;
            channel->internal_count &= count_mask[i];
            if (reload) {
                channel->internal_dst = channel->dst.raw & (channel->control.unit_size ? ~3 : ~1);
                channel->internal_dst &= dst_mask[i];
            }
        } else {
            channel->control.enable = false;
        }
    }
    gba->memory.pbuffer.enabled = prefetch_state;
    gba->core.processing_dma = dma_was_enabled;
}

void
mem_schedule_dma_transfer(
    struct gba *gba,
    enum dma_timings timing
) {
    sched_add_event(
        gba,
        NEW_FIX_EVENT_DATA(
            gba->core.cycles + 3,
            mem_do_dma_transfer,
            (union event_data){ .u32 = timing }
        )
    );
}

static
void
mem_do_dma_fifo_transfer(
    struct gba *gba,
    union event_data data
) {
    struct dma_channel *channel;
    enum access_type access;
    bool dma_was_enabled;
    int32_t src_step;

    channel = &gba->io.dma[data.u32];

    // Skip channels that aren't enabled or that shouldn't happen at the given timing
    if (!channel->control.enable || channel->control.timing != DMA_TIMING_SPECIAL) {
        return;
    }

    dma_was_enabled = gba->core.processing_dma;
    gba->core.processing_dma = true;

    // The first DMA take at least two internal cycles
    // (Supposedly to transition from CPU to DMA)
    if (!dma_was_enabled) {
        core_idle_for(gba, 2);
    }

    switch (channel->control.src_ctl) {
        case 0b00:      src_step = 4; break;
        case 0b01:      src_step = -4; break;
        case 0b10:      src_step = 0; break;
        case 0b11:      src_step = 0; break;
    }

    access = NON_SEQUENTIAL;
    while (channel->internal_count > 0) {
        mem_write32(gba, channel->internal_dst, mem_read32(gba, channel->internal_src, access), access);
        channel->internal_src += src_step;
        channel->internal_count -= 1;
        access = SEQUENTIAL;
    }

    if (channel->control.irq_end) {
        core_trigger_irq(gba, IRQ_DMA0 + data.u32);
    }

    if (channel->control.repeat) {
        channel->internal_count = 4;
    } else {
        channel->control.enable = false;
    }

    gba->core.processing_dma = dma_was_enabled;
}

void
mem_schedule_dma_fifo(
    struct gba *gba,
    uint32_t dma_channel_idx
) {
    sched_add_event(
        gba,
        NEW_FIX_EVENT_DATA(
            gba->core.cycles + 3,
            mem_do_dma_fifo_transfer,
            (union event_data){ .u32 = dma_channel_idx }
        )
    );
}

bool
mem_dma_is_fifo(
    struct gba const *gba,
    uint32_t dma_channel_idx,
    uint32_t fifo_idx
) {
    struct dma_channel const *dma;

    dma = &gba->io.dma[dma_channel_idx];
    return (
           dma->control.enable
        && dma->control.timing == DMA_TIMING_SPECIAL
        && dma->dst.raw == (fifo_idx == FIFO_A ? IO_REG_FIFO_A : IO_REG_FIFO_B)
    );
}

static
void
mem_do_dma_video_transfer(
    struct gba *gba,
    union event_data data __unused
) {
    struct dma_channel *channel;
    enum access_type access;
    bool dma_was_enabled;
    int32_t src_step;
    int32_t dst_step;
    bool reload;
    size_t unit_size;

    channel = &gba->io.dma[3];

    // Skip channels that aren't enabled or that shouldn't happen at the given timing
    if (!channel->control.enable || channel->control.timing != DMA_TIMING_SPECIAL) {
        return;
    }

    printf("YES! %u\n", gba->io.vcount.raw);

    dma_was_enabled = gba->core.processing_dma;
    gba->core.processing_dma = true;

    // The first DMA take at least two internal cycles
    // (Supposedly to transition from CPU to DMA)
    if (!dma_was_enabled) {
        core_idle_for(gba, 2);
    }

        bool src_in_gamepak = (((channel->internal_src >> 24) & 0xF) >= CART_REGION_START && ((channel->internal_src >> 24) & 0xF) <= CART_REGION_END);
        bool dst_in_gamepak = (((channel->internal_dst >> 24) & 0xF) >= CART_REGION_START && ((channel->internal_dst >> 24) & 0xF) <= CART_REGION_END);

        if ((src_in_gamepak && dst_in_gamepak)) {
            core_idle_for(gba, 2);
        }


    reload = false;
    unit_size = channel->control.unit_size ? 4 : 2; // In  bytes

    switch (channel->control.dst_ctl) {
        case 0b00:      dst_step = unit_size; break;
        case 0b01:      dst_step = -unit_size; break;
        case 0b10:      dst_step = 0; break;
        case 0b11:      dst_step = unit_size; reload = true; break;
    }

    switch (channel->control.src_ctl) {
        case 0b00:      src_step = unit_size; break;
        case 0b01:      src_step = -unit_size; break;
        case 0b10:      src_step = 0; break;
        case 0b11:      src_step = 0; break;
    }

    // A count of 0 is treated as max length.
    if (channel->internal_count == 0) {
        channel->internal_count = count_mask[3] + 1;
    }

    printf("SRC_CTL=%u\n", channel->control.src_ctl);
    printf("SRC=%08x DST=%08x SRC STEP=%i DST STEP=%i LEN=%u\n", channel->internal_src, channel->internal_dst, src_step, dst_step, channel->internal_count);

    access = NON_SEQUENTIAL;
    while (channel->internal_count > 0) {
        if (unit_size == 4) {
            mem_write32(gba, channel->internal_dst, mem_read32(gba, channel->internal_src, access), access);
        } else { // unit_size == 2
            mem_write16(gba, channel->internal_dst, mem_read16(gba, channel->internal_src, access), access);
            printf("WRITING %04x to %08x\n", mem_read16(gba, channel->internal_src, access), channel->internal_dst);
        }
        channel->internal_src += src_step;
        channel->internal_dst += dst_step;
        channel->internal_count -= 1;
        access = SEQUENTIAL;
    }

    if (channel->control.irq_end) {
        core_trigger_irq(gba, IRQ_DMA3);
    }

    if (channel->control.repeat) {
        channel->internal_count = channel->count.raw;
        channel->internal_count &= count_mask[3];
        if (reload) {
            channel->internal_dst = channel->dst.raw & (channel->control.unit_size ? ~3 : ~1);
            channel->internal_dst &= dst_mask[3];
        }
    } else {
        channel->control.enable = false;
    }

    gba->core.processing_dma = dma_was_enabled;
}

void
mem_schedule_dma_video(
    struct gba *gba
) {
    sched_add_event(
        gba,
        NEW_FIX_EVENT(
            gba->core.cycles + 3,
            mem_do_dma_video_transfer
        )
    );
}