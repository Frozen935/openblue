/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/fff.h>

#include "bap_unicast_server.h"

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)                                                                       \
	FAKE(mock_bap_unicast_server_cb_config)                                                    \
	FAKE(mock_bap_unicast_server_cb_reconfig)                                                  \
	FAKE(mock_bap_unicast_server_cb_qos)                                                       \
	FAKE(mock_bap_unicast_server_cb_enable)                                                    \
	FAKE(mock_bap_unicast_server_cb_start)                                                     \
	FAKE(mock_bap_unicast_server_cb_metadata)                                                  \
	FAKE(mock_bap_unicast_server_cb_disable)                                                   \
	FAKE(mock_bap_unicast_server_cb_stop)                                                      \
	FAKE(mock_bap_unicast_server_cb_release)                                                   \

void mock_bap_unicast_server_init(void)
{
	mock_bap_unicast_server_cb_config_reset();
	mock_bap_unicast_server_cb_reconfig_reset();
	mock_bap_unicast_server_cb_qos_reset();
	mock_bap_unicast_server_cb_enable_reset();
	mock_bap_unicast_server_cb_start_reset();
	mock_bap_unicast_server_cb_metadata_reset();
	mock_bap_unicast_server_cb_disable_reset();
	mock_bap_unicast_server_cb_stop_reset();
	mock_bap_unicast_server_cb_release_reset();
}

void mock_bap_unicast_server_cleanup(void)
{

}

mock_bap_unicast_server_cb_config_Fake mock_bap_unicast_server_cb_config_fake;
mock_bap_unicast_server_cb_reconfig_Fake mock_bap_unicast_server_cb_reconfig_fake;
mock_bap_unicast_server_cb_qos_Fake mock_bap_unicast_server_cb_qos_fake;
mock_bap_unicast_server_cb_enable_Fake mock_bap_unicast_server_cb_enable_fake;
mock_bap_unicast_server_cb_start_Fake mock_bap_unicast_server_cb_start_fake;
mock_bap_unicast_server_cb_metadata_Fake mock_bap_unicast_server_cb_metadata_fake;
mock_bap_unicast_server_cb_disable_Fake mock_bap_unicast_server_cb_disable_fake;
mock_bap_unicast_server_cb_stop_Fake mock_bap_unicast_server_cb_stop_fake;
mock_bap_unicast_server_cb_release_Fake mock_bap_unicast_server_cb_release_fake;

void mock_bap_unicast_server_cb_config_reset(void) { memset(&mock_bap_unicast_server_cb_config_fake, 0, sizeof(mock_bap_unicast_server_cb_config_fake)); }
void mock_bap_unicast_server_cb_reconfig_reset(void) { memset(&mock_bap_unicast_server_cb_reconfig_fake, 0, sizeof(mock_bap_unicast_server_cb_reconfig_fake)); }
void mock_bap_unicast_server_cb_qos_reset(void) { memset(&mock_bap_unicast_server_cb_qos_fake, 0, sizeof(mock_bap_unicast_server_cb_qos_fake)); }
void mock_bap_unicast_server_cb_enable_reset(void) { memset(&mock_bap_unicast_server_cb_enable_fake, 0, sizeof(mock_bap_unicast_server_cb_enable_fake)); }
void mock_bap_unicast_server_cb_start_reset(void) { memset(&mock_bap_unicast_server_cb_start_fake, 0, sizeof(mock_bap_unicast_server_cb_start_fake)); }
void mock_bap_unicast_server_cb_metadata_reset(void) { memset(&mock_bap_unicast_server_cb_metadata_fake, 0, sizeof(mock_bap_unicast_server_cb_metadata_fake)); }
void mock_bap_unicast_server_cb_disable_reset(void) { memset(&mock_bap_unicast_server_cb_disable_fake, 0, sizeof(mock_bap_unicast_server_cb_disable_fake)); }
void mock_bap_unicast_server_cb_stop_reset(void) { memset(&mock_bap_unicast_server_cb_stop_fake, 0, sizeof(mock_bap_unicast_server_cb_stop_fake)); }
void mock_bap_unicast_server_cb_release_reset(void) { memset(&mock_bap_unicast_server_cb_release_fake, 0, sizeof(mock_bap_unicast_server_cb_release_fake)); }

