/*
Test per il wrapper meshtastic.h/meshtastic.cpp: crea un messaggio
meshtastic_User, lo codifica e lo decodifica, verificando che il
round-trip restituisca esattamente i dati originali. Incluso solo da
functions_test.cpp sotto #ifdef TESTS, stesso schema di functions.cpp e
crypto_test.cpp.
*/

#include "doctest.h"
#include "../meshtastic.h"
#include "../meshtastic.cpp"
#include "test_functions.h"
#include "../keys.h"


TEST_CASE("crea e codifica/decodifica un messaggio meshtastic_User") {
    MeshtasticUser user;
    user.id = "!deadbeef";
    user.longName = "Test Node";
    user.shortName = "TN";
    user.publicKey = std::string(32, '\x01'); // chiave X25519 fittizia, 32 byte
    user.hwModel = meshtastic_HardwareModel_TBEAM;
    user.isLicensed = true;
    user.role = meshtastic_Config_DeviceConfig_Role_CLIENT;

    auto buffer = encodeUser(user);
    REQUIRE(!buffer.empty());

    auto decoded = decodeUser(buffer);
    REQUIRE(decoded.has_value());

    CHECK(decoded->id == user.id);
    CHECK(decoded->longName == user.longName);
    CHECK(decoded->shortName == user.shortName);
    CHECK(decoded->publicKey == user.publicKey);
    CHECK(decoded->hwModel == user.hwModel);
    CHECK(decoded->isLicensed == user.isLicensed);
    CHECK(decoded->role == user.role);
}

TEST_CASE("encodeUser lancia se lo User non entra nel buffer") {
    MeshtasticUser user;
    user.id = "!deadbeef";
    user.longName = std::string(300, 'A'); // oltre i 256 byte del buffer
    CHECK_THROWS_AS(encodeUser(user), std::invalid_argument);
}

TEST_CASE("decodeUser ritorna nullopt su un buffer non valido") {
    std::vector<uint8_t> garbage = { 0xFF, 0xFF, 0xFF, 0xFF };
    auto decoded = decodeUser(garbage);
    CHECK_FALSE(decoded.has_value());
}



TEST_CASE("crea un Data con portnum NODEINFO che contiene uno User, codifica e decodifica") {
    MeshtasticUser user;
    user.id = "!deadbeef";
    user.longName = "Test Node";
    user.shortName = "TN";
    user.publicKey = std::string(32, '\x01');
    user.hwModel = meshtastic_HardwareModel_TBEAM;
    user.isLicensed = true;
    user.role = meshtastic_Config_DeviceConfig_Role_CLIENT;

    auto userBytes = encodeUser(user);
    REQUIRE(!userBytes.empty());

    MeshtasticData data;
    data.portnum = meshtastic_PortNum_NODEINFO_APP;
    data.payload = std::string(userBytes.begin(), userBytes.end());
    //metti le informazioni in un buffer

    auto dataBytes = encodeData(data);
    REQUIRE(!dataBytes.empty());
    //codifica i dati in un formato protobuf

    // "torna in una nuova struct": decodifica in un oggetto Data separato,
    // poi lo User annidato nel suo payload, in una nuova struct a sua volta.
    auto decodedData = decodeData(dataBytes);
    REQUIRE(decodedData.has_value());
    CHECK(decodedData->portnum == meshtastic_PortNum_NODEINFO_APP);

    std::vector<uint8_t> decodedUserBytes(decodedData->payload.begin(), decodedData->payload.end());
    auto decodedUser = decodeUser(decodedUserBytes);
    REQUIRE(decodedUser.has_value());

    CHECK(decodedUser->id == user.id);
    CHECK(decodedUser->longName == user.longName);
    CHECK(decodedUser->shortName == user.shortName);
    CHECK(decodedUser->publicKey == user.publicKey);
    CHECK(decodedUser->hwModel == user.hwModel);
    CHECK(decodedUser->isLicensed == user.isLicensed);
    CHECK(decodedUser->role == user.role);
}



/*
Il test è stato commentato perché non funzionava. errore:

test/meshtastic_test.cpp:109: ERROR: CHECK( result->portnum == meshtastic_PortNum_TEXT_MESSAGE_APP ) is NOT correct!
  values: CHECK( 4034793728 == 1 )

*/

