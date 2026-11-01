/*
here you will find all the code I
used to create the custom functions
for the decoder_meshtastic



*/
#include "functions.h"
#include <string>
#include <iostream>
#include <sstream>
#include <cstddef>
#include <stdint.h>
#include <vector>
#include <iomanip>



uint32_t timeDifference(uint32_t t1, uint32_t t2) {
    if ((t1 < t2)) {
        return(t2 - t1);
    }
    if (t1 == t2) {
        return(0);
    }
    else {
        t1 = ~t1;
        return(t2 + t1 + 1);
    }

}


std::string stringClear(std::string s) {
    int a = 0;

    unsigned long string_length = s.size();
    int strlen = static_cast<int>(string_length);

    for (int i = 0;i + a != strlen + a;i++) {
        if ((i) % 4 == 0) {
            s.insert(i + a, 1, '-');
            a++;
        }
    }
    return(s);

}


std::string bufferToString(uint8_t* buffer, int size_buffer) {
    std::ostringstream oss;

    for (int i = 0;i < size_buffer;i++)
    {
        uint8_t b = buffer[i]; //it's the same of saying

        oss << std::hex
            << std::setw(2)
            << std::setfill('0')
            << static_cast<int>(b);
    }
    return(oss.str());
}

void hexStringToByte(const std::string& hex, std::vector<uint8_t>& v) {
    isMessageValid(hex);
    for (size_t i = 0;i < hex.length();i += 2) {
      int val = std::stoul(hex.substr(i, 2), nullptr, 16);
      addValue(val, v);
    }
}


std::string vectorToHexString(std::vector<uint8_t> v) {
    
    std::ostringstream oss;

    for (uint8_t b : v)
    {
      oss << std::hex
        << std::setw(2)
        << std::setfill('0')
        << static_cast<int>(b);
    }
    return(oss.str());
}

std::string b64StrToHexStr(std::string s) {
    std::vector<uint8_t> v = base64Decoder(s);
    return(vectorToHexString(v));
}



void addValue(int val, std::vector<uint8_t>& v) {
    v.push_back(static_cast<uint8_t>(val));
}

std::string isMessageValid(const std::string& s) {
        std::ostringstream oss{ "" };

    if (s.empty()) {
      throw std::invalid_argument("in isMessageValid,string empty");
    }
    if (s.length() % 2 != 0)
    {
      throw std::invalid_argument("in isMessageValid,string not hexadecimal");
    }
    for (unsigned long int i = 0;i < s.size();i++) {
      if (!std::isxdigit(s[i])) {
        oss << "character is not a hexadecimal value in position: " << i << " ..." << s[i] << "...\n";
      }
    }
    if (oss.str() != "") {
      throw std::invalid_argument(oss.str());
    }

    return(oss.str());
  }




uint takeBits(uint8_t byte, int sx, int dx) {
    
    uint mask{ 0x00 };
    int value{ -1 };



    if (dx > sx) {
        throw std::invalid_argument("dx bit is greather than sx bit");
    }
    if (sx < 0 || sx>7 || dx < 0 || dx>7) {
        throw std::invalid_argument("asked for bits outside the byte");
    }
    if (dx == sx) {
        mask |= 1u << dx;

        byte = byte & mask;
        value = static_cast<uint>(byte >> dx);
        return(value);
    }
    else {
        /*create the mask without a for loop*/
        mask = ((1u << (sx - dx + 1)) - 1) << dx; //  

        byte = byte & mask;
        value = byte >> dx;

        return(value);
    }
}
void strLitEndiConverter(std::string s, uint8_t bf[4],int sz_bf){
   Packet pk;
    
    if(sz_bf !=4){
        throw std::invalid_argument("strLitEndiConverter got a buffer of invalid length");
    }
    if(s.size()!=8){
        throw std::invalid_argument("strLitEndiConverter got a string of invalid length(8)");
    }

    pk.hexStringToData(s); //convert string into a vector, check it's a hex string
    
    
    for(int i=0;i<4;i++){
        bf[i]=pk.data.at(3-i);
    }
}

uint32_t litEndiConverter(std::vector<uint8_t> v, int begin, int end) {
       int sz = static_cast<int>(v.size());
    if (end > sz || begin > sz || begin > end)
    {
        throw std::invalid_argument("invalid 'begin' and 'end' numbers");
    }
    if (end - begin != 3) {
        throw std::invalid_argument("vecLitEndiConverter expects to take 4 bytes");
    }
    else {
        uint32_t new_number =

            (static_cast<uint32_t>(v.at(begin))) |
            (static_cast<uint32_t>(v.at(begin + 1)) << 8) |
            (static_cast<uint32_t>(v.at(begin + 2)) << 16) |
            (static_cast<uint32_t>(v.at(begin + 3)) << 24);

        return(new_number);
    }
}


