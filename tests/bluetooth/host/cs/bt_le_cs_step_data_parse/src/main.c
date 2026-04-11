/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include <cmocka.h>

#include <base/bt_buf.h>
#include <bluetooth/cs.h>

struct step_parse_ctx {
	int call_count;
	bool return_value;
	const uint8_t *data;
	size_t len;
	bool validate_contents;
};

static bool step_parse_cb(struct bt_le_cs_subevent_step *step, void *user_data)
{
	struct step_parse_ctx *ctx = user_data;

	ctx->call_count++;
	if (!ctx->validate_contents) {
		return ctx->return_value;
	}

	assert_true(ctx->len-- > 0);
	assert_int_equal(step->mode, *ctx->data++);

	assert_true(ctx->len-- > 0);
	assert_int_equal(step->channel, *ctx->data++);

	assert_true(ctx->len-- > 0);
	assert_int_equal(step->data_len, *ctx->data++);

	assert_true(ctx->len >= step->data_len);
	assert_memory_equal(step->data, ctx->data, step->data_len);
	ctx->data += step->data_len;
	ctx->len -= step->data_len;

	return ctx->return_value;
}

/*
 *  Test empty data buffer
 *
 *  Constraints:
 *   - buffer len set to 0
 *
 *  Expected behaviour:
 *   - Callback function is not called
 */
static void test_parsing_empty_buf(void **state)
{
	(void)state;
	struct bt_buf_simple *buf = BT_BUF_SIMPLE(0);
	struct step_parse_ctx ctx = {0};

	bt_le_cs_step_data_parse(buf, step_parse_cb, &ctx);

	assert_int_equal(ctx.call_count, 0);
}

/*
 *  Test malformed step data
 *
 *  Constraints:
 *   - step data with a step length going out of bounds
 *
 *  Expected behaviour:
 *   - Callback function is called once
 */
static void test_parsing_invalid_length(void **state)
{
	(void)state;
	struct bt_buf_simple buf;
	uint8_t data[] = {
		0x00, 0x01, 0x01, 0x00,       /* mode 0 */
		0x03, 0x20, 0x03, 0x00, 0x11, /* mode 3 step with bad length */
	};
	struct step_parse_ctx ctx = {
		.return_value = true,
	};

	bt_buf_simple_init_with_data(&buf, data, ARRAY_SIZE(data));

	bt_le_cs_step_data_parse(&buf, step_parse_cb, &ctx);

	assert_int_equal(ctx.call_count, 1);
}

/*
 *  Test parsing stopped
 *
 *  Constraints:
 *   - Data contains valid step data
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
		0x00, 0x05, 0x01, 0x00,       /* mode 0 */
		0x01, 0x10, 0x02, 0x00, 0x11, /* mode 1 */
		0x02, 0x11, 0x02, 0x00, 0x11, /* mode 2 */
	};
	struct step_parse_ctx ctx = {
		.return_value = false,
	};

	bt_buf_simple_init_with_data(&buf, data, ARRAY_SIZE(data));

	bt_le_cs_step_data_parse(&buf, step_parse_cb, &ctx);

	assert_int_equal(ctx.call_count, 1);
}

/*
 *  Test parsing successfully
 *
 *  Constraints:
 *   - Data contains valid step data
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
		0x00, 0x05, 0x01, 0x00,       /* mode 0 */
		0x03, 0x11, 0x01, 0x11,       /* mode 3 */
		0x02, 0x12, 0x02, 0x00, 0x11, /* mode 2 */
		0x03, 0x13, 0x01, 0x11,       /* mode 3 */
		0x02, 0x14, 0x02, 0x00, 0x11, /* mode 2 */
	};
	struct step_parse_ctx ctx = {
		.data = data,
		.len = ARRAY_SIZE(data),
		.return_value = true,
		.validate_contents = true,
	};

	bt_buf_simple_init_with_data(&buf, data, ARRAY_SIZE(data));

	bt_le_cs_step_data_parse(&buf, step_parse_cb, &ctx);

	assert_int_equal(ctx.call_count, 5);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_parsing_empty_buf),
		cmocka_unit_test(test_parsing_invalid_length),
		cmocka_unit_test(test_parsing_stopped),
		cmocka_unit_test(test_parsing_success),
	};

	return cmocka_run_group_tests_name("bt_le_cs_step_data_parse", tests, NULL, NULL);
}
