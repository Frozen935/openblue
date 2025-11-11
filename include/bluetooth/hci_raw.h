/** @file
 *  @brief Bluetooth HCI RAW channel handling
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __INCLUDE_BLUETOOTH_HCI_RAW_H__
#define __INCLUDE_BLUETOOTH_HCI_RAW_H__

/**
 * @brief HCI RAW channel
 * @defgroup hci_raw HCI RAW channel
 * @ingroup bluetooth
 * @{
 */

#include <stdint.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif

/** @brief Send packet to the Bluetooth controller
 *
 * Send packet to the Bluetooth controller. The buffers should be allocated using
 * bt_buf_get_tx().
 *
 * @param buf HCI packet to be sent.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_send(struct bt_buf *buf);

/** @brief Enable Bluetooth RAW channel:
 *
 *  Enable Bluetooth RAW HCI channel.
 *
 *  @param rx_queue netbuf queue where HCI packets received from the Bluetooth
 *  controller are to be queued. The queue is defined in the caller while
 *  the available buffers pools are handled in the stack.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_enable_raw(struct bt_fifo *rx_queue);

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* __INCLUDE_BLUETOOTH_HCI_RAW_H__ */
