/* main.c - Application main entry point */

/*
 * Copyright (c) 2024-2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#include <cmocka.h>

#include <bluetooth/audio/ccp.h>
#include <bluetooth/audio/tbs.h>
#include <base/utils.h>

#define DEFAULT_BEARER_NAME "test"
#define DEFAULT_BEARER_UCI  "un999"

struct ccp_call_control_server_test_fixture {
	/* Need 1 additional bearer than the max to trigger some corner cases */
	struct bt_ccp_call_control_server_bearer
		*bearers[CONFIG_BT_CCP_CALL_CONTROL_SERVER_BEARER_COUNT + 1];
};

static struct ccp_call_control_server_test_fixture *get_fixture(void **state)
{
	assert_non_null(state);
	assert_non_null(*state);

	return *state;
}

static void cleanup_fixture(struct ccp_call_control_server_test_fixture *fixture)
{
	/* We unregister from largest to lowest index, as GTBS shall be unregistered last and is
	 * always at index 0.
	 */
	for (size_t i = ARRAY_SIZE(fixture->bearers); i > 0; i--) {
		const size_t index_to_unreg = i - 1;

		if (fixture->bearers[index_to_unreg] != NULL) {
			(void)bt_ccp_call_control_server_unregister_bearer(
				fixture->bearers[index_to_unreg]);
		}

		fixture->bearers[index_to_unreg] = NULL;
	}
}

static int test_setup(void **state)
{
	struct ccp_call_control_server_test_fixture *fixture;

	fixture = calloc(1, sizeof(*fixture));
	if (fixture == NULL) {
		return -ENOMEM;
	}

	*state = fixture;

	return 0;
}

static int test_teardown(void **state)
{
	struct ccp_call_control_server_test_fixture *fixture = get_fixture(state);

	cleanup_fixture(fixture);
	free(fixture);
	*state = NULL;

	return 0;
}

static void register_default_bearer(struct ccp_call_control_server_test_fixture *fixture)
{
	const struct bt_tbs_register_param register_param = {
		.provider_name = DEFAULT_BEARER_NAME,
		.uci = DEFAULT_BEARER_UCI,
		.uri_schemes_supported = "tel",
		.gtbs = true,
		.authorization_required = false,
		.technology = BT_TBS_TECHNOLOGY_3G,
		.supported_features = 0,
	};

	assert_int_equal(
		bt_ccp_call_control_server_register_bearer(&register_param, &fixture->bearers[0]), 0);
}

static void test_ccp_call_control_server_register_bearer(void **state)
{
	struct ccp_call_control_server_test_fixture *fixture = get_fixture(state);

	register_default_bearer(fixture);
}

static void test_ccp_call_control_server_register_multiple_bearers(void **state)
{
	struct ccp_call_control_server_test_fixture *fixture = get_fixture(state);

	if (CONFIG_BT_CCP_CALL_CONTROL_SERVER_BEARER_COUNT == 1) {
		skip();
	}

	register_default_bearer(fixture);

	for (int i = 1; i < CONFIG_BT_CCP_CALL_CONTROL_SERVER_BEARER_COUNT; i++) {
		struct bt_tbs_register_param register_param = {
			.provider_name = "test",
			.uci = DEFAULT_BEARER_UCI,
			.uri_schemes_supported = "tel",
			.gtbs = false,
			.authorization_required = false,
			.technology = BT_TBS_TECHNOLOGY_3G,
			.supported_features = 0,
		};

		assert_int_equal(
			bt_ccp_call_control_server_register_bearer(&register_param, &fixture->bearers[i]),
			0);
	}
}

static void test_ccp_call_control_server_register_bearer_inval_null_param(void **state)
{
	struct ccp_call_control_server_test_fixture *fixture = get_fixture(state);

	assert_int_equal(
		bt_ccp_call_control_server_register_bearer(NULL, &fixture->bearers[0]), -EINVAL);
}

