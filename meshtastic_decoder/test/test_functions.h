/*
Funzioni di utilita' condivise tra i file di test. Tutti i test finiscono
nella stessa translation unit (functions_test.cpp include gli altri), quindi
le funzioni sono inline e l'header ha una include guard.
*/

#ifndef TEST_FUNCTIONS_H
#define TEST_FUNCTIONS_H

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>


inline std::string toHex(const std::vector<uint8_t>& v) {
    std::string s;
    char buf[3];
    for (uint8_t b : v) {
        std::snprintf(buf, 3, "%02x", b);
        s += buf;
    }
    return s;
}

inline std::string toHex(const uint8_t* p, size_t n) {
    return toHex(std::vector<uint8_t>(p, p + n));
}

inline std::string toHex(uint32_t val) {
    char buf[10];
    std::snprintf(buf, sizeof(buf), "%08x", val);
    return std::string(buf);
}

#endif // TEST_FUNCTIONS_H
