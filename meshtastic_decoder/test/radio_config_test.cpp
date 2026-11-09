#ifdef TESTS

/**
 * Tests for the Meshtastic region and modem preset calculations.
 *
 * The expected frequencies come from the Meshtastic firmware
 * (src/mesh/RadioInterface.cpp): the default slot is
 * djb2(primary channel name) % slot count, and the centre frequency is
 * freqStart + bandwidth / 2 + slot * bandwidth.
 */

#include "doctest.h"
#include "../radio_config.h"
#include "../radio_config.cpp"

namespace
{
    // US band limits from the Meshtastic region table. Used only as a
    // known answer: the US LongFast default is slot 20, 906.875 MHz.
    const Region US_FOR_TEST{"US", 902000000, 928000000, 100, 30};

    uint32_t defaultFrequencyHz(const Region &region, const ModemPreset &preset, const std::string &channelName)
    {
        return slotFrequencyHz(region, preset, frequencySlot(region, preset, channelName));
    }
}

TEST_CASE("djb2Hash of an empty string is the seed")
{
    CHECK(djb2Hash("") == 5381u);
}

TEST_CASE("US LongFast uses frequency slot 20")
{
    // frequencySlot is zero-based; the Meshtastic app shows it one-based.
    CHECK(frequencySlot(US_FOR_TEST, Presets::LONG_FAST, "LongFast") == 19u);
}

TEST_CASE("US LongFast default frequency is 906.875 MHz")
{
    CHECK(defaultFrequencyHz(US_FOR_TEST, Presets::LONG_FAST, "LongFast") == 906875000u);
}

TEST_CASE("EU_868 MediumFast default frequency is 869.525 MHz")
{
    CHECK(defaultFrequencyHz(Regions::EU_868, Presets::MEDIUM_FAST, "MediumFast") == 869525000u);
}

TEST_CASE("EU_433 LongFast default frequency is 433.875 MHz")
{
    CHECK(defaultFrequencyHz(Regions::EU_433, Presets::LONG_FAST, "LongFast") == 433875000u);
}

TEST_CASE("EU_433 MediumFast default frequency is 433.125 MHz")
{
    CHECK(defaultFrequencyHz(Regions::EU_433, Presets::MEDIUM_FAST, "MediumFast") == 433125000u);
}

TEST_CASE("EU_433 LongModerate hashes the firmware name LongMod")
{
    CHECK(defaultFrequencyHz(Regions::EU_433, Presets::LONG_MODERATE, Presets::LONG_MODERATE.name) == 433687500u);
}

TEST_CASE("An empty channel name uses the preset name")
{
    CHECK(frequencySlot(Regions::EU_433, Presets::LONG_FAST, "") ==
          frequencySlot(Regions::EU_433, Presets::LONG_FAST, "LongFast"));
}

TEST_CASE("A preset wider than the region throws")
{
    // EU_868 is 250 kHz wide; ShortTurbo needs 500 kHz.
    CHECK_THROWS_AS(frequencySlot(Regions::EU_868, Presets::SHORT_TURBO, "ShortTurbo"), std::invalid_argument);
}

TEST_CASE("A slot past the end of the region throws")
{
    // EU_433 has four 250 kHz slots: 0 to 3.
    CHECK_THROWS_AS(slotFrequencyHz(Regions::EU_433, Presets::LONG_FAST, 4), std::invalid_argument);
}

TEST_CASE("TX power above the region limit is capped")
{
    CHECK(clampTxPower(17, Regions::EU_433) == 10);
}

TEST_CASE("TX power at or below the region limit is kept")
{
    CHECK(clampTxPower(5, Regions::EU_433) == 5);
}

#endif
