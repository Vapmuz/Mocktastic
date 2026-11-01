


#include "meshtastic.h"


extern "C" {
#include "meshtastic/mesh.pb.h"
#include "pb_decode.h"
#include "pb_encode.h"
}

#include "functions.h"
#include "crypto.h"
#include "keys.h"



/*
meshtastic_User (nanopb/meshtastic/mesh.pb.h) ha i campi stringa/bytes
(id, long_name, short_name, macaddr, public_key) generati come
pb_callback_t: nanopb non alloca un buffer statico per loro, quindi serve
una funzione di callback sia per la codifica che per la decodifica.
Questo e' il pattern standard di nanopb per campi di dimensione variabile.
*/





// Callback di codifica: arg punta a una std::string col contenuto da scrivere.
static bool encodeStringCb(pb_ostream_t* stream, const pb_field_t* field, void* const* arg) {
    const auto* str = (const std::string*)(*arg);
    if (!pb_encode_tag_for_field(stream, field)) {
        return false;
    }
    return pb_encode_string(stream, (const uint8_t*)str->data(), str->size());
}

// Callback di decodifica: arg punta a una std::string che riceve il contenuto letto.
static bool decodeStringCb(pb_istream_t* stream, const pb_field_t* /*field*/, void** arg) {
    auto* out = (std::string*)(*arg);
    out->resize(stream->bytes_left);
    return pb_read(stream, (uint8_t*)out->data(), stream->bytes_left);
}

std::vector<uint8_t> encodeUser(const MeshtasticUser& user) {
    meshtastic_User msg = meshtastic_User_init_zero;

    msg.id.funcs.encode = encodeStringCb;
    msg.id.arg = (void*)&user.id;
    msg.long_name.funcs.encode = encodeStringCb;
    msg.long_name.arg = (void*)&user.longName;
    msg.short_name.funcs.encode = encodeStringCb;
    msg.short_name.arg = (void*)&user.shortName;
    msg.public_key.funcs.encode = encodeStringCb;
    msg.public_key.arg = (void*)&user.publicKey;
    msg.hw_model = (meshtastic_HardwareModel)user.hwModel;
    msg.is_licensed = user.isLicensed;
    msg.role = (meshtastic_Config_DeviceConfig_Role)user.role;

    // Un messaggio User non supera mai qualche centinaio di byte: 256 basta
    // ampiamente per id/nomi/chiave pubblica.
    std::vector<uint8_t> buffer(256);
    pb_ostream_t stream = pb_ostream_from_buffer(buffer.data(), buffer.size());
    if (!pb_encode(&stream, meshtastic_User_fields, &msg)) {
        throw std::invalid_argument(
            std::string("could not encode protobuf for user: ") + PB_GET_ERROR(&stream) +
            " (long name " + std::to_string(user.longName.size()) + " bytes)");
    }
    buffer.resize(stream.bytes_written);
    return buffer;
}

std::optional<MeshtasticUser> decodeUser(const std::vector<uint8_t>& buffer) {
    MeshtasticUser result;

    meshtastic_User msg = meshtastic_User_init_zero;
    msg.id.funcs.decode = decodeStringCb;
    msg.id.arg = &result.id;
    msg.long_name.funcs.decode = decodeStringCb;
    msg.long_name.arg = &result.longName;
    msg.short_name.funcs.decode = decodeStringCb;
    msg.short_name.arg = &result.shortName;
    msg.public_key.funcs.decode = decodeStringCb;
    msg.public_key.arg = &result.publicKey;

    pb_istream_t stream = pb_istream_from_buffer(buffer.data(), buffer.size());
    if (!pb_decode(&stream, meshtastic_User_fields, &msg)) {
        return std::nullopt;
    }

    result.hwModel = msg.hw_model;
    result.isLicensed = msg.is_licensed;
    result.role = msg.role;
    return result;
}