TEST_CASE("read decrypted message") {
    MeshtasticData data;
    MeshtasticUser user;


    //    0801120f4b6c65696e65207061706167656e6f4800

    std::string bytes = "0801120f4b6c65696e65207061706167656e6f4800";
    std::vector<uint8_t> raw_bytes;
    hexStringToByte(bytes, raw_bytes);
    auto result = decodeData(raw_bytes);

    REQUIRE(result.has_value());
    CHECK(result->portnum == meshtastic_PortNum_TEXT_MESSAGE_APP);
    CHECK(result->payload == "Kleine papageno");
    //legge dati dal formato protobuf
}


TEST_CASE("decryptData") {
    Packet pk;

    std::vector<uint8_t> pubblic_key{
   0xd4,  0xf1,  0xbb,  0x3a,
   0x20,  0x29,  0x07,  0x59,
   0xf0,  0xbc,  0xff,  0xab,
   0xcf,  0x4e,  0x69,  0x01,
    };


    std::string message = "fffffffffc8eb3f96ed13ce7631f00fca5fef76860cd9b9bf3cf1a050e076e6f0db40468dc";

    pk.hexStringToData(message);//the message

    std::vector<uint8_t> raw_bytes = decryptBroadcast(pk.data, pubblic_key);

    //gets data, return the payload decrypted as vector
    CHECK(toHex(raw_bytes) == "0801120f4b6c65696e65207061706167656e6f4800");

    std::vector<uint8_t> bad_vector = { 0x00,0x00 };
    CHECK_THROWS(decryptBroadcast(bad_vector, pubblic_key));
}



TEST_CASE("readProto") {
    Packet pk;

    std::vector<uint8_t> pubblic_key{
   0xd4,  0xf1,  0xbb,  0x3a,
   0x20,  0x29,  0x07,  0x59,
   0xf0,  0xbc,  0xff,  0xab,
   0xcf,  0x4e,  0x69,  0x01,
    };


    std::string message = "fffffffffc8eb3f96ed13ce7631f00fca5fef76860cd9b9bf3cf1a050e076e6f0db40468dc";
    pk.hexStringToData(message);//the message
    std::vector<uint8_t> raw_bytes = decryptBroadcast(pk.data, pubblic_key);

    auto result = readProto(raw_bytes);

    REQUIRE(result.has_value());
    CHECK(result->payload == "Kleine papageno");
    CHECK(result->portnum == meshtastic_PortNum_TEXT_MESSAGE_APP);

//messaggio non valido (corrotto)
    std::vector<uint8_t> v_1={8, 1, 18, 67, 67, 67, 67, 67, 0xca, 0xcc, 0xa0, 111, 115, 116, 97, 32, 195, 168, 32, 52, 50, 72, 0};
    CHECK_THROWS(readProto(v_1));    

//  messaggio DM
    std::vector<uint8_t> v={8, 1, 18, 17, 76, 97, 32, 114, 105, 115, 112, 111, 115, 116, 97, 32, 195, 168, 32, 52, 50, 72, 0};
    auto result1 =readProto(v);
    CHECK(result1->payload=="La risposta è 42");
}

TEST_CASE("xorVector") {
    std::vector<uint8_t> pippo{ 0x67,0x81,0x18,0x01 };
    std::vector<uint8_t> empty_v{};

    uint8_t result = xorVector(pippo);
    CHECK(result == 0xFF);
    CHECK_THROWS(xorVector(empty_v));
}

TEST_CASE("channelHash string"){
    uint8_t chHash= channelHash(Config::PublicChannel.key,Config::PublicChannel.name);
    CHECK(chHash==0x1f);
}

TEST_CASE("channelHash") {
    Packet pk;

    std::vector<uint8_t> psk_key;
    std::string channel_str = "MediumFast";
    std::vector<uint8_t> channel_name(channel_str.begin(), channel_str.end());

    std::vector<uint8_t> check_channel_name{ 77, 101, 100, 105, 117, 109, 70, 97, 115, 116 };

    CHECK(channel_name == check_channel_name);

    psk_key = {
   0xd4,  0xf1,  0xbb,  0x3a,
   0x20,  0x29,  0x07,  0x59,
   0xf0,  0xbc,  0xff,  0xab,
   0xcf,  0x4e,  0x69,  0x01,
    };

    uint8_t hash_medium_fast = channelHash(psk_key, channel_name);

    CHECK(hash_medium_fast == 0x1f);

}

