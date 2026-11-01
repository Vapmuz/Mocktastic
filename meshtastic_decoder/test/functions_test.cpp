#ifdef TESTS


#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../functions.h"
#include "../functions.cpp"
#include "test_functions.h"
#include "test_functions_test.cpp"
#include "crypto_test.cpp"
#include "meshtastic_test.cpp"
#include "lora_shell_test.cpp"


TEST_CASE("Prova Packet") {
    Packet pk;
    char buffer[50];
    pk.size = 10;
    pk.rssi = 1;
    pk.information(buffer);
    CHECK(std::string(buffer) == "Packet received: 10 bytes - RSSI 1");
}


TEST_CASE("info") {
    Packet pk;
    std::string thepacket{ "0123456789abcdef01234567891abcdef1" };
    pk.hexStringToData(thepacket);
    std::string recd = "Recd packets:146 RSSI:-8";
    pk.rssi = -8;
    pk.size = 146;
    CHECK(pk.info().find("Recd packets:146 RSSI -8"));
}

TEST_CASE("info preload") {
    Packet pk;
    std::vector<uint8_t> test_byte = { 160, 160, 160, 1, 35, 69, 103, 137, 171, 205, 239, 1, 35, 69, 103, 137 };
    std::string tststring = "A0A0A00123456789ABCDEF0123456789";
    pk.hexStringToData(tststring);
    pk.rssi = -8;

    CHECK(pk.info().find("Recd packets:4 RSSI:-8"));
    CHECK(pk.data == test_byte);
}



TEST_CASE("hexStringToData") {
    Packet pk;
    std::string t1{ "abababab" };
    std::string t2{ "a" };

    pk.hexStringToData(t1);
    CHECK(pk.dataToString() == "abababab");
    CHECK_THROWS(pk.hexStringToData(t2));



}

TEST_CASE("hexStringToByte") {
    Packet pk;
    std::string t1{ "abababab" };
    std::string t2{ "a" };

    hexStringToByte(t1, pk.data);
    CHECK(pk.dataToString() == "abababab");
    CHECK_THROWS(hexStringToByte(t2, pk.data));

    hexStringToByte(t1, pk.data);
    CHECK(pk.dataToString() == "abababababababab");
}

TEST_CASE("addValue test receiving LoRa packet") {
    Packet pk;
    int buffer[6] = { 128,64,32,16,8,42 };
    std::vector<uint8_t> testBytes{ 128, 64, 32, 16, 8, 42 };


    for (int i = 0;i < 6;i++)
        addValue(buffer[i], pk.data);
    CHECK(pk.data == testBytes);
}


TEST_CASE("vectorToHexString") {
    Packet pk;

    std::string expected_str = "010203";
    std::string str;
    std::vector<uint8_t> v = { 0x01,0x02,0x03 };

    str = vectorToHexString(v);

    CHECK(str == expected_str);
}


TEST_CASE("vectorToString") {
    Packet pk;
    std::string str;
    std::string expected_str = "I whish that I could lie, but I'm way to self aware";
    std::vector<uint8_t> v(expected_str.begin(), expected_str.end());

    str = vectorToString(v);

    CHECK(str == expected_str);

}

TEST_CASE("test dataToString") {
    Packet pk;
    int buffer[6] = { 0x67,0xAB,0xCD,0xEF,0xFF,0x00 };
    std::vector<uint8_t> testBytes{ 128, 64, 32, 16, 8, 42 };


    for (int i = 0;i < 6;i++) {
        addValue(buffer[i], pk.data);
    }

    CHECK(pk.dataToString() == "67abcdefff00");

}



TEST_CASE("time difference") {
    uint32_t case1 = timeDifference(10, 20);
    uint32_t case2 = timeDifference(10, 10);
    uint32_t case3 = timeDifference(0xFFFFFFFE, 10);
    uint32_t case4 = timeDifference(0xFFFFFFF0, 2);

    CHECK(case1 == 10);
    CHECK(case2 == 0);
    CHECK(case3 == 12);
    CHECK(case4 == 18);
}

TEST_CASE("stringClear test") {
    CHECK(stringClear("9890b3f9fc8eb3f936bf57936b0000fc0018d9122ed3e01c56674d4b977005e8081e72ee0137ac07186848892b") == "-9890-b3f9-fc8e-b3f9-36bf-5793-6b00-00fc-0018-d912-2ed3-e01c-5667-4d4b-9770-05e8-081e-72ee-0137-ac07-1868-4889-2b");
}