std::vector<uint8_t> encodeData(const MeshtasticData& data) {
    meshtastic_Data msg = meshtastic_Data_init_zero;

    msg.portnum = (meshtastic_PortNum)data.portnum;
    msg.payload.funcs.encode = encodeStringCb;
    msg.payload.arg = (void*)&data.payload;
    msg.want_response = data.wantResponse;
    msg.dest = data.dest;
    msg.source = data.source;
    msg.request_id = data.requestId;
    msg.reply_id = data.replyId;
    msg.emoji = data.emoji;
    msg.has_bitfield = true;
    msg.bitfield = data.bitfield;

    // SIZEDATA (512, vedi functions.h) e' la dimensione massima di un
    // pacchetto LoRa Meshtastic; l'overhead di Data oltre al payload e'
    // minimo, quindi basta ampiamente.
    std::vector<uint8_t> buffer(512);
    pb_ostream_t stream = pb_ostream_from_buffer(buffer.data(), buffer.size());
    if (!pb_encode(&stream, meshtastic_Data_fields, &msg)) {
        throw std::invalid_argument(
            std::string("could not encode protobuf for message: ") + PB_GET_ERROR(&stream) +
            " (payload " + std::to_string(data.payload.size()) + " bytes)");
    }
    buffer.resize(stream.bytes_written);
    return buffer;
}

std::optional<MeshtasticData> decodeData(const std::vector<uint8_t>& buffer) {
    MeshtasticData result;

    meshtastic_Data msg = meshtastic_Data_init_zero;
    msg.payload.funcs.decode = decodeStringCb;
    msg.payload.arg = &result.payload;

    pb_istream_t stream = pb_istream_from_buffer(buffer.data(), buffer.size());
    if (!pb_decode(&stream, meshtastic_Data_fields, &msg)) {
        return std::nullopt;
    }

    result.portnum = msg.portnum;
    result.wantResponse = msg.want_response;
    result.dest = msg.dest;
    result.source = msg.source;
    result.requestId = msg.request_id;
    result.replyId = msg.reply_id;
    result.emoji = msg.emoji;
    result.bitfield = msg.bitfield;
    return result;
}


uint8_t xorVector(std::vector<uint8_t> v) {
    if (v.size() < 1) {
        throw std::invalid_argument("Vector called in xorVector funtion as no elements, therefore can't xor elements");
    }
    uint8_t result = 0x00;
    for (size_t i = 0;v.size() > i;i++) {
        result = result ^ v.at(i);
    }
    return(result);
}



uint8_t channelHash(std::vector<uint8_t> psk_key, std::vector<uint8_t> name_channel) {
    uint8_t channel_hash;
    channel_hash = xorVector(psk_key) ^ xorVector(name_channel);
    return(channel_hash);
}

uint8_t channelHash(std::string psk_key, std::string name_channel) {
    std::vector<uint8_t> psk_key_v;
    hexStringToByte(psk_key, psk_key_v);
    std::vector<uint8_t> name_channel_v(name_channel.begin(), name_channel.end());

    return(channelHash(psk_key_v, name_channel_v));
}





std::vector<uint8_t> decryptBroadcast(std::vector<uint8_t>& data, std::vector<uint8_t> aes_key) {
    if (data.size() < 16) {
        throw std::invalid_argument("packet corrupted, decryptBroadcast got a packet<16 bytes");
    }


    Packet pk;
    struct Packet::header h;

    pk.data = data;
    pk.fillHeader(&h);

    uint32_t id = litEndiConverter(h.id, 4);
    uint32_t from = litEndiConverter(h.from, 4);

    std::vector<uint8_t> payload(pk.data.begin() + 16, pk.data.end());
    //put the payload into the payload vector
    std::vector<uint8_t> decripted = channelCryptCtr(aes_key, id, from, payload);

    return(decripted);
}