TEST_CASE("Decrypt message from a private channel (temp_sal)") {
    /*
    key : "c1BmUmhBV2w1UFVJS3I4VQ==" in hex: "735066526841576c355055494b723855"
    packet:
    "fffffffffc8eb3f979e51160630500fcd6bdec759a48e13b79b430961fe3a07d"
    */

    Packet pk;

    std::string psk_b64 = "c1BmUmhBV2w1UFVJS3I4VQ==";

    std::vector<uint8_t> pubblic_key = base64Decoder(psk_b64);


    std::string message = "fffffffffc8eb3f979e51160630500fcd6bdec759a48e13b79b430961fe3a07d";
    pk.hexStringToData(message);//the message

    std::vector<uint8_t> raw_bytes = decryptBroadcast(pk.data, pubblic_key);

    auto result = readProto(raw_bytes);

    REQUIRE(result.has_value());
    CHECK(result->payload == "Palla rosa");
    CHECK(result->portnum == meshtastic_PortNum_TEXT_MESSAGE_APP);
    //CHECK(result.)

}

TEST_CASE("strinToPb") {

    std::string message_carried = "Klein papageno";
    std::vector<uint8_t> pr_data = stringToPb(message_carried);

    auto result = readProto(pr_data);

    CHECK(result.has_value());
    CHECK(result->payload == message_carried);
    CHECK(result->portnum == meshtastic_PortNum_TEXT_MESSAGE_APP);

    std::string too_long(600, 'A'); // oltre i 512 byte del buffer di encodeData
    CHECK_THROWS_AS(stringToPb(too_long), std::invalid_argument);
}

TEST_CASE("makeHeaderDM") {
    Packet pk;
    Packet tst_pk;
    //                             1...2...3...4...5...6...7...8...
    std::string expected_string = "fffffffffc8eb3f979e51160630000fc";
    tst_pk.hexStringToData(expected_string);

    uint8_t to[4] = { 0xff, 0xff, 0xff, 0xff };
    uint8_t from[4] = { 0xfc, 0x8e, 0xb3, 0xf9 };
    uint8_t id[4] = { 0x79, 0xe5, 0x11, 0x60 };
    uint8_t hop_start = 0x03;
    uint8_t want_ack = 0x00;
    uint8_t received_with_mqtt = 0x00;
    //channel hash in 13 is 31
    uint8_t hop_limit = 0x03;
    uint8_t next_hop = 0x00;
    uint8_t relay_node = 0xfc;

    pk.data = makeHeaderDM(
        to,
        from,
        id,
        hop_start,
        want_ack,
        received_with_mqtt,
        hop_limit,
        next_hop,
        relay_node
    );

    REQUIRE(pk.data.size() == tst_pk.data.size());

    CHECK(toHex(pk.data) == toHex(tst_pk.data));
}

TEST_CASE("makeHeaderBroadcast") {
    Packet pk;
    Packet tst_pk;
    //                                1...2...3...4...5...6...7...8...
    std::string expected_string = "fffffffffc8eb3f979e51160630500fc";
    tst_pk.hexStringToData(expected_string);

    std::string psk_b64 = "c1BmUmhBV2w1UFVJS3I4VQ==";

    std::vector<uint8_t> psk_key = base64Decoder(psk_b64);

    uint8_t to[4] = { 0xff, 0xff, 0xff, 0xff };
    uint8_t from[4] = { 0xfc, 0x8e, 0xb3, 0xf9 };
    uint8_t id[4] = { 0x79, 0xe5, 0x11, 0x60 };
    uint8_t hop_start = 0x03;
    uint8_t want_ack = 0x00;
    uint8_t received_with_mqtt = 0x00;
    //channel hash in 13 is 31
    std::string channel_name = "MediumFast";
    std::vector<uint8_t> channel_name_v(channel_name.begin(), channel_name.end());
    uint8_t hop_limit = 0x03;
    uint8_t next_hop = 0x00;
    uint8_t relay_node = 0xfc;

    pk.data = makeHeaderBroadcast(
        psk_key,
        "temp_sal",
        to,
        from,
        id,
        hop_start,
        want_ack,
        received_with_mqtt,
        hop_limit,
        next_hop,
        relay_node
    );

    REQUIRE(pk.data.size() == tst_pk.data.size());

    CHECK(pk.data == tst_pk.data);
}

