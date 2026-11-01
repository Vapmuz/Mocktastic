#include "crypto.h"
#include "functions.h"

#define MBEDTLS_ALLOW_PRIVATE_ACCESS

extern "C" {
#include "mbedtls/aes.h"
#include "mbedtls/ccm.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/sha256.h"
}

#include <cstdio>
#include <cstring>
#include <stdexcept>

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_random.h>
#else
#include <random>
#endif

/*
Sorgente di casualita' per il blinding delle coordinate nella ladder di
Montgomery: mbedtls_ecp_mul() rifiuta una f_rng NULL con
MBEDTLS_ERR_ECP_BAD_INPUT_DATA (-0x4F80). Serve solo a randomizzare i
calcoli intermedi, non a generare chiavi.
*/
static int cryptoRng(void* /*p_rng*/, unsigned char* out, size_t len) {
#if defined(ARDUINO_ARCH_ESP32)
    esp_fill_random(out, len);
#else
    static std::random_device rd;
    for (size_t i = 0; i < len; ++i) {
        out[i] = static_cast<unsigned char>(rd() & 0xff);
    }
#endif
    return 0;
}

X25519Key clampX25519PrivateKey(const X25519Key& privateKey) {
   X25519Key clamped = privateKey;
    clamped.at(0) &= 248;
    clamped.at(31) &= 127;
    clamped.at(31) |= 64;
    return clamped;
}


X25519KeyPair generateX25519KeyPair() {
    mbedtls_ecp_group grp;
    mbedtls_mpi d;
    mbedtls_ecp_point Q;

    mbedtls_ecp_group_init(&grp);
    mbedtls_mpi_init(&d);
    mbedtls_ecp_point_init(&Q);

    int rc = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_CURVE25519);
    if (rc == 0) {
        // Per le curve di Montgomery mbedtls_ecp_gen_keypair() estrae uno
        // scalare casuale gia' clampato secondo RFC 7748 e calcola Q = d*G.
        rc = mbedtls_ecp_gen_keypair(&grp, &d, &Q, cryptoRng, nullptr);
    }

    X25519KeyPair pair{};
    if (rc == 0) {
        rc = mbedtls_mpi_write_binary_le(&d, pair.privateKey.data(), pair.privateKey.size());
    }
    size_t olen = 0;
    if (rc == 0) {
        // Su Curve25519 la forma "uncompressed" e' la sola u-coordinate,
        // scritta in little-endian su 32 byte.
        rc = mbedtls_ecp_point_write_binary(&grp, &Q, MBEDTLS_ECP_PF_UNCOMPRESSED, &olen,
            pair.publicKey.data(), pair.publicKey.size());
    }

    mbedtls_ecp_point_free(&Q);
    mbedtls_mpi_free(&d);
    mbedtls_ecp_group_free(&grp);

    if (rc != 0 || olen != pair.publicKey.size()) {
        throw std::runtime_error("mbedtls x25519 keypair generation failed -rc: " +
            std::to_string(rc) + " -olen: " + std::to_string(olen));
    }
    return pair;
}


std::string sha256Hex(const std::string& input) {
    unsigned char digest[32];
    mbedtls_sha256((const unsigned char*)input.data(), input.size(), digest, 0);

    char hex[65];
    for (int i = 0; i < 32; i++) {
        std::snprintf(hex + i * 2, 3, "%02x", digest[i]);
    }
    return std::string(hex, 64);
}

AesKey256 sha256Raw(const uint8_t* input, size_t len) {
    AesKey256 digest;
    mbedtls_sha256(input, len, digest.data(), 0);
    return digest;
}

static void writeLE32(uint8_t* out, uint32_t v) {
    out[0] = (uint8_t)(v);
    out[1] = (uint8_t)(v >> 8);
    out[2] = (uint8_t)(v >> 16);
    out[3] = (uint8_t)(v >> 24);
}

std::vector<uint8_t> channelCryptCtr(const std::vector<uint8_t>& pskKey,
    uint32_t packetId,
    uint32_t fromNode,
    const std::vector<uint8_t>& data) {
    if (pskKey.size() != 16 && pskKey.size() != 32) {
        throw std::invalid_argument("channel PSK must be 16 or 32 bytes");
    }

    uint8_t nonceCounter[16] = { 0 };
    // id e' zero-esteso a 64 bit, little-endian
    writeLE32(nonceCounter, packetId);
    writeLE32(nonceCounter + 4, 0);
    writeLE32(nonceCounter + 8, fromNode);
    writeLE32(nonceCounter + 12, 0); // extraNonce, zero per la PSK

    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    mbedtls_aes_setkey_enc(&ctx, pskKey.data(), (unsigned int)(pskKey.size() * 8));

    std::vector<uint8_t> out(data.size());
    uint8_t streamBlock[16] = { 0 };
    size_t ncOff = 0;
    mbedtls_aes_crypt_ctr(&ctx, data.size(), &ncOff, nonceCounter, streamBlock,
        data.data(), out.data());

    mbedtls_aes_free(&ctx);
    return out;
}