TEST_CASE("Understand how to copy bytes from buffer to struct ") {
    // definitions

    std::vector<unsigned char> rcvbuf = { 'a','b','c','d',0x2A,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x43,0x00,0x00,0x00 }; /*it shall be 16 byte long*/
    std::vector<unsigned char> checkbuf1 = {};
    char checkbuf2[] = { 'a','b','c','d' };


    struct somethingParams
    {
        /*remember that the struct contains 16 bytes and memcpy can overflow*/
        char Magic[4];
        int Version;
        int MsgID;
        int MsgLen;
    };

    struct somethingParams mParams;
    memcpy(&mParams, rcvbuf.data(), sizeof(somethingParams)); /*read and write 16 bytes*/
    rcvbuf.erase(rcvbuf.begin(), rcvbuf.begin() + sizeof(somethingParams));

    CHECK(checkbuf1 == rcvbuf); /*rcvbuf is empty test*/
    CHECK(memcmp(mParams.Magic, checkbuf2, sizeof(checkbuf2)) == 0); //not working because if I compare the two arrays then i decays as pointers. Use memcmp to compare
    CHECK(sizeof(mParams.Magic) == 4);
    CHECK(mParams.Version == 42);
    CHECK(mParams.MsgID == 1);
    CHECK(mParams.MsgLen == 67);
}


TEST_CASE("fillHeader tests") {
    Packet pk;

    std::string tststring = "abcdef"; //add bytes until 3 bytes --> too short 

    struct Packet::header h; //instantiate the header


    pk.hexStringToData(tststring);

    CHECK_THROWS(pk.fillHeader(&h)); //gets a throw because the vector is not long enough

    std::string sectststring = "aa0123456789ABCDEF01234567";

    pk.hexStringToData(sectststring); //add values until 16 bytes

    CHECK_NOTHROW(pk.fillHeader(&h)); //now it should work
    CHECK(pk.data.size() == sizeof(h));


    //check values into the header struct
    //the current header has the next values: {171, 205, 239, 170, 1, 35, 69, 103, 137, 171, 205, 239, 1, 35, 69, 103}
    uint8_t to[4] = { 171, 205, 239, 170 };
    uint8_t from[4] = { 1, 35, 69, 103 };
    uint8_t id[4] = { 137, 171, 205, 239 };
    uint8_t flags[1] = { 1 };   // hop limit, want ack, via mqtt, hop start
    uint8_t channel[1] = { 35 };
    uint8_t next_hop[1] = { 69 };
    uint8_t relay_node[1] = { 103 };

    CHECK(memcmp(h.to, to, sizeof(to)) == 0);
    CHECK(memcmp(h.from, from, sizeof(from)) == 0);
    CHECK(memcmp(h.id, id, sizeof(id)) == 0);
    CHECK(memcmp(h.flags, flags, sizeof(flags)) == 0);
    CHECK(memcmp(h.channel, channel, sizeof(channel)) == 0);
    CHECK(memcmp(h.next_hop, next_hop, sizeof(next_hop)) == 0);
    CHECK(memcmp(h.relay_node, relay_node, sizeof(relay_node)) == 0);
}



TEST_CASE("bufferToString tests") {
    uint8_t buffer[] = { 0x01,0x02,0x03,0x04 };

    std::string tststring = bufferToString(buffer, sizeof(buffer));
    std::string correctstring = "01020304";

    CHECK(tststring == correctstring);
}


TEST_CASE("makeStringHeader") {
    Packet pk;
    std::string hexvalue = "fffffffffc8eb3f93d53d4e8631f00fcc6bcab399f241c8472088bf4";
    std::string result;

    pk.hexStringToData(hexvalue);
    result = pk.makeStringHeader();

    CHECK(result.find("----Header----") != std::string::npos);
    CHECK(result.find("to: ffffffff") != std::string::npos);
    CHECK(result.find("from: f9b38efc") != std::string::npos);
    CHECK(result.find("ID packet: e8d4533d") != std::string::npos);
    CHECK(result.find("flags: 63") != std::string::npos);
    CHECK(result.find("(hop_limit:3 hop_start:3 want acknowledge:0 via_mqtt:0)") != std::string::npos);
    CHECK(result.find("channel: 1f") != std::string::npos);
    CHECK(result.find("nexthop: 00") != std::string::npos);
    CHECK(result.find("relay node: fc") != std::string::npos);
    CHECK(result.find("packet complete") != std::string::npos);
    CHECK(result.size() == 263);

}

