/* sdp_client.c - Bluetooth classic SDP client smoke test */

/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/classic/rfcomm.h>
#include <bluetooth/classic/sdp.h>

#include <base/bt_buf.h>
#include <base/byteorder.h>
#include <base/utils.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

static struct bt_sdp_discover_params sdp_discover;
static union {
	struct bt_uuid_16 u16;
	struct bt_uuid_32 u32;
	struct bt_uuid_128 u128;
} sdp_discover_uuid;

const uint16_t svclass_list[] = {
	BT_SDP_SDP_SERVER_SVCLASS,
	BT_SDP_BROWSE_GRP_DESC_SVCLASS,
	BT_SDP_PUBLIC_BROWSE_GROUP,
	BT_SDP_SERIAL_PORT_SVCLASS,
	BT_SDP_LAN_ACCESS_SVCLASS,
	BT_SDP_DIALUP_NET_SVCLASS,
	BT_SDP_IRMC_SYNC_SVCLASS,
	BT_SDP_OBEX_OBJPUSH_SVCLASS,
	BT_SDP_OBEX_FILETRANS_SVCLASS,
	BT_SDP_IRMC_SYNC_CMD_SVCLASS,
	BT_SDP_HEADSET_SVCLASS,
	BT_SDP_CORDLESS_TELEPHONY_SVCLASS,
	BT_SDP_AUDIO_SOURCE_SVCLASS,
	BT_SDP_AUDIO_SINK_SVCLASS,
	BT_SDP_AV_REMOTE_TARGET_SVCLASS,
	BT_SDP_ADVANCED_AUDIO_SVCLASS,
	BT_SDP_AV_REMOTE_SVCLASS,
	BT_SDP_AV_REMOTE_CONTROLLER_SVCLASS,
	BT_SDP_INTERCOM_SVCLASS,
	BT_SDP_FAX_SVCLASS,
	BT_SDP_HEADSET_AGW_SVCLASS,
	BT_SDP_WAP_SVCLASS,
	BT_SDP_WAP_CLIENT_SVCLASS,
	BT_SDP_PANU_SVCLASS,
	BT_SDP_NAP_SVCLASS,
	BT_SDP_GN_SVCLASS,
	BT_SDP_DIRECT_PRINTING_SVCLASS,
	BT_SDP_REFERENCE_PRINTING_SVCLASS,
	BT_SDP_IMAGING_SVCLASS,
	BT_SDP_IMAGING_RESPONDER_SVCLASS,
	BT_SDP_IMAGING_ARCHIVE_SVCLASS,
	BT_SDP_IMAGING_REFOBJS_SVCLASS,
	BT_SDP_HANDSFREE_SVCLASS,
	BT_SDP_HANDSFREE_AGW_SVCLASS,
	BT_SDP_DIRECT_PRT_REFOBJS_SVCLASS,
	BT_SDP_REFLECTED_UI_SVCLASS,
	BT_SDP_BASIC_PRINTING_SVCLASS,
	BT_SDP_PRINTING_STATUS_SVCLASS,
	BT_SDP_HID_SVCLASS,
	BT_SDP_HCR_SVCLASS,
	BT_SDP_HCR_PRINT_SVCLASS,
	BT_SDP_HCR_SCAN_SVCLASS,
	BT_SDP_CIP_SVCLASS,
	BT_SDP_VIDEO_CONF_GW_SVCLASS,
	BT_SDP_UDI_MT_SVCLASS,
	BT_SDP_UDI_TA_SVCLASS,
	BT_SDP_AV_SVCLASS,
	BT_SDP_SAP_SVCLASS,
	BT_SDP_PBAP_PCE_SVCLASS,
	BT_SDP_PBAP_PSE_SVCLASS,
	BT_SDP_PBAP_SVCLASS,
	BT_SDP_MAP_MSE_SVCLASS,
	BT_SDP_MAP_MCE_SVCLASS,
	BT_SDP_MAP_SVCLASS,
	BT_SDP_GNSS_SVCLASS,
	BT_SDP_GNSS_SERVER_SVCLASS,
	BT_SDP_MPS_SC_SVCLASS,
	BT_SDP_MPS_SVCLASS,
	BT_SDP_PNP_INFO_SVCLASS,
	BT_SDP_GENERIC_NETWORKING_SVCLASS,
	BT_SDP_GENERIC_FILETRANS_SVCLASS,
	BT_SDP_GENERIC_AUDIO_SVCLASS,
	BT_SDP_GENERIC_TELEPHONY_SVCLASS,
	BT_SDP_UPNP_SVCLASS,
	BT_SDP_UPNP_IP_SVCLASS,
	BT_SDP_UPNP_PAN_SVCLASS,
	BT_SDP_UPNP_LAP_SVCLASS,
	BT_SDP_UPNP_L2CAP_SVCLASS,
	BT_SDP_VIDEO_SOURCE_SVCLASS,
	BT_SDP_VIDEO_SINK_SVCLASS,
	BT_SDP_VIDEO_DISTRIBUTION_SVCLASS,
	BT_SDP_HDP_SVCLASS,
	BT_SDP_HDP_SOURCE_SVCLASS,
	BT_SDP_HDP_SINK_SVCLASS,
	BT_SDP_GENERIC_ACCESS_SVCLASS,
	BT_SDP_GENERIC_ATTRIB_SVCLASS,
	BT_SDP_APPLE_AGENT_SVCLASS,
};