X25519Key x25519SharedSecret(const X25519Key& myPrivateKey, const X25519Key& peerPublicKey) {
    mbedtls_ecdh_context ctx;
    mbedtls_ecdh_init(&ctx);

    int rc = mbedtls_ecdh_setup(&ctx, MBEDTLS_ECP_DP_CURVE25519);
    if (rc != 0) {
        mbedtls_ecdh_free(&ctx);
        throw std::runtime_error("mbedtls_ecdh_setup failed");
    }

    // Carica la nostra chiave privata statica e la chiave pubblica statica
    // del peer nel contesto ECDH tramite il percorso standard "importa una
    // keypair esistente" (mbedtls_ecdh_get_params), invece della coppia
    // make_public/read_public, pensata per chiavi effimere generate da RNG
    // in stile TLS.
    const X25519Key clampedPrivateKey = clampX25519PrivateKey(myPrivateKey);

    mbedtls_ecp_keypair ours;
    mbedtls_ecp_keypair_init(&ours);
    mbedtls_ecp_group_load(&ours.grp, MBEDTLS_ECP_DP_CURVE25519);
    rc = mbedtls_mpi_read_binary_le(&ours.d, clampedPrivateKey.data(), clampedPrivateKey.size());
    if (rc == 0) {
        rc = mbedtls_ecdh_get_params(&ctx, &ours, MBEDTLS_ECDH_OURS);
    }
    mbedtls_ecp_keypair_free(&ours);
    if (rc != 0) {
        mbedtls_ecdh_free(&ctx);
        throw std::runtime_error("failed to import private key into ecdh context");
    }

    mbedtls_ecp_keypair theirs;
    mbedtls_ecp_keypair_init(&theirs);
    mbedtls_ecp_group_load(&theirs.grp, MBEDTLS_ECP_DP_CURVE25519);
    rc = mbedtls_ecp_point_read_binary(&theirs.grp, &theirs.Q, peerPublicKey.data(), peerPublicKey.size());
    if (rc == 0) {
        rc = mbedtls_ecdh_get_params(&ctx, &theirs, MBEDTLS_ECDH_THEIRS);
    }
    mbedtls_ecp_keypair_free(&theirs);
    if (rc != 0) {
        mbedtls_ecdh_free(&ctx);
        throw std::runtime_error("failed to import peer public key into ecdh context");
    }

    X25519Key shared;
    size_t olen = 0;
    rc = mbedtls_ecdh_calc_secret(&ctx, &olen, shared.data(), shared.size(), cryptoRng, nullptr);
    mbedtls_ecdh_free(&ctx);
    if (rc != 0 || olen != shared.size()) {
        throw std::runtime_error("mbedtls_ecdh_calc_secret failed -rc: " + std::to_string(rc) + " -olen: "+ std::to_string(olen));
    }
    return shared;
}

AesKey256 deriveAesKey(const X25519Key& sharedSecret) {
    return sha256Raw(sharedSecret.data(), sharedSecret.size());
}

static void ccmNonce(uint8_t out[13], uint32_t packetId, uint32_t extraNonce, uint32_t fromNode) {
    writeLE32(out, packetId);
    writeLE32(out + 4, extraNonce);
    writeLE32(out + 8, fromNode);
    out[12] = 0x00;
}

CcmCiphertext ccmEncrypt(const AesKey256& aesKey,
    uint32_t packetId,
    uint32_t extraNonce,
    uint32_t fromNode,
    const std::vector<uint8_t>& plaintext) {
    uint8_t nonce[13];
    ccmNonce(nonce, packetId, extraNonce, fromNode);

    mbedtls_ccm_context ctx;
    mbedtls_ccm_init(&ctx);
    mbedtls_ccm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, aesKey.data(), 256);

    CcmCiphertext result;
    result.ciphertext.resize(plaintext.size());

    int rc = mbedtls_ccm_encrypt_and_tag(&ctx, plaintext.size(), nonce, sizeof(nonce),
        nullptr, 0, plaintext.data(),
        result.ciphertext.data(),
        result.tag.data(), result.tag.size());
    mbedtls_ccm_free(&ctx);

    if (rc != 0) {
        throw std::runtime_error("mbedtls_ccm_encrypt_and_tag failed");
    }
    return result;
}