std::optional<MeshtasticData> readProto(std::vector<uint8_t> v) {
    std::optional<MeshtasticData> result = decodeData(v);
    if(result.has_value() == false){
        throw std::invalid_argument("passed invalid protobuf datas to readProto");
    }
    return(result);
}


std::vector<uint8_t> stringToPb(std::string s) {

    MeshtasticData data;

    data.portnum = meshtastic_PortNum_TEXT_MESSAGE_APP;
    data.payload = s;


    std::vector<uint8_t> pr_data = encodeData(data);
    return(pr_data);
}

std::vector<uint8_t> makeHeaderDM(
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
) {
    Packet pk;

    for (int i = 0;i < 4;i++) {
        addValue(to[i], pk.data);
    }
    for (int i = 0;i < 4;i++) {
        addValue(from[i], pk.data);
    }
    for (int i = 0;i < 4;i++) {
        addValue(id[i], pk.data);
    }
    uint8_t flags = 00;
    flags |= (hop_start << 5) | (want_ack << 3) | (receveid_with_mqtt << 4) | (hop_limit);

    addValue(flags, pk.data);

    uint8_t channel_hash = 0x00;
    addValue(channel_hash, pk.data);
    addValue(next_hop, pk.data);
    addValue(relay_node, pk.data);

    return(pk.data);
}

std::vector<uint8_t> makeHeaderBroadcast(std::vector<uint8_t> psk_key,
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
) {

    Packet pk;
    for (int i = 0;i < 4;i++) {
        addValue(to[i], pk.data);
    }
    for (int i = 0;i < 4;i++) {
        addValue(from[i], pk.data);
    }
    for (int i = 0;i < 4;i++) {
        addValue(id[i], pk.data);
    }
    uint8_t flags = 00;
    flags |= (hop_start << 5) | (want_ack << 3) | (receveid_with_mqtt << 4) | (hop_limit);

    addValue(flags, pk.data);

    std::vector<uint8_t> name_channel_v(name_channel.begin(), name_channel.end());
    uint8_t channel_hash = channelHash(psk_key, name_channel_v);
    addValue(channel_hash, pk.data);
    addValue(next_hop, pk.data);
    addValue(relay_node, pk.data);

    return(pk.data);
}

