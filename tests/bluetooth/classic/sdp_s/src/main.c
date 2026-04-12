/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../../classic_shell_host.h"

extern int bt_shell_cmd_sdp_server_register(struct bt_shell *sh);

int main(void)
{
	const classic_shell_register_fn registrars[] = {
		bt_shell_cmd_sdp_server_register,
	};

	return classic_shell_host_run(registrars, ARRAY_SIZE(registrars));
}
