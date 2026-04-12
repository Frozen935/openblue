/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bt_stack_init.h"
#include "common/bt_shell_private.h"

#include "classic_shell_host.h"

int classic_shell_host_run(const classic_shell_register_fn *registrars, size_t registrar_count)
{
	struct bt_shell sh;
	char *buffer = NULL;
	char *argv[32] = { 0 };
	char *saveptr;
	char *token;
	size_t size = 0;
	ssize_t len;
	size_t i;
	int argc;
	int err;
	int ret = 0;

	err = bt_stack_init_once();
	if (err != 0) {
		fprintf(stderr, "bt_stack_init_once failed: %d\n", err);
		return err;
	}

	bt_shell_init(&sh);

	for (i = 0; i < registrar_count; i++) {
		err = registrars[i](&sh);
		if (err != 0) {
			fprintf(stderr, "classic shell command register failed: %d\n", err);
			bt_shell_uninit(&sh);
			return err;
		}
	}

	while (1) {
		argc = 0;
		memset(argv, 0, sizeof(argv));

		bt_shell_fprintf_print("openblue> ");
		fflush(stdout);

		len = getline(&buffer, &size, stdin);
		if (len < 0) {
			break;
		}

		if (len > 0 && buffer[len - 1] == '\n') {
			buffer[len - 1] = '\0';
		}

		if (buffer[0] == '\0') {
			continue;
		}

		if (strcmp(buffer, "q") == 0) {
			break;
		}

		if (strcmp(buffer, "help") == 0) {
			bt_shell_cmds_show(&sh);
			continue;
		}

		token = strtok_r(buffer, " ", &saveptr);
		while (token != NULL && argc < (int)(sizeof(argv) / sizeof(argv[0]))) {
			argv[argc++] = token;
			token = strtok_r(NULL, " ", &saveptr);
		}

		if (argc == 0) {
			continue;
		}

		ret = bt_shell_exec(&sh, (const char **)argv, argc);
		if (ret != 0) {
			bt_shell_cmds_show(&sh);
		}
	}

	bt_shell_uninit(&sh);
	free(buffer);

	return ret;
}
