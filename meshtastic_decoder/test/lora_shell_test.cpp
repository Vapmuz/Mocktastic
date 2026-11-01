#ifdef TESTS



#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../functions.h"
#include "../lora_shell.h"
#include "../lora_shell.cpp"
#include "test_functions.h"




TEST_CASE("echo") {
    std::string msg = "Hello, world!";
    CHECK(echo(msg) == "Hello, world!");
}

TEST_CASE("help") {
    std::string expected =
    "Available commands:\n"
    "  /echo <text>        - print text back to the serial console\n"
    "  /pub <text>         - send a public Meshtastic packet\n"
    "  /cantine <text>     - send a private channel Meshtastic packet\n"
    "  /admn <text>         - send a direct private message to LEO1\n"
    "  /r                  - repeat the last command\n"
    "  /rssi               - show current LoRa RSSI\n"
    "  /pwr17 <value>      - set LoRa TX power to <value>\n"
    "  /gain <value>       - set LoRa gain to <value>\n"
    "  /help               - show this help text\n"
    "  /send_key           - send public_key of Device\n"
    "  /raw <text>         - send a packet pre-made, for the tests\n"
    "\n"
    "Each command must start with '/' .\n";
    CHECK(help() == expected);
}


TEST_CASE("publicMessage") {
    std::string expec_msg = "fffffffffc8eb3f96ed13ce7631f00fca5fef76860cd9b9bf3cf1a050e076e6f0db40468dc";
    //                       fffffffffc8eb3f96ed13ce7631f00fca5fef76860cd9b9bf3cf1a050e076e6f0db40468dc
    uint8_t id[4] = {0x6e,0xd1,0x3c,0xe7}; //6ed13ce7
    uint8_t from[4] = {0xfc,0x8e,0xb3,0xf9}; //fc8eb3f9
    std::vector<uint8_t> msg_v = publicMessage("Kleine papageno",id,from);
    CHECK(toHex(msg_v)==expec_msg);
}

TEST_CASE("privateChannelMessage") {
    std::string expec_msg = "fffffffffc8eb3f979e51160630500fcd6bdec759a48e13b79b430961fe3a07d";
    uint8_t id[4] = {0x79,0xe5,0x11,0x60}; //79e51160
    uint8_t from[4]; //fc8eb3f9
    strLitEndiConverter(Config::Leo1.name,from,sizeof(from));
    std::vector<uint8_t> msg_v = privateChannelMessage("Palla rosa",id,from);
    CHECK(toHex(msg_v)==expec_msg);
}

TEST_CASE("privateMessage"){
    std::string expected_message = "34123412fc8eb3f9ab2b05546b000000eecb3f24aadd679ddc7b6c2701354a0f5bbb7f675613d148b63ba5934c920c095e0376";
    //                              34123412fc8eb3f9ab2b05546b000000eecb3f24aadd679ddc7b6c2701354a0f5bbb7f675613d148b63ba5934c920c095e0376
    uint8_t from[4];
    strLitEndiConverter(Config::Leo1.name,from, sizeof(from));
    uint8_t to[4];
    strLitEndiConverter(Config::Device.name,to,sizeof(to));

    uint8_t id[4] ={0xab,0x2b,0x05,0x54};
    uint8_t extra_nonce[4] ={0x09,0x5e,0x03,0x76};
    std::vector<uint8_t> message = privateMessage(Config::Leo1.privateKey,Config::Device.publicKey,"La risposta è 42",extra_nonce,to,from,id);
    CHECK(toHex(message)==expected_message);
}

TEST_CASE("strBetweenSpace"){
    std::string msg ="/priv arrub, kipolli "; 
    std::string command =strBetweenSpace(msg, 0);
    CHECK(command =="/priv");

    std::string command1 = strBetweenSpace(msg,1);
    CHECK(command1=="arrub,");


    std::string command2 = strBetweenSpace(msg,2);
    CHECK(command2=="kipolli");


    std::string space = "glandesaggiodice                    d ";
    CHECK(strBetweenSpace(space,3)=="");

    std::string leading_space = " ba bac ";
    CHECK(strBetweenSpace(leading_space,0)=="ba");

    std::string bad_str ="senzaspazio";
    CHECK_THROWS(strBetweenSpace(bad_str,2));


    std::string bad_str1 ="senzaspazio ";
    CHECK_THROWS(strBetweenSpace(bad_str,2));


    
}

TEST_CASE("strAfterSpace"){

    std::string msg ="/priv arrub, kipolli "; 
    std::string command =strAfterSpace(msg, 0);
    CHECK(command ==msg);

    std::string command1 = strAfterSpace(msg,1);
    CHECK(command1=="arrub, kipolli ");


    std::string command2 = strAfterSpace(msg,2);
    CHECK(command2=="kipolli ");

    std::string space = "glandesaggiodice                    d ";
    CHECK(strAfterSpace(space,1)=="                   d ");

    std::string leading_space = " bac ba ";
    CHECK(strAfterSpace(leading_space,0)==leading_space);
    CHECK(strAfterSpace(leading_space,1)=="ba ");

    std::string bad_str ="senzaspazio";
    CHECK_THROWS(strAfterSpace(bad_str,2));


    std::string bad_str1 ="senzaspazio ";
    CHECK_THROWS(strAfterSpace(bad_str,2));

}


TEST_CASE("countSpaces"){
    std::string msg = "glandesaggiodice                    d ";
    size_t counter = countSpaces(msg);
    
    CHECK_NOTHROW(strBetweenSpace(msg, counter-1));
}


TEST_CASE("parser's command"){
    Parser ps;
    CHECK(ps.command("/echo with stuff ")== "/echo");


}

