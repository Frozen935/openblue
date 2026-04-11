/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include <cmocka.h>

#include <base/bt_buf.h>
#include <bluetooth/bluetooth.h>

struct parse_ctx {
	int call_count;
	bool return_value;
	const uint8_t *data;
	size_t len;
	bool validate_contents;
};

static bool bt_data_parse_counting_cb(struct bt_data *data, void *user_data)
{
	struct parse_ctx *ctx = user_data;

	ctx->call_count++;
	if (!ctx->validate_contents) {
		return ctx->return_value;
	}

	assert_true(ctx->len-- > 0);
	assert_int_equal(data->data_len, *ctx->data - 1);
	ctx->data++;

	assert_true(ctx->len-- > 0);
	assert_int_equal(data->type, *ctx->data);
	ctx->data++;

	assert_true(ctx->len >= data->data_len);
	assert_memory_equal(data->data, ctx->data, data->data_len);
	ctx->data += data->data_len;
	ctx->len -= data->data_len;

	return ctx->return_value;
}

/*
 *  Test empty data buffer
 *
 *  Constraints:
 *   - data.len set to 0
 *
 *  Expected behaviour:
 *   - Callback function is not called
 */
static void test_parsing_empty_buf(void **state)
{
	(void)state;
	struct bt_buf_simple *buf = BT_BUF_SIMPLE(0);
	struct parse_ctx ctx = {0};

	bt_data_parse(buf, bt_data_parse_counting_cb, &ctx);

	assert_int_equal(ctx.call_count, 0);
}

/*
 *  Test AD Structure invalid length
 *
 *  Constraints:
 *   - AD Structure N length > number of bytes after
 *
 *  Expected behaviour:
 *   - Callback function is called N - 1 times
 */
static void test_parsing_invalid_length(void **state)
{
	(void)state;
	struct bt_buf_simple buf;
	uint8_t data[] = {
		/* Significant part */
		0x02, 0x01, 0x00,                       /* AD Structure 1 */
		0x03, 0x02, 0x01, 0x00,                 /* AD Structure 2 */
		/* Invalid length 0xff */
		0xff, 0x03, 0x02, 0x01,                 /* AD Structure N */
		0x05, 0x04, 0x03, 0x02, 0x01, 0x00,     /* AD Structure N + 1 */
	};

	struct parse_ctx ctx = {
		.return_value = true,
	};

	bt_buf_simple_init_with_data(&buf, data, ARRAY_SIZE(data));

	bt_data_parse(&buf, bt_data_parse_counting_cb, &ctx);

	assert_int_equal(ctx.call_count, 2);
}

/*
 *  Test early termination of the significant part
 *
 *  Constraints:
 *   - The significant part contains a sequence of N AD structures
 *   - The non-significant part extends the data with all-zero octets
 *
 *  Expected behaviour:
 *   - Callback function is called N times
 */
static void test_parsing_early_termination(void **state)
{
	(void)state;
	struct bt_buf_simple buf;
	uint8_t data[] = {
		/* Significant part */
		0x02, 0x01, 0x00,                       /* AD Structure 1 */
		0x03, 0x02, 0x01, 0x00,                 /* AD Structure 2 */
		0x04, 0x03, 0x02, 0x01, 0x00,           /* AD Structure 3 */
		/* Non-significant part */
		0x00, 0x00, 0x00, 0x00, 0x00
	};

	struct parse_ctx ctx = {
		.return_value = true,
	};

	bt_buf_simple_init_with_data(&buf, data, ARRAY_SIZE(data));

	bt_data_parse(&buf, bt_data_parse_counting_cb, &ctx);

	assert_int_equal(ctx.call_count, 3);
}

/*
 *  Test parsing stopped
 *
 *  Constraints:
 *   - Data contains valid AD Structures
 *   - Callback function returns false to stop parsing
 *
 *  Expected behaviour:
 *   - Once parsing is stopped, the callback is not called anymore
 */
static void test_parsing_stopped(void **state)
{
	(void)state;
	struct bt_buf_simple buf;
	uint8_t data[] = {
		/* Significant part */
		0x02, 0x01, 0x00,                       /* AD Structure 1 */
		0x03, 0x02, 0x01, 0x00,                 /* AD Structure 2 */
	};

	struct parse_ctx ctx = {
		.return_value = false,
	};

	bt_buf_simple_init_with_data(&buf, data, ARRAY_SIZE(data));

	bt_data_parse(&buf, bt_data_parse_counting_cb, &ctx);

	assert_int_equal(ctx.call_count, 1);
}

/*
 *  Test parsing AD Data
 *
 *  Constraints:
 *   - Data contains valid AD Structures
 *   - Callback function returns false to stop parsing
 *
 *  Expected behaviour:
 *   - Data passed to the callback match the expected data
 */
static void test_parsing_success(void **state)
{
	(void)state;
	struct bt_buf_simple buf;
	uint8_t data[] = {
		/* Significant part */
		0x02, 0x01, 0x00,                       /* AD Structure 1 */
		0x03, 0x02, 0x01, 0x00,                 /* AD Structure 2 */
	};
	struct parse_ctx ctx = {
		.data = data,
		.len = ARRAY_SIZE(data),
		.return_value = true,
		.validate_contents = true,
	};

	bt_buf_simple_init_with_data(&buf, data, ARRAY_SIZE(data));

	bt_data_parse(&buf, bt_data_parse_counting_cb, &ctx);

	assert_int_equal(ctx.call_count, 2);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_parsing_empty_buf),
		cmocka_unit_test(test_parsing_invalid_length),
		cmocka_unit_test(test_parsing_early_termination),
		cmocka_unit_test(test_parsing_stopped),
		cmocka_unit_test(test_parsing_success),
	};

	return cmocka_run_group_tests_name("bt_data_parse", tests, NULL, NULL);
}