int mock_bap_unicast_server_cb_config(struct bt_conn *arg0, const struct bt_bap_ep *arg1,
				      enum bt_audio_dir arg2,
				      const struct bt_audio_codec_cfg *arg3,
				      struct bt_bap_stream **arg4,
				      struct bt_bap_qos_cfg_pref *const arg5,
				      struct bt_bap_ascs_rsp *arg6)
{
	unsigned int idx = mock_bap_unicast_server_cb_config_fake.call_count++;
	mock_bap_unicast_server_cb_config_fake.arg0_val = arg0;
	mock_bap_unicast_server_cb_config_fake.arg1_val = arg1;
	mock_bap_unicast_server_cb_config_fake.arg2_val = arg2;
	mock_bap_unicast_server_cb_config_fake.arg3_val = arg3;
	mock_bap_unicast_server_cb_config_fake.arg4_val = arg4;
	mock_bap_unicast_server_cb_config_fake.arg5_val = arg5;
	mock_bap_unicast_server_cb_config_fake.arg6_val = arg6;
	if (idx < FFF_ARG_HISTORY_LEN) {
		mock_bap_unicast_server_cb_config_fake.arg0_history[idx] = arg0;
		mock_bap_unicast_server_cb_config_fake.arg1_history[idx] = arg1;
		mock_bap_unicast_server_cb_config_fake.arg2_history[idx] = arg2;
		mock_bap_unicast_server_cb_config_fake.arg3_history[idx] = arg3;
		mock_bap_unicast_server_cb_config_fake.arg4_history[idx] = arg4;
		mock_bap_unicast_server_cb_config_fake.arg5_history[idx] = arg5;
		mock_bap_unicast_server_cb_config_fake.arg6_history[idx] = arg6;
	}
	if (mock_bap_unicast_server_cb_config_fake.custom_fake != NULL) {
		return mock_bap_unicast_server_cb_config_fake.custom_fake(arg0, arg1, arg2, arg3,
									  arg4, arg5, arg6);
	}
	return 0;
}

#define DEFINE_SIMPLE_FAKE_5(_name, _fake, _t0, _t1, _t2, _t3, _t4)                                \
	int _name(_t0 arg0, _t1 arg1, _t2 arg2, _t3 arg3, _t4 arg4)                                 \
	{                                                                                           \
		unsigned int idx = _fake.call_count++;                                               \
		_fake.arg0_val = arg0;                                                               \
		_fake.arg1_val = arg1;                                                               \
		_fake.arg2_val = arg2;                                                               \
		_fake.arg3_val = arg3;                                                               \
		_fake.arg4_val = arg4;                                                               \
		if (idx < FFF_ARG_HISTORY_LEN) {                                                     \
			_fake.arg0_history[idx] = arg0;                                                \
			_fake.arg1_history[idx] = arg1;                                                \
			_fake.arg2_history[idx] = arg2;                                                \
			_fake.arg3_history[idx] = arg3;                                                \
			_fake.arg4_history[idx] = arg4;                                                \
		}                                                                                  \
		return 0;                                                                          \
	}

#define DEFINE_SIMPLE_FAKE_4(_name, _fake, _t0, _t1, _t2, _t3)                                    \
	int _name(_t0 arg0, _t1 arg1, _t2 arg2, _t3 arg3)                                            \
	{                                                                                           \
		unsigned int idx = _fake.call_count++;                                               \
		_fake.arg0_val = arg0;                                                               \
		_fake.arg1_val = arg1;                                                               \
		_fake.arg2_val = arg2;                                                               \
		_fake.arg3_val = arg3;                                                               \
		if (idx < FFF_ARG_HISTORY_LEN) {                                                     \
			_fake.arg0_history[idx] = arg0;                                                \
			_fake.arg1_history[idx] = arg1;                                                \
			_fake.arg2_history[idx] = arg2;                                                \
			_fake.arg3_history[idx] = arg3;                                                \
		}                                                                                  \
		return 0;                                                                          \
	}

