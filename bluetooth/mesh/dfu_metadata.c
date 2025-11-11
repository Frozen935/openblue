/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <bluetooth/mesh.h>
#include "crypto.h"
#include "access.h"

int bt_mesh_dfu_metadata_decode(struct bt_buf_simple *buf,
				struct bt_mesh_dfu_metadata *metadata)
{
	if (buf->len < 12) {
		return -EMSGSIZE;
	}

	metadata->fw_ver.major = bt_buf_simple_pull_u8(buf);
	metadata->fw_ver.minor = bt_buf_simple_pull_u8(buf);
	metadata->fw_ver.revision = bt_buf_simple_pull_le16(buf);
	metadata->fw_ver.build_num = bt_buf_simple_pull_le32(buf);
	metadata->fw_size = bt_buf_simple_pull_le24(buf);
	metadata->fw_core_type = bt_buf_simple_pull_u8(buf);

	if (metadata->fw_core_type & BT_MESH_DFU_FW_CORE_TYPE_APP) {
		if (buf->len < 6) {
			return -EMSGSIZE;
		}

		metadata->comp_hash = bt_buf_simple_pull_le32(buf);
		metadata->elems = bt_buf_simple_pull_le16(buf);
	}

	metadata->user_data = buf->len > 0 ? buf->data : NULL;
	metadata->user_data_len = buf->len;

	return 0;
}


int bt_mesh_dfu_metadata_encode(const struct bt_mesh_dfu_metadata *metadata,
				struct bt_buf_simple *buf)
{
	size_t md_len_min = 12 + metadata->user_data_len;

	if (metadata->fw_core_type & BT_MESH_DFU_FW_CORE_TYPE_APP) {
		md_len_min += 6;
	}

	if (bt_buf_simple_tailroom(buf) < md_len_min) {
		return -EMSGSIZE;
	}

	bt_buf_simple_add_u8(buf, metadata->fw_ver.major);
	bt_buf_simple_add_u8(buf, metadata->fw_ver.minor);
	bt_buf_simple_add_le16(buf, metadata->fw_ver.revision);
	bt_buf_simple_add_le32(buf, metadata->fw_ver.build_num);
	bt_buf_simple_add_le24(buf, metadata->fw_size);
	bt_buf_simple_add_u8(buf, metadata->fw_core_type);
	bt_buf_simple_add_le32(buf, metadata->comp_hash);
	bt_buf_simple_add_le16(buf, metadata->elems);

	if (metadata->user_data_len > 0) {
		bt_buf_simple_add_mem(buf, metadata->user_data, metadata->user_data_len);
	}

	return 0;
}

int bt_mesh_dfu_metadata_comp_hash_get(struct bt_buf_simple *buf, uint8_t *key, uint32_t *hash)
{
	uint8_t mac[16];
	int err;
	struct bt_mesh_sg sg = {.data = buf->data, .len = buf->len};

	err = bt_mesh_aes_cmac_raw_key(key, &sg, 1, mac);
	if (err) {
		return err;
	}

	*hash = sys_get_le32(mac);

	return 0;
}

int bt_mesh_dfu_metadata_comp_hash_local_get(uint8_t *key, uint32_t *hash)
{
	BT_BUF_SIMPLE_DEFINE(buf, BT_MESH_TX_SDU_MAX);
	int err;

	err = bt_mesh_comp_data_get_page_0(&buf, 0);
	if (err) {
		return err;
	}

	err = bt_mesh_dfu_metadata_comp_hash_get(&buf, key, hash);
	return err;
}
