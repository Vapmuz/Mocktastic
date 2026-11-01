#ifndef CRYPTO_H
#define CRYPTO_H

/**
 * Cryptography helper functions for Meshtastic packet encryption and hashing.
 *
 * This header is compatible with both the test build that links vendored
 * mbedtls sources and the Arduino/ESP32 build that uses the platform's
 * built-in mbedtls component.
 *
 * For Curve25519 on ESP-IDF, CONFIG_MBEDTLS_ECP_DP_CURVE25519_ENABLED is
 * required. The generic ECP path used here also requires a clamped private
 * scalar and a non-null RNG for shared secret calculation.
 */

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

using AesKey256 = std::array<uint8_t, 32>;
using X25519Key = std::array<uint8_t, 32>;
using CcmTag = std::array<uint8_t, 8>;

/**
 * Coppia di chiavi Curve25519 (chiavi grezze di 32 byte, RFC 7748).
 */
struct X25519KeyPair {
  X25519Key privateKey;
  X25519Key publicKey;
};

/**
 * Generate a new random X25519 key pair.
 *
 * The private key is already clamped as required by RFC 7748 and the public
 * key is the corresponding u-coordinate of the base point multiple.
 *
 * @return freshly generated key pair
 */
extern X25519KeyPair generateX25519KeyPair();

/**
 * Clamp a Curve25519 private key according to RFC 7748 §5.
 *
 * @param privateKey raw 32-byte private key
 * @return clamped 32-byte private key
 */
extern X25519Key clampX25519PrivateKey(const X25519Key& privateKey);

/**
 * Compute the SHA-256 digest and return it as a lowercase hex string.
 *
 * @param input input string to hash
 * @return SHA-256 digest in hex form
 */
extern std::string sha256Hex(const std::string& input);

/**
 * Compute the SHA-256 digest of raw bytes.
 *
 * @param input pointer to input data
 * @param len length of input data
 * @return 32-byte SHA-256 digest
 */
extern AesKey256 sha256Raw(const uint8_t* input, size_t len);

/**
 * Encrypt or decrypt a public-channel payload with AES-CTR.
 *
 * The PSK is used directly as the AES key (16 or 32 bytes). The nonce is
 * formed as: id (8 bytes LE, zero-extended) || from (4 bytes LE) || extraNonce
 * (4 bytes, 0 for PSK use).
 *
 * @param pskKey pre-shared key
 * @param packetId packet identifier
 * @param fromNode source node ID
 * @param data payload bytes
 * @return transformed payload bytes
 */
extern std::vector<uint8_t> channelCryptCtr(const std::vector<uint8_t>& pskKey,
  uint32_t packetId,
  uint32_t fromNode,
  const std::vector<uint8_t>& data);

/**
 * Compute an X25519 shared secret from a private key and peer public key.
 *
 * Both keys are raw 32-byte RFC 7748 u-coordinates.
 *
 * @param myPrivateKey local private key
 * @param peerPublicKey peer public key
 * @return shared secret
 */
extern X25519Key x25519SharedSecret(const X25519Key& myPrivateKey, const X25519Key& peerPublicKey);

/**
 * Derive an AES key for direct messages by hashing the shared secret.
 *
 * @param sharedSecret shared secret bytes
 * @return 256-bit AES key
 */
extern AesKey256 deriveAesKey(const X25519Key& sharedSecret);

struct CcmCiphertext {
  std::vector<uint8_t> ciphertext;
  CcmTag tag;
};

/**
 * Encrypt a direct message using AES-256-CCM.
 *
 * No AAD is used. The nonce is constructed as: id (4 bytes LE) || extraNonce
 * (4 bytes) || from (4 bytes LE) || 0x00.
 *
 * @param aesKey AES-256 key
 * @param packetId packet identifier
 * @param extraNonce extra nonce value
 * @param fromNode source node ID
 * @param plaintext input plaintext bytes
 * @return ciphertext and authentication tag
 */
extern CcmCiphertext ccmEncrypt(const AesKey256& aesKey,
  uint32_t packetId,
  uint32_t extraNonce,
  uint32_t fromNode,
  const std::vector<uint8_t>& plaintext);

/**
 * Decrypt and verify an AES-256-CCM direct message.
 *
 * @param aesKey AES-256 key
 * @param packetId packet identifier
 * @param extraNonce extra nonce value
 * @param fromNode source node ID
 * @param ciphertext encrypted bytes
 * @param tag authentication tag
 * @return plaintext bytes, or std::nullopt if authentication fails
 */
extern std::optional<std::vector<uint8_t>> ccmDecrypt(const AesKey256& aesKey,
  uint32_t packetId,
  uint32_t extraNonce,
  uint32_t fromNode,
  const std::vector<uint8_t>& ciphertext,
  const CcmTag& tag);

/**
 * Encrypt or decrypt a broadcast packet payload using a PSK.
 *
 * The id and from buffers are interpreted as little-endian values.
 *
 * @param psk_key pre-shared key bytes
 * @param id packet id buffer
 * @param from sender id buffer
 * @param payload packet payload bytes
 * @return encrypted/decrypted payload bytes
 */
extern std::vector<uint8_t> payloadEncrypterBroadcast(std::vector<uint8_t> psk_key,
  uint8_t id[4],
  uint8_t from[4],
  std::vector<uint8_t> payload);

/**
 * Compute the X25519 shared secret from string-encoded keys.
 *
 * @param priv_key_str private key string
 * @param publ_key_str public key string
 * @return shared secret bytes
 */
extern X25519Key stringToShSecret(std::string priv_key_str, std::string publ_key_str);

/**
 * Convert a 32-byte X25519 key array to a byte vector.
 *
 * @param key input X25519 key
 * @return key bytes
 */
extern std::vector<uint8_t> x25519KeyToVector(X25519Key key);

/**
 * Derive an AES key using string-encoded public/private keys.
 *
 * @param priv_key private key string
 * @param publ_key public key string
 * @return derived AES key
 */
extern X25519Key stringToAesKey(std::string priv_key, std::string publ_key);

/**
 * Decrypt a direct message and return the protobuf plaintext.
 *
 * @param message_data encrypted packet bytes
 * @param aes_key AES key
 * @return plaintext bytes
 */
extern std::vector<uint8_t> decryptMessageDM(std::vector<uint8_t> message_data, X25519Key aes_key);

#endif
 