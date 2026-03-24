#include <stdio.h>
#include <stdint.h>

#include "shim/include/preinclude.h"
#include <bluetooth/bluetooth.h>


static void ready_cb(int err)
{
    if (err) {
        LOG_INF("Bluetooth enable failed: %d", err);
        return;
    }

    LOG_INF("Bluetooth ready");
}

int main(void)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	LOG_INF("Hello World!");

    bt_stack_init_once();

    bt_enable(ready_cb);

    LOG_INF("Bluetooth enabled");

    while (1) {
        os_sleep_ms(1000);
    }

	return 0;
}
