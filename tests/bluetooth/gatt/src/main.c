/* main.c - Application main entry point */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>

#include <cmocka.h>

#include <bluetooth/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt.h>

#include <bt_stack_init.h>

/* Custom Service Variables */
static const struct bt_uuid_128 test_uuid = BT_UUID_INIT_128(
	0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);
static const struct bt_uuid_128 test_chrc_uuid = BT_UUID_INIT_128(
	0xf2, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static const uint8_t default_test_value[] = { 'T', 'e', 's', 't', '\0' };
static uint8_t test_value[] = { 'T', 'e', 's', 't', '\0' };

static const struct bt_uuid_128 test1_uuid = BT_UUID_INIT_128(
	0xf4, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static const struct bt_uuid_128 test1_nfy_uuid = BT_UUID_INIT_128(
	0xf5, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static uint8_t nfy_enabled;

static void test1_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	(void)attr;
	nfy_enabled = (value == BT_GATT_CCC_NOTIFY) ? 1U : 0U;
}

static ssize_t test1_ccc_cfg_write_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				      uint16_t value)
{
	(void)conn;
	(void)attr;
	return sizeof(value);
}

static ssize_t read_test(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, strlen(value));
}

static ssize_t write_test(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  const void *buf, uint16_t len, uint16_t offset,
			  uint8_t flags)
{
	uint8_t *value = attr->user_data;

	(void)conn;
	(void)flags;

	if (offset + len > sizeof(test_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);

	return len;
}

static struct bt_gatt_attr test_attrs[] = {
	/* Vendor Primary Service Declaration */
	BT_GATT_PRIMARY_SERVICE(&test_uuid),

	BT_GATT_CHARACTERISTIC(&test_chrc_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ_AUTHEN | BT_GATT_PERM_WRITE_AUTHEN,
			       read_test, write_test, test_value),
};

static struct bt_gatt_service test_svc = BT_GATT_SERVICE(test_attrs);

static struct bt_gatt_attr test1_attrs[] = {
	/* Vendor Primary Service Declaration */
	BT_GATT_PRIMARY_SERVICE(&test1_uuid),

	BT_GATT_CHARACTERISTIC(&test1_nfy_uuid.uuid,
			       BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE,
			       NULL, NULL, &nfy_enabled),
	BT_GATT_CCC(test1_ccc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
};

static struct bt_gatt_service test1_svc = BT_GATT_SERVICE(test1_attrs);

static void reset_test_state(void)
{
	memcpy(test_value, default_test_value, sizeof(test_value));
	nfy_enabled = 0U;
	(void)bt_gatt_service_unregister(&test1_svc);
	(void)bt_gatt_service_unregister(&test_svc);
}

static int test_group_setup(void **state)
{
	int err;

	(void)state;
	err = bt_stack_init_once();
	assert_int_equal(err, 0);
	reset_test_state();

	return 0;
}

static int test_case_setup(void **state)
{
	(void)state;
	reset_test_state();

	return 0;
}

static int test_case_teardown(void **state)
{
	(void)state;
	reset_test_state();

	return 0;
}

static int test_group_teardown(void **state)
{
	(void)state;
	reset_test_state();

	return 0;
}

static void test_gatt_register(void **state)
{
	(void)state;

	/* Ensure our test services are not already registered */
	bt_gatt_service_unregister(&test_svc);
	bt_gatt_service_unregister(&test1_svc);

	/* Attempt to register services */
	assert_int_equal(bt_gatt_service_register(&test_svc), 0);
	assert_int_equal(bt_gatt_service_register(&test1_svc), 0);

	/* Attempt to register already registered services */
	assert_true(bt_gatt_service_register(&test_svc) != 0);
	assert_true(bt_gatt_service_register(&test1_svc) != 0);
}

static void test_gatt_unregister(void **state)
{
	(void)state;

	assert_int_equal(bt_gatt_service_register(&test_svc), 0);
	assert_int_equal(bt_gatt_service_register(&test1_svc), 0);

	/* Attempt to unregister last */
	assert_int_equal(bt_gatt_service_unregister(&test1_svc), 0);
	assert_int_equal(bt_gatt_service_register(&test1_svc), 0);

	/* Attempt to unregister first/middle */
	assert_int_equal(bt_gatt_service_unregister(&test_svc), 0);
	assert_int_equal(bt_gatt_service_register(&test_svc), 0);

	/* Attempt to unregister all reverse order */
	assert_int_equal(bt_gatt_service_unregister(&test1_svc), 0);
	assert_int_equal(bt_gatt_service_unregister(&test_svc), 0);

	assert_int_equal(bt_gatt_service_register(&test_svc), 0);
	assert_int_equal(bt_gatt_service_register(&test1_svc), 0);

	/* Attempt to unregister all same order */
	assert_int_equal(bt_gatt_service_unregister(&test_svc), 0);
	assert_int_equal(bt_gatt_service_unregister(&test1_svc), 0);
}

/* Test that a service A can be re-registered after registering it once, unregistering it, and then
 * registering another service B.
 * No pre-allocated handles. Repeat the process multiple times.
 */
static void test_gatt_reregister(void **state)
{
	struct bt_gatt_attr local_test_attrs[] = {
		/* Vendor Primary Service Declaration */
		BT_GATT_PRIMARY_SERVICE(&test_uuid),

		BT_GATT_CHARACTERISTIC(&test_chrc_uuid.uuid,
				       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
				       BT_GATT_PERM_READ_AUTHEN | BT_GATT_PERM_WRITE_AUTHEN,
				       read_test, write_test, test_value),
	};

	struct bt_gatt_attr local_test1_attrs[] = {
		/* Vendor Primary Service Declaration */
		BT_GATT_PRIMARY_SERVICE(&test1_uuid),

		BT_GATT_CHARACTERISTIC(&test1_nfy_uuid.uuid,
				       BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE,
				       NULL, NULL, &nfy_enabled),
		BT_GATT_CCC(test1_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	};
	struct bt_gatt_service local_test_svc = BT_GATT_SERVICE(local_test_attrs);
	struct bt_gatt_service local_test1_svc = BT_GATT_SERVICE(local_test1_attrs);

	(void)state;

	/* Check that the procedure is successful for a few iterations to verify stability and
 	 * detect residual state or memory issues.
 	 */
	for (int i = 0; i < 10; i++) {
		/* Check that the handles are initially 0x0000 before registering the service */
		for (int j = 0; j < local_test_svc.attr_count; j++) {
			assert_int_equal(local_test_svc.attrs[j].handle, 0x0000);
		}

		assert_int_equal(bt_gatt_service_register(&local_test_svc), 0);
		assert_int_equal(bt_gatt_service_unregister(&local_test_svc), 0);

		/* Check that the handles are the same as before registering the service */
		for (int j = 0; j < local_test_svc.attr_count; j++) {
			assert_int_equal(local_test_svc.attrs[j].handle, 0x0000);
		}

		assert_int_equal(bt_gatt_service_register(&local_test1_svc), 0);
		assert_int_equal(bt_gatt_service_register(&local_test_svc), 0);

		/* Clean up */
		assert_int_equal(bt_gatt_service_unregister(&local_test_svc), 0);
		assert_int_equal(bt_gatt_service_unregister(&local_test1_svc), 0);
	}
}

/* Test that a service A can be re-registered after registering it once, unregistering it, and then
 * registering another service B.
 * Service A and B both have pre-allocated handles for their attributes.
 * Check that pre-allocated handles are the same after unregistering as they were before
 * registering the service.
 */
static void test_gatt_reregister_pre_allocated_handles(void **state)
{
	struct bt_gatt_attr local_test_attrs[] = {
		/* Vendor Primary Service Declaration */
		BT_GATT_PRIMARY_SERVICE(&test_uuid),

		BT_GATT_CHARACTERISTIC(&test_chrc_uuid.uuid,
				       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
				       BT_GATT_PERM_READ_AUTHEN | BT_GATT_PERM_WRITE_AUTHEN,
				       read_test, write_test, test_value),
	};

	struct bt_gatt_attr local_test1_attrs[] = {
		/* Vendor Primary Service Declaration */
		BT_GATT_PRIMARY_SERVICE(&test1_uuid),

		BT_GATT_CHARACTERISTIC(&test1_nfy_uuid.uuid,
				       BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE,
				       NULL, NULL, &nfy_enabled),
		BT_GATT_CCC(test1_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	};

	struct bt_gatt_service prealloc_test_svc = BT_GATT_SERVICE(local_test_attrs);
	struct bt_gatt_service prealloc_test1_svc = BT_GATT_SERVICE(local_test1_attrs);

	(void)state;

	/* Pre-allocate handles for both services */
	for (int i = 0; i < prealloc_test_svc.attr_count; i++) {
		prealloc_test_svc.attrs[i].handle = 0x0100 + i;
	}
	for (int i = 0; i < prealloc_test1_svc.attr_count; i++) {
		prealloc_test1_svc.attrs[i].handle = 0x0200 + i;
	}

	assert_int_equal(bt_gatt_service_register(&prealloc_test_svc), 0);
	assert_int_equal(bt_gatt_service_unregister(&prealloc_test_svc), 0);

	/* Check that the handles are the same as before registering the service */
	for (int i = 0; i < prealloc_test_svc.attr_count; i++) {
		assert_int_equal(prealloc_test_svc.attrs[i].handle, 0x0100 + i);
	}

	assert_int_equal(bt_gatt_service_register(&prealloc_test1_svc), 0);
	assert_int_equal(bt_gatt_service_register(&prealloc_test_svc), 0);

	/* Clean up */
	assert_int_equal(bt_gatt_service_unregister(&prealloc_test_svc), 0);
	assert_int_equal(bt_gatt_service_unregister(&prealloc_test1_svc), 0);
}

/* Test that a service A can be re-registered after registering it once, unregistering it, and then
 * registering another service B.
 * Service A has pre-allocated handles for its attributes, while Service B has handles assigned by
 * the stack when registered.
 * Check that pre-allocated handles are the same after unregistering as they were before
 * registering the service.
 */
static void test_gatt_reregister_pre_allocated_handle_single(void **state)
{
	struct bt_gatt_attr local_test_attrs[] = {
		/* Vendor Primary Service Declaration */
		BT_GATT_PRIMARY_SERVICE(&test_uuid),

		BT_GATT_CHARACTERISTIC(&test_chrc_uuid.uuid,
				       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
				       BT_GATT_PERM_READ_AUTHEN | BT_GATT_PERM_WRITE_AUTHEN,
				       read_test, write_test, test_value),
	};

	struct bt_gatt_attr local_test1_attrs[] = {
		/* Vendor Primary Service Declaration */
		BT_GATT_PRIMARY_SERVICE(&test1_uuid),

		BT_GATT_CHARACTERISTIC(&test1_nfy_uuid.uuid,
				       BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE,
				       NULL, NULL, &nfy_enabled),
		BT_GATT_CCC(test1_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	};

	struct bt_gatt_service prealloc_test_svc = BT_GATT_SERVICE(local_test_attrs);
	struct bt_gatt_service auto_test_svc = BT_GATT_SERVICE(local_test1_attrs);

	(void)state;

	/* Pre-allocate handles for one service only */
	for (int j = 0; j < prealloc_test_svc.attr_count; j++) {
		prealloc_test_svc.attrs[j].handle = 0x0100 + j;
	}

	assert_int_equal(bt_gatt_service_register(&prealloc_test_svc), 0);
	assert_int_equal(bt_gatt_service_unregister(&prealloc_test_svc), 0);

	/* Check that the handles are the same as before registering the service */
	for (int i = 0; i < prealloc_test_svc.attr_count; i++) {
		assert_int_equal(prealloc_test_svc.attrs[i].handle, 0x0100 + i);
	}

	assert_int_equal(bt_gatt_service_register(&auto_test_svc), 0);
	assert_int_equal(bt_gatt_service_register(&prealloc_test_svc), 0);

	/* Clean up */
	assert_int_equal(bt_gatt_service_unregister(&prealloc_test_svc), 0);
	assert_int_equal(bt_gatt_service_unregister(&auto_test_svc), 0);
}

static uint8_t count_attr(const struct bt_gatt_attr *attr, uint16_t handle,
			  void *user_data)
{
	uint16_t *count = user_data;

	(void)attr;
	(void)handle;
	(*count)++;

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t find_attr(const struct bt_gatt_attr *attr, uint16_t handle,
			 void *user_data)
{
	const struct bt_gatt_attr **tmp = user_data;

	(void)handle;
	*tmp = attr;

	return BT_GATT_ITER_CONTINUE;
}

static void test_gatt_foreach(void **state)
{
	const struct bt_gatt_attr *attr;
	uint16_t num = 0;

	(void)state;

	/* Attempt to register services */
	assert_int_equal(bt_gatt_service_register(&test_svc), 0);
	assert_int_equal(bt_gatt_service_register(&test1_svc), 0);

	/* Iterate attributes */
	bt_gatt_foreach_attr(test_attrs[0].handle, 0xffff, count_attr, &num);
	assert_int_equal(num, 7);

	/* Iterate 1 attribute */
	num = 0;
	bt_gatt_foreach_attr_type(test_attrs[0].handle, 0xffff, NULL, NULL, 1,
				  count_attr, &num);
	assert_int_equal(num, 1);

	/* Find attribute by UUID */
	attr = NULL;
	bt_gatt_foreach_attr_type(test_attrs[0].handle, 0xffff,
				  &test_chrc_uuid.uuid, NULL, 0, find_attr,
				  &attr);
	assert_non_null(attr);
	if (attr != NULL) {
		assert_ptr_equal(attr->uuid, &test_chrc_uuid.uuid);
	}

	/* Find attribute by DATA */
	attr = NULL;
	bt_gatt_foreach_attr_type(test_attrs[0].handle, 0xffff, NULL,
				  test_value, 0, find_attr, &attr);
	assert_non_null(attr);
	if (attr != NULL) {
		assert_ptr_equal(attr->user_data, test_value);
	}

	/* Find all characteristics */
	num = 0;
	bt_gatt_foreach_attr_type(test_attrs[0].handle, 0xffff,
				  BT_UUID_GATT_CHRC, NULL, 0, count_attr, &num);
	assert_int_equal(num, 2);

	/* Find 1 characteristic */
	attr = NULL;
	bt_gatt_foreach_attr_type(test_attrs[0].handle, 0xffff,
				  BT_UUID_GATT_CHRC, NULL, 1, find_attr, &attr);
	assert_non_null(attr);

	/* Find attribute by UUID and DATA */
	attr = NULL;
	bt_gatt_foreach_attr_type(test_attrs[0].handle, 0xffff,
				  &test1_nfy_uuid.uuid, &nfy_enabled, 1,
				  find_attr, &attr);
	assert_non_null(attr);
	if (attr != NULL) {
		assert_ptr_equal(attr->uuid, &test1_nfy_uuid.uuid);
		assert_ptr_equal(attr->user_data, &nfy_enabled);
	}
}

static void test_gatt_read(void **state)
{
	const struct bt_gatt_attr *attr;
	uint8_t buf[256];
	ssize_t ret;

	(void)state;
	assert_int_equal(bt_gatt_service_register(&test_svc), 0);

	/* Find attribute by UUID */
	attr = NULL;
	bt_gatt_foreach_attr_type(test_attrs[0].handle, 0xffff,
				  &test_chrc_uuid.uuid, NULL, 0, find_attr,
				  &attr);
	assert_non_null(attr);
	assert_ptr_equal(attr->uuid, &test_chrc_uuid.uuid);

	ret = attr->read(NULL, attr, (void *)buf, sizeof(buf), 0);
	assert_int_equal(ret, (ssize_t)strlen((const char *)test_value));
	assert_memory_equal(buf, test_value, ret);
}

static void test_gatt_write(void **state)
{
	const struct bt_gatt_attr *attr;
	const char value[] = "    ";
	ssize_t ret;

	(void)state;

	/* Need our service to be registered */
	assert_int_equal(bt_gatt_service_register(&test_svc), 0);

	/* Find attribute by UUID */
	attr = NULL;
	bt_gatt_foreach_attr_type(test_attrs[0].handle, 0xffff,
				  &test_chrc_uuid.uuid, NULL, 0, find_attr,
				  &attr);
	assert_non_null(attr);

	ret = attr->write(NULL, attr, (const void *)value, strlen(value), 0, 0);
	assert_int_equal(ret, (ssize_t)strlen(value));
	assert_memory_equal(value, test_value, ret);
}

static void test_bt_att_err_to_str(void **state)
{
	(void)state;

	/* Test a couple of entries */
	assert_string_equal(bt_att_err_to_str(BT_ATT_ERR_SUCCESS),
			    "BT_ATT_ERR_SUCCESS");
	assert_string_equal(bt_att_err_to_str(BT_ATT_ERR_INSUFFICIENT_ENCRYPTION),
			    "BT_ATT_ERR_INSUFFICIENT_ENCRYPTION");
	assert_string_equal(bt_att_err_to_str(BT_ATT_ERR_OUT_OF_RANGE),
			    "BT_ATT_ERR_OUT_OF_RANGE");

	/* Test a entries that is not used */
	assert_memory_equal(bt_att_err_to_str(0x14),
			    "(unknown)", strlen("(unknown)"));
	assert_memory_equal(bt_att_err_to_str(0xFB),
			    "(unknown)", strlen("(unknown)"));

	for (uint16_t i = 0; i <= UINT8_MAX; i++) {
		assert_non_null(bt_att_err_to_str(i));
	}
}

static void test_bt_gatt_err_to_str(void **state)
{
	(void)state;

	/* Test a couple of entries */
	assert_string_equal(bt_gatt_err_to_str(BT_GATT_ERR(BT_ATT_ERR_SUCCESS)),
			    "BT_ATT_ERR_SUCCESS");
	assert_string_equal(
		bt_gatt_err_to_str(BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_ENCRYPTION)),
		"BT_ATT_ERR_INSUFFICIENT_ENCRYPTION");
	assert_string_equal(bt_gatt_err_to_str(BT_GATT_ERR(BT_ATT_ERR_OUT_OF_RANGE)),
			    "BT_ATT_ERR_OUT_OF_RANGE");

	/* Test entries that are not used */
	assert_memory_equal(bt_gatt_err_to_str(BT_GATT_ERR(0x14)),
			    "(unknown)", strlen("(unknown)"));
	assert_memory_equal(bt_gatt_err_to_str(BT_GATT_ERR(0xFB)),
			    "(unknown)", strlen("(unknown)"));

	/* Test positive values */
	for (uint16_t i = 0; i <= UINT8_MAX; i++) {
		assert_non_null(bt_gatt_err_to_str(i));
	}

	/* Test negative values */
	for (uint16_t i = 0; i <= UINT8_MAX; i++) {
		assert_non_null(bt_gatt_err_to_str(-i));
	}
}

static void test_gatt_ccc_write_cb(void **state)
{
	struct bt_gatt_attr test_write_cb_attrs[] = {
		/* Vendor Primary Service Declaration */
		BT_GATT_PRIMARY_SERVICE(&test1_uuid),

		BT_GATT_CHARACTERISTIC(&test1_nfy_uuid.uuid,
				       BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE,
				       NULL, NULL, &nfy_enabled),
		BT_GATT_CCC_WITH_WRITE_CB(test1_ccc_cfg_changed,
					 test1_ccc_cfg_write_cb,
					 BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
	};

	struct bt_gatt_service test_write_cb_svc = BT_GATT_SERVICE(test_write_cb_attrs);

	(void)state;
	assert_int_equal(bt_gatt_service_register(&test_write_cb_svc), 0);
	assert_int_equal(bt_gatt_service_unregister(&test_write_cb_svc), 0);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(test_gatt_register, test_case_setup,
					       test_case_teardown),
		cmocka_unit_test_setup_teardown(test_gatt_unregister, test_case_setup,
					       test_case_teardown),
		cmocka_unit_test_setup_teardown(test_gatt_reregister, test_case_setup,
					       test_case_teardown),
		cmocka_unit_test_setup_teardown(test_gatt_reregister_pre_allocated_handles,
					       test_case_setup, test_case_teardown),
		cmocka_unit_test_setup_teardown(test_gatt_reregister_pre_allocated_handle_single,
					       test_case_setup, test_case_teardown),
		cmocka_unit_test_setup_teardown(test_gatt_foreach, test_case_setup,
					       test_case_teardown),
		cmocka_unit_test_setup_teardown(test_gatt_read, test_case_setup,
					       test_case_teardown),
		cmocka_unit_test_setup_teardown(test_gatt_write, test_case_setup,
					       test_case_teardown),
		cmocka_unit_test_setup_teardown(test_bt_att_err_to_str, test_case_setup,
					       test_case_teardown),
		cmocka_unit_test_setup_teardown(test_bt_gatt_err_to_str, test_case_setup,
					       test_case_teardown),
		cmocka_unit_test_setup_teardown(test_gatt_ccc_write_cb, test_case_setup,
					       test_case_teardown),
	};

	return cmocka_run_group_tests_name("bt_gatt", tests, test_group_setup,
					  test_group_teardown);
}
