/**
 * This file is the main file to include in all real-world project
 */



#ifndef FUNCTIONS_H
#define FUNCTIONS_H
#define SIZEDATA 512

#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cctype>

#include "lora_shell.h"

#if defined(ARDUINO_ARCH_ESP32)
#include <bootloader_random.h>
#endif



/**
 * Compute the time difference between two timestamps.
 *
 * This function handles rollover by treating the timer as unsigned.
 *
 * @param t1 first timestamp
 * @param t2 second timestamp
 * @return elapsed time between t1 and t2
 */
extern uint32_t timeDifference(uint32_t t1, uint32_t t2);

/**
 * Normalize a LoRa message string to be human readable.
 *
 * @param s input string
 * @return normalized string
 */
extern std::string stringClear(std::string s);

/**
 * Convert raw bytes to a hexadecimal string.
 *
 * @param buffer pointer to input bytes
 * @param size_buffer number of bytes to encode
 * @return hexadecimal string representation
 */
extern std::string bufferToString(uint8_t* buffer, int size_buffer);

/**
 * Parse a hex string into a byte vector.
 *
 * The input must contain an even number of hex digits.
 *
 * @param hex input hexadecimal string
 * @param v output byte vector
 */
extern void hexStringToByte(const std::string& hex, std::vector<uint8_t>& v);
/**
 * Convert a byte vector to a hexadecimal string.
 *
 * @param v input byte vector
 * @return hexadecimal string
 */
extern std::string vectorToHexString(std::vector<uint8_t> v);

/**
 * Convert a Base64 string to a hexadecimal string.
 *
 * @param s Base64 input string
 * @return equivalent hex string
 */
extern std::string b64StrToHexStr(std::string s);

/**
 * Append an integer value to a byte vector.
 *
 * @param val integer value to append
 * @param v output vector
 */
extern void addValue(int val, std::vector<uint8_t>& v);

/**
 * Check whether a string is a valid Meshtastic message.
 *
 * @param s input string
 * @return boolean-like string result
 */
extern std::string isMessageValid(const std::string& s);
/**
 * Extract a bitfield from a byte.
 *
 * @param byte source byte
 * @param sx start bit index
 * @param dx end bit index
 * @return extracted integer value
 */
extern uint takeBits(uint8_t byte, int sx, int dx);

/**
 * Convert a 4-character hexadecimal string to a little-endian 4-byte buffer.
 *
 * @param s input hexadecimal string
 * @param bf output buffer of length 4
 * @param sz_bf buffer size (should be 4)
 */
extern void strLitEndiConverter(std::string s, uint8_t bf[4], int sz_bf);

/**
 * Convert four bytes from a vector into a little-endian 32-bit value.
 *
 * @param v source byte vector
 * @param begin inclusive start index
 * @param end inclusive end index
 * @return little-endian 32-bit result
 */
extern uint32_t litEndiConverter(std::vector<uint8_t> v, int begin, int end);

/**
 * Convert four bytes in-place to little-endian order.
 *
 * @param v source byte vector
 * @param begin inclusive start index
 * @param end inclusive end index
 */
extern void InPlaceLitEndiConverter(std::vector<uint8_t>& v, int begin, int end);

/**
 * Convert a 4-byte buffer into a little-endian 32-bit integer.
 *
 * @param bf input byte buffer
 * @param len buffer length
 * @return converted 32-bit integer
 */
extern uint32_t litEndiConverter(uint8_t bf[], int len);

/**
 * Convert a 32-bit value to a hexadecimal string.
 *
 * @param vaff input value
 * @return hex string
 */
extern std::string hexString32(uint32_t vaff);

/**
 * Encode bytes as a Base64 string.
 *
 * @param input bytes to encode
 * @return Base64-encoded string, padded with '='
 */
extern std::string base64Encoder(const std::vector<unsigned char>& input);

/**
 * Decode a Base64 string to bytes.
 *
 * @param input Base64-encoded string
 * @return decoded byte vector
 */
extern std::vector<unsigned char> base64Decoder(const std::string& input);

/**
 * Convert a byte vector to a raw string.
 *
 * @param v input vector
 * @return string representation
 */
extern std::string vectorToString(std::vector<uint8_t>& v);

/**
 * Fill a 4-byte buffer with random bytes.
 *
 * @param bf output buffer
 * @param sz_buffer buffer length
 */
extern void randomBuffer(uint8_t bf[4], int sz_buffer);


/**
 * Packet container for raw Meshtastic packet bytes and metadata.
 */
class Packet {
public:
  int size = -1, rssi = 0;
  bool is_complete = 0;

  std::vector<uint8_t> data;

  /**
   * Packet header fields for the first 16 bytes of a Meshtastic packet.
   */
  struct header {
    uint8_t to[4];
    uint8_t from[4];
    uint8_t id[4];
    uint8_t flags[1]; ///< hop limit, want ack, via mqtt, hop start
    uint8_t channel[1];
    uint8_t next_hop[1];
    uint8_t relay_node[1];
  };

  /**
   * Parsed flags values extracted from the packet header.
   */
  struct flags {
    int hop_limit; ///< 2:0
    int want_ack; ///< 3
    int via_mqtt; ///< 4
    int hop_start; ///< 7:5
  };