static void test_ccp_call_control_server_register_bearer_inval_null_bearer(void **state)
{
	const struct bt_tbs_register_param register_param = {
		.provider_name = "test",
		.uci = DEFAULT_BEARER_UCI,
		.uri_schemes_supported = "tel",
		.gtbs = true,
		.authorization_required = false,
		.technology = BT_TBS_TECHNOLOGY_3G,
		.supported_features = 0,
	};

	(void)get_fixture(state);

	assert_int_equal(bt_ccp_call_control_server_register_bearer(&register_param, NULL), -EINVAL);
}

static void test_ccp_call_control_server_register_bearer_inval_no_gtbs(void **state)
{
	struct ccp_call_control_server_test_fixture *fixture = get_fixture(state);
	const struct bt_tbs_register_param register_param = {
		.provider_name = "test",
		.uci = DEFAULT_BEARER_UCI,
		.uri_schemes_supported = "tel",
		.gtbs = false,
		.authorization_required = false,
		.technology = BT_TBS_TECHNOLOGY_3G,
		.supported_features = 0,
	};

	assert_int_equal(
		bt_ccp_call_control_server_register_bearer(&register_param, &fixture->bearers[0]),
		-EAGAIN);
}

static void test_ccp_call_control_server_register_bearer_inval_double_gtbs(void **state)
{
	struct ccp_call_control_server_test_fixture *fixture = get_fixture(state);
	const struct bt_tbs_register_param register_param = {
		.provider_name = "test",
		.uci = DEFAULT_BEARER_UCI,
		.uri_schemes_supported = "tel",
		.gtbs = true,
		.authorization_required = false,
		.technology = BT_TBS_TECHNOLOGY_3G,
		.supported_features = 0,
	};

	if (CONFIG_BT_CCP_CALL_CONTROL_SERVER_BEARER_COUNT == 1) {
		skip();
	}

	register_default_bearer(fixture);

	assert_int_equal(
		bt_ccp_call_control_server_register_bearer(&register_param, &fixture->bearers[1]),
		-EALREADY);
}

static void test_ccp_call_control_server_register_bearer_inval_cnt(void **state)
{
	struct ccp_call_control_server_test_fixture *fixture = get_fixture(state);
	const struct bt_tbs_register_param register_param = {
		.provider_name = "test",
		.uci = DEFAULT_BEARER_UCI,
		.uri_schemes_supported = "tel",
		.gtbs = false,
		.authorization_required = false,
		.technology = BT_TBS_TECHNOLOGY_3G,
		.supported_features = 0,
	};

	if (CONFIG_BT_CCP_CALL_CONTROL_SERVER_BEARER_COUNT == 1) {
		skip();
	}

	register_default_bearer(fixture);

	for (int i = 1; i < CONFIG_BT_CCP_CALL_CONTROL_SERVER_BEARER_COUNT; i++) {
		assert_int_equal(
			bt_ccp_call_control_server_register_bearer(&register_param, &fixture->bearers[i]),
			0);
	}

	assert_int_equal(bt_ccp_call_control_server_register_bearer(
				 &register_param,
				 &fixture->bearers[CONFIG_BT_CCP_CALL_CONTROL_SERVER_BEARER_COUNT]),
			 -ENOMEM);
}

static void test_ccp_call_control_server_unregister_bearer(void **state)
{
	struct ccp_call_control_server_test_fixture *fixture = get_fixture(state);

	register_default_bearer(fixture);

	assert_int_equal(bt_ccp_call_control_server_unregister_bearer(fixture->bearers[0]), 0);
}

static void test_ccp_call_control_server_unregister_bearer_inval_double_unregister(void **state)
{
	struct ccp_call_control_server_test_fixture *fixture = get_fixture(state);

	register_default_bearer(fixture);

	assert_int_equal(bt_ccp_call_control_server_unregister_bearer(fixture->bearers[0]), 0);
	assert_int_equal(bt_ccp_call_control_server_unregister_bearer(fixture->bearers[0]),
			 -EALREADY);

	fixture->bearers[0] = NULL;
}