TEST_CASE("encodeMessageDM"){
    Packet pk;
    Packet tst_pk;
    std::string expected_string = "34123412fc8eb3f9ab2b05546b000000eecb3f24aadd679ddc7b6c2701354a0f5bbb7f675613d148b63ba5934c920c095e0376";
  //                               34123412fc8eb3f9ab2b05546b000000eecb3f24aadd679ddc7b6c2701354a0f5bbb7f675613d148b63ba5934c920c095e0376 
    tst_pk.hexStringToData(expected_string);

    
    std::string leo1_priv = {
        "b0fdb0e27f48ad404ee7fe340ae81322ec0bda50371268b838f2a0e22dabe75d"
    };
    std::string zepu_publ{
       "f355b53990d016e7c4722aaa8790f84bd45494723a3be3689c93978da1924a08"
    };

    X25519Key aes_key = stringToAesKey(leo1_priv, zepu_publ);

    std::string message = "La risposta è 42";
    uint8_t extra_nonce[4] = { 0x09, 0x5e, 0x03, 0x76 }; //095e0376
    uint8_t to[4] = { 0x34, 0x12, 0x34, 0x12 };
    uint8_t from[4] = { 0xfc, 0x8e, 0xb3, 0xf9 } ;
    uint8_t id[4] = { 0xab, 0x2b, 0x05, 0x54 }; //ab2b0554
    uint8_t hop_start = 0x03;
    uint8_t want_ack = 0x01;
    uint8_t received_with_mqtt = 0x00;
    //channel hash in 13 is 31
    uint8_t hop_limit = 0x03;
    uint8_t next_hop = 0x00;
    uint8_t relay_node = 0x00;
  
    CHECK_THROWS(encodeMessageDM(
        aes_key,
        message,
        extra_nonce,
        to,
        from,
        id,
        hop_start+8,
        want_ack,
        received_with_mqtt,
        hop_limit+8,
        next_hop,
        relay_node)
    );

    CHECK_THROWS(encodeMessageDM(
       aes_key,
        message,
        extra_nonce,
        to,
        from,
        id,
        hop_start,
        want_ack+2,
        received_with_mqtt+2,
        hop_limit,
        next_hop,
        relay_node)
    );

    std::string long_message="unmessaggiotroppokungounmessaggiotroppolungounmessaggiotroppolungounmessaggiotroppolungounmessaggiotroppolungounmessaggiotropppolungounmessaggiotroppolungounmessaggiotrroppolungounmessaggiotroppolungounmessaggiotroppolungo";

    CHECK_THROWS(encodeMessageDM(
        aes_key,
        long_message,
        extra_nonce,
        to,
        from,
        id,
        hop_start,
        want_ack,
        received_with_mqtt,
        hop_limit,
        next_hop,
        relay_node)
    );


    pk.data = encodeMessageDM(
        aes_key,
        message,
        extra_nonce,
        to,
        from,
        id,
        hop_start,
        want_ack,
        received_with_mqtt,
        hop_limit,
        next_hop,
        relay_node
    );

    CHECK(pk.data.size() == tst_pk.data.size());
    CHECK(pk.data == tst_pk.data);

    auto result = decryptMessageDM(pk.data, aes_key);

    REQUIRE(!result.empty());
    CHECK(toHex(result) == "080112114c6120726973706f73746120c3a82034324800");
    CHECK(toHex(pk.data) == expected_string);

    /*
    1...2...3...4...5...6...7...8...
    fffffffffc8eb3f979e51160630500fcd6bdec759a48e13b79b430961fe3 ==
    fffffffffc8eb3f979e51160630500fcd6bdec759a48e13b79b430961fe3a07d

    */


}