#define DEFINE_SIMPLE_FAKE_3(_name, _fake, _t0, _t1, _t2)                                         \
	int _name(_t0 arg0, _t1 arg1, _t2 arg2)                                                     \
	{                                                                                           \
		unsigned int idx = _fake.call_count++;                                               \
		_fake.arg0_val = arg0;                                                               \
		_fake.arg1_val = arg1;                                                               \
		_fake.arg2_val = arg2;                                                               \
		if (idx < FFF_ARG_HISTORY_LEN) {                                                     \
			_fake.arg0_history[idx] = arg0;                                                \
			_fake.arg1_history[idx] = arg1;                                                \
			_fake.arg2_history[idx] = arg2;                                                \
		}                                                                                  \
		return 0;                                                                          \
	}

#define DEFINE_SIMPLE_FAKE_2(_name, _fake, _t0, _t1)                                              \
	int _name(_t0 arg0, _t1 arg1)                                                              \
	{                                                                                           \
		unsigned int idx = _fake.call_count++;                                               \
		_fake.arg0_val = arg0;                                                               \
		_fake.arg1_val = arg1;                                                               \
		if (idx < FFF_ARG_HISTORY_LEN) {                                                     \
			_fake.arg0_history[idx] = arg0;                                                \
			_fake.arg1_history[idx] = arg1;                                                \
		}                                                                                  \
		return 0;                                                                          \
	}

DEFINE_SIMPLE_FAKE_5(mock_bap_unicast_server_cb_reconfig, mock_bap_unicast_server_cb_reconfig_fake,
			     struct bt_bap_stream *, enum bt_audio_dir,
			     const struct bt_audio_codec_cfg *, struct bt_bap_qos_cfg_pref *const,
			     struct bt_bap_ascs_rsp *)
DEFINE_SIMPLE_FAKE_3(mock_bap_unicast_server_cb_qos, mock_bap_unicast_server_cb_qos_fake,
			     struct bt_bap_stream *, const struct bt_bap_qos_cfg *,
			     struct bt_bap_ascs_rsp *)
DEFINE_SIMPLE_FAKE_4(mock_bap_unicast_server_cb_enable, mock_bap_unicast_server_cb_enable_fake,
			     struct bt_bap_stream *, const uint8_t *, size_t,
			     struct bt_bap_ascs_rsp *)
DEFINE_SIMPLE_FAKE_2(mock_bap_unicast_server_cb_start, mock_bap_unicast_server_cb_start_fake,
			     struct bt_bap_stream *, struct bt_bap_ascs_rsp *)
DEFINE_SIMPLE_FAKE_4(mock_bap_unicast_server_cb_metadata, mock_bap_unicast_server_cb_metadata_fake,
			     struct bt_bap_stream *, const uint8_t *, size_t,
			     struct bt_bap_ascs_rsp *)
DEFINE_SIMPLE_FAKE_2(mock_bap_unicast_server_cb_disable, mock_bap_unicast_server_cb_disable_fake,
			     struct bt_bap_stream *, struct bt_bap_ascs_rsp *)
DEFINE_SIMPLE_FAKE_2(mock_bap_unicast_server_cb_stop, mock_bap_unicast_server_cb_stop_fake,
			     struct bt_bap_stream *, struct bt_bap_ascs_rsp *)
DEFINE_SIMPLE_FAKE_2(mock_bap_unicast_server_cb_release, mock_bap_unicast_server_cb_release_fake,
			     struct bt_bap_stream *, struct bt_bap_ascs_rsp *)

const struct bt_bap_unicast_server_cb mock_bap_unicast_server_cb = {
	.config = mock_bap_unicast_server_cb_config,
	.reconfig = mock_bap_unicast_server_cb_reconfig,
	.qos = mock_bap_unicast_server_cb_qos,
	.enable = mock_bap_unicast_server_cb_enable,
	.start = mock_bap_unicast_server_cb_start,
	.metadata = mock_bap_unicast_server_cb_metadata,
	.disable = mock_bap_unicast_server_cb_disable,
	.stop = mock_bap_unicast_server_cb_stop,
	.release = mock_bap_unicast_server_cb_release,
};
