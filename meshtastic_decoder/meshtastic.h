#ifndef MESHTASTIC_H
#define MESHTASTIC_H

/**
 * C++ wrapper around Meshtastic nanopb-generated protobuf messages.
 *
 * This exposes convenient C++ types and functions instead of raw nanopb
 * callbacks for variable-length strings and byte fields.
 */

#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include "crypto.h"
#include "keys.h"

extern "C" {
#include "meshtastic/mesh.pb.h"
}

struct MeshtasticUser {
  std::string id;
  std::string longName;
  std::string shortName;
  std::string publicKey; // chiave pubblica raw (es. X25519), 32 byte
  uint32_t hwModel = 0;
  bool isLicensed = false;
  uint32_t role = 0;
};

/**
 * Encode a Meshtastic user structure as protobuf bytes.
 *
 * @param user Meshtastic user data
 * @return encoded protobuf bytes
 */
extern std::vector<uint8_t> encodeUser(const MeshtasticUser& user);

/**
 * Decode protobuf bytes into a Meshtastic user.
 *
 * @param buffer protobuf bytes
 * @return decoded user or std::nullopt on failure
 */
extern std::optional<MeshtasticUser> decodeUser(const std::vector<uint8_t>& buffer);

/*
Il payload di un pacchetto Data (es. il portnum NODEINFO_APP porta come
payload uno User codificato). portnum usa i valori dell'enum
meshtastic_PortNum (nanopb/meshtastic/portnums.pb.h).

Quando viene codificato, il valore (optional) bitfield
viene sempre scritto a zero.

*/
struct MeshtasticData {
  uint32_t portnum = 0;
  std::string payload; // byte grezzi, non testo
  bool wantResponse = false;
  uint32_t dest = 0;
  uint32_t source = 0;
  uint32_t requestId = 0;
  uint32_t replyId = 0;
  uint32_t emoji = 0;
  uint32_t bitfield = 0;
};

/**
 * Encode Meshtastic data as protobuf bytes.
 *
 * @param data Meshtastic data payload
 * @return encoded protobuf bytes
 */
extern std::vector<uint8_t> encodeData(const MeshtasticData& data);

/**
 * Decode protobuf bytes into Meshtastic data.
 *
 * @param buffer protobuf bytes
 * @return decoded data or std::nullopt on failure
 */
extern std::optional<MeshtasticData> decodeData(const std::vector<uint8_t>& buffer);

/**
 * XOR all bytes in a vector together.
 *
 * @param v input byte vector
 * @return XOR reduction result
 */
extern uint8_t xorVector(std::vector<uint8_t> v);
/**
 * Compute a simple channel hash from PSK and channel name bytes.
 *
 * @param psk_key pre-shared key bytes
 * @param name_channel channel name bytes
 * @return hash byte
 */
extern uint8_t channelHash(std::vector<uint8_t> psk_key, std::vector<uint8_t> name_channel);

/**
 * Compute a channel hash from PSK and channel name strings.
 *
 * @param psk_key pre-shared key string
 * @param name_channel channel name string
 * @return hash byte
 */
extern uint8_t channelHash(std::string psk_key, std::string name_channel);
/**
 * Decrypt a broadcast packet payload with a public key.
 *
 * @param data packet bytes
 * @param pubblic_key public key bytes
 * @return decrypted payload bytes
 */
extern std::vector<uint8_t> decryptBroadcast(std::vector<uint8_t>& data, std::vector<uint8_t> pubblic_key);
/**
 * Parse a protobuf data packet into MeshtasticData.
 *
 * @param v protobuf packet bytes
 * @return parsed data or std::nullopt on failure
 */
extern std::optional<MeshtasticData> readProto(std::vector<uint8_t> v);

/**
 * Convert a protobuf text representation to bytes.
 *
 * @param s protobuf text message
 * @return encoded protobuf bytes
 */
extern std::vector<uint8_t> stringToPb(std::string s);


/**
 * Build a Meshtastic header for direct messages.
 *
 * @param to destination node id buffer
 * @param from source node id buffer
 * @param id packet id buffer
 * @param hop_start hop start flag
 * @param want_ack ack request flag
 * @param receveid_with_mqtt MQTT received flag
 * @param hop_limit hop limit value
 * @param next_hop next hop node id
 * @param relay_node relay node id
 * @return constructed header bytes
 */