#define SDP_CLIENT_USER_BUF_LEN 4096
BT_BUF_POOL_FIXED_DEFINE(sdp_c_client_pool, 1, SDP_CLIENT_USER_BUF_LEN,
			 CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static bool sdp_record_found;
static char sdp_raw_hex[(SDP_CLIENT_USER_BUF_LEN * 2U) + 1U];

static uint8_t sdp_discover_func(struct bt_conn *conn, struct bt_sdp_client_result *result,
				 const struct bt_sdp_discover_params *params)
{
	int err;
	uint16_t param;

	if ((result == NULL) || (result->resp_buf == NULL) || (result->resp_buf->len == 0)) {
		if (sdp_record_found) {
			sdp_record_found = false;
			bt_shell_print("SDP Discovery Done");
		} else {
			bt_shell_print("No SDP Record");
		}
		return BT_SDP_DISCOVER_UUID_STOP;
	}

	sdp_record_found = true;

	bt_shell_print("SDP Rsp Data:");
	err = bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_L2CAP, &param);
	if (!err) {
		bt_shell_print("    PROTOCOL: L2CAP: %d", param);
	}
	err = bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_RFCOMM, &param);
	if (!err) {
		bt_shell_print("    PROTOCOL: RFCOMM: %d", param);
	}
	for (size_t i = 0; i < ARRAY_SIZE(svclass_list); i++) {
		err = bt_sdp_get_profile_version(result->resp_buf, svclass_list[i], &param);
		if (!err) {
			bt_shell_print("    VERSION: %04X: %d", svclass_list[i], param);
		}
	}
	err = bt_sdp_get_features(result->resp_buf, &param);
	if (!err) {
		bt_shell_print("    FEATURE: %04X", param);
	}
	if (bin2hex(result->resp_buf->data, result->resp_buf->len, sdp_raw_hex,
		    sizeof(sdp_raw_hex)) != 0U) {
		bt_shell_print("    RAW:%s", sdp_raw_hex);
	}

	if (!result->next_record_hint) {
		sdp_record_found = false;
		bt_shell_print("SDP Discovery Done");
	}

	return BT_SDP_DISCOVER_UUID_CONTINUE;
}

static struct bt_sdp_attribute_id_list attr_ids;
static struct bt_sdp_attribute_id_range attr_id_ranges[1];

