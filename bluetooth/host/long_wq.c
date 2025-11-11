/* long_work.c - Workqueue intended for long-running operations. */

/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


static struct bt_work_q bt_long_wq;

int bt_long_wq_schedule(struct bt_work_delayable *dwork, os_timeout_t timeout)
{
	return bt_work_schedule_for_queue(&bt_long_wq, dwork, timeout);
}

int bt_long_wq_reschedule(struct bt_work_delayable *dwork, os_timeout_t timeout)
{
	return bt_work_reschedule_for_queue(&bt_long_wq, dwork, timeout);
}

int bt_long_wq_submit(struct bt_work *work)
{
	return bt_work_submit_to_queue(&bt_long_wq, work);
}

static int long_wq_init(void)
{

	const struct bt_work_queue_config cfg = {.name = "BT LW WQ"};

	bt_work_queue_init(&bt_long_wq);

	bt_work_queue_start(&bt_long_wq, CONFIG_BT_LONG_WQ_STACK_SIZE,
			   OS_PRIORITY(CONFIG_BT_LONG_WQ_PRIO), &cfg);

	return 0;
}

STACK_INIT(long_wq_init, STACK_RUN_INIT, CONFIG_BT_LONG_WQ_INIT_PRIO);
