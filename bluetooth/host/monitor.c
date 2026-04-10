/** @file
 *  @brief Custom logging over UART
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <bluetooth/addr.h>
#include <bluetooth/buf.h>

#include "monitor.h"

#include <base/bt_atomic.h>
#include <base/byteorder.h>
#include <utils/bt_utils.h>

#include <bt_stack_init.h>
#include <osdep/os.h>
/* This is the same default priority as for other console handlers,
 * except that we're not exporting it as a Kconfig variable until a
 * clear need arises.
 */
#define MONITOR_INIT_PRIORITY 60

/* These defines follow the values used by syslog(2) */
#define BT_LOG_ERR      3
#define BT_LOG_WARN     4
#define BT_LOG_INFO     6
#define BT_LOG_DBG      7

/* TS resolution is 1/10th of a millisecond */
#define MONITOR_TS_FREQ 10000

/* Maximum (string) length of a log message */
#define MONITOR_MSG_MAX 128

enum {
	BT_LOG_BUSY,
	BT_CONSOLE_BUSY,
};

static bt_atomic_t flags;

static struct {
	bt_atomic_t cmd;
	bt_atomic_t evt;
	bt_atomic_t acl_tx;
	bt_atomic_t acl_rx;
#if defined(CONFIG_BT_CLASSIC)
	bt_atomic_t sco_tx;
	bt_atomic_t sco_rx;
#endif
	bt_atomic_t other;
} drops;

static void drop_add(uint16_t opcode)
{
	switch (opcode) {
	case BT_MONITOR_COMMAND_PKT:
		bt_atomic_inc(&drops.cmd);
		break;
	case BT_MONITOR_EVENT_PKT:
		bt_atomic_inc(&drops.evt);
		break;
	case BT_MONITOR_ACL_TX_PKT:
		bt_atomic_inc(&drops.acl_tx);
		break;
	case BT_MONITOR_ACL_RX_PKT:
		bt_atomic_inc(&drops.acl_rx);
		break;
#if defined(CONFIG_BT_CLASSIC)
	case BT_MONITOR_SCO_TX_PKT:
		bt_atomic_inc(&drops.sco_tx);
		break;
	case BT_MONITOR_SCO_RX_PKT:
		bt_atomic_inc(&drops.sco_rx);
		break;
#endif
	default:
		bt_atomic_inc(&drops.other);
		break;
	}
}

#if defined(CONFIG_BT_DEBUG_MONITOR_UART)
static void poll_out(char c)
{
	ARG_UNUSED(c);
}

static void monitor_send(const void *data, size_t len)
{
	ARG_UNUSED(data);
	ARG_UNUSED(len);
}
#endif /* CONFIG_BT_DEBUG_MONITOR_UART */

static void encode_drops(struct bt_monitor_hdr *hdr, uint8_t type,
			 bt_atomic_t *val)
{
	bt_atomic_val_t count;

	count = bt_atomic_set(val, 0);
	if (count) {
		hdr->ext[hdr->hdr_len++] = type;
		hdr->ext[hdr->hdr_len++] = MIN(count, 255);
	}
}

static log_timestamp_t monitor_ts_get(void)
{
	return (log_timestamp_t)(os_time_get_ms() * (MONITOR_TS_FREQ / 1000));
}

static inline void encode_hdr(struct bt_monitor_hdr *hdr, log_timestamp_t timestamp,
			      uint16_t opcode, uint16_t len)
{
	struct bt_monitor_ts32 *ts;

	hdr->opcode   = sys_cpu_to_le16(opcode);
	hdr->flags    = 0U;

	ts = (void *)hdr->ext;
	ts->type = BT_MONITOR_TS32;
	/* The btsnoop protocol only supports 32-bit timestamps (in 1/10th ms).
	 * This overflows after 4.97 days, which is acceptable as it only affects
	 * rendering in btmon/wireshark. Recordings are usually not that long
	 * anyway, and packet ordering is still preserved.
	 */
	ts->ts32 = (uint32_t)timestamp;
	hdr->hdr_len = sizeof(*ts);

	encode_drops(hdr, BT_MONITOR_COMMAND_DROPS, &drops.cmd);
	encode_drops(hdr, BT_MONITOR_EVENT_DROPS, &drops.evt);
	encode_drops(hdr, BT_MONITOR_ACL_TX_DROPS, &drops.acl_tx);
	encode_drops(hdr, BT_MONITOR_ACL_RX_DROPS, &drops.acl_rx);
#if defined(CONFIG_BT_CLASSIC)
	encode_drops(hdr, BT_MONITOR_SCO_TX_DROPS, &drops.sco_tx);
	encode_drops(hdr, BT_MONITOR_SCO_RX_DROPS, &drops.sco_rx);
#endif
	encode_drops(hdr, BT_MONITOR_OTHER_DROPS, &drops.other);

	hdr->data_len = sys_cpu_to_le16(4 + hdr->hdr_len + len);
}

void bt_monitor_send(uint16_t opcode, const void *data, size_t len)
{
	struct bt_monitor_hdr hdr;

	if (bt_atomic_test_and_set_bit(&flags, BT_LOG_BUSY)) {
		drop_add(opcode);
		return;
	}

	encode_hdr(&hdr, monitor_ts_get(), opcode, len);

	monitor_send(&hdr, BT_MONITOR_BASE_HDR_LEN + hdr.hdr_len);
	monitor_send(data, len);

	bt_atomic_clear_bit(&flags, BT_LOG_BUSY);
}