TEST_CASE("encodeMessageBroadcast") {
    Packet pk;
    Packet tst_pk;
    std::string expected_string = "fffffffffc8eb3f979e51160630500fcd6bdec759a48e13b79b430961fe3a07d";

    tst_pk.hexStringToData(expected_string);

    std::string psk_b64 = "c1BmUmhBV2w1UFVJS3I4VQ==";

    std::vector<uint8_t> psk_key = base64Decoder(psk_b64);

    uint8_t to[4] = { 0xff, 0xff, 0xff, 0xff };
    uint8_t from[4] = { 0xfc, 0x8e, 0xb3, 0xf9 };
    uint8_t id[4] = { 0x79, 0xe5, 0x11, 0x60 };
    uint8_t hop_start = 0x03;
    uint8_t want_ack = 0x00;
    uint8_t received_with_mqtt = 0x00;
    //channel hash in 13 is 31
    std::string channel_name = "MediumFast";
    std::vector<uint8_t> channel_name_v(channel_name.begin(), channel_name.end());
    uint8_t hop_limit = 0x03;
    uint8_t next_hop = 0x00;
    uint8_t relay_node = 0xfc;

    CHECK_THROWS(encodeMessageBroadcast(
        psk_key,
        "Palla rosa", //messaggio
        "temp_sal",
        to,
        from,
        id,
        hop_start,
        want_ack + 10,
        received_with_mqtt,
        hop_limit,
        next_hop,
        relay_node)
    );

    CHECK_THROWS(encodeMessageBroadcast(
        psk_key,
        "Palla rosa", //messaggio
        "temp_sal",
        to,
        from,
        id,
        hop_start,
        want_ack,
        received_with_mqtt + 10,
        hop_limit,
        next_hop,
        relay_node)
    );

    CHECK_THROWS(encodeMessageBroadcast(
        psk_key,
        "Palla rosa", //messaggio
        "temp_sal",
        to,
        from,
        id,
        hop_start + 10,
        want_ack,
        received_with_mqtt,
        hop_limit,
        next_hop,
        relay_node)
    );

    CHECK_THROWS(encodeMessageBroadcast(
        psk_key,
        "Palla rosa", //messaggio
        "temp_sal",
        to,
        from,
        id,
        hop_start,
        want_ack,
        received_with_mqtt,
        hop_limit + 10,
        next_hop,
        relay_node)
    );
    CHECK_THROWS(encodeMessageBroadcast(
        psk_key,
        "posso fare tutto quello che voglio nel messaggio, nessuno mi limita tranne i 200 caratteri che auspicabilmente incontrerò nel mio percorso e vediamo se ho fatto bene i throw. Però per farlo devo dilungarmi moooolto di più di quello che Romania farebbe lo sai? Poi penso di non aver mai testato questa funzionalità anche s emi ricordo di aver fatto il throw", //messaggio
        "temp_sal",
        to,
        from,
        id,
        hop_start,
        want_ack + 10,
        received_with_mqtt,
        hop_limit,
        next_hop,
        relay_node)
    );


    pk.data = encodeMessageBroadcast(
        psk_key,
        "Palla rosa", //messaggio
        "temp_sal",
        to,
        from,
        id,
        hop_start,
        want_ack,
        received_with_mqtt,
        hop_limit,
        next_hop,
        relay_node
    );

    CHECK(pk.data.size() == tst_pk.data.size());

    CHECK(pk.data == tst_pk.data);


    std::vector<uint8_t> raw_bytes = decryptBroadcast(pk.data, psk_key);

    auto result = readProto(raw_bytes);

    REQUIRE(result.has_value());
    CHECK(result->payload == "Palla rosa");
    CHECK(toHex(pk.data) == expected_string);
    CHECK(result->portnum == meshtastic_PortNum_TEXT_MESSAGE_APP);

    /*
    1...2...3...4...5...6...7...8...
    fffffffffc8eb3f979e51160630500fcd6bdec759a48e13b79b430961fe3 ==
    fffffffffc8eb3f979e51160630500fcd6bdec759a48e13b79b430961fe3a07d

    */


}
TEST_CASE("confronto lunga corta") {
    Packet pk;
    std::string message_short = "fffffffffc8eb3f979e51160630500fcd6bdec759a48e13b79b430961fe3";
    pk.hexStringToData(message_short);

    std::string psk_b64 = "c1BmUmhBV2w1UFVJS3I4VQ==";

    std::vector<uint8_t> psk_key = base64Decoder(psk_b64);


    std::vector<uint8_t> raw_bytes = decryptBroadcast(pk.data, psk_key);


    CHECK(toHex(raw_bytes) == "0801120a50616c6c6120726f7361");


    Packet pk_long;

    std::string message_long = "fffffffffc8eb3f979e51160630500fcd6bdec759a48e13b79b430961fe3a07d";

    pk_long.hexStringToData(message_long);

    std::vector<uint8_t> raw_bytes_long = decryptBroadcast(pk_long.data, psk_key);


    CHECK(toHex(raw_bytes_long) == "0801120a50616c6c6120726f73614800");
}