static void test_ccp_call_control_server_unregister_bearer_inval_null_bearer(void **state)
{
	(void)get_fixture(state);

	assert_int_equal(bt_ccp_call_control_server_unregister_bearer(NULL), -EINVAL);
}

static void test_bt_ccp_call_control_server_set_bearer_provider_name(void **state)
{
	struct ccp_call_control_server_test_fixture *fixture = get_fixture(state);
	char res_bearer_name[CONFIG_BT_CCP_CALL_CONTROL_SERVER_PROVIDER_NAME_MAX_LENGTH + 1];
	const char *new_bearer_name = "New bearer name";

	register_default_bearer(fixture);

	assert_int_equal(
		bt_ccp_call_control_server_set_bearer_provider_name(fixture->bearers[0],
						   new_bearer_name),
		0);
	assert_int_equal(bt_ccp_call_control_server_get_bearer_provider_name(
				 fixture->bearers[0], res_bearer_name, sizeof(res_bearer_name)),
			 0);
	assert_string_equal(new_bearer_name, res_bearer_name);
}

static void test_bt_ccp_call_control_server_set_bearer_provider_name_inval_not_registered(
	void **state)
{
	struct ccp_call_control_server_test_fixture *fixture = get_fixture(state);
	const char *new_bearer_name = "New bearer name";

	/* Register and unregister bearer to get a valid pointer but where it is unregistered. */
	register_default_bearer(fixture);
	assert_int_equal(bt_ccp_call_control_server_unregister_bearer(fixture->bearers[0]), 0);

	assert_int_equal(
		bt_ccp_call_control_server_set_bearer_provider_name(fixture->bearers[0],
						   new_bearer_name),
		-EFAULT);
}

static void test_bt_ccp_call_control_server_set_bearer_provider_name_inval_null_bearer(
	void **state)
{
	struct ccp_call_control_server_test_fixture *fixture = get_fixture(state);
	const char *new_bearer_name = "New bearer name";

	register_default_bearer(fixture);

	assert_int_equal(bt_ccp_call_control_server_set_bearer_provider_name(NULL, new_bearer_name),
			 -EINVAL);
}

static void test_bt_ccp_call_control_server_set_bearer_provider_name_inval_null_name(
	void **state)
{
	struct ccp_call_control_server_test_fixture *fixture = get_fixture(state);

	register_default_bearer(fixture);

	assert_int_equal(bt_ccp_call_control_server_set_bearer_provider_name(fixture->bearers[0], NULL),
			 -EINVAL);
}

static void test_bt_ccp_call_control_server_set_bearer_provider_name_inval_empty_name(
	void **state)
{
	struct ccp_call_control_server_test_fixture *fixture = get_fixture(state);
	const char *inval_bearer_name = "";

	register_default_bearer(fixture);

	assert_int_equal(bt_ccp_call_control_server_set_bearer_provider_name(
				 fixture->bearers[0], inval_bearer_name),
			 -EINVAL);
}

static void test_bt_ccp_call_control_server_set_bearer_provider_name_inval_long_name(
	void **state)
{
	struct ccp_call_control_server_test_fixture *fixture = get_fixture(state);
	char inval_bearer_name[CONFIG_BT_CCP_CALL_CONTROL_SERVER_PROVIDER_NAME_MAX_LENGTH + 2];

	for (size_t i = 0; i < ARRAY_SIZE(inval_bearer_name) - 1; i++) {
		inval_bearer_name[i] = 'a';
	}
	inval_bearer_name[ARRAY_SIZE(inval_bearer_name) - 1] = '\0';

	register_default_bearer(fixture);

	assert_int_equal(bt_ccp_call_control_server_set_bearer_provider_name(
				 fixture->bearers[0], inval_bearer_name),
			 -EINVAL);
}

