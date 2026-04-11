/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <psa/crypto.h>

struct psa_crypto_init_fake_state {
	int call_count;
	psa_status_t return_val;
};

struct psa_generate_random_fake_state {
	int call_count;
	psa_status_t return_val;
	uint8_t *arg0_val;
	size_t arg1_val;
};

extern struct psa_crypto_init_fake_state psa_crypto_init_fake;
extern struct psa_generate_random_fake_state psa_generate_random_fake;

void reset_prng_fakes(void);
psa_status_t psa_crypto_init(void);
psa_status_t psa_generate_random(uint8_t *out, size_t outlen);