void bt_monitor_new_index(uint8_t type, uint8_t bus, const bt_addr_t *addr,
			  const char *name)
{
	struct bt_monitor_new_index pkt;

	pkt.type = type;
	pkt.bus = bus;
	memcpy(pkt.bdaddr, addr, 6);
	strncpy(pkt.name, name, sizeof(pkt.name) - 1);
	pkt.name[sizeof(pkt.name) - 1] = '\0';

	bt_monitor_send(BT_MONITOR_NEW_INDEX, &pkt, sizeof(pkt));
}

#if !defined(CONFIG_UART_CONSOLE) && !defined(CONFIG_RTT_CONSOLE) && !defined(CONFIG_LOG_PRINTK)
static int monitor_console_out(int c)
{
	static char buf[MONITOR_MSG_MAX];
	static size_t len;

	if (bt_atomic_test_and_set_bit(&flags, BT_CONSOLE_BUSY)) {
		return c;
	}

	if (c != '\n' && len < sizeof(buf) - 1) {
		buf[len++] = c;
		bt_atomic_clear_bit(&flags, BT_CONSOLE_BUSY);
		return c;
	}

	buf[len++] = '\0';

	bt_monitor_send(BT_MONITOR_SYSTEM_NOTE, buf, len);
	len = 0;

	bt_atomic_clear_bit(&flags, BT_CONSOLE_BUSY);

	return c;
}
#endif /* !CONFIG_UART_CONSOLE && !CONFIG_RTT_CONSOLE && !CONFIG_LOG_PRINTK */

#ifndef CONFIG_LOG_MODE_MINIMAL
struct monitor_log_ctx {
	size_t total_len;
	char msg[MONITOR_MSG_MAX];
};

static int monitor_log_out(uint8_t *data, size_t length, void *user_data)
{
	struct monitor_log_ctx *ctx = user_data;
	size_t i;

	for (i = 0; i < length && ctx->total_len < sizeof(ctx->msg); i++) {
		/* With CONFIG_LOG_PRINTK the line terminator will come as
		 * as part of messages.
		 */
		if (IS_ENABLED(CONFIG_LOG_PRINTK) &&
		    (data[i] == '\r' || data[i] == '\n')) {
			break;
		}

		ctx->msg[ctx->total_len++] = data[i];
	}

	return length;
}

static uint8_t buf;

LOG_OUTPUT_DEFINE(monitor_log_output, monitor_log_out, &buf, 1);

static inline uint8_t monitor_priority_get(uint8_t log_level)
{
	static const uint8_t prios[] = {
		[LOG_LEVEL_NONE]  = 0,
		[LOG_LEVEL_ERR]   = BT_LOG_ERR,
		[LOG_LEVEL_WRN]   = BT_LOG_WARN,
		[LOG_LEVEL_INF]   = BT_LOG_INFO,
		[LOG_LEVEL_DBG]   = BT_LOG_DBG,
	};

	if (log_level < ARRAY_SIZE(prios)) {
		return prios[log_level];
	}

	return BT_LOG_DBG;
}

static void monitor_log_process(const struct log_backend *const backend,
				union log_msg_generic *msg)
{
	struct bt_monitor_user_logging user_log;
	struct monitor_log_ctx ctx;
	struct bt_monitor_hdr hdr;
	static const char id[] = "bt";

	log_output_ctx_set(&monitor_log_output, &ctx);

	ctx.total_len = 0;
	log_output_msg_process(&monitor_log_output, &msg->log,
			       LOG_OUTPUT_FLAG_CRLF_NONE);

	if (bt_atomic_test_and_set_bit(&flags, BT_LOG_BUSY)) {
		drop_add(BT_MONITOR_USER_LOGGING);
		return;
	}

	encode_hdr(&hdr, log_msg_get_timestamp(&msg->log),
		   BT_MONITOR_USER_LOGGING,
		   sizeof(user_log) + sizeof(id) + ctx.total_len + 1);

	user_log.priority = monitor_priority_get(log_msg_get_level(&msg->log));
	user_log.ident_len = sizeof(id);

	monitor_send(&hdr, BT_MONITOR_BASE_HDR_LEN + hdr.hdr_len);
	monitor_send(&user_log, sizeof(user_log));
	monitor_send(id, sizeof(id));
	monitor_send(ctx.msg, ctx.total_len);

	/* Terminate the string with null */
	poll_out('\0');

	bt_atomic_clear_bit(&flags, BT_LOG_BUSY);
}

static void monitor_log_panic(const struct log_backend *const backend)
{
	ARG_UNUSED(backend);
}

static void monitor_log_init(const struct log_backend *const backend)
{
	log_set_timestamp_func(monitor_ts_get, MONITOR_TS_FREQ);
}

static const struct log_backend_api monitor_log_api = {
	.process = monitor_log_process,
	.panic = monitor_log_panic,
	.init = monitor_log_init,
};

LOG_BACKEND_DEFINE(bt_monitor, monitor_log_api, true);
#endif /* CONFIG_LOG_MODE_MINIMAL */

static int bt_monitor_init(void)
{
	#if defined(CONFIG_BT_DEBUG_MONITOR_UART)
	#endif /* CONFIG_BT_DEBUG_MONITOR_UART */

#if !defined(CONFIG_UART_CONSOLE) && !defined(CONFIG_RTT_CONSOLE) && !defined(CONFIG_LOG_PRINTK)
	__printk_hook_install(monitor_console_out);
	__stdout_hook_install(monitor_console_out);
#endif /* !CONFIG_UART_CONSOLE && !CONFIG_RTT_CONSOLE && !CONFIG_LOG_PRINTK */

	return 0;
}

STACK_INIT(bt_monitor_init, STACK_BASE_INIT, MONITOR_INIT_PRIORITY);