TEST_CASE("takeBits") {
    /*
    passes as argument a byte, sx bit and dx bit
    return the numeric value of the bits in between
    */

    uint8_t ex_byte{ 0x6b };//01101011
    int bit6to2{ 26 }; //11010
    int bit6to0{ 11 + 96 };
    int bit1{ 1 }; //1
    int bit7{ 0 }; //0


    CHECK_THROWS(takeBits(ex_byte, 0, 6));
    CHECK_THROWS(takeBits(ex_byte, 8, 2));
    CHECK_THROWS(takeBits(ex_byte, 8, 8));
    CHECK(takeBits(ex_byte, 6, 2) == bit6to2);
    CHECK(takeBits(ex_byte, 7, 7) == bit7);
    CHECK(takeBits(ex_byte, 1, 1) == bit1);
    CHECK(takeBits(ex_byte, 6, 0) == bit6to0);
}
TEST_CASE("fillFlags") {
    /*takes as argument a pointedr to flags struct*/
    Packet pk;
    Packet::flags f;
    std::string s1 = "0123456789abcdf012345678";
    pk.hexStringToData(s1); //first 12 bytes, without flags 

    CHECK_THROWS(pk.fillFlags(&f));

    std::string s2 = "6a"; // hop lim 2, hop start 3, want ack 1, via mqtt 0 11001010
    pk.hexStringToData(s2);

    CHECK_NOTHROW(pk.fillFlags(&f));

    CHECK(f.hop_limit == 2);
    CHECK(f.hop_start == 3);
    CHECK(f.want_ack == 1);
    CHECK(f.via_mqtt == 0);
}

TEST_CASE("addName") {
    Packet pk;

    std::string tststring{ "9890b3f9" };
    std::string tststring2{ "nostringa" };

    CHECK(pk.addName(tststring) == "9890b3f9 Giumello");
    CHECK(pk.addName(tststring2) == "nostringa");
}


TEST_CASE("liEndConverter") {
    uint8_t bf[4]{ 0xaa,0xbb,0xcc,0xdd };

    CHECK_THROWS(litEndiConverter(bf, 2));

    uint32_t test{ 0xddccbbaa };
    uint32_t nr_exit{ litEndiConverter(bf,4) };

    CHECK(test == nr_exit);
}



TEST_CASE("hexString32") {
    uint32_t number = { 0xaaaa };
    std::string aaaa = "aaaa";
    CHECK(hexString32(number) == aaaa);
    uint32_t number2 = 0x00aa;
    CHECK(hexString32(number2)=="00aa");
}

TEST_CASE("isMessageValid") {
    std::string test_length{ "" };
    std::string test_not_hex{ "abcdefgh" };
    std::string test_odd{ "abcdef0" };
    std::string test_right{ "abcdef" };


    CHECK_THROWS(isMessageValid(test_length));
    CHECK_THROWS(isMessageValid(test_not_hex));
    CHECK_THROWS(isMessageValid(test_odd));
    CHECK(isMessageValid(test_right) == "");

}


TEST_CASE("LitEndiConverter") {
    std::vector<uint8_t> v{ 0xaa,0xbb,0xcc,0xdd,0xee };
    uint32_t new_number = litEndiConverter(v, 1, 4);

    CHECK_THROWS(litEndiConverter(v, 0, 4));
    CHECK_THROWS(litEndiConverter(v, 64, 67));
    CHECK(new_number == 0xeeddccbb);
}


TEST_CASE("InPlaceLitEndiConverter") {
    std::vector<uint8_t> v{ 0xaa,0xbb,0xcc,0xdd,0xee };
    std::vector<uint8_t> expected{ 0xaa,0xee,0xdd,0xcc,0xbb };
    InPlaceLitEndiConverter(v, 1, 4);

    CHECK(v == expected);

    CHECK_THROWS(InPlaceLitEndiConverter(v, 0, 4));
    CHECK_THROWS(InPlaceLitEndiConverter(v, 64, 67));
    CHECK(v == expected);
}

TEST_CASE("base64Encoder") {
    auto bytes = [](const char* s) {
        return std::vector<unsigned char>(s, s + strlen(s));
    };

    //vettori di prova della RFC 4648 10
    CHECK(base64Encoder(bytes("")) == "");
    CHECK(base64Encoder(bytes("f")) == "Zg==");
    CHECK(base64Encoder(bytes("fo")) == "Zm8=");
    CHECK(base64Encoder(bytes("foo")) == "Zm9v");
    CHECK(base64Encoder(bytes("foob")) == "Zm9vYg==");
    CHECK(base64Encoder(bytes("fooba")) == "Zm9vYmE=");
    CHECK(base64Encoder(bytes("foobar")) == "Zm9vYmFy");

    //round-trip con il decoder
    std::vector<unsigned char> raw;
    for (int i = 0; i < 32; i++) raw.push_back((unsigned char)(i * 7));
    CHECK(base64Decoder(base64Encoder(raw)) == raw);
}


TEST_CASE("base64Decoder") {
    std::string abc = "ABCD";

    std::vector<uint8_t> expected{ 0,16,131 };
    std::vector<uint8_t> v = base64Decoder(abc);

    CHECK(v == expected);

    std::string bad_str = "bad§trinƒ@ª∂ƒ@ª®†°";

    CHECK_THROWS(base64Decoder(bad_str));

    std::string spaces = "a  c    abc";

    CHECK_THROWS(base64Decoder(spaces));
}

