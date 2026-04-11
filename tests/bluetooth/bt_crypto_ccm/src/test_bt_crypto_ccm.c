/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include <cmocka.h>
#include <mbedtls/aes.h>

#include <bluetooth/crypto.h>

#include "common/bt_str.h"

#include "test_vectors.h"

int bt_encrypt_be(const uint8_t key[16], const uint8_t plaintext[16], uint8_t enc_data[16])
{
	mbedtls_aes_context ctx;
	int rc;

	if (key == NULL || plaintext == NULL || enc_data == NULL) {
		return -EINVAL;
	}

	mbedtls_aes_init(&ctx);
	rc = mbedtls_aes_setkey_enc(&ctx, key, 128);
	if (rc != 0) {
		mbedtls_aes_free(&ctx);
		return -EINVAL;
	}

	rc = mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_ENCRYPT, plaintext, enc_data);
	mbedtls_aes_free(&ctx);

	return rc == 0 ? 0 : -EIO;
}

static void test_result_rfc_test_vectors(void **state)
{
	(void)state;

	for (int i = 0; i < NUMBER_OF_TEST; i++) {
		int err;
		struct test_data *p = input_packets[i];

		p->expected_output_len = p->input_len + p->mic_len;

		const uint8_t *aad = p->input;

		uint8_t *encrypted_data =
			(uint8_t *)malloc(p->expected_output_len * sizeof(uint8_t));

		memcpy(encrypted_data, p->input, p->input_len);

		err = bt_ccm_encrypt(p->key, p->nonce, &p->input[p->aad_len],
				     p->input_len - p->aad_len, aad, p->aad_len,
				     &encrypted_data[p->aad_len], p->mic_len);
		assert_int_equal(err, 0);

		assert_memory_equal(encrypted_data, p->expected_output, p->expected_output_len);

		uint8_t *decrypted_data = (uint8_t *)malloc(p->input_len * sizeof(uint8_t));

		memcpy(decrypted_data, encrypted_data, p->aad_len);

		err = bt_ccm_decrypt(p->key, p->nonce, &encrypted_data[p->aad_len],
				     p->input_len - p->aad_len, aad, p->aad_len,
				     &decrypted_data[p->aad_len], p->mic_len);
		assert_int_equal(err, 0);

		assert_memory_equal(decrypted_data, p->input, p->input_len);

		free(encrypted_data);
		free(decrypted_data);
	}
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_result_rfc_test_vectors),
	};

	return cmocka_run_group_tests_name("bt_crypto_ccm", tests, NULL, NULL);
}
