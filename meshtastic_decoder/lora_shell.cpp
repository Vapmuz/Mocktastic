#include "lora_shell.h"
#include "./keys.h"
#include "crypto.h"
#include "meshtastic.h"
#include "functions.h"
#include <utility>
#include <cctype>


std::string echo(std::string s) {
    return(s);
}

std::vector<uint8_t> publicMessage(std::string msg,
    uint8_t id[4],
    uint8_t from[4],
    std::string key,
    std::string name_channel)
{
    Packet pk;

    return(broadcastMessageSh(key, name_channel, msg, id, from));
}

std::vector<uint8_t> privateChannelMessage(std::string msg, uint8_t id[4], uint8_t from[4], std::string key, std::string name_channel) {
    Packet pk;


    return(broadcastMessageSh(key, name_channel, msg, id, from));
}


std::vector<uint8_t> privateMessage(std::string priv_key,std::string publ_key, std::string msg, uint8_t extra_nonce[4], uint8_t to[4], uint8_t from[4], uint8_t id[4]) {
    return(DMMessageSh(priv_key, publ_key, msg, extra_nonce, to, from, id,3,1,0,3,0,0));
}

size_t countSpaces(std::string str) {
    size_t count = 0;

    for (char c : str) {
        if (c == ' ') {
            count++;
        }
    }
    return count;
}

std::string strAfterSpace(std::string s, size_t space_index) {
    if (s.empty()) {
        throw std::invalid_argument("s string passed in strAfterSpace is empty");
    }
    if (s.at(s.length() - 1) != ' ') {
        throw std::invalid_argument("s in strAfterSpace has not a clear end with a space");
    }
    if (space_index == 0) {
        return s;
    }

    size_t remaining_start = s.front() == ' ' ? 1 : 0; //id the first character is " " then start as one 
    // Skip one separator per step so the result starts after space_index spaces.
    for (size_t skipped_spaces = 0; skipped_spaces < space_index; ++skipped_spaces) {
        const size_t space_position = s.find(' ', remaining_start); 
        if (space_position == std::string::npos) {
            throw std::runtime_error("passed bad space_index in strAfterSpace, the string has too few spaces");
        }
        remaining_start = space_position + 1;
    }
    return s.substr(remaining_start);
}

std::string strBetweenSpace(std::string s, size_t segment_index) {
    if (s.empty()) {
        throw std::invalid_argument("s string passed in strBetweenSpace is empty");
    }
    if (s.at(s.length() - 1) != ' ') {
        throw std::invalid_argument("s in strBetweenSpace has not a clear end with a space");
    }

    size_t segment_start = s.front() == ' ' ? 1 : 0;
    // Skip preceding segments until segment_start points to the requested one.
    for (size_t current_segment = 0; current_segment < segment_index; ++current_segment) {
        const size_t separator_position = s.find(' ', segment_start);
        if (separator_position == std::string::npos) {
            throw std::runtime_error("passed bad segment_index for strBetweenSpace, the string has too few segments");
        }
        segment_start = separator_position + 1;
    }

    const size_t segment_end = s.find(' ', segment_start);
    if (segment_end == std::string::npos) {
        throw std::runtime_error("passed bad segment_index for strBetweenSpace, the string has too few segments");
    }
    return s.substr(segment_start, segment_end - segment_start);
}

std::string help() {
    return "Available commands:\n"
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
}


std::string findBroadcastKey(std::vector<uint8_t> packet) {

    if (packet.size() < 16) {
        throw std::invalid_argument("passed to findBroadcastKey a bad packet");
    }
    if (packet.at(13) == 0x00) {
        throw std::invalid_argument("channel hash not compatible with broadcast");
    }

    uint8_t hash = packet.at(13);

    //add here custom extra nodes:
    if (hash == channelHash(Config::PublicChannel.key, Config::PublicChannel.name)) {
        return(Config::PublicChannel.key);
    }
    if (hash == channelHash(Config::PrivateChannel.key, Config::PrivateChannel.name)) {
        return(Config::PrivateChannel.key);
    }
    else {
        throw std::invalid_argument("couldn't find a matching key for findBroadcastKey");
    }
}

std::string findDMKey(std::vector<uint8_t> packet) {
    if (packet.size() < 16) {
        throw std::invalid_argument("passed to findDMKey a bad packet");
    }
    if (packet.at(13) != 0x00) {
        throw std::invalid_argument("channel hash not compatible with dm");
    }

    std::string from = hexString32(litEndiConverter(packet, 4, 7));


    //add here custom extra nodes:
    if (from == Config::Admn.name) {
        return(Config::Admn.publicKey);
    }
    if (from == Config::Device.name) {
        return(Config::Device.publicKey);
    }
    else {
        throw std::invalid_argument("couldn't find a matching key for findDMKey");
    }

}




std::string tryPrintPayload(std::vector<uint8_t> packet) {
    if(packet.size()<14){
        throw std::invalid_argument("passed bad packet to tryPrintPayload, size of the packet<13");
    }
    
    if (packet.at(13) == 0x00) {
        return(msgPayloadDM(packet, Config::Device.privateKey, findDMKey(packet)));
    }
    
    else {
        return(msgPayloadBroadcast(packet, findBroadcastKey(packet)));
    }
}