static int cmd_ssa_discovery(const struct bt_shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	size_t len;
	uint8_t uuid128[BT_UUID_SIZE_128];

	len = strlen(argv[1]);

	if (len == (BT_UUID_SIZE_16 * 2)) {
		uint16_t val;

		sdp_discover_uuid.u16.uuid.type = BT_UUID_TYPE_16;
		hex2bin(argv[1], len, (uint8_t *)&val, sizeof(val));
		sdp_discover_uuid.u16.val = sys_be16_to_cpu(val);
		sdp_discover.uuid = &sdp_discover_uuid.u16.uuid;
	} else if (len == (BT_UUID_SIZE_32 * 2)) {
		uint32_t val;

		sdp_discover_uuid.u32.uuid.type = BT_UUID_TYPE_32;
		hex2bin(argv[1], len, (uint8_t *)&val, sizeof(val));
		sdp_discover_uuid.u32.val = sys_be32_to_cpu(val);
		sdp_discover.uuid = &sdp_discover_uuid.u32.uuid;
	} else if (len == (BT_UUID_SIZE_128 * 2)) {
		sdp_discover_uuid.u128.uuid.type = BT_UUID_TYPE_128;
		hex2bin(argv[1], len, &uuid128[0], sizeof(uuid128));
		sys_memcpy_swap(sdp_discover_uuid.u128.val, uuid128, sizeof(uuid128));
		sdp_discover.uuid = &sdp_discover_uuid.u128.uuid;
	} else {
		bt_shell_error("Invalid UUID");
		return -ENOEXEC;
	}

	attr_ids.count = 0;

	if (argc > 2) {
		attr_ids.count = ARRAY_SIZE(attr_id_ranges);
		attr_ids.ranges = attr_id_ranges;
		attr_id_ranges[0].beginning = (uint16_t)bt_shell_strtol(argv[2], 0, &err);
		if (err < 0) {
			bt_shell_error("Invalid beginning ATTR ID");
			return -ENOEXEC;
		}
		attr_id_ranges[0].ending = 0xffff;
	}

	if (argc > 3) {
		attr_id_ranges[0].ending = (uint16_t)bt_shell_strtol(argv[3], 0, &err);
		if (err < 0) {
			bt_shell_error("Invalid ending ATTR ID");
			return -ENOEXEC;
		}
	}

	sdp_discover.func = sdp_discover_func;
	sdp_discover.pool = &sdp_c_client_pool;
	sdp_discover.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR;
	sdp_discover.ids = NULL;
	if (attr_ids.count != 0) {
		sdp_discover.ids = &attr_ids;
	}

	err = bt_sdp_discover(default_conn, &sdp_discover);
	if (err) {
		bt_shell_error("Fail to start SDP Discovery (err %d)", err);
		return err;
	}
	return 0;
}

static int cmd_ss_discovery(const struct bt_shell *sh, size_t argc, char *argv[])
{
	int err;
	size_t len;
	uint8_t uuid128[BT_UUID_SIZE_128];

	len = strlen(argv[1]);

	if (len == (BT_UUID_SIZE_16 * 2)) {
		uint16_t val;

		sdp_discover_uuid.u16.uuid.type = BT_UUID_TYPE_16;
		hex2bin(argv[1], len, (uint8_t *)&val, sizeof(val));
		sdp_discover_uuid.u16.val = sys_be16_to_cpu(val);
		sdp_discover.uuid = &sdp_discover_uuid.u16.uuid;
	} else if (len == (BT_UUID_SIZE_32 * 2)) {
		uint32_t val;

		sdp_discover_uuid.u32.uuid.type = BT_UUID_TYPE_32;
		hex2bin(argv[1], len, (uint8_t *)&val, sizeof(val));
		sdp_discover_uuid.u32.val = sys_be32_to_cpu(val);
		sdp_discover.uuid = &sdp_discover_uuid.u32.uuid;
	} else if (len == (BT_UUID_SIZE_128 * 2)) {
		sdp_discover_uuid.u128.uuid.type = BT_UUID_TYPE_128;
		hex2bin(argv[1], len, &uuid128[0], sizeof(uuid128));
		sys_memcpy_swap(sdp_discover_uuid.u128.val, uuid128, sizeof(uuid128));
		sdp_discover.uuid = &sdp_discover_uuid.u128.uuid;
	} else {
		bt_shell_error("Invalid UUID");
		return -ENOEXEC;
	}

	sdp_discover.func = sdp_discover_func;
	sdp_discover.pool = &sdp_c_client_pool;
	sdp_discover.type = BT_SDP_DISCOVER_SERVICE_SEARCH;

	err = bt_sdp_discover(default_conn, &sdp_discover);
	if (err) {
		bt_shell_error("Fail to start SDP Discovery (err %d)", err);
		return err;
	}
	return 0;
}