std::optional<std::vector<uint8_t>> ccmDecrypt( const AesKey256& aesKey,
                                                uint32_t packetId,
                                                uint32_t extraNonce,
                                                uint32_t fromNode,
                                                const std::vector<uint8_t>& ciphertext,
                                                const CcmTag& tag){
    
    uint8_t nonce[13];
    ccmNonce(nonce, packetId, extraNonce, fromNode);

    mbedtls_ccm_context ctx;
    mbedtls_ccm_init(&ctx);
    mbedtls_ccm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, aesKey.data(), 256);

    std::vector<uint8_t> plaintext(ciphertext.size());
    int rc = mbedtls_ccm_auth_decrypt(&ctx, ciphertext.size(), nonce, sizeof(nonce),
        nullptr, 0, ciphertext.data(), plaintext.data(),
        tag.data(), tag.size());
    mbedtls_ccm_free(&ctx);

    if (rc != 0) {
        return std::nullopt; // il tag non verifica -- scarta il pacchetto
    }
    return plaintext;
}

std::vector<uint8_t> payloadEncrypterBroadcast(std::vector<uint8_t> psk_key,
    uint8_t id[4],
    uint8_t from[4],
    std::vector<uint8_t> payload) {
    
    uint32_t from_int = litEndiConverter(from, 4);
    uint32_t id_int = litEndiConverter(id, 4);


    std::vector<uint8_t> encr_payload = channelCryptCtr(psk_key, id_int, from_int, payload);
    return(encr_payload);
}




X25519Key stringToShSecret(std::string priv_key_str, std::string publ_key_str) {
    //Packet pk;

    std::vector<uint8_t> priv_key_v;
    std::vector<uint8_t> publ_key_v;

    hexStringToByte(priv_key_str, priv_key_v);
    hexStringToByte(publ_key_str, publ_key_v);

    if(priv_key_str.length()!=64 || publ_key_str.length()!=64){
        throw std::invalid_argument("public or private key passed to stringToShSecret has an invalid length (len!=64)");
    }

    X25519Key priv_key{};
    X25519Key publ_key{};

    for (size_t i = 0; i < priv_key.size() && i < priv_key_v.size(); ++i) {
        priv_key.at(i) = static_cast<unsigned int>(priv_key_v.at(i));
    }

    for (size_t i = 0; i < publ_key.size() && i < publ_key_v.size(); ++i) {
        publ_key.at(i) = static_cast<unsigned int>(publ_key_v.at(i));
    }

    X25519Key shared_secret = x25519SharedSecret(priv_key, publ_key);
    return(shared_secret);
}

std::vector<uint8_t> x25519KeyToVector(X25519Key key) {
   std::vector<uint8_t> key_v(key.size());
    for (size_t i = 0;key.size() > i;i++) {
        key_v.at(i) = key.at(i);
    }
    return(key_v);
}


X25519Key stringToAesKey(std::string priv_key, std::string publ_key){
    X25519Key sh_secret=stringToShSecret(priv_key,publ_key);
    X25519Key aes_key = deriveAesKey(sh_secret);
    return(aes_key);
}

std::vector<uint8_t> decryptMessageDM(std::vector<uint8_t> message_data, X25519Key aes_key){
    
    uint32_t id=0; 
    uint32_t extra_nonce=0; 
    uint32_t from=0;
    CcmTag tag;

    int sz_msg = static_cast<int>(message_data.size());
    
    if(sz_msg<28){
        throw std::invalid_argument("passed bad vector, decryptPayloadDM got a vector containing less than 28 elements");
    }
   
    //extra nonce on bytes -4 to -1, from bytes 4 to 7, id bytes 8 to 11
    
    from =litEndiConverter(message_data,4,7);
    id = litEndiConverter(message_data,8,11);
    extra_nonce = litEndiConverter(message_data,sz_msg-4,sz_msg-1);
    
    for(size_t i=0;i<tag.size();i++){
        //size of tag == 8, from bytes -12 to -5 in the message_data vector
        tag.at(i)= message_data.at(message_data.size()-12+i);
    }

    const std::vector<uint8_t> cipher_text(message_data.begin()+16,message_data.end()-12);
    //chiper_text == payload without last 12 bytes

    auto decrypted_payload = ccmDecrypt(aes_key,id,extra_nonce,from,cipher_text,tag);
    if (!decrypted_payload.has_value()) {
        throw std::runtime_error("decryptPayloadDM failed to decrypt packet");
    }
    return *decrypted_payload;
}
