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

static void test_bt_rand_succeeds_host_crypto_prng_disabled(void **state)
{
	(void)state;

#if defined(CONFIG_BT_HOST_CRYPTO_PRNG)
	skip();
#else
	int err;
	uint8_t buf[16];
	size_t buf_len = 16;
	uint8_t expected_args_history[] = {16};

	bt_hci_le_rand_fake.return_val = 0;

	err = bt_rand(buf, buf_len);

	expect_call_count_bt_hci_le_rand(1, expected_args_history);

	assert_int_equal(err, 0);
#endif
}

/*
 *  Test bt_rand() succeeds when psa_generate_random() succeeds on the first call while
 * 'CONFIG_BT_HOST_CRYPTO_PRNG' is enabled.
 *
 *  Constraints:
 *   - 'CONFIG_BT_HOST_CRYPTO_PRNG' is enabled
 *   - psa_generate_random() succeeds and returns 'PSA_SUCCESS' on the first call.
 *
 *  Expected behaviour:
 *   - bt_rand() returns 0 (success)
 */
static void test_psa_generate_random_succeeds_on_first_call(void **state)
{
	(void)state;

#if !defined(CONFIG_BT_HOST_CRYPTO_PRNG)
	skip();
#else
	int err;
	uint8_t buf[16];
	size_t buf_len = 16;

	psa_generate_random_fake.return_val = PSA_SUCCESS;

	err = bt_rand(buf, buf_len);

	expect_single_call_psa_generate_random(buf, buf_len);

	assert_int_equal(err, 0);
#endif
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup(test_bt_rand_succeeds_host_crypto_prng_disabled, setup),
		cmocka_unit_test_setup(test_psa_generate_random_succeeds_on_first_call, setup),
	};

	return cmocka_run_group_tests_name("bt_rand", tests, NULL, NULL);
}
