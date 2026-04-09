/* Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OPENBLUE_PSA_KEY_IDS_H_
#define OPENBLUE_PSA_KEY_IDS_H_

/**
 * @file psa/key_ids.h
 *
 * @brief Registry of persistent PSA key ID ranges used inside openblue.
 *
 * Different subsystems may persist keys through the PSA Crypto API. To avoid
 * collisions, each subsystem must use a dedicated key ID range allocated from
 * a central registry.
 */

#include <psa/crypto.h>

/** PSA key ID range reserved for Bluetooth Mesh persistent keys. */
#define OPENBLUE_PSA_BT_MESH_KEY_ID_RANGE_BEGIN (psa_key_id_t)0x20000000
#define OPENBLUE_PSA_BT_MESH_KEY_ID_RANGE_SIZE  0xC000 /* 48 Ki */

#endif /* OPENBLUE_PSA_KEY_IDS_H_ */
