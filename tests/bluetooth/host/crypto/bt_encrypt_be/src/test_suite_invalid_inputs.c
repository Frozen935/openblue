/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/aes.h"

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

/*
 *  Test passing NULL reference for the key argument
 *
 *  Constraints:
 *   - NULL reference is used for the key argument
 *   - Valid references are used for the other arguments
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
static void test_null_key_reference(void **state)
{
	(void)state;
	const uint8_t plaintext[16] = {0};
	uint8_t enc_data[16] = {0};

	assert_int_equal(bt_encrypt_be(NULL, plaintext, enc_data), -EINVAL);
}

/*
 *  Test passing NULL reference for the plain text argument
 *
 *  Constraints:
 *   - NULL reference is used for the plain text argument
 *   - Valid references are used for the other arguments
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
static void test_null_plaintext_reference(void **state)
{
	(void)state;
	const uint8_t key[16] = {0};
	uint8_t enc_data[16] = {0};

	assert_int_equal(bt_encrypt_be(key, NULL, enc_data), -EINVAL);
}

/*
 *  Test passing NULL reference for the encrypted data destination buffer argument
 *
 *  Constraints:
 *   - NULL reference is used for the encrypted data destination buffer argument
 *   - Valid references are used for the other arguments
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
static void test_null_enc_data_reference(void **state)
{
	(void)state;
	const uint8_t key[16] = {0};
	const uint8_t plaintext[16] = {0};

	assert_int_equal(bt_encrypt_be(key, plaintext, NULL), -EINVAL);
}

/*
 *  Test bt_encrypt_le() fails when tc_aes128_set_encrypt_key() fails
 *
 *  Constraints:
 *   - psa_import_key() fails and returns 'PSA_ERROR_GENERIC_ERROR'.
 *
 *  Expected behaviour:
 *   - bt_encrypt_le() returns a negative error code '-EINVAL' (failure)
 */
static void test_psa_import_key_fails(void **state)
{
	(void)state;
	int err;
	const uint8_t key[16] = {0};
	const uint8_t plaintext[16] = {0};
	uint8_t enc_data[16] = {0};

	psa_import_key_fake.return_val = PSA_ERROR_GENERIC_ERROR;

	err = bt_encrypt_le(key, plaintext, enc_data);

	assert_int_equal(err, -EINVAL);
}

/*
 *  Test bt_encrypt_le() fails when tc_aes_encrypt() fails
 *
 *  Constraints:
 *   - psa_import_key() succeeds and returns 'PSA_SUCCESS'.
 *   - psa_cipher_encrypt() fails and returns 'PSA_ERROR_GENERIC_ERROR'.
 *
 *  Expected behaviour:
 *   - bt_encrypt_le() returns a negative error code '-EINVAL' (failure)
 */
static void test_psa_cipher_encrypt_fails(void **state)
{
	(void)state;
	int err;
	const uint8_t key[16] = {0};
	const uint8_t plaintext[16] = {0};
	uint8_t enc_data[16] = {0};

	psa_import_key_fake.return_val = PSA_SUCCESS;
	psa_cipher_encrypt_fake.return_val = -EINVAL;

	err = bt_encrypt_be(key, plaintext, enc_data);

	assert_int_equal(err, -EIO);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup(test_null_key_reference, setup),
		cmocka_unit_test_setup(test_null_plaintext_reference, setup),
		cmocka_unit_test_setup(test_null_enc_data_reference, setup),
		cmocka_unit_test_setup(test_psa_import_key_fails, setup),
		cmocka_unit_test_setup(test_psa_cipher_encrypt_fails, setup),
	};

	return cmocka_run_group_tests_name("bt_encrypt_be_invalid_cases", tests, NULL, NULL);
}