  /**
   * Fill a header struct from packet data.
   *
   * @param theHeader pointer to header struct to fill
   */
  void fillHeader(struct header* theHeader) {

    if (data.size() < sizeof(*theHeader)) {
      throw std::invalid_argument("vector size is not enough");
    }
    else {
      // Read and write the first 16 bytes into the header.
      memcpy(theHeader, data.data(), sizeof(*theHeader));
    }
  }

  /**
   * Fill a flags struct from packet header bytes.
   *
   * @param f pointer to flags struct to fill
   */
  void fillFlags(struct flags* f) {
    if (data.size() < 13) {
      throw std::invalid_argument("data vector's size is not enough");
    }
    else {
      uint8_t byte = data[12];

      f->hop_limit = takeBits(byte, 2, 0);
      f->hop_start = takeBits(byte, 7, 5);
      f->via_mqtt = takeBits(byte, 4, 4);
      f->want_ack = takeBits(byte, 3, 3);
    }
  }
  /**
   * Populate packet data from a hex string.
   *
   * @param hex input hexadecimal string
   */
  void hexStringToData(const std::string& hex) {
    hexStringToByte(hex, data);
    size = hex.length() / 2;
  }

  /**
   * Convert packet data to a hexadecimal string.
   *
   * @return hex string of packet data
   */
  std::string dataToString() {
    std::ostringstream oss;

    for (uint8_t b : data)
    {
      oss << std::hex
        << std::setw(2)
        << std::setfill('0')
        << static_cast<int>(b);
    }
    return(oss.str());
  }

  void information(char* buffer) {
    snprintf(buffer, 50, "Packet received: %d bytes - RSSI %d", this->size, this->rssi);
  }

  std::string info() {
    std::ostringstream buffer;


    buffer << makeStringHeader();
    buffer << "Recd packets:";
    buffer << this->size;
    buffer << " RSSI:";
    buffer << this->rssi;

    std::string result = buffer.str();
    return result;
  }

  /**
   * Format the packet header as a human-readable string.
   *
   * @return formatted header string
   */
  std::string makeStringHeader() {
    struct header h;
    struct flags f;

    fillHeader(&h);
    fillFlags(&f);

    std::ostringstream buffer;


    std::string tos{ hexString32(litEndiConverter(h.to,4)) };
    std::string froms{ hexString32(litEndiConverter(h.from,4)) };
    std::string ids{ hexString32(litEndiConverter(h.id,4)) };

    buffer << '\n' << "------Header------" << '\n';
    buffer << "to: " << addName(tos) << '\n';
    buffer << "from: " << addName(froms) << '\n';
    buffer << "ID packet: " << (ids) << '\n';
    buffer << "flags: " << bufferToString(h.flags, 1)
      << " (hop_limit:" << f.hop_limit << " hop_start:" << f.hop_start
      << " want acknowledge:" << f.want_ack << " via_mqtt:" << f.via_mqtt << ") " << '\n';
    buffer << "channel: " << bufferToString(h.channel, 1) << '\n';
    buffer << "nexthop: " << bufferToString(h.next_hop, 1) << '\n';
    buffer << "relay node: " << bufferToString(h.relay_node, 1) << '\n';
    buffer << dataToString() << '\n';

    if (isComplete()) {
      buffer << "packet complete" << '\n';
    }
    else {
      buffer << "packet incomplete exp:" << size << " act:" << data.size() << '\n';
    }
    buffer << "------";

    return(buffer.str());
  }

  /**
   * Append a known node name suffix to a Meshtastic ID string.
   *
   * @param s node ID string
   * @return ID string with known name suffix
   */
  std::string addName(std::string s) {

    const std::string broadcast = "ffffffff";
    const std::string esplilygo1 = "fc8eb3f9";
    const std::string giumello = "9890b3f9";

    if (s == broadcast) {
      std::string p = " broadcast";
      return(s + p);
    }
    if (s == giumello) {
      std::string p = " Giumello";
      return(s + p);
    }
    if (s == esplilygo1) {
      std::string p = " esplilygo1";
      return(s + p);
    }
    else { return s; }
  }

  bool isComplete() {
    return((size) == static_cast<int>(data.size()));
  }
};
/**
 * Convert a public key into the desired format to send into a NODEINFO message in a Mehstastic packet.
 * Convert the string into an array of uint8_t, then it reintepreters every byte as char, 
 * then makes a std::string out of the new char buffer 
 * 
 * @param public_key as std::string in hexadecimal value
 * @return publ_key_char a std::string of a char buffer of the public_key
 */
std::string convertKeysToInfoPacket(std::string publ_key);

/**
 * mask a specific bit in a specific byte of a std::vector of uint8_t 
 */
void maskBitVector(std::vector<uint8_t> &v,int byte,int bit);

/**
 * retrieve bit of a specific byte in a vector of uint8_t
 * @return uint8_t bit (1 or 0)
 */
uint8_t retrieveBitVector(std::vector<uint8_t> v,int byte,int bit);


/**
 *  remove want acknowledge from a packet 
 * 
 * @param std::string packet
 */
void removeWantAcknowledge(std::vector<uint8_t> &v);
 




#endif