extern std::vector<uint8_t> makeHeaderDM(
  uint8_t to[4], //0-3
  uint8_t from[4], // 4-7
  uint8_t id[4], // 8-11
  uint8_t hop_start, //12
  uint8_t want_ack, //12
  uint8_t receveid_with_mqtt, //12
  uint8_t hop_limit, //12
  //13 hash ==0
  uint8_t next_hop, //14
  uint8_t relay_node //15 
);
/**
 * Build a Meshtastic header for broadcast messages.
 *
 * @param psk_key channel key bytes
 * @param name_channel channel name
 * @param to destination node id buffer
 * @param from source node id buffer
 * @param id packet id buffer
 * @param hop_start hop start flag
 * @param want_ack ack request flag
 * @param receveid_with_mqtt MQTT received flag
 * @param hop_limit hop limit value
 * @param next_hop next hop node id
 * @param relay_node relay node id
 * @return constructed header bytes
 */
extern std::vector<uint8_t> makeHeaderBroadcast(std::vector<uint8_t> psk_key,
  std::string name_channel,
  uint8_t to[4], //0-3
  uint8_t from[4], // 4-7
  uint8_t id[4], // 8-11
  uint8_t hop_start, //12
  uint8_t want_ack, //12
  uint8_t receveid_with_mqtt, //12
  uint8_t hop_limit, //12
  //13 hash calculated with psk and name_channel
  uint8_t next_hop, //14
  uint8_t relay_node //15 
);

/**
 * Encode and encrypt a direct message packet.
 *
 * @param aesKey AES key derived from the shared secret
 * @param message plaintext message
 * @param extra_nonce extra nonce buffer
 * @param to destination node id buffer
 * @param from source node id buffer
 * @param id packet id buffer
 * @param hop_start hop start flag
 * @param want_ack ack request flag
 * @param received_with_mqtt MQTT received flag
 * @param hop_limit hop limit value
 * @param next_hop next hop node id
 * @param relay_node relay node id
 * @return encoded encrypted packet bytes
 */
extern std::vector<uint8_t> encodeMessageDM(const AesKey256& aesKey,
  std::string& message,
  uint8_t extra_nonce[4],
  uint8_t to[4], //0-3
  uint8_t from[4], // 4-7
  uint8_t id[4], // 8-11
  uint8_t hop_start, //12
  uint8_t want_ack, //12
  uint8_t received_with_mqtt, //12
  uint8_t hop_limit, //12
  //13 hash =0x00
  uint8_t next_hop, //14
  uint8_t relay_node
);
/**
 * Encode and encrypt a broadcast message packet.
 *
 * @param psk_key channel key bytes
 * @param message plaintext message
 * @param name_channel channel name
 * @param to destination node id buffer
 * @param from source node id buffer
 * @param id packet id buffer
 * @param hop_start hop start flag
 * @param want_ack ack request flag
 * @param receveid_with_mqtt MQTT received flag
 * @param hop_limit hop limit value
 * @param next_hop next hop node id
 * @param relay_node relay node id
 * @return encoded encrypted packet bytes
 */
extern std::vector<uint8_t> encodeMessageBroadcast(std::vector<uint8_t> psk_key,
  std::string message,
  std::string name_channel,
  uint8_t to[4], //0-3
  uint8_t from[4], // 4-7
  uint8_t id[4], // 8-11
  uint8_t hop_start, //12
  uint8_t want_ack, //12
  uint8_t receveid_with_mqtt, //12
  uint8_t hop_limit, //12
  //13 hash calculated with psk and name_channel
  uint8_t next_hop, //14
  uint8_t relay_node //15
);
/**
 * Encode into protobuf a MeshtasticUser struct. Every packet outside of text messages is nested inside a text message
 * The MeshtasticUser is encoded as a vector<uint8_t> and put as string into the payload camp 
 * 
 * @param publ_key public key of sender
 * @param short_name short name of sender, max 4 characters long
 
 * @return encoded payload bytes
 */

std::vector<uint8_t> encodePerson(std::string short_name="1234",
                                std::string publ_key=Config::Device.publicKey,
                                std::string id= "!12341234",
                                std::string long_name="Device",
                                bool is_licensed=false,
                                uint32_t meshtastic_role=meshtastic_Config_DeviceConfig_Role_CLIENT,
                                uint32_t meshtastic_hardware=meshtastic_HardwareModel_TBEAM);
