/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/aes.h"
#include "mocks/aes_expects.h"

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include <cmocka.h>

#include <bluetooth/crypto.h>

#include <host/crypto.h>

static int setup(void **state)
{
	(void)state;
	reset_aes_fakes();
	return 0;
}

static void test_bt_encrypt_be_succeeds(void **state)
{
	(void)state;
	int err;
	const uint8_t key[16] = {0};
	const uint8_t plaintext[16] = {0};
	uint8_t enc_data[16] = {0};

	psa_import_key_fake.return_val = PSA_SUCCESS;
	psa_cipher_encrypt_fake.return_val = PSA_SUCCESS;

	err = bt_encrypt_be(key, plaintext, enc_data);

	expect_single_call_psa_cipher_encrypt(enc_data);

	assert_int_equal(err, 0);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup(test_bt_encrypt_be_succeeds, setup),
	};

	return cmocka_run_group_tests_name("bt_encrypt_be", tests, NULL, NULL);
}