std::vector<uint8_t> encodeMessageDM(const AesKey256& aesKey,
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
) {
    if (hop_start > 7 || hop_limit > 7) {
        throw std::invalid_argument("bad mesage encoding, hop_limit or hop_start setted too high in eencodeMessage");
    }
    if (want_ack > 1 || received_with_mqtt > 1) {
        throw std::invalid_argument("bad mesage encoding, want_ack or received_with_mqtt over one in encodeMessage");
    }
    if (message.size() > 200) {
        throw std::invalid_argument("bad message encoding: size message too big");
    }


    uint32_t from32 = litEndiConverter(from, 4);
    uint32_t id32 = litEndiConverter(id, 4);
    uint32_t extra_nonce32 = litEndiConverter(extra_nonce, 4);
    std::vector<uint8_t> message_v = stringToPb(message);

    CcmCiphertext result_encryption = ccmEncrypt(aesKey, id32, extra_nonce32, from32, message_v);

    std::vector<uint8_t> header = makeHeaderDM(
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

    std::vector<uint8_t> payload = result_encryption.ciphertext;
    CcmTag tag = result_encryption.tag;
    std::array<uint8_t, 4> extra_nonce_a;

    for (size_t i = 0;i < extra_nonce_a.size();i++) {
        extra_nonce_a.at(i) = extra_nonce[i];
    }

    std::vector<uint8_t> final_packet;
    //final packet = header+ payload + tag +extra nonce
    final_packet.reserve(header.size() + payload.size() + tag.size() + 4);
    final_packet.insert(final_packet.end(), header.begin(), header.end());
    final_packet.insert(final_packet.end(), payload.begin(), payload.end());

    final_packet.insert(final_packet.end(), tag.begin(), tag.end());

    final_packet.insert(final_packet.end(), extra_nonce_a.begin(), extra_nonce_a.end());

    return(final_packet);
}


std::vector<uint8_t> encodeMessageBroadcast(std::vector<uint8_t> psk_key,
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
) {



    //error gates:
    if (hop_start > 7 || hop_limit > 7) {
        throw std::invalid_argument("bad mesage encoding, hop_limit or hop_start setted too high in eencodeMessage");
    }
    if (want_ack > 1 || receveid_with_mqtt > 1) {
        throw std::invalid_argument("bad mesage encoding, want_ack or received_with_mqtt over one in encodeMessage");
    }
    if (message.size() > 200) {
        throw std::invalid_argument("bad message encoding: size message too big");
    }

    //payload:
    std::vector<uint8_t> payload = payloadEncrypterBroadcast(psk_key, id, from, stringToPb(message));
    //encodeData(payload);


    //header
    std::vector<uint8_t> header;
    header = makeHeaderBroadcast(psk_key, name_channel, to, from, id, hop_start, want_ack, receveid_with_mqtt, hop_limit, next_hop, relay_node);
    if (header.size() != 16) {
        throw std::invalid_argument("bad header concatenation: the header is not 16 byte long in encodeMessageBroadcast");
    }

    std::vector<uint8_t> final_packet;
    final_packet.reserve(header.size() + payload.size());
    final_packet.insert(final_packet.end(), header.begin(), header.end());
    final_packet.insert(final_packet.end(), payload.begin(), payload.end());

    return final_packet;
}

std::vector<uint8_t> encodeMessageInfo(
    std::vector<uint8_t> raw_protobuf_bytes,
    uint8_t from[4], // 4-7
    uint8_t id[4], // 8-11
    std::string psk_key_pubblic_channel,
    uint8_t hop_start, //12
    uint8_t want_ack, //12
    uint8_t receveid_with_mqtt, //12
    uint8_t hop_limit, //12
    //13 hash calculated with psk and name_channel
    uint8_t next_hop, //14
    uint8_t relay_node //15
) {

    //error gates:
    if (hop_start > 7 || hop_limit > 7) {
        throw std::invalid_argument("bad mesage encoding, hop_limit or hop_start setted too high in eencodeMessage");
    }
    if (want_ack > 1 || receveid_with_mqtt > 1) {
        throw std::invalid_argument("bad mesage encoding, want_ack or received_with_mqtt over one in encodeMessage");
    }
    if (raw_protobuf_bytes.size() > 200) {
        throw std::invalid_argument("bad message encoding: size message too big");
    }

    std::string psk_key_s= psk_key_pubblic_channel;
    std::vector<uint8_t> psk_key;
    hexStringToByte(psk_key_s,psk_key);
    std::string name_channel= Config::PublicChannel.name;

    //payload:
    std::vector<uint8_t> payload = payloadEncrypterBroadcast(psk_key, id, from, raw_protobuf_bytes);
    //encodeData(payload);

    uint8_t to[4]= {0xff,0xff,0xff,0xff};

    //header
    std::vector<uint8_t> header;
    header = makeHeaderBroadcast(psk_key, name_channel, to, from, id, hop_start, want_ack, receveid_with_mqtt, hop_limit, next_hop, relay_node);
    if (header.size() != 16) {
        throw std::invalid_argument("bad header concatenation: the header is not 16 byte long in encodeMessageBroadcast");
    }

    std::vector<uint8_t> final_packet;
    final_packet.reserve(header.size() + payload.size());
    final_packet.insert(final_packet.end(), header.begin(), header.end());
    final_packet.insert(final_packet.end(), payload.begin(), payload.end());

    return final_packet;
}
std::vector<uint8_t> encodePerson(std::string short_name,
                                std::string publ_key,
                                std::string id,
                                std::string long_name,
                                bool is_licensed,
                                uint32_t meshtastic_role,
                                uint32_t meshtastic_hardware) {
    MeshtasticUser user;
    

    std::ostringstream oss;

    if(long_name.size()>20){
        throw std::invalid_argument("passed long name too long to encodePerson");
    }
    //id has to be 8 character long
    if(id.size()!=9){
        throw std::invalid_argument("passed id to encodePerson with invalid length");
    }
    if(!(id.at(0)==('!'))){
        throw std::invalid_argument("passed invalid id at encodePerson");
    } 
    //id has to be a hex string 
    for (unsigned long int i = 1;i < id.size();i++) {
        if (!std::isxdigit(id[i])) {  
            throw std::invalid_argument("passed id to encodePerson that is not a hex string");
        }
    }


    //make publ key a std::string of the char buffer of his bytes
    
    
    std::string publ_key_short = convertKeysToInfoPacket(publ_key);

    user.id = id;
    user.longName = long_name;
    user.shortName = short_name;
    user.publicKey = publ_key_short;
    user.hwModel = meshtastic_hardware;
    user.isLicensed = is_licensed;
    user.role = meshtastic_role;

    std::vector<uint8_t> person_data = encodeUser(user); //now the User is encoded as raw bytes

    MeshtasticData data;
    data.portnum = meshtastic_PortNum_NODEINFO_APP;
    data.payload = std::string(person_data.begin(),person_data.end()); //the message encoded as protobuf
    
    return(encodeData(data));
}
std::vector<uint8_t> DMMessageSh(
    std::string priv_key,
    std::string publ_key,
    std::string& message,
    uint8_t extra_nonce[4],
    uint8_t to[4], //0-3
    uint8_t from[4], // 4-7
    uint8_t id[4], // 8-11

    //optionals:
    uint8_t hop_start, //12
    uint8_t want_ack, //12
    uint8_t received_with_mqtt, //12
    uint8_t hop_limit, //12
    //13 hash =0x00
    uint8_t next_hop, //14
    uint8_t relay_node
) {

    X25519Key aes_key = stringToAesKey(priv_key, publ_key);
    return(encodeMessageDM(
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
    ));

}

std::vector<uint8_t> broadcastMessageSh(std::string psk_key,
    std::string name_channel,
    std::string msg,
    uint8_t id[4],
    uint8_t from[4],
    //optional:
    uint8_t hop_start,
    uint8_t want_ack,
    uint8_t received_with_mqtt,
    uint8_t hop_limit,
    uint8_t next_hop,
    uint8_t relay_node

) {
    std::vector<uint8_t> psk_key_v;
    hexStringToByte(psk_key, psk_key_v);

    uint8_t to[4] = { 0xff, 0xff, 0xff, 0xff };

    return(
        encodeMessageBroadcast(
            psk_key_v,
            msg,
            name_channel,
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
}

std::string msgPayloadDM(std::vector<uint8_t> packet, std::string priv_key, std::string publ_key) {
    AesKey256 aes_key = stringToAesKey(priv_key, publ_key);

    std::vector<uint8_t> raw_bytes = decryptMessageDM(packet, aes_key);
    auto proto = readProto(raw_bytes);

    if (proto->portnum == meshtastic_PortNum_TEXT_MESSAGE_APP) {
        return(proto->payload);
    }
    else {
        std::string portnum = std::to_string(proto->portnum);
        return("message type:" + portnum);
    }
}


std::string msgPayloadBroadcast(std::vector<uint8_t> packet, std::string key) {
    std::vector<uint8_t> key_v;
    hexStringToByte(key, key_v);

    std::vector<uint8_t> raw_bytes = decryptBroadcast(packet, key_v);
    auto proto = readProto(raw_bytes);
    if (proto->portnum == meshtastic_PortNum_TEXT_MESSAGE_APP) {
        return(proto->payload);
    }
    else {
        std::string portnum = std::to_string(proto->portnum);
        return("message type:" + portnum);
    }
}