static void test_bt_ccp_call_control_server_get_bearer_provider_name(void **state)
{
	struct ccp_call_control_server_test_fixture *fixture = get_fixture(state);
	char res_bearer_name[CONFIG_BT_CCP_CALL_CONTROL_SERVER_PROVIDER_NAME_MAX_LENGTH + 1];

	register_default_bearer(fixture);

	assert_int_equal(bt_ccp_call_control_server_get_bearer_provider_name(
				 fixture->bearers[0], res_bearer_name, sizeof(res_bearer_name)),
			 0);
	assert_string_equal(DEFAULT_BEARER_NAME, res_bearer_name);
}

static void test_bt_ccp_call_control_server_get_bearer_provider_name_inval_not_registered(
	void **state)
{
	struct ccp_call_control_server_test_fixture *fixture = get_fixture(state);
	char res_bearer_name[CONFIG_BT_CCP_CALL_CONTROL_SERVER_PROVIDER_NAME_MAX_LENGTH + 1];

	/* Register and unregister bearer to get a valid pointer but where it is unregistered. */
	register_default_bearer(fixture);
	assert_int_equal(bt_ccp_call_control_server_unregister_bearer(fixture->bearers[0]), 0);

	assert_int_equal(bt_ccp_call_control_server_get_bearer_provider_name(
				 fixture->bearers[0], res_bearer_name, sizeof(res_bearer_name)),
			 -EFAULT);
}

static void test_bt_ccp_call_control_server_get_bearer_provider_name_inval_null_bearer(
	void **state)
{
	struct ccp_call_control_server_test_fixture *fixture = get_fixture(state);
	char res_bearer_name[CONFIG_BT_CCP_CALL_CONTROL_SERVER_PROVIDER_NAME_MAX_LENGTH + 1];

	register_default_bearer(fixture);

	assert_int_equal(bt_ccp_call_control_server_get_bearer_provider_name(
				 NULL, res_bearer_name, sizeof(res_bearer_name)),
			 -EINVAL);
}

static void test_bt_ccp_call_control_server_get_bearer_provider_name_inval_null_name(void **state)
{
	struct ccp_call_control_server_test_fixture *fixture = get_fixture(state);

	register_default_bearer(fixture);

	assert_int_equal(bt_ccp_call_control_server_get_bearer_provider_name(fixture->bearers[0], NULL,
							     0),
			 -EINVAL);
}

static void test_bt_ccp_call_control_server_get_bearer_provider_name_inval_size_0(void **state)
{
	struct ccp_call_control_server_test_fixture *fixture = get_fixture(state);
	char res_bearer_name[CONFIG_BT_CCP_CALL_CONTROL_SERVER_PROVIDER_NAME_MAX_LENGTH + 1];

	register_default_bearer(fixture);

	assert_int_equal(bt_ccp_call_control_server_get_bearer_provider_name(
				 fixture->bearers[0], res_bearer_name, 0),
			 -ENOMEM);
}

static void test_bt_ccp_call_control_server_get_bearer_provider_name_inval_small_size(
	void **state)
{
	struct ccp_call_control_server_test_fixture *fixture = get_fixture(state);
	char res_bearer_name[CONFIG_BT_CCP_CALL_CONTROL_SERVER_PROVIDER_NAME_MAX_LENGTH + 1];

	register_default_bearer(fixture);

	assert_int_equal(bt_ccp_call_control_server_get_bearer_provider_name(
				 fixture->bearers[0], res_bearer_name, 1),
			 -ENOMEM);
}

static void test_bt_ccp_call_control_server_get_bearer_uci(void **state)
{
	struct ccp_call_control_server_test_fixture *fixture = get_fixture(state);
	char res_bearer_uci[BT_TBS_MAX_UCI_SIZE];

	register_default_bearer(fixture);

	assert_int_equal(bt_ccp_call_control_server_get_bearer_uci(fixture->bearers[0], res_bearer_uci),
			 0);
	assert_string_equal(DEFAULT_BEARER_UCI, res_bearer_uci);
}

