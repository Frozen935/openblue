/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OPENBLUE_TESTS_BLUETOOTH_CLASSIC_SHELL_HOST_H_
#define OPENBLUE_TESTS_BLUETOOTH_CLASSIC_SHELL_HOST_H_

#include <stddef.h>

struct bt_shell;

typedef int (*classic_shell_register_fn)(struct bt_shell *sh);

int classic_shell_host_run(const classic_shell_register_fn *registrars, size_t registrar_count);

#endif /* OPENBLUE_TESTS_BLUETOOTH_CLASSIC_SHELL_HOST_H_ */
