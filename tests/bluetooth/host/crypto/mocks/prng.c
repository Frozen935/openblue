/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/prng.h"

struct psa_crypto_init_fake_state psa_crypto_init_fake;
struct psa_generate_random_fake_state psa_generate_random_fake;

void reset_prng_fakes(void)
{
	psa_crypto_init_fake.call_count = 0;
	psa_crypto_init_fake.return_val = PSA_SUCCESS;
	psa_generate_random_fake.call_count = 0;
	psa_generate_random_fake.return_val = PSA_SUCCESS;
	psa_generate_random_fake.arg0_val = NULL;
	psa_generate_random_fake.arg1_val = 0;
}

psa_status_t psa_crypto_init(void)
{
	psa_crypto_init_fake.call_count++;
	return psa_crypto_init_fake.return_val;
}

psa_status_t psa_generate_random(uint8_t *out, size_t outlen)
{
	psa_generate_random_fake.call_count++;
	psa_generate_random_fake.arg0_val = out;
	psa_generate_random_fake.arg1_val = outlen;
	return psa_generate_random_fake.return_val;
}
