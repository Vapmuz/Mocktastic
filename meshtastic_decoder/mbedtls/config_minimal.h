/*
 * Minimal mbedtls config: enables only what Meshtastic decoding needs
 * (SHA-256, AES-CTR for channel PSK, AES-CCM + X25519/ECDH for direct
 * messages). Passed in via -DMBEDTLS_CONFIG_FILE so the stock (huge)
 * mbedtls/mbedtls_config.h is never pulled in.
 */
#ifndef MBEDTLS_CONFIG_MINIMAL_H
#define MBEDTLS_CONFIG_MINIMAL_H

#define MBEDTLS_SHA256_C

#define MBEDTLS_AES_C
#define MBEDTLS_CIPHER_MODE_CTR
#define MBEDTLS_BLOCK_CIPHER_C
#define MBEDTLS_CCM_C

#define MBEDTLS_BIGNUM_C
#define MBEDTLS_ECP_C
#define MBEDTLS_ECP_DP_CURVE25519_ENABLED
#define MBEDTLS_ECDH_C
/*
 * NON abilitare MBEDTLS_ECDH_VARIANT_EVEREST_ENABLED.
 *
 * Everest e' la variante X25519 di HACL*: ignora la f_rng passata a
 * mbedtls_ecdh_calc_secret e clampa lo scalare al suo interno. L'mbedtls
 * fornito da ESP-IDF/Arduino-esp32 non la abilita, quindi la stessa
 * chiamata li' finisce sul percorso generico ECP (ladder di Montgomery),
 * che invece pretende una f_rng non NULL per il blinding e uno scalare
 * gia' clampato. Tenendo Everest spento anche qui, i test su host
 * esercitano esattamente il codice che gira sull'ESP32.
 */

#endif
