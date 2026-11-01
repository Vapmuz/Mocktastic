/*
Test per il wrapper crypto.h/crypto.cpp basato su mbedtls. Incluso solo da
functions_test.cpp sotto #ifdef TESTS, stesso schema di functions.cpp.

I valori attesi per AES-CTR e AES-CCM sono stati calcolati in modo
indipendente con pycryptodome (non solo tramite round-trip col nostro
stesso codice), cosi' questi controlli individuano anche un layout del
nonce sbagliato o una dimensione di chiave errata, non solo un
disallineamento tra cifratura e decifratura.
*/


#include "doctest.h"
#include "../crypto.h"
#include "../crypto.cpp"
#include "../functions.h"
#include "test_functions.h"


TEST_CASE("sha256Hex of empty string") {
    CHECK(sha256Hex("") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST_CASE("sha256Hex of abc") {
    CHECK(sha256Hex("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST_CASE("channelCryptCtr matches independently computed AES-128-CTR vector") {
    // key = 00 01 02 ... 0f, packetId=0x12345678, from=0xAABBCCDD, extraNonce=0
    std::vector<uint8_t> key;
    for (int i = 0; i < 16; i++) key.push_back((uint8_t)i);

    std::vector<uint8_t> plaintext(strlen("hello meshtastic"));
    memcpy(plaintext.data(), "hello meshtastic", plaintext.size());

    auto ct = channelCryptCtr(key, 0x12345678, 0xAABBCCDD, plaintext);
    CHECK(toHex(ct) == "7d1d10949f6e68b5dfc258514da1cd98");

    // CTR e' simmetrico: decifrare il ciphertext restituisce il plaintext
    auto back = channelCryptCtr(key, 0x12345678, 0xAABBCCDD, ct);
    CHECK(back == plaintext);
}

TEST_CASE("channelCryptCtr rejects bad key sizes") {
    std::vector<uint8_t> badKey(10, 0);
    std::vector<uint8_t> data(4, 0);
    CHECK_THROWS(channelCryptCtr(badKey, 0, 0, data));
}

TEST_CASE("x25519SharedSecret matches RFC 7748 6.1 test vectors") {
    X25519Key alicePriv = {
        0x77,0x07,0x6d,0x0a,0x73,0x18,0xa5,0x7d,0x3c,0x16,0xc1,0x72,0x51,0xb2,0x66,0x45,
        0xdf,0x4c,0x2f,0x87,0xeb,0xc0,0x99,0x2a,0xb1,0x77,0xfb,0xa5,0x1d,0xb9,0x2c,0x2a
    };
    X25519Key bobPriv = {
        0x5d,0xab,0x08,0x7e,0x62,0x4a,0x8a,0x4b,0x79,0xe1,0x7f,0x8b,0x83,0x80,0x0e,0xe6,
        0x6f,0x3b,0xb1,0x29,0x26,0x18,0xb6,0xfd,0x1c,0x2f,0x8b,0x27,0xff,0x88,0xe0,0xeb
    };
    X25519Key expectedAlicePub = {
        0x85,0x20,0xf0,0x09,0x89,0x30,0xa7,0x54,0x74,0x8b,0x7d,0xdc,0xb4,0x3e,0xf7,0x5a,
        0x0d,0xbf,0x3a,0x0d,0x26,0x38,0x1a,0xf4,0xeb,0xa4,0xa9,0x8e,0xaa,0x9b,0x4e,0x6a
    };
    X25519Key bobPub = {
        0xde,0x9e,0xdb,0x7d,0x7b,0x7d,0xc1,0xb4,0xd3,0x5b,0x61,0xc2,0xec,0xe4,0x35,0x37,
        0x3f,0x83,0x43,0xc8,0x5b,0x78,0x67,0x4d,0xad,0xfc,0x7e,0x14,0x6f,0x88,0x2b,0x4f
    };
    X25519Key expectedShared = {
        0x4a,0x5d,0x9d,0x5b,0xa4,0xce,0x2d,0xe1,0x72,0x8e,0x3b,0xf4,0x80,0x35,0x0f,0x25,
        0xe0,0x7e,0x21,0xc9,0x47,0xd1,0x9e,0x33,0x76,0xf0,0x9b,0x3c,0x1e,0x16,0x17,0x42
    };

    auto sharedFromAlice = x25519SharedSecret(alicePriv, bobPub);
    auto sharedFromBob = x25519SharedSecret(bobPriv, expectedAlicePub);

    CHECK(sharedFromAlice == expectedShared);
    CHECK(sharedFromAlice == sharedFromBob);
}

TEST_CASE("deriveAesKey is SHA256 of the shared secret") {
    X25519Key shared;
    for (int i = 0; i < 32; i++) shared[i] = (uint8_t)i;

    auto key = deriveAesKey(shared);
    auto expected = sha256Raw(shared.data(), shared.size());
    CHECK(key == expected);
}

TEST_CASE("ccmEncrypt matches independently computed AES-256-CCM vector") {
    AesKey256 key;
    for (int i = 0; i < 32; i++) key[i] = (uint8_t)i;

    std::vector<uint8_t> pt(strlen("direct message!!"));
    memcpy(pt.data(), "direct message!!", pt.size());

    auto result = ccmEncrypt(key, 0x01020304, 0x05060708, 0x090a0b0c, pt);

    CHECK(toHex(result.ciphertext) == "2937a87807f48be504274f1a2efaf61f");
    CHECK(toHex(result.tag.data(), result.tag.size()) == "d8af58331195c1ca");
}

TEST_CASE("ccmDecrypt round-trips and rejects tampered tags") {
    AesKey256 key;
    for (int i = 0; i < 32; i++) key[i] = (uint8_t)(i * 3 + 1);

    std::vector<uint8_t> pt(strlen("packet body payload"));
    memcpy(pt.data(), "packet body payload", pt.size());

    auto enc = ccmEncrypt(key, 42, 7, 99, pt);

    auto dec = ccmDecrypt(key, 42, 7, 99, enc.ciphertext, enc.tag);
    REQUIRE(dec.has_value());
    CHECK(*dec == pt);

    CcmTag badTag = enc.tag;
    badTag[0] ^= 0xFF;
    auto decBad = ccmDecrypt(key, 42, 7, 99, enc.ciphertext, badTag);
    CHECK_FALSE(decBad.has_value());

    // anche un campo del nonce sbagliato (fromNode) deve far fallire l'autenticazione
    auto decWrongFrom = ccmDecrypt(key, 42, 7, 100, enc.ciphertext, enc.tag);
    CHECK_FALSE(decWrongFrom.has_value());
}

TEST_CASE("encrypt/decrypt message first tests") {
    // key = 00 01 02 ... 0f, packetId=0x12345678, from=0xAABBCCDD, extraNonce=0
    std::vector<uint8_t> key;
    for (int i = 0; i < 16; i++) key.push_back((uint8_t)i);

    //make a vector the size of "hello meshtastic"
    std::vector<uint8_t> plaintext(strlen("Leo Pirata"));
    memcpy(plaintext.data(), "Leo Pirata", plaintext.size()); //copy hello into the vector

    auto ct = channelCryptCtr(key, 0x12345678, 0xAABBCCDD, plaintext);
    CHECK(toHex(ct) == "591d13d8a02777b1d8cb");

    // CTR e' simmetrico: decifrare il ciphertext restituisce il plaintext
    auto back = channelCryptCtr(key, 0x12345678, 0xAABBCCDD, ct);
    CHECK(back == plaintext); //controlla che sia Leo Pirata 
}

TEST_CASE("decrypt real Meshtastic broadcast message") {


    //to decrypt it I have to take only the bytes after the fifteenth,
    //because we don't want the header
    //fffffffffc8eb3f96ed13ce7631f00fca5fef76860cd9b9bf3cf1a050e076e6f0db40468dc is the message
    Packet pk;
    struct Packet::header h;

    std::vector<uint8_t> tst_payload = { 165, 254, 247, 104, 96, 205, 155, 155, 243, 207, 26, 5, 14, 7, 110, 111, 13, 180, 4, 104, 220 };
    std::string message = "fffffffffc8eb3f96ed13ce7631f00fca5fef76860cd9b9bf3cf1a050e076e6f0db40468dc";
    //                     1...2...3...4...5...6...7...8     
    std::vector<uint8_t> pubblic_key{
   0xd4,  0xf1,  0xbb,  0x3a,
   0x20,  0x29,  0x07,  0x59,
   0xf0,  0xbc,  0xff,  0xab,
   0xcf,  0x4e,  0x69,  0x01,
    };
    //common channel's key


    //save header
    pk.hexStringToData(message);
    pk.fillHeader(&h);

    std::vector<uint8_t> payload(pk.data.begin() + 16, pk.data.end());

    //put the payload into the payload vector


    CHECK(payload == tst_payload);
    CHECK(toHex(payload) == "a5fef76860cd9b9bf3cf1a050e076e6f0db40468dc");

    CHECK(toHex(pubblic_key) == "d4f1bb3a20290759f0bcffabcf4e6901");


    uint32_t id = litEndiConverter(h.id, 4);
    uint32_t from = litEndiConverter(h.from, 4);

    //CHECK(pk.makeStringHeader()=="x");
    CHECK(id == 0xe73cd16e);
    CHECK(from == 0xf9b38efc);

    std::vector<uint8_t> decripted = channelCryptCtr(pubblic_key, id, from, payload);
    //decript
    CHECK(toHex(decripted) == "0801120f4b6c65696e65207061706167656e6f4800");
    /*expected value:        0801120f4b6c65696e65207061706167656e6f4800*/
    std::vector<uint8_t> decripted_bytes = { 8, 1, 18, 15, 75, 108, 101, 105, 110, 101, 32, 112, 97, 112, 97, 103, 101, 110, 111, 72, 0 };


    CHECK(decripted == decripted_bytes);
}

TEST_CASE("test an implementation of payload encryption") {
    /*
        ID: "12341234"¨
        from: "56785678"
    */
    Packet pk;

    std::vector<uint8_t> pubblic_key{
   0xd4,  0xf1,  0xbb,  0x3a,
   0x20,  0x29,  0x07,  0x59,
   0xf0,  0xbc,  0xff,  0xab,
   0xcf,  0x4e,  0x69,  0x01,
    };

    uint8_t id[4] = { 0x12,0x34,0x12,0x34 };
    uint8_t from[4] = { 0x56,0x78,0x56,0x78 };

    uint32_t from_int = litEndiConverter(from, 4);
    uint32_t id_int = litEndiConverter(id, 4);

    std::vector<uint8_t> v_payload(strlen("tutti morimmo a stento"));
    memcpy(v_payload.data(), "tutti morimmo a stento", v_payload.size());

    std::vector<uint8_t> expected_v = { 116, 117, 116, 116, 105, 32, 109, 111, 114, 105, 109, 109, 111, 32, 97, 32, 115, 116, 101, 110, 116, 111 };
    CHECK(v_payload == expected_v);

    std::vector<uint8_t> encr_payload = channelCryptCtr(pubblic_key, id_int, from_int, v_payload);
    std::string expected_encr_payload = "f3a141fff27d151384413127de79e51a2f1d15c6dedb";

    CHECK(toHex(encr_payload) == expected_encr_payload);

    //the encryption is symmetrical, hence I can do the same thing I did to encrypt to decrypt a payload
    std::vector<uint8_t> decr_payload = channelCryptCtr(pubblic_key, id_int, from_int, encr_payload);
    CHECK(decr_payload == v_payload); //tutti morimmo a stento


}


TEST_CASE("deriveAesKey"){
    Packet pk;

    std::string leo1_priv = {
        "b0fdb0e27f48ad404ee7fe340ae81322ec0bda50371268b838f2a0e22dabe75d"
    };
    std::string leo1_publ{
        "5886a580f2fa72aa0e17d3fbd2e5acd804ae8915aa2718030dbd0a449aca247e"
    };
    std::string zepu_publ{
       "f355b53990d016e7c4722aaa8790f84bd45494723a3be3689c93978da1924a08"
    };
    std::string  zepu_priv{
       "c80207fa7a245842762ccc395bd356457688e05d2f9c1466323c7684b5a28b5c"
        //  0...1...2...3...4...5...6...7...8...9...a...b...c...d...e...f... 
    };

    X25519Key shared_secret_1 = stringToShSecret(leo1_priv, zepu_publ);
    X25519Key shared_secret_2= stringToShSecret(zepu_priv, leo1_publ);

    CHECK(shared_secret_1==shared_secret_2);

    X25519Key aes_key =deriveAesKey(shared_secret_1);
    CHECK(aes_key == stringToAesKey(leo1_priv,zepu_publ));   
    
    std::vector<uint8_t> aes_key_v = x25519KeyToVector(aes_key);
    std::vector<uint8_t> expected_uint8_v{ 231, 248, 127, 151, 91, 177, 91, 157, 230, 189, 167, 252, 182, 14, 12, 120, 247, 92, 28, 188, 244, 242, 238, 186, 123, 130, 9, 112, 183, 62, 145, 191 };
    
    CHECK(expected_uint8_v==aes_key_v);

    CHECK(bufferToString(aes_key_v.data(),aes_key_v.size())=="e7f87f975bb15b9de6bda7fcb60e0c78f75c1cbcf4f2eeba7b820970b73e91bf");
}

X25519Key stringToKey(std::string s){

    std::vector<uint8_t> priv_key_v;

    hexStringToByte(s, priv_key_v);

    X25519Key priv_key{};

    for (size_t i = 0; i < priv_key.size() && i < priv_key_v.size(); ++i) {
        priv_key.at(i) = static_cast<unsigned int>(priv_key_v.at(i));
    }

    return(priv_key);
}

TEST_CASE("scambio chiavi non riuscito"){
//priv: c80207fa7a245842762ccc395bd356457688e05d2f9c1466323c7684b5a28b5c publ_key: 5886a580f2fa72aa0e17d3fbd2e5acd804ae8915aa2718030dbd0a449aca247e
    std::string priv_key_s = "c80207fa7a245842762ccc395bd356457688e05d2f9c1466323c7684b5a28b5c";
    std::string publ_key_s = "5886a580f2fa72aa0e17d3fbd2e5acd804ae8915aa2718030dbd0a449aca247e";

    X25519Key priv_key = stringToKey(priv_key_s);
    X25519Key publ_key = stringToKey(publ_key_s);

    CHECK_NOTHROW(x25519SharedSecret(priv_key, publ_key));
}

TEST_CASE("stringToShSecret") {

    std::string alice_priv = {
        "b0fdb0e27f48ad404ee7fe340ae81322ec0bda50371268b838f2a0e22dabe75d"
    };
    std::string alice_publ{
        "5886a580f2fa72aa0e17d3fbd2e5acd804ae8915aa2718030dbd0a449aca247e"
    };
    std::string  bob_priv{
       "c80207fa7a245842762ccc395bd356457688e05d2f9c1466323c7684b5a28b5c"
    };
    std::string bob_publ{
       "f355b53990d016e7c4722aaa8790f84bd45494723a3be3689c93978da1924a08"
    };

    CHECK_NOTHROW(stringToShSecret(alice_priv, bob_publ));
    X25519Key sh_secret_1 = stringToShSecret(alice_priv, bob_publ);
    X25519Key sh_secret_2 = stringToShSecret(bob_priv, alice_publ);
    CHECK(sh_secret_1 == sh_secret_2);

// test case: public or private key has an incorrect length
    std::string bad_string = "abcbababababbabababababbababababab"; //valid hex string
    CHECK_THROWS(stringToShSecret(bad_string,alice_publ));
}

TEST_CASE("x25519KeyToVector"){
    X25519Key pippo = {0xff,0xab,0xcc};
    std::vector<uint8_t> tst = x25519KeyToVector(pippo);
    std::vector<uint8_t> expected = {0xff, 0xab, 0xcc, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    
    CHECK(tst==expected);
}


TEST_CASE("payloadEncrypter") {
    /*
       ID: "12341234"¨
       from: "56785678"
   */
    Packet pk;

    std::vector<uint8_t> pubblic_key{
   0xd4,  0xf1,  0xbb,  0x3a,
   0x20,  0x29,  0x07,  0x59,
   0xf0,  0xbc,  0xff,  0xab,
   0xcf,  0x4e,  0x69,  0x01,
    };

    uint8_t id[4] = { 0x12,0x34,0x12,0x34 };
    uint8_t from[4] = { 0x56,0x78,0x56,0x78 };


    std::vector<uint8_t> v_payload(strlen("tutti morimmo a stento"));
    memcpy(v_payload.data(), "tutti morimmo a stento", v_payload.size());

    std::vector<uint8_t> expected_v = { 116, 117, 116, 116, 105, 32, 109, 111, 114, 105, 109, 109, 111, 32, 97, 32, 115, 116, 101, 110, 116, 111 };
    CHECK(v_payload == expected_v);

    std::vector<uint8_t> encr_payload = payloadEncrypterBroadcast(pubblic_key, id, from, v_payload);
    std::string expected_encr_payload = "f3a141fff27d151384413127de79e51a2f1d15c6dedb";

    CHECK(toHex(encr_payload) == expected_encr_payload);

    //the encryption is symmetrical, hence I can do the same thing I did to encrypt to decrypt a payload
    std::vector<uint8_t> decr_payload = payloadEncrypterBroadcast(pubblic_key, id, from, encr_payload);
    CHECK(decr_payload == v_payload); //tutti morimmo a stento


}

TEST_CASE("decryptMessageDM"){
    Packet pk;

    std::string message = "34123412fc8eb3f9ab2b05546b0000fceecb3f24aadd679ddc7b6c2701354a0f5bbb7f675613d148b63ba5934c920c095e0376";
    //                     0...1...2...3...4...5...6...7...8...                                  ---1---2---3---4---5---6---7---8
    std::string leo1_priv = {
        "b0fdb0e27f48ad404ee7fe340ae81322ec0bda50371268b838f2a0e22dabe75d"
    };
    std::string leo1_publ{
        "5886a580f2fa72aa0e17d3fbd2e5acd804ae8915aa2718030dbd0a449aca247e"
    };
    std::string zepu_publ{
       "f355b53990d016e7c4722aaa8790f84bd45494723a3be3689c93978da1924a08"
    };
    std::string  zepu_priv{
       "c80207fa7a245842762ccc395bd356457688e05d2f9c1466323c7684b5a28b5c"
    };

    X25519Key aes_key = stringToAesKey(leo1_priv, zepu_publ);
    pk.hexStringToData(message);

    std::vector<uint8_t> plain_message = decryptMessageDM(pk.data,aes_key);
    std::vector<uint8_t> expected_message = {8, 1, 18, 17, 76, 97, 32, 114, 105, 115, 112, 111, 115, 116, 97, 32, 195, 168, 32, 52, 50, 72, 0};

    CHECK(toHex(plain_message)=="080112114c6120726973706f73746120c3a82034324800");

    CHECK(plain_message == expected_message);
}


TEST_CASE("ccmNonce"){
    Packet pk; 
    std::string message = "34123412fc8eb3f9ab2b05546b0000fceecb3f24aadd679ddc7b6c2701354a0f5bbb7f675613d148b63ba5934c920c095e0376";
    std::vector<uint8_t> message_data;
    hexStringToByte(message,message_data);
    
    uint32_t pk_id;
    uint32_t from;
    uint32_t extra_nonce;
    

    int sz_msg = static_cast<int>(message_data.size());

    for(int i =0;i<4;i++){
        //extra nonce on bytes -4 to -1, from bytes 4 to 7, id bytes 8 to 11
        from =litEndiConverter(message_data,4,7);
        pk_id =litEndiConverter(message_data,8, 11);
        extra_nonce = litEndiConverter(message_data,sz_msg-4, sz_msg-1);
    }
    
    uint8_t bf[13];
    ccmNonce(bf,pk_id,extra_nonce,from);

    
    CHECK(bufferToString(bf,13)== "ab2b0554095e0376fc8eb3f900");
}



TEST_CASE("generateX25519KeyPair genera chiavi valide Curve25519") {
    X25519KeyPair key1 = generateX25519KeyPair();
    X25519KeyPair key2 = generateX25519KeyPair();

    MESSAGE(
        "Key1 Pvt: ", base64Encoder(x25519KeyToVector(key1.privateKey)),
        "\n Pub: ", base64Encoder(x25519KeyToVector(key1.publicKey)),
        "\n hex ",
        "\n pvt: ", toHex(x25519KeyToVector(key1.privateKey)),
        "\n publ: ", toHex(x25519KeyToVector(key1.publicKey))
    );
    MESSAGE(
        "key2 Pvt: ", base64Encoder(x25519KeyToVector(key2.privateKey)),
        "\n Pub: ", base64Encoder(x25519KeyToVector(key2.publicKey)),
        "\n hex ",
        "\n pvt: ", toHex(x25519KeyToVector(key2.privateKey)),
        "\n publ: ", toHex(x25519KeyToVector(key2.publicKey))
    );


    // due generazioni consecutive devono dare chiavi diverse
    CHECK(key1.privateKey != key2.privateKey);
    CHECK(key1.publicKey != key2.publicKey);

    // la privata esce gia' clampata secondo RFC 7748 5
    CHECK(clampX25519PrivateKey(key1.privateKey) == key1.privateKey);
    CHECK(clampX25519PrivateKey(key2.privateKey) == key2.privateKey);

    // ECDH: i due lati devono derivare lo stesso segreto condiviso
    X25519Key shared1 = x25519SharedSecret(key1.privateKey, key2.publicKey);
    X25519Key shared2 = x25519SharedSecret(key2.privateKey, key1.publicKey);

    MESSAGE("shared secret: ", base64Encoder(x25519KeyToVector(shared1)));

    CHECK(shared1 == shared2);
    CHECK(shared1 != X25519Key{}); // mai il segreto tutto-zero (punto di ordine piccolo)

    // e la chiave AES derivata dal segreto e' la stessa per entrambi
    CHECK(deriveAesKey(shared1) == deriveAesKey(shared2));
}
