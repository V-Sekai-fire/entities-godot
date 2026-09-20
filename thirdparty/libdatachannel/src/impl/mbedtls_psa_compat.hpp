/**
 * mbedtls 4 removed ctr_drbg.h, entropy.h and mbedtls_ssl_conf_rng: randomness
 * now comes from PSA, which seeds itself. These map libdatachannel's 3.x calls
 * onto psa_generate_random so the call sites can stay where they are.
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef RTC_IMPL_MBEDTLS_PSA_COMPAT_H
#define RTC_IMPL_MBEDTLS_PSA_COMPAT_H

#include "psa/crypto.h"

#include <cstddef>

#define MBEDTLS_CTR_DRBG_PR_OFF 0
#define MBEDTLS_CTR_DRBG_PR_ON 1

typedef struct mbedtls_entropy_context {
	char unused;
} mbedtls_entropy_context;

typedef struct mbedtls_ctr_drbg_context {
	char unused;
} mbedtls_ctr_drbg_context;

static inline void mbedtls_entropy_init(mbedtls_entropy_context *) {}
static inline void mbedtls_entropy_free(mbedtls_entropy_context *) {}

static inline int mbedtls_entropy_func(void *, unsigned char *output, size_t len) {
	return psa_generate_random(output, len) == PSA_SUCCESS ? 0 : -1;
}

static inline void mbedtls_ctr_drbg_init(mbedtls_ctr_drbg_context *) {}
static inline void mbedtls_ctr_drbg_free(mbedtls_ctr_drbg_context *) {}
static inline void mbedtls_ctr_drbg_set_prediction_resistance(mbedtls_ctr_drbg_context *, int) {}

// PSA seeds its own generator, so seeding is just making sure PSA is up.
static inline int mbedtls_ctr_drbg_seed(mbedtls_ctr_drbg_context *,
		int (*)(void *, unsigned char *, size_t), void *, const unsigned char *, size_t) {
	const psa_status_t status = psa_crypto_init();
	return status == PSA_SUCCESS ? 0 : -1;
}

static inline int mbedtls_ctr_drbg_random(void *, unsigned char *output, size_t len) {
	return psa_generate_random(output, len) == PSA_SUCCESS ? 0 : -1;
}

#endif
