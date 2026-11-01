#ifndef LORA_SHELL_H
#define LORA_SHELL_H


/**
 * Shell command helpers for Meshtastic message construction and parsing.
 *
 * Note: shell functionality requires the shell to be enabled; otherwise it
 * operates in test mode.
 */
#include "./keys.h"
#include "meshtastic.h"
#include <stdexcept>


enum Values {
    SH_DEFAULT,
    ECHO,
    PUB,
    CANTINE,
    ADMN,
    R,
    RSSI,
    PWR17,
    GAIN,
    DHT,
    SEND_KEY,
    RAW
};


/**
 * Echo a string back unchanged.
 *
 * @param s input string
 * @return echoed string
 */
extern std::string echo(std::string s);

/**
 * Build a public channel encrypted message.
 *
 * The key and channel name default to Config::PublicChannel.
 *
 * @param msg plaintext message
 * @param id packet id buffer
 * @param from sender id buffer
 * @param key channel key
 * @param name_channel channel name
 * @return packet bytes
 */
extern std::vector<uint8_t> publicMessage(std::string msg,
    uint8_t id[4],
    uint8_t from[4],
    std::string key = Config::PublicChannel.key,
    std::string name_channel = Config::PublicChannel.name);
/*
 encrypt message using psk_key of the private channel,
 the key and the channel_name are optional, because by default the channel name and key are taken from Config::PrivateChannel
 */

/**
 * Build an encrypted private channel message.
 *
 * @param msg plaintext message
 * @param id packet id buffer
 * @param from sender id buffer
 * @param key channel key
 * @param name_channel channel name
 * @return packet bytes
 */
extern std::vector<uint8_t> privateChannelMessage(std::string msg,
    uint8_t id[4],
    uint8_t from[4],
    std::string key = Config::PrivateChannel.key,
    std::string name_channel = Config::PrivateChannel.name);

/**
 * Build a direct private message encrypted with derived AES key.
 *
 * @param priv_key private key string
 * @param publ_key public key string
 * @param msg plaintext message
 * @param extra_nonce extra nonce buffer
 * @param to recipient id buffer
 * @param from sender id buffer
 * @param id packet id buffer
 * @return packet bytes
 */
extern std::vector<uint8_t> privateMessage(std::string priv_key,
    std::string publ_key,
    std::string msg,
    uint8_t extra_nonce[4],
    uint8_t to[4],
    uint8_t from[4],
    uint8_t id[4]);
/**
 * Count spaces in a string.
 *
 * @param str input string
 * @return number of spaces
 */
extern size_t countSpaces(std::string str);

/**
 * Return the part of a space-terminated string after a given number of spaces.
 * An index of 0 returns the original string. If the input begins with a space,
 * that leading space marks the start and is not counted; subsequent spaces
 * count separately.
 *
 * @param s non-empty input string ending in a space
 * @param space_index number of spaces to skip after the start of the string
 * @return remaining string, including its trailing space
 * @throws std::invalid_argument if s is empty or does not end in a space
 * @throws std::runtime_error if s contains fewer than space_index spaces
 */
extern std::string strAfterSpace(std::string s, size_t space_index);

/**
 * Return the zero-based segment delimited by spaces. A leading space marks
 * the start of the first segment rather than producing an empty segment.
 * Consecutive later spaces produce empty segments; the required trailing space
 * does not add a segment.
 *
 * @param s non-empty input string ending in a space
 * @param segment_index zero-based segment index
 * @return selected segment, without its surrounding spaces
 * @throws std::invalid_argument if s is empty or does not end in a space
 * @throws std::runtime_error if the requested segment does not exist
 */
extern std::string strBetweenSpace(std::string s, size_t segment_index);

/**
 * Return a help message for shell commands.
 *
 * @return help text
 */
extern std::string help();

/**
 * Find the broadcast decryption key for a packet.
 *
 * @param packet packet bytes
 * @return key identifier or key string
 */
extern std::string findBroadcastKey(std::vector<uint8_t> packet);

/**
 * Find the direct-message decryption key for a packet.
 *
 * @param packet packet bytes
 * @return key identifier or key string
 */
extern std::string findDMKey(std::vector<uint8_t> packet);

/**
 * Attempt to decrypt packet payload and return it as a string.
 *
 * @param packet packet bytes
 * @return payload as string
 */
extern std::string tryPrintPayload(std::vector<uint8_t> packet);



class Parser {
public:

    std::string last_command = "";

    int main(std::string& s) {

        if (s != "/r") {
            last_command = s;
        }
     
        s.append(" ");
        
        if (s.at(0) != '/') {
            throw std::invalid_argument("passed to parser invalid command: " + s);
        }
        
        if (command(s) == "/echo") {
            return(ECHO);
        }
        
        if (command(s) == "/pub") {
            return(PUB);
        }
        
        if (command(s) == "/cantine") {
            return(CANTINE);
        }
        
        if (command(s) == "/admn") {
            return(ADMN);
        }
        
        if (command(s) == "/r") {
            return(R);
        }

        if (command(s) == "/rssi") {
            return(RSSI);
        }

        if (command(s) == "/pwr17") {
            return(PWR17);
        }

        if (command(s) == "/gain") {
            return(GAIN);
        }
        if(command(s)=="/dht"){
            return(DHT);
        }
        if (command(s) == "/help") {
            return(SH_DEFAULT);
        }
        if(command(s) == "/send_key"){
            return(SEND_KEY);
        }
        if(command(s)=="/raw"){
            return(RAW);
        }
        else {
            return(SH_DEFAULT);
        }
    }

    std::string printLastCommand() {
        return(last_command);
    }

    /**
     * Extract the first token from a shell command string.
     *
     * @param msg full command string
     * @return first token of the command
     */
     std::string command(std::string msg) {

        std::string command = strBetweenSpace(msg, 0);
        return(command);
    }
};


#endif