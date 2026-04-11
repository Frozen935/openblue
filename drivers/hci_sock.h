/*
 * Copyright (C) 2026
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __DRIVER_HCI_SOCK_BOTTOM_H
#define __DRIVER_HCI_SOCK_BOTTOM_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool hci_sock_rx_ready(int fd);
int hci_sock_is_ipaddr_ok(char ip_addr[]);
int hci_sock_socket_open(unsigned short bt_dev_index);
int hci_sock_net_connect(char ip_addr[], unsigned int port);
int hci_sock_unix_connect(char socket_path[]);

#ifdef __cplusplus
}
#endif

#endif /* __DRIVER_HCI_SOCK_BOTTOM_H */
