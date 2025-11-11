/*
 * Copyright (c) 2024 Croxel, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BLUETOOTH_SERVICES_NUS_INTERNAL_H__
#define __BLUETOOTH_SERVICES_NUS_INTERNAL_H__

#include <bluetooth/services/nus.h>
#include <bluetooth/services/nus/inst.h>

struct bt_nus_inst *bt_nus_inst_default(void);
struct bt_nus_inst *bt_nus_inst_get_from_attr(const struct bt_gatt_attr *attr);

#endif /* __BLUETOOTH_SERVICES_NUS_INTERNAL_H__ */
