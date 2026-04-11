/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/aes.h"

struct psa_import_key_fake_state psa_import_key_fake;
struct psa_cipher_encrypt_fake_state psa_cipher_encrypt_fake;
struct psa_destroy_key_fake_state psa_destroy_key_fake;

void reset_aes_fakes(void)
{
	psa_import_key_fake.call_count = 0;
	psa_import_key_fake.return_val = PSA_SUCCESS;
	psa_import_key_fake.arg0_val = NULL;
	psa_import_key_fake.arg1_val = NULL;
	psa_import_key_fake.arg2_val = 0;
	psa_import_key_fake.arg3_val = NULL;

	psa_cipher_encrypt_fake.call_count = 0;
	psa_cipher_encrypt_fake.return_val = PSA_SUCCESS;
	psa_cipher_encrypt_fake.arg0_val = MBEDTLS_SVC_KEY_ID_INIT;
	psa_cipher_encrypt_fake.arg1_val = 0;
	psa_cipher_encrypt_fake.arg2_val = NULL;
	psa_cipher_encrypt_fake.arg3_val = 0;
	psa_cipher_encrypt_fake.arg4_val = NULL;
	psa_cipher_encrypt_fake.arg5_val = 0;
	psa_cipher_encrypt_fake.arg6_val = NULL;

	psa_destroy_key_fake.call_count = 0;
	psa_destroy_key_fake.return_val = PSA_SUCCESS;
	psa_destroy_key_fake.arg0_val = MBEDTLS_SVC_KEY_ID_INIT;
}

psa_status_t psa_import_key(const psa_key_attributes_t *attributes, const uint8_t *data,
			    size_t data_length, mbedtls_svc_key_id_t *id)
{
	psa_import_key_fake.call_count++;
	psa_import_key_fake.arg0_val = attributes;
	psa_import_key_fake.arg1_val = data;
	psa_import_key_fake.arg2_val = data_length;
	psa_import_key_fake.arg3_val = id;
	if (id != NULL) {
		*id = 1;
	}
	return psa_import_key_fake.return_val;
}

psa_status_t psa_cipher_encrypt(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
				const uint8_t *input, size_t input_length, uint8_t *output,
				size_t output_size, size_t *output_length)
{
	psa_cipher_encrypt_fake.call_count++;
	psa_cipher_encrypt_fake.arg0_val = key;
	psa_cipher_encrypt_fake.arg1_val = alg;
	psa_cipher_encrypt_fake.arg2_val = input;
	psa_cipher_encrypt_fake.arg3_val = input_length;
	psa_cipher_encrypt_fake.arg4_val = output;
	psa_cipher_encrypt_fake.arg5_val = output_size;
	psa_cipher_encrypt_fake.arg6_val = output_length;
	if (output_length != NULL) {
		*output_length = 16;
	}
	return psa_cipher_encrypt_fake.return_val;
}

psa_status_t psa_destroy_key(mbedtls_svc_key_id_t key)
{
	psa_destroy_key_fake.call_count++;
	psa_destroy_key_fake.arg0_val = key;
	return psa_destroy_key_fake.return_val;
}