TEST_CASE("parser's main"){
    Parser ps;
    
    //invalid
    std::string tst_bad = "echo with stuff";
    CHECK_THROWS(ps.main(tst_bad));
    
    std::string tst = "/echo with stuff";
    CHECK(ps.main(tst)==ECHO);
    
    std::string tst_pub = "/pub update";
    CHECK(ps.main(tst_pub)== PUB);
    
    std::string tst_cant = "/cantine";
    CHECK(ps.main(tst_cant) == CANTINE);
    
    std::string tst_leo = "/admn"; 
    CHECK(ps.main(tst_leo) == ADMN);
   
    std::string tst_r = "/r"; 
    CHECK(ps.main(tst_r) == R);

    std::string tst_pwr17 = "/pwr17"; 
    CHECK(ps.main(tst_pwr17) == PWR17);
    
    std::string tst_gain = "/gain"; 
    CHECK(ps.main(tst_gain) == GAIN);
    
    std::string tst_rssi = "/rssi -67"; 
    CHECK(ps.main(tst_rssi) == RSSI);
    
    std::string tst_help = "/help"; 
    CHECK(ps.main(tst_help) == SH_DEFAULT);
    
    std::string tst_unk = "/nonexistent"; CHECK(ps.main(tst_unk)== SH_DEFAULT); 
    
    REQUIRE_THROWS_AS(ps.command(""), std::exception);
}


TEST_CASE("repeat command"){
    Parser ps;
    std::string command = "/echo";
    std::string repeat = "/r";
    ps.main(command);
    CHECK(ps.last_command=="/echo");
    CHECK(ps.main(repeat)==R);
    CHECK_FALSE(ps.last_command=="/r");
}

TEST_CASE("parser's printLastCommand"){
    Parser ps; 
    std::string tst = "/echo with stuff";
    CHECK(ps.main(tst)==ECHO);
    
    CHECK(ps.printLastCommand()=="/echo with stuff");
}


TEST_CASE("findDMKey"){
    std::string priv_msg ="34123412fc8eb3f9978addbb6b0000fc8d7d9d42bb6171939b40f644df66740f87d9993710188a9091715a117ae2e6f21d8bca227c9f2f4bb4938d594c";
    // test case: leo1 sends a packet to zepu
    std::vector<uint8_t> packet;
    hexStringToByte(priv_msg,packet);

    CHECK(findDMKey(packet)=="2d72f07c5f1e83195807cedb64b26fd713ea215b5da89c940f51c09d0c5e4954"); //zepu public key
    
    std::string payload = msgPayloadDM(packet,Config::Device.privateKey,findDMKey(packet));

    CHECK(payload=="Messaggio custom DM privato");
    
}

TEST_CASE("findBroadcastKey"){
    std::string pub_msg = "fffffffffc8eb3f96ed13ce7631f00fca5fef76860cd9b9bf3cf1a050e076e6f0db40468dc";
    std::vector<uint8_t> packet;
    hexStringToByte(pub_msg,packet);

    CHECK( msgPayloadBroadcast(packet,findBroadcastKey(packet))== "Kleine papageno");

    std::string bad_msg = "fffffffffc8eb3f96ed13ce7630000fca5fef76860cd9b9bf3cf1a050e076e6f0db40468dc";
    std::vector<uint8_t> packet_b;
    hexStringToByte(bad_msg,packet_b);

    CHECK_THROWS(msgPayloadBroadcast(packet_b,findBroadcastKey(packet_b)));


    bad_msg = "fffffffffc8eb3f96ed13ce763ef00fca5fef76860cd9b9bf3cf1a050e076e6f0db40468dc"; 
    std::vector<uint8_t> packet_b2;
    hexStringToByte(bad_msg,packet_b);
    CHECK_THROWS(msgPayloadBroadcast(packet_b,findBroadcastKey(packet_b)));


    std::string prv_ch ="fffffffffc8eb3f979e51160630500fcd6bdec759a48e13b79b430961fe3a07d"; 
    std::vector<uint8_t> packet_prv;
    hexStringToByte(prv_ch,packet_prv);
    CHECK(msgPayloadBroadcast(packet_prv,findBroadcastKey(packet_prv))=="Palla rosa"); 
}


TEST_CASE("tryPrintPayload"){
    Packet pk_pubch;
    Packet pk_prvch;
    Packet pk_prvms;

    pk_pubch.hexStringToData("fffffffffc8eb3f96ed13ce7631f00fca5fef76860cd9b9bf3cf1a050e076e6f0db40468dc");
    pk_prvch.hexStringToData( "fffffffffc8eb3f979e51160630500fcd6bdec759a48e13b79b430961fe3a07d");
    pk_prvms.hexStringToData("34123412fc8eb3f9978addbb6b0000fc8d7d9d42bb6171939b40f644df66740f87d9993710188a9091715a117ae2e6f21d8bca227c9f2f4bb4938d594c");

    CHECK(tryPrintPayload(pk_pubch.data)=="Kleine papageno");
    CHECK(tryPrintPayload(pk_prvch.data)=="Palla rosa");
    CHECK(tryPrintPayload(pk_prvms.data)=="Messaggio custom DM privato");

    Packet pk_bad;

    pk_bad.hexStringToData("abababababababababababbaababababababaababbaabbbabababababababababa");
    CHECK_THROWS(tryPrintPayload(pk_bad.data));


    //address bad packets passed 
    std::string short_string = "caccac"; //short packet of invalid data
    std::vector<uint8_t> short_packet;
    hexStringToByte(short_string,short_packet);
    CHECK_THROWS(tryPrintPayload(short_packet));

}
#endif 