TEST_CASE("b64StrToHexStr") {
    /*c1BmUmhBV2w1UFVJS3I4VQ==*/
    /*+LDDp+hC5OlRQj4D6Kj04Dw3hs9lZWbaLNbTBdjpWUw=*/
    Packet pk;
    std::string tst1 = "helloo==";
    std::string expected_str = "85e965a2";
    std::string str1 = b64StrToHexStr(tst1);

    CHECK(str1 == expected_str);

    std::string key = "c1BmUmhBV2w1UFVJS3I4VQ==";
    std::string str = b64StrToHexStr(key);
    expected_str = "735066526841576c355055494b723855";    
    CHECK(str == expected_str);

    //CHECK(""==b64StrToHexStr("+LDDp+hC5OlRQj4D6Kj04Dw3hs9lZWbaLNbTBdjpWUw="));
}

TEST_CASE("strLitEndiConverter"){
    std::string bad_str = "abcdefgh";
    std::string bad_str1 ="abcdef";
    std::string good_str = "89abcdef"; 
    uint8_t bad_bf[3];
    uint8_t bf[4];
 
    CHECK_THROWS(strLitEndiConverter(bad_str,bad_bf,sizeof(bad_bf)));
    CHECK_THROWS(strLitEndiConverter(bad_str,bf,sizeof(bf)));
    CHECK_THROWS(strLitEndiConverter(bad_str1,bf,sizeof(bf)));

    strLitEndiConverter(good_str,bf,sizeof(bf));
    uint8_t bf_exp[4] ={0xef,0xcd,0xab,0x89};

    CHECK(std::equal(std::begin(bf_exp), std::end(bf_exp), std::begin(bf)));    
}


TEST_CASE("randomBuffer"){
    uint8_t bf1[4]={0,0,0,0};
    uint8_t bf2[4]={0,0,0,0};

    // the buffer size must be 4
    CHECK_THROWS_AS(randomBuffer(bf1, 3), std::invalid_argument);
    CHECK_THROWS_AS(randomBuffer(bf1, 5), std::invalid_argument);

    // two consecutive calls must give different buffers
    randomBuffer(bf1, sizeof(bf1));
    randomBuffer(bf2, sizeof(bf2));
    CHECK_FALSE(std::equal(std::begin(bf1),std::end(bf1), std::begin(bf2)));
}

TEST_CASE("convertKeysToInfoPacket"){
    std::vector<uint8_t> string_v;
    hexStringToByte("abcdef12",string_v);
    
    std::string new_str = convertKeysToInfoPacket("abcdef12");


    //convert chars into uint8_t
    CHECK(static_cast<uint8_t>(new_str.at(0))== 0xab);    
    CHECK(static_cast<uint8_t>(new_str.at(1))== 0xcd);    
    CHECK(static_cast<uint8_t>(new_str.at(2))== 0xef);    
    CHECK(static_cast<uint8_t>(new_str.at(3))== 0x12);    
}


TEST_CASE("maskBitVector"){
    std::vector<uint8_t> v= {0xff,0xff,0xff,0xff};
    maskBitVector(v,2,7);
    maskBitVector(v,3,6);
    CHECK(vectorToHexString(v)=="ffff7fbf");

    CHECK_THROWS(maskBitVector(v,2,8));
    CHECK_THROWS(maskBitVector(v,4,3));
}

TEST_CASE("retrieveBitVector"){
    std::vector<uint8_t> v= {0xff,0xff,0x7f,0xf7};
    CHECK(retrieveBitVector(v,2,7)==0); 
    CHECK(retrieveBitVector(v,2,6)==1);
    CHECK(retrieveBitVector(v,3,3)==0);

    CHECK_THROWS(retrieveBitVector(v,2,8));
    CHECK_THROWS(retrieveBitVector(v,5,3));
}

TEST_CASE("removeWantAcknowledge"){
    Packet pk;
    std::string packet_s = "ffffffff21433412000000036b1f000091da552481548f8a12807f420e1857b51af4c9d04ee817ed13cdcbcf0744e44c7ce6f35244d5a404c229d206ddbf1e295df64f0f19c3e5f692883a36";
    //                      ffffffff21433412000000036b1f000091da552481548f8a12807f420e1857b51af4c9d04ee817ed13cdcbcf0744e44c7ce6f35244d5a404c229d206ddbf1e295df64f0f19c3e5f692883a36
    //it has "want acknowledge"
    
    hexStringToByte(packet_s,pk.data);

    CHECK(retrieveBitVector(pk.data,12,3)==1);
    removeWantAcknowledge(pk.data);
    CHECK(retrieveBitVector(pk.data,12,3)==0);

}

#endif

