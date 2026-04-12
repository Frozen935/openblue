/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_BAP_UNICAST_SERVER_H_
#define MOCKS_BAP_UNICAST_SERVER_H_
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/fff.h>
#include <zephyr/bluetooth/audio/bap.h>

#ifndef FFF_ARG_HISTORY_LEN
#define FFF_ARG_HISTORY_LEN 32
#endif

extern const struct bt_bap_unicast_server_cb mock_bap_unicast_server_cb;

void mock_bap_unicast_server_init(void);
void mock_bap_unicast_server_cleanup(void);

typedef int (*mock_bap_unicast_server_cb_config_custom_fake_t)(
	struct bt_conn *, const struct bt_bap_ep *, enum bt_audio_dir,
	const struct bt_audio_codec_cfg *, struct bt_bap_stream **,
	struct bt_bap_qos_cfg_pref *const, struct bt_bap_ascs_rsp *);

typedef struct mock_bap_unicast_server_cb_config_Fake {
	unsigned int call_count;
	struct bt_conn *arg0_val;
	struct bt_conn *arg0_history[FFF_ARG_HISTORY_LEN];
	const struct bt_bap_ep *arg1_val;
	const struct bt_bap_ep *arg1_history[FFF_ARG_HISTORY_LEN];
	enum bt_audio_dir arg2_val;
	enum bt_audio_dir arg2_history[FFF_ARG_HISTORY_LEN];
	const struct bt_audio_codec_cfg *arg3_val;
	const struct bt_audio_codec_cfg *arg3_history[FFF_ARG_HISTORY_LEN];
	struct bt_bap_stream **arg4_val;
	struct bt_bap_stream **arg4_history[FFF_ARG_HISTORY_LEN];
	struct bt_bap_qos_cfg_pref *arg5_val;
	struct bt_bap_qos_cfg_pref *arg5_history[FFF_ARG_HISTORY_LEN];
	struct bt_bap_ascs_rsp *arg6_val;
	struct bt_bap_ascs_rsp *arg6_history[FFF_ARG_HISTORY_LEN];
	mock_bap_unicast_server_cb_config_custom_fake_t custom_fake;
} mock_bap_unicast_server_cb_config_Fake;

extern mock_bap_unicast_server_cb_config_Fake mock_bap_unicast_server_cb_config_fake;
int mock_bap_unicast_server_cb_config(struct bt_conn *, const struct bt_bap_ep *, enum bt_audio_dir,
				      const struct bt_audio_codec_cfg *, struct bt_bap_stream **,
				      struct bt_bap_qos_cfg_pref *const, struct bt_bap_ascs_rsp *);
void mock_bap_unicast_server_cb_config_reset(void);

typedef struct mock_bap_unicast_server_cb_reconfig_Fake {
	unsigned int call_count;
	struct bt_bap_stream *arg0_val;
	struct bt_bap_stream *arg0_history[FFF_ARG_HISTORY_LEN];
	enum bt_audio_dir arg1_val;
	enum bt_audio_dir arg1_history[FFF_ARG_HISTORY_LEN];
	const struct bt_audio_codec_cfg *arg2_val;
	const struct bt_audio_codec_cfg *arg2_history[FFF_ARG_HISTORY_LEN];
	struct bt_bap_qos_cfg_pref *arg3_val;
	struct bt_bap_qos_cfg_pref *arg3_history[FFF_ARG_HISTORY_LEN];
	struct bt_bap_ascs_rsp *arg4_val;
	struct bt_bap_ascs_rsp *arg4_history[FFF_ARG_HISTORY_LEN];
} mock_bap_unicast_server_cb_reconfig_Fake;

extern mock_bap_unicast_server_cb_reconfig_Fake mock_bap_unicast_server_cb_reconfig_fake;
int mock_bap_unicast_server_cb_reconfig(struct bt_bap_stream *, enum bt_audio_dir,
					const struct bt_audio_codec_cfg *,
					struct bt_bap_qos_cfg_pref *const,
					struct bt_bap_ascs_rsp *);
void mock_bap_unicast_server_cb_reconfig_reset(void);

typedef struct mock_bap_unicast_server_cb_qos_Fake {
	unsigned int call_count;
	struct bt_bap_stream *arg0_val;
	struct bt_bap_stream *arg0_history[FFF_ARG_HISTORY_LEN];
	const struct bt_bap_qos_cfg *arg1_val;
	const struct bt_bap_qos_cfg *arg1_history[FFF_ARG_HISTORY_LEN];
	struct bt_bap_ascs_rsp *arg2_val;
	struct bt_bap_ascs_rsp *arg2_history[FFF_ARG_HISTORY_LEN];
} mock_bap_unicast_server_cb_qos_Fake;

extern mock_bap_unicast_server_cb_qos_Fake mock_bap_unicast_server_cb_qos_fake;
int mock_bap_unicast_server_cb_qos(struct bt_bap_stream *, const struct bt_bap_qos_cfg *,
				       struct bt_bap_ascs_rsp *);
void mock_bap_unicast_server_cb_qos_reset(void);