static int cmd_sa_discovery(const struct bt_shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	size_t len;
	uint32_t handle;

	len = strlen(argv[1]);

	if (len == (sizeof(handle) * 2)) {
		hex2bin(argv[1], len, (uint8_t *)&handle, sizeof(handle));
		sdp_discover.handle = sys_be32_to_cpu(handle);
	} else {
		bt_shell_error("Invalid UUID");
		return -ENOEXEC;
	}

	attr_ids.count = 0;

	if (argc > 2) {
		attr_ids.count = ARRAY_SIZE(attr_id_ranges);
		attr_ids.ranges = attr_id_ranges;
		attr_id_ranges[0].beginning = (uint16_t)bt_shell_strtol(argv[2], 0, &err);
		if (err < 0) {
			bt_shell_error("Invalid beginning ATTR ID");
			return -ENOEXEC;
		}
		attr_id_ranges[0].ending = 0xffff;
	}

	if (argc > 3) {
		attr_id_ranges[0].ending = (uint16_t)bt_shell_strtol(argv[3], 0, &err);
		if (err < 0) {
			bt_shell_error("Invalid ending ATTR ID");
			return -ENOEXEC;
		}
	}

	sdp_discover.func = sdp_discover_func;
	sdp_discover.pool = &sdp_c_client_pool;
	sdp_discover.type = BT_SDP_DISCOVER_SERVICE_ATTR;
	sdp_discover.ids = NULL;
	if (attr_ids.count != 0) {
		sdp_discover.ids = &attr_ids;
	}

	err = bt_sdp_discover(default_conn, &sdp_discover);
	if (err) {
		bt_shell_error("Fail to start SDP Discovery (err %d)", err);
		return err;
	}
	return 0;
}

static uint8_t sdp_discover_fail_func(struct bt_conn *conn, struct bt_sdp_client_result *result,
				      const struct bt_sdp_discover_params *params)
{
	if ((result == NULL) || (result->resp_buf == NULL) || (result->resp_buf->len == 0)) {
		bt_shell_print("test pass");
	} else {
		bt_shell_print("test fail");
	}

	return BT_SDP_DISCOVER_UUID_STOP;
}

static int cmd_ssa_discovery_fail(const struct bt_shell *sh, size_t argc, char *argv[])
{
	int err;

	sdp_discover.uuid = BT_UUID_DECLARE_16(BT_SDP_HANDSFREE_SVCLASS);
	sdp_discover.func = sdp_discover_fail_func;
	sdp_discover.pool = &sdp_c_client_pool;
	sdp_discover.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR;
	sdp_discover.ids = NULL;

	err = bt_sdp_discover(default_conn, &sdp_discover);
	if (err) {
		bt_shell_error("Fail to start SDP Discovery (err %d)", err);
		return err;
	}
	return 0;
}

#define HELP_ATTR_ID_LIST " [start] [end]"

BT_SHELL_STATIC_SUBCMD_SET_CREATE(sdp_client_cmds,
	BT_SHELL_CMD_ARG(ss_discovery, NULL, "<Big endian UUID>", cmd_ss_discovery, 2, 0),
	BT_SHELL_CMD_ARG(sa_discovery, NULL, "<Service Record Handle>" HELP_ATTR_ID_LIST,
		      cmd_sa_discovery, 2, 2),
	BT_SHELL_CMD_ARG(ssa_discovery, NULL, "<Big endian UUID>" HELP_ATTR_ID_LIST,
		      cmd_ssa_discovery, 2, 2),
	BT_SHELL_CMD_ARG(ssa_discovery_fail, NULL, "", cmd_ssa_discovery_fail, 1, 0),
	BT_SHELL_SUBCMD_SET_END
);

static int cmd_default_handler(const struct bt_shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		bt_shell_help(sh);
		return BT_SHELL_CMD_HELP_PRINTED;
	}

	bt_shell_error("%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

BT_SHELL_CMD_ARG_REGISTER(sdp_client, &sdp_client_cmds,
			  "Bluetooth classic SDP client shell commands",
			  cmd_default_handler, 1, 0);
