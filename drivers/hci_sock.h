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
