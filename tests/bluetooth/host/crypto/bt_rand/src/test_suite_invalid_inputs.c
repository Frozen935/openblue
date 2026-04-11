/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/hci_core.h"
#include "mocks/hci_core_expects.h"
#include "mocks/prng.h"
#include "mocks/prng_expects.h"

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include <cmocka.h>

#include <bluetooth/crypto.h>

#include <host/crypto.h>

static int setup(void **state)
{
	(void)state;
	reset_bt_hci_le_rand_fake();
	reset_prng_fakes();
	return 0;
}

/*
 *  Test passing NULL reference destination buffer argument
 *
 *  Constraints:
 *   - NULL reference is used as an argument for the destination buffer
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
static void test_null_dst_buf_reference(void **state)
{
	(void)state;

#if defined(CONFIG_BT_HOST_CRYPTO_PRNG)
	skip();
#else
	assert_int_equal(bt_rand(NULL, 1), -EINVAL);
#endif
}

/*
 *  Test passing a valid destination buffer reference with size 0
 *
 *  Constraints:
 *   - A valid reference is used as an argument for the destination buffer
 *   - Destination buffer size is passed as 0
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
static void test_zero_dst_buf_size_reference(void **state)
{
	(void)state;
	uint8_t buf[16];


#if defined(CONFIG_BT_HOST_CRYPTO_PRNG)
	skip();
#else
	assert_int_equal(bt_rand(buf, 0), -EINVAL);
#endif
}

/*
 *  Test bt_rand() fails when bt_hci_le_rand() fails while 'CONFIG_BT_HOST_CRYPTO_PRNG'
 *  isn't enabled.
 *
 *  Constraints:
 *   - 'CONFIG_BT_HOST_CRYPTO_PRNG' isn't enabled
 *   - bt_hci_le_rand() fails and returns a negative error code.
 *
 *  Expected behaviour:
 *   - bt_rand() returns a negative error code (failure)
 */
static void test_bt_hci_le_rand_fails(void **state)
{
	(void)state;

#if defined(CONFIG_BT_HOST_CRYPTO_PRNG)
	skip();
#else
	int err;
	uint8_t buf[16];
	size_t buf_len = 16;
	uint8_t expected_args_history[] = {16};

	bt_hci_le_rand_fake.return_val = -1;

	err = bt_rand(buf, buf_len);

	expect_call_count_bt_hci_le_rand(1, expected_args_history);

	assert_true(err < 0);
#endif
}

/*
 *  Test bt_rand() fails when psa_generate_random() fails on the first call while
 * 'CONFIG_BT_HOST_CRYPTO_PRNG' is enabled.
 *
 *  Constraints:
 *   - 'CONFIG_BT_HOST_CRYPTO_PRNG' is enabled
 *   - psa_generate_random() fails and returns '-EIO' on the first call.
 *
 *  Expected behaviour:
 *   - bt_rand() returns a negative error code '-EIO' (failure)
 */
static void test_tc_hmac_prng_generate_fails_on_first_call(void **state)
{
	(void)state;

#if !defined(CONFIG_BT_HOST_CRYPTO_PRNG)
	skip();
#else
	int err;
	uint8_t buf[16];
	size_t buf_len = 16;

	psa_generate_random_fake.return_val = -EIO;

	err = bt_rand(buf, buf_len);

	expect_single_call_psa_generate_random(buf, buf_len);

	assert_int_equal(err, -EIO);
#endif
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup(test_null_dst_buf_reference, setup),
		cmocka_unit_test_setup(test_zero_dst_buf_size_reference, setup),
		cmocka_unit_test_setup(test_bt_hci_le_rand_fails, setup),
		cmocka_unit_test_setup(test_tc_hmac_prng_generate_fails_on_first_call, setup),
	};

	return cmocka_run_group_tests_name("bt_rand_invalid_cases", tests, NULL, NULL);
}
