/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

struct bt_hci_le_rand_fake_state {
	int call_count;
	int return_val;
	void *arg0_val;
	size_t arg1_val;
	void *arg0_history[8];
	size_t arg1_history[8];
};

extern struct bt_hci_le_rand_fake_state bt_hci_le_rand_fake;

void reset_bt_hci_le_rand_fake(void);
int bt_hci_le_rand(void *buf, size_t len);