TEST_CASE("test messages for july the twentyseventh") {
    //All messages are for the temp_sal channel (hash 31)

    Packet pk;

    std::string psk_b64 = "c1BmUmhBV2w1UFVJS3I4VQ==";
    std::vector<uint8_t> psk_key = base64Decoder(psk_b64);

    uint8_t to[4] = { 0xff, 0xff, 0xff, 0xff };
    uint8_t from[4] = { 0xba, 0xbb, 0xbb, 0xba };
    uint8_t id[4] = { 0x69, 0xaf, 0x01, 0x35 };
    uint8_t hop_start = 0x03;
    uint8_t want_ack = 0x00;
    uint8_t received_with_mqtt = 0x00;
    //channel hash in 13 is 31
    std::string channel_name = "temp_sal";
    std::vector<uint8_t> channel_name_v(channel_name.begin(), channel_name.end());
    uint8_t hop_limit = 0x03;
    uint8_t next_hop = 0x00;
    uint8_t relay_node = 0xfc;



    pk.data = encodeMessageBroadcast(
        psk_key,
        "prova",
        channel_name,
        to,
        from,
        id,
        hop_start,
        want_ack,
        received_with_mqtt,
        hop_limit,
        next_hop,
        relay_node
    );

    std::string message = pk.dataToString();
    CHECK(message == "ffffffffbabbbbba69af0135630500fcab07521ca70411ce77cd0d");
}


TEST_CASE("decrypt message from private channel"){
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

    auto result = readProto(plain_message);
    CHECK(result->payload=="La risposta è 42");
}


TEST_CASE("broadcastMessageSh"){


    std::string key = Config::PublicChannel.key;
    std::string name = Config::PublicChannel.name;

    std::string expec_msg = "fffffffffc8eb3f96ed13ce7631f00fca5fef76860cd9b9bf3cf1a050e076e6f0db40468dc";
    //                       fffffffffc0eb3f96ed13ce7631f00fc03d47b9700bb537c1b3035eb3fc079c117afc3697d 
    uint8_t id[4] = {0x6e,0xd1,0x3c,0xe7}; //6ed13ce7
    uint8_t from[4] = {0xfc,0x8e,0xb3,0xf9}; //fc8eb3f9
    std::vector<uint8_t> msg_v = broadcastMessageSh(key,name,"Kleine papageno",id,from);
    CHECK(toHex(msg_v)==expec_msg);
}


TEST_CASE("DMMessageSh"){
    Packet pk;

    std::string expected_message = "34123412fc8eb3f9ab2b055463000000eecb3f24aadd679ddc7b6c2701354a0f5bbb7f675613d148b63ba5934c920c095e0376";
    //                              34123412fc8eb3f9ab2b055463000000eecb3f24aadd679ddc7b6c2701354a0f5bbb7f675613d148b63ba5934c920c095e0376
    
    
    std::string priv_key = Config::Device.privateKey;
    std::string publ_key = Config::Leo1.publicKey;

    uint8_t extra_nonce[4] ={0x09,0x5e,0x03,0x76}; //095e0376
    uint8_t id[4] = {0xab,0x2b,0x05,0x54}; 
    uint8_t from[4] = {0xfc,0x8e,0xb3,0xf9}; 
    uint8_t to[4] = {0x34,0x12,0x34,0x12};
    std::string msg = "La risposta è 42";
    
    std::vector<uint8_t> message = DMMessageSh(priv_key,publ_key,msg,extra_nonce,to,from,id);

    CHECK(toHex(message)==expected_message);

}


TEST_CASE("msgPayloadBroadcast"){
    Packet pk;

    pk.hexStringToData( "fffffffffc8eb3f96ed13ce7631f00fca5fef76860cd9b9bf3cf1a050e076e6f0db40468dc");
    CHECK("Kleine papageno"==msgPayloadBroadcast(pk.data,Config::PublicChannel.key));
}


TEST_CASE("msgPayloadDm"){
    std::string message = "34123412fc8eb3f9ab2b05546b0000fceecb3f24aadd679ddc7b6c2701354a0f5bbb7f675613d148b63ba5934c920c095e0376";
    std::vector<uint8_t> v;
    hexStringToByte(message,v);
    std::string payload = msgPayloadDM(v,Config::Leo1.privateKey,Config::Device.publicKey);
    CHECK(payload== "La risposta è 42");

    //create fake, non valid encoded message
}