static void test_bt_ccp_call_control_server_get_bearer_uci_inval_not_registered(void **state)
{
	struct ccp_call_control_server_test_fixture *fixture = get_fixture(state);
	char res_bearer_uci[BT_TBS_MAX_UCI_SIZE];

	/* Register and unregister bearer to get a valid pointer but where it is unregistered. */
	register_default_bearer(fixture);
	assert_int_equal(bt_ccp_call_control_server_unregister_bearer(fixture->bearers[0]), 0);

	assert_int_equal(bt_ccp_call_control_server_get_bearer_uci(fixture->bearers[0], res_bearer_uci),
			 -EFAULT);
}

static void test_bt_ccp_call_control_server_get_bearer_uci_inval_null_bearer(void **state)
{
	struct ccp_call_control_server_test_fixture *fixture = get_fixture(state);
	char res_bearer_uci[BT_TBS_MAX_UCI_SIZE];

	register_default_bearer(fixture);

	assert_int_equal(bt_ccp_call_control_server_get_bearer_uci(NULL, res_bearer_uci), -EINVAL);
}

static void test_bt_ccp_call_control_server_get_bearer_uci_inval_null_uci(void **state)
{
	struct ccp_call_control_server_test_fixture *fixture = get_fixture(state);

	register_default_bearer(fixture);

	assert_int_equal(bt_ccp_call_control_server_get_bearer_uci(fixture->bearers[0], NULL), -EINVAL);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(test_ccp_call_control_server_register_bearer,
					       test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(test_ccp_call_control_server_register_multiple_bearers,
					       test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(
			test_ccp_call_control_server_register_bearer_inval_null_param, test_setup,
			test_teardown),
		cmocka_unit_test_setup_teardown(
			test_ccp_call_control_server_register_bearer_inval_null_bearer, test_setup,
			test_teardown),
		cmocka_unit_test_setup_teardown(test_ccp_call_control_server_register_bearer_inval_no_gtbs,
					       test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(
			test_ccp_call_control_server_register_bearer_inval_double_gtbs, test_setup,
			test_teardown),
		cmocka_unit_test_setup_teardown(test_ccp_call_control_server_register_bearer_inval_cnt,
					       test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(test_ccp_call_control_server_unregister_bearer,
					       test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(
			test_ccp_call_control_server_unregister_bearer_inval_double_unregister,
			test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(
			test_ccp_call_control_server_unregister_bearer_inval_null_bearer,
			test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(test_bt_ccp_call_control_server_set_bearer_provider_name,
					       test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(
			test_bt_ccp_call_control_server_set_bearer_provider_name_inval_not_registered,
			test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(
			test_bt_ccp_call_control_server_set_bearer_provider_name_inval_null_bearer,
			test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(
			test_bt_ccp_call_control_server_set_bearer_provider_name_inval_null_name,
			test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(
			test_bt_ccp_call_control_server_set_bearer_provider_name_inval_empty_name,
			test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(
			test_bt_ccp_call_control_server_set_bearer_provider_name_inval_long_name,
			test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(test_bt_ccp_call_control_server_get_bearer_provider_name,
					       test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(
			test_bt_ccp_call_control_server_get_bearer_provider_name_inval_not_registered,
			test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(
			test_bt_ccp_call_control_server_get_bearer_provider_name_inval_null_bearer,
			test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(
			test_bt_ccp_call_control_server_get_bearer_provider_name_inval_null_name,
			test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(
			test_bt_ccp_call_control_server_get_bearer_provider_name_inval_size_0,
			test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(
			test_bt_ccp_call_control_server_get_bearer_provider_name_inval_small_size,
			test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(test_bt_ccp_call_control_server_get_bearer_uci,
					       test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(
			test_bt_ccp_call_control_server_get_bearer_uci_inval_not_registered,
			test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(
			test_bt_ccp_call_control_server_get_bearer_uci_inval_null_bearer,
			test_setup, test_teardown),
		cmocka_unit_test_setup_teardown(
			test_bt_ccp_call_control_server_get_bearer_uci_inval_null_uci,
			test_setup, test_teardown),
	};

	return cmocka_run_group_tests_name("bt_audio_ccp_call_control_server", tests, NULL, NULL);
}
