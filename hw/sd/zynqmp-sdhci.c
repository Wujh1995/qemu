/*
 * ZynqMP SDHCI controller.
 *
 * Copyright (c) 2013 Xilinx Inc
 * Copyright (c) 2013 Peter Crosthwaite <peter.crosthwaite@xilinx.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "hw/sysbus.h"
#include "qemu/log.h"

#include "qemu/bitops.h"
#include "qapi/qmp/qerror.h"
#include "sysemu/blockdev.h"
#include "sysemu/block-backend.h"

#include "hw/fdt_generic_util.h"

#include "sdhci-internal.h"

#ifndef ZYNQMP_SDHCI_ERR_DEBUG
#define ZYNQMP_SDHCI_ERR_DEBUG 0
#endif

#define TYPE_ZYNQMP_SDHCI "xilinx.zynqmp-sdhci"

#define ZYNQMP_SDHCI(obj) \
     OBJECT_CHECK(ZynqMPSDHCIState, (obj), TYPE_ZYNQMP_SDHCI)

#define ZYNQMP_SDHCI_PARENT_CLASS \
    object_class_get_parent(object_class_by_name(TYPE_ZYNQMP_SDHCI))

typedef struct ZynqMPSDHCIState {
    /*< private >*/
    SDHCIState parent_obj;
    /*< public >*/
    SDState *sd_card;
    SDState *mmc_card;
    uint8_t drive_index;
} ZynqMPSDHCIState;

static void zynqmp_sdhci_slottype_handler(void *opaque, int n, int level)
{
    SDHCIState *ss = SYSBUS_SDHCI(opaque);
    ZynqMPSDHCIState *s = ZYNQMP_SDHCI(opaque);

    assert(n == 0);

    ss->capareg = deposit64(ss->capareg, 30, 2, level);
    ss->card = extract64(ss->capareg, 30, 2) ? s->mmc_card : s->sd_card;
    sd_set_cb(ss->card, ss->ro_cb, ss->eject_cb);
}

static void zynqmp_sdhci_reset(DeviceState *dev)
{
    ZynqMPSDHCIState *s = ZYNQMP_SDHCI(dev);
    SDHCIState *ss = SYSBUS_SDHCI(dev);

    ss->card = s->sd_card;
    sd_set_cb(ss->card, ss->ro_cb, ss->eject_cb);
}

static void zynqmp_sdhci_realize(DeviceState *dev, Error **errp)
{
    DeviceClass *dc_parent = DEVICE_CLASS(ZYNQMP_SDHCI_PARENT_CLASS);
    ZynqMPSDHCIState *s = ZYNQMP_SDHCI(dev);
    DriveInfo *di_sd;
    DriveInfo *di_mmc;

    di_sd = drive_get_by_index(IF_SD , s->drive_index);
    di_mmc = drive_get_by_index(IF_SD, (s->drive_index + 2));

    s->sd_card = sd_init(di_sd ? blk_by_legacy_dinfo(di_sd) : NULL, false);
    s->mmc_card = mmc_init(di_mmc ? blk_by_legacy_dinfo(di_mmc) : NULL);

    dc_parent->realize(dev, errp);

    qdev_init_gpio_in_named(dev, zynqmp_sdhci_slottype_handler, "SLOTTYPE", 1);
}

static Property zynqmp_sdhci_properties[] = {
    DEFINE_PROP_UINT8("drive-index", ZynqMPSDHCIState, drive_index, 0),
    DEFINE_PROP_END_OF_LIST(),
};

static void zynqmp_sdhci_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = zynqmp_sdhci_realize;
    dc->props = zynqmp_sdhci_properties;
    dc->reset = zynqmp_sdhci_reset;
}

static const TypeInfo zynqmp_sdhci_info = {
    .name          = TYPE_ZYNQMP_SDHCI,
    .parent        = TYPE_SYSBUS_SDHCI,
    .class_init    = zynqmp_sdhci_class_init,
    .instance_size = sizeof(ZynqMPSDHCIState),
};

static void zynqmp_sdhci_register_types(void)
{
    type_register_static(&zynqmp_sdhci_info);
}

type_init(zynqmp_sdhci_register_types)