typedef struct mock_bap_unicast_server_cb_enable_Fake {
	unsigned int call_count;
	struct bt_bap_stream *arg0_val;
	struct bt_bap_stream *arg0_history[FFF_ARG_HISTORY_LEN];
	const uint8_t *arg1_val;
	const uint8_t *arg1_history[FFF_ARG_HISTORY_LEN];
	size_t arg2_val;
	size_t arg2_history[FFF_ARG_HISTORY_LEN];
	struct bt_bap_ascs_rsp *arg3_val;
	struct bt_bap_ascs_rsp *arg3_history[FFF_ARG_HISTORY_LEN];
} mock_bap_unicast_server_cb_enable_Fake;

extern mock_bap_unicast_server_cb_enable_Fake mock_bap_unicast_server_cb_enable_fake;
int mock_bap_unicast_server_cb_enable(struct bt_bap_stream *, const uint8_t *, size_t,
					  struct bt_bap_ascs_rsp *);
void mock_bap_unicast_server_cb_enable_reset(void);

typedef struct mock_bap_unicast_server_cb_start_Fake {
	unsigned int call_count;
	struct bt_bap_stream *arg0_val;
	struct bt_bap_stream *arg0_history[FFF_ARG_HISTORY_LEN];
	struct bt_bap_ascs_rsp *arg1_val;
	struct bt_bap_ascs_rsp *arg1_history[FFF_ARG_HISTORY_LEN];
} mock_bap_unicast_server_cb_start_Fake;

extern mock_bap_unicast_server_cb_start_Fake mock_bap_unicast_server_cb_start_fake;
int mock_bap_unicast_server_cb_start(struct bt_bap_stream *, struct bt_bap_ascs_rsp *);
void mock_bap_unicast_server_cb_start_reset(void);

typedef struct mock_bap_unicast_server_cb_metadata_Fake {
	unsigned int call_count;
	struct bt_bap_stream *arg0_val;
	struct bt_bap_stream *arg0_history[FFF_ARG_HISTORY_LEN];
	const uint8_t *arg1_val;
	const uint8_t *arg1_history[FFF_ARG_HISTORY_LEN];
	size_t arg2_val;
	size_t arg2_history[FFF_ARG_HISTORY_LEN];
	struct bt_bap_ascs_rsp *arg3_val;
	struct bt_bap_ascs_rsp *arg3_history[FFF_ARG_HISTORY_LEN];
} mock_bap_unicast_server_cb_metadata_Fake;

extern mock_bap_unicast_server_cb_metadata_Fake mock_bap_unicast_server_cb_metadata_fake;
int mock_bap_unicast_server_cb_metadata(struct bt_bap_stream *, const uint8_t *, size_t,
					    struct bt_bap_ascs_rsp *);
void mock_bap_unicast_server_cb_metadata_reset(void);

typedef struct mock_bap_unicast_server_cb_disable_Fake {
	unsigned int call_count;
	struct bt_bap_stream *arg0_val;
	struct bt_bap_stream *arg0_history[FFF_ARG_HISTORY_LEN];
	struct bt_bap_ascs_rsp *arg1_val;
	struct bt_bap_ascs_rsp *arg1_history[FFF_ARG_HISTORY_LEN];
} mock_bap_unicast_server_cb_disable_Fake;

extern mock_bap_unicast_server_cb_disable_Fake mock_bap_unicast_server_cb_disable_fake;
int mock_bap_unicast_server_cb_disable(struct bt_bap_stream *, struct bt_bap_ascs_rsp *);
void mock_bap_unicast_server_cb_disable_reset(void);

typedef struct mock_bap_unicast_server_cb_stop_Fake {
	unsigned int call_count;
	struct bt_bap_stream *arg0_val;
	struct bt_bap_stream *arg0_history[FFF_ARG_HISTORY_LEN];
	struct bt_bap_ascs_rsp *arg1_val;
	struct bt_bap_ascs_rsp *arg1_history[FFF_ARG_HISTORY_LEN];
} mock_bap_unicast_server_cb_stop_Fake;

extern mock_bap_unicast_server_cb_stop_Fake mock_bap_unicast_server_cb_stop_fake;
int mock_bap_unicast_server_cb_stop(struct bt_bap_stream *, struct bt_bap_ascs_rsp *);
void mock_bap_unicast_server_cb_stop_reset(void);

typedef struct mock_bap_unicast_server_cb_release_Fake {
	unsigned int call_count;
	struct bt_bap_stream *arg0_val;
	struct bt_bap_stream *arg0_history[FFF_ARG_HISTORY_LEN];
	struct bt_bap_ascs_rsp *arg1_val;
	struct bt_bap_ascs_rsp *arg1_history[FFF_ARG_HISTORY_LEN];
} mock_bap_unicast_server_cb_release_Fake;

extern mock_bap_unicast_server_cb_release_Fake mock_bap_unicast_server_cb_release_fake;
int mock_bap_unicast_server_cb_release(struct bt_bap_stream *, struct bt_bap_ascs_rsp *);
void mock_bap_unicast_server_cb_release_reset(void);

#endif /* MOCKS_BAP_UNICAST_SERVER_H_ */