TEST_CASE("encodePerson"){
    std::string chiave_pubblica = "MWz6SCOLHbu7KpzgquvW58j/Lh6R0ZgjnAwKUG92+Rk=";
    std::vector<uint8_t> chiave_decoded = base64Decoder(chiave_pubblica);
    std::string publ_key = vectorToHexString(chiave_decoded);

    
    //decryot zebrax packet
    std::string pk = "ffffffff6606606601001000631f000073577eda753ae5c664ec2b6ee5a17210dbbb67a1e0f753742ab7c62f0eefa0102746c0221e3a14f97f7af4959c53b720a86a5e09e0efea5cd148b5e921fbec883b9f9f0e1310d8853b70dd82540887f0";
    std::vector<uint8_t> test_v;
    hexStringToByte(pk,test_v);

    std::vector<uint8_t> key_public;
    hexStringToByte(Config::PublicChannel.key,key_public);


    auto payload = decodeData(decryptBroadcast(test_v,key_public));
    REQUIRE(payload.has_value());
    auto user_zebrax = decodeUser(std::vector<uint8_t>(payload->payload.begin(),payload->payload.end()));
    REQUIRE(user_zebrax.has_value());

    //encode packet

    std::vector<uint8_t> v = encodePerson("ZBRX",publ_key,"!66600666","Zebrax il Malvagio",false,0,255);
    
    auto data = decodeData(v);
    REQUIRE(data.has_value());
    CHECK(data->portnum == 4);

    auto user = decodeUser(std::vector<uint8_t>(data->payload.begin(),data->payload.end()));
    REQUIRE(user.has_value());
    CHECK(user->shortName==user_zebrax->shortName);
    CHECK(user->id == user_zebrax->id);
    CHECK(user->longName == user_zebrax->longName);
    CHECK(user->shortName == user_zebrax->shortName);
    CHECK(user->publicKey==user_zebrax->publicKey);
    CHECK(user->hwModel == user_zebrax->hwModel);
    CHECK(user->isLicensed == user_zebrax->isLicensed);
    CHECK(user->role == user_zebrax->role); 
}


TEST_CASE("encodeMessageInfo"){
    std::string tst = "ffffffff6606606601001000631f000073577eac753ae5c664ec2b6ee5a17210dbbb67a1e0f753742ab7c62f0eefa0102746c0221e3a14f97f7af4959c6352bde9185dabfa0da1cab203d04c9e1d6c77";
    
    std::vector<uint8_t> payload = encodePerson("ZBRX",Config::PublicChannel.key,"!66600666","Zebrax il Malvagio",false,0,255);
    uint8_t from[4]={0x66,0x06,0x60,0x66};
    uint8_t id[4]{0x01,0x00,0x10,0x00};

    std::vector<uint8_t> packet = encodeMessageInfo(payload,from,id,Config::PublicChannel.key,3,0,0,3,0,0);

    CHECK(vectorToHexString(packet) == tst);


    //TESTS FOR THE meshtastic_decoder.ino
    /*
    prima ho mandato: ffffffff2143341200000001631f0000129fe20d7ca39e5d04ecba5cc6de316cbca26fdd698f276c6361265179ed9df3538b72d40ff8dd1400f42dcb9ddae8c3787296501dfafb7aba223cb8
    poi cerco di fregare il dispositivo ufficiale mandandogli una nuova chiave e un nuovo nome
    poi mando: ffffffff2143341200000003631f000091da552481548f8a12807f420e1857b51af4c9d04ee817ed13cdcbcf0744e44c7ce6f35244d5a404c229d206ddbf1e295df64f0f19c3e5f692883a36
    che cambia nome ma non chiave pubblica 
    */
    uint8_t from_t[4]={0x01,0x06,0x19,0x91};
    uint8_t id_t[4]{0x01,0x00,0x01,0x03};
    std::vector<uint8_t> payload_t = encodePerson("BTLS",Config::PublicChannel.key,"!12344321","Sono un mammone",false,0,4);
    std::vector<uint8_t> packet_t = encodeMessageInfo(payload_t,from_t,id_t,Config::PublicChannel.key,3,1,0,3,0,0);
    CHECK(vectorToHexString(packet_t)== "ffffffff01061991010001036b1f0000b23a598fdd4a3d846b7567014ecb5c806cab9e3e5e2f9310e2487a07a08ffeb95a0418e12f15bf7eda5776c9e1762523e2db17fce883f0bebd254a36");
//                                       ffffffff01061991010001036b1f0000b23a598fdd4a3d846b7567014ecb5c806cab9e3e5e2f9310e2487a07a08ffeb95a0418e12f15bf7eda5776c9e1762523e2db17fce883f0bebd254a36
}
