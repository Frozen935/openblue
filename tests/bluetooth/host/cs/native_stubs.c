#include <stdbool.h>
#include <stdint.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/crypto.h>

#include <host/adv.h>
#include <host/ecc.h>
#include <host/hci_core.h>

const bt_addr_le_t *bt_lookup_id_addr(uint8_t id, const bt_addr_le_t *addr)
{
	(void)id;
	return addr;
}

void bt_pub_key_hci_disrupted(void)
{
}

bool bt_pub_key_is_debug(uint8_t *cmp_pub_key)
{
	(void)cmp_pub_key;
	return false;
}

bool bt_pub_key_is_valid(const uint8_t key[BT_PUB_KEY_LEN])
{
	(void)key;
	return true;
}

int bt_pub_key_gen(struct bt_pub_key_cb *cb)
{
	(void)cb;
	return 0;
}

int bt_crypto_f4(const uint8_t *u, const uint8_t *v, const uint8_t *x, uint8_t z, uint8_t res[16])
{
	(void)u;
	(void)v;
	(void)x;
	(void)z;
	if (res != NULL) {
		for (int i = 0; i < 16; i++) {
			res[i] = 0;
		}
	}
	return 0;
}

bool bt_rpa_irk_matches(const uint8_t irk[16], const bt_addr_t *addr)
{
	(void)irk;
	(void)addr;
	return false;
}

struct bt_le_ext_adv *bt_le_adv_lookup_legacy(void)
{
	return NULL;
}

int bt_le_lim_adv_cancel_timeout(struct bt_le_ext_adv *adv)
{
	(void)adv;
	return 0;
}
