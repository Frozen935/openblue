#include <stdio.h>
#include <stdint.h>

#include <bluetooth/bluetooth.h>


static void ready_cb(int err)
{
    if (err) {
        printf("Bluetooth enable failed: %d\n", err);
        return;
    }
    
    printf("Bluetooth ready\n");
}

int main(void)
{
	printf("Hello World!\n");

    bt_stack_init_once();

    bt_enable(ready_cb);

    printf("Bluetooth enabled\n");

    while (1) {
        os_sleep_ms(1000);
    }

	return 0;
}