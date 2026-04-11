/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/hci.h>
#include <host/hci_core.h>
#include "mocks/hci_core.h"

struct bt_dev bt_dev = {
	.manufacturer = 0x1234,
};

struct bt_hci_le_rand_fake_state bt_hci_le_rand_fake;

void reset_bt_hci_le_rand_fake(void)
{
	bt_hci_le_rand_fake.call_count = 0;
	bt_hci_le_rand_fake.return_val = 0;
	bt_hci_le_rand_fake.arg0_val = NULL;
	bt_hci_le_rand_fake.arg1_val = 0;
	for (size_t i = 0; i < 8; i++) {
		bt_hci_le_rand_fake.arg0_history[i] = NULL;
		bt_hci_le_rand_fake.arg1_history[i] = 0;
	}
}

int bt_hci_le_rand(void *buf, size_t len)
{
	int idx = bt_hci_le_rand_fake.call_count;

	bt_hci_le_rand_fake.arg0_val = buf;
	bt_hci_le_rand_fake.arg1_val = len;
	if (idx < 8) {
		bt_hci_le_rand_fake.arg0_history[idx] = buf;
		bt_hci_le_rand_fake.arg1_history[idx] = len;
	}
	bt_hci_le_rand_fake.call_count++;

	return bt_hci_le_rand_fake.return_val;
}