/**
 * encrypt NODEINFO message
 * 
 * @param from from what node
 * @param id id of the packet
 * @param hop_start with how many hops to start
* @param want_ack ack request flag
 * @param receveid_with_mqtt MQTT received flag
 * @param hop_limit hop limit value
 * @param next_hop next hop node id
 * @param relay_node relay node id
 * @return encoded encrypted packet bytes
 */
extern std::vector<uint8_t> encodeMessageInfo(
    std::vector<uint8_t> raw_protobuf_bytes,
    uint8_t from[4], // 4-7
    uint8_t id[4], // 8-11
    std::string psk_key_public_channel=Config::PublicChannel.key,
    uint8_t hop_start = 0x03, //12
    uint8_t want_ack = 0x00, //12
    uint8_t receveid_with_mqtt = 0x00, //12
    uint8_t hop_limit = 0x03, //12
    //13 hash calculated with psk and name_channel
    uint8_t next_hop = 0x00, //14
    uint8_t relay_node = 0x00//15
);

/** 
* Encode and encrypt a DM message packet
* Note: ...Sh functions are made to simplify the human work when encrypting
*
* @param priv_key string
* @param publ_key string
* @param message string
*
* @param extra_nonce = { 0x09, 0x5e, 0x03, 0x76 }
* @param to = { 0x34, 0x12, 0x34, 0x12 }
* @param from = { 0xfc, 0x8e, 0xb3, 0xf9 }
* @param id = { 0xab, 0x2b, 0x05, 0x54 }
* @param hop_start = 0x03
* @param want_ack = 0x00;
* @param received_with_mqtt = 0x01
* @param hop_limit = 0x03
* @param next_hop = 0x00
* @param relay_node = 0xfc
*
* @return encoded encrypted packet bytes
*/

extern std::vector<uint8_t> DMMessageSh(
  std::string priv_key,
  std::string publ_key,
  std::string& message,
  uint8_t extra_nonce[4],
  uint8_t to[4], //0-3
  uint8_t from[4], // 4-7
  uint8_t id[4], // 8-11
  //optionals:

  uint8_t hop_start = 0x03, //12
  uint8_t want_ack = 0x00, //12
  uint8_t received_with_mqtt = 0x00, //12
  uint8_t hop_limit = 0x03, //12
  //13 hash =0x00
  uint8_t next_hop = 0x00, //14
  uint8_t relay_node = 0x00
);
/**
 * Encrypt a broadcast message with optional defaults for shell use.
 *
 * @param psk_key channel key string
 * @param name channel name
 * @param msg message text
 * @param id packet id buffer
 * @param from source id buffer
 * @param hop_start hop start flag
 * @param want_ack ack request flag
 * @param received_with_mqtt MQTT received flag
 * @param hop_limit hop limit value
 * @param next_hop next hop node id
 * @param relay_node relay node id
 * @return encoded encrypted packet bytes
 */
extern std::vector<uint8_t> broadcastMessageSh(std::string psk_key,
  std::string name_channel,
  std::string msg,
  uint8_t id[4],
  uint8_t from[4],
  uint8_t hop_start = 0x03,
  uint8_t want_ack = 0x00,
  uint8_t received_with_mqtt = 0x00,
  uint8_t hop_limit = 0x03,
  uint8_t next_hop = 0x00,
  uint8_t relay_node = 0xfc
);
/**
 * Decrypt a private message packet using public/private keys.
 *
 * @param packet packet bytes
 * @param priv_key private key string
 * @param publ_key public key string
 * @return plaintext payload string
 */
extern std::string msgPayloadDM(std::vector<uint8_t> packet, std::string priv_key, std::string publ_key);

/**
 * Decrypt a broadcast packet using a channel key.
 *
 * @param packet packet bytes
 * @param key channel key string
 * @return plaintext payload string
 */
extern std::string msgPayloadBroadcast(std::vector<uint8_t> packet, std::string key);
/**
 * Decrypt packet containing user's info using decryptBroadcastUser
 * 
 * @param packet vector containing the packet
 * 
 * @return struct containing infos
 */
std::optional<MeshtasticUser> decryptBroadcastUser(std::vector<uint8_t> packet);
#endif
