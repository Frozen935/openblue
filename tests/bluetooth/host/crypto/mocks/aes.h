/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <psa/crypto.h>

struct psa_import_key_fake_state {
	int call_count;
	psa_status_t return_val;
	const psa_key_attributes_t *arg0_val;
	const uint8_t *arg1_val;
	size_t arg2_val;
	mbedtls_svc_key_id_t *arg3_val;
};

struct psa_cipher_encrypt_fake_state {
	int call_count;
	psa_status_t return_val;
	mbedtls_svc_key_id_t arg0_val;
	psa_algorithm_t arg1_val;
	const uint8_t *arg2_val;
	size_t arg3_val;
	uint8_t *arg4_val;
	size_t arg5_val;
	size_t *arg6_val;
};

struct psa_destroy_key_fake_state {
	int call_count;
	psa_status_t return_val;
	mbedtls_svc_key_id_t arg0_val;
};

extern struct psa_import_key_fake_state psa_import_key_fake;
extern struct psa_cipher_encrypt_fake_state psa_cipher_encrypt_fake;
extern struct psa_destroy_key_fake_state psa_destroy_key_fake;

void reset_aes_fakes(void);
psa_status_t psa_import_key(const psa_key_attributes_t *attributes, const uint8_t *data,
			    size_t data_length, mbedtls_svc_key_id_t *id);
psa_status_t psa_cipher_encrypt(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
				const uint8_t *input, size_t input_length, uint8_t *output,
				size_t output_size, size_t *output_length);
psa_status_t psa_destroy_key(mbedtls_svc_key_id_t key);
