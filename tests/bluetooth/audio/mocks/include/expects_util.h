/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_UTIL_H_
#define MOCKS_UTIL_H_
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>

#include <cmocka.h>

#define expect_data(_func_name, _arg_name, _exp_data, _data, _len)                                 \
	do {                                                                                           \
		if ((_exp_data) != NULL) {                                                               \
			expect_data_equal(_func_name, _arg_name, _exp_data, _data, _len);                \
		}                                                                                          \
	} while (0)

#define expect_call_count(_func_name, _expected, _actual)                                          \
	assert_int_equal((_expected), (_actual))

static inline void expect_data_equal(const char *func_name, const char *arg_name,
				     const uint8_t *expect, const uint8_t *data, size_t len)
{
	for (size_t i = 0U; i < len; i++) {
		assert_int_equal(expect[i], data[i]);
	}
}

#endif /* MOCKS_UTIL_H_ */
