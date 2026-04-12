/* smp_reboot.c - Bluetooth classic SMP key persist test shell cmds */

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "common/bt_shell_private.h"

static int cmd_reboot(const struct bt_shell *sh, size_t argc, char **argv)
{
	(void)sh;
	(void)argc;
	(void)argv;

	/*
	 * OpenBlue native 测试以宿主进程承载 shell。
	 * 无法在本地直接做 Zephyr 的 sys_reboot(SYS_REBOOT_COLD)，因此以成功退出
	 * 当前进程的方式保留“重启后由外部 harness 重新拉起”的运行模型。
	 */
	exit(EXIT_SUCCESS);

	return 0;
}

#define HELP_NONE "[none]"

BT_SHELL_STATIC_SUBCMD_SET_CREATE(
	test_smp_cmds,
	BT_SHELL_CMD_ARG(reboot, NULL, HELP_NONE, cmd_reboot, 1, 0),
	BT_SHELL_SUBCMD_SET_END);

static int cmd_test_smp(const struct bt_shell *sh, size_t argc, char **argv)
{
	(void)argc;
	(void)argv;

	/* 在 OpenBlue 的 shell 实现里，root 命令被单独执行时打印帮助。 */
	bt_shell_help(sh);

	return 0;
}

BT_SHELL_CMD_REGISTER(test_smp, &test_smp_cmds, "smp test cmds", cmd_test_smp);
