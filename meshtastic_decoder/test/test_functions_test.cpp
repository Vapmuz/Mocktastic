/*
Test per le utilita' condivise in test_functions.h. Incluso solo da
functions_test.cpp sotto #ifdef TESTS, stesso schema degli altri test.
*/

#include "doctest.h"
#include "test_functions.h"


TEST_CASE("toHex da vector") {
    std::vector<uint8_t> v = { 0x00, 0x0a, 0xff };
    CHECK(toHex(v) == "000aff");
    CHECK(toHex(std::vector<uint8_t>{}) == "");
}

TEST_CASE("toHex da puntatore e lunghezza") {
    uint8_t buf[] = { 0xde, 0xad, 0xbe, 0xef };
    CHECK(toHex(buf, 4) == "deadbeef");
    CHECK(toHex(buf, 2) == "dead");
}

TEST_CASE("toHex da uint32_t") {
    CHECK(toHex(uint32_t{ 0 }) == "00000000");
    CHECK(toHex(uint32_t{ 0x095e0376 }) == "095e0376");
    CHECK(toHex(uint32_t{ 0xffffffff }) == "ffffffff");
}