void InPlaceLitEndiConverter(std::vector<uint8_t>& v, int begin, int end) {
       int sz = static_cast<int>(v.size());
    if (end > sz || begin > sz || begin > end)
    {
        throw std::invalid_argument ("invalid 'begin' and 'end' numbers");
    }
    if (end - begin != 3) {
        throw std::invalid_argument("vecLitEndiConverter is expected to take 4 bytes");
    }
    else {
        uint8_t bf[4];
        for (int i = 0;i != 4;i++) {
            bf[i] = ((v.at(begin + 3 - i)));
        }
        for (int i = 0;i != 4;i++) {
            v.at(begin + i) = bf[i];
        }

    }
}

uint32_t litEndiConverter(uint8_t bf[], int len)
{
   if (len != 4) {
        throw std::runtime_error(
            "converter expected 1 bytes and got:" + std::to_string(len));
    }
    else {
        uint32_t new_number =

            static_cast<uint32_t>(bf[0]) |
            (static_cast<uint32_t>(bf[1]) << 8) |
            (static_cast<uint32_t>(bf[2]) << 16) |
            (static_cast<uint32_t>(bf[3]) << 24);

        return(new_number);
    }

}


std::string hexString32(uint32_t vaff) {
    std::stringstream ss;
    ss << std::hex << std::setw(4) << std::setfill('0') << vaff;

    return(ss.str());
}
        
std::string base64Encoder(const std::vector<unsigned char>& input) {
    const std::string chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string output;

    for (size_t i = 0; i < input.size(); i += 3) {
        //ogni gruppo di 3 byte diventa un blocco di 4 caratteri
        const size_t left = input.size() - i;

        uint32_t block = static_cast<uint32_t>(input.at(i)) << 16;
        if (left > 1) {
            block |= static_cast<uint32_t>(input.at(i + 1)) << 8;
        }
        if (left > 2) {
            block |= static_cast<uint32_t>(input.at(i + 2));
        }

        output += chars.at((block >> 18) & 0x3F);
        output += chars.at((block >> 12) & 0x3F);
        //padding se i byte rimasti non riempiono il blocco
        output += (left > 1) ? chars.at((block >> 6) & 0x3F) : '=';
        output += (left > 2) ? chars.at(block & 0x3F) : '=';
    }

    return output;
}


std::vector<unsigned char> base64Decoder(const std::string& input) {
    if (input.size() % 4 != 0) {
        throw std::invalid_argument("bad string passed to base64Decoder, therefore some bits would be discarded");
    }
    const std::string chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::vector<uint8_t> characters(chars.begin(), chars.end());
    std::vector<uint8_t> output;

    int val = 0;
    int valb = -8;

    for (unsigned char c : input) {
        if (isspace(c)) {
            throw std::invalid_argument("base64 decoder got an exception: space found");
        }
        if (c == '=') {
            //padding, used if the bytes are not a multiple of 32 bytes
            break;

        }

        size_t index = chars.find(c);
        if (index == std::string::npos) {
            throw std::runtime_error("Invalid Base64 character");
        }

        val = (val << 6) | static_cast<int>(index);
        valb += 6;

        if (valb >= 0)
        {
            output.push_back(static_cast<uint8_t>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }

    return output;
}


std::string vectorToString(std::vector<uint8_t>& v) {
    std::string str;
    for (uint8_t x : v) {
        str += static_cast<uint8_t>(x);
    }
    return(str);

}

void randomBuffer(uint8_t bf[4], int sz_buffer){

  if(sz_buffer != 4){
    throw std::invalid_argument("randomBuffer function got a buffer of size!=4");
  }

#if defined(ARDUINO_ARCH_ESP32)
  bootloader_fill_random(bf, sz_buffer);
#else
  for(int i = 0; i < sz_buffer; i++){
    uint8_t nr = std::rand() % 256;
    bf[i] = nr;
  }
#endif

}



std::string convertKeysToInfoPacket(std::string publ_key){
    std::vector<uint8_t> key_vector;
    hexStringToByte(publ_key,key_vector); 
    std::string publ_key_short(
        reinterpret_cast<const char*>(key_vector.data()),
        key_vector.size()
    );
    return(publ_key_short);
}

uint8_t retrieveBitVector(std::vector<uint8_t> v,int byte,int bit){
    //BIT FROM THE DX
    if( v.size() < static_cast<uint8_t>(byte)+1){
        throw std::invalid_argument("byte and vector size passed aren't valid in maskBitVector");
    }
    if(bit>7 || bit<0){
        throw std::invalid_argument("position of bit passed is invalid");
    }

    uint8_t mask = 0x00;
    mask ^= 1 << bit;

    return((v.at(byte) & mask)>>bit);
}



void maskBitVector(std::vector<uint8_t> &v,int byte,int bit){
    //BIT FROM THE DX
    if( v.size() < static_cast<uint8_t>(byte)+1){
        throw std::invalid_argument("byte and vector size passed aren't valid in maskBitVector");
    }
    if(bit>7 || bit<0){
        throw std::invalid_argument("position of bit passed is invalid");
    }

    uint8_t mask = static_cast<uint8_t>(1u << bit);

    v.at(byte) ^= mask;
}

void removeWantAcknowledge(std::vector<uint8_t> &v){
  if(retrieveBitVector(v,12,3)==1){
    maskBitVector(v,12,3);
  }
}