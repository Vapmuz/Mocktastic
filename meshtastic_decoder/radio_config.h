/**
 * Meshtastic radio regions and modem presets.
 *
 * The values are copied from the Meshtastic firmware:
 *  - regions: the RDEF table in src/mesh/RadioInterface.cpp
 *  - presets: modemPresetToParams() in src/mesh/MeshRadio.h
 *  - preset names: getModemPresetDisplayName() in src/DisplayFormatters.cpp
 *
 * The functions compute the same default frequency the firmware uses, so
 * this node lands on the same frequency as an official Meshtastic node with
 * the same region, preset, and primary channel name.
 */

#ifndef RADIO_CONFIG_H
#define RADIO_CONFIG_H

#include <cstdint>
#include <stdexcept>
#include <string>

/**
 * Legal band limits for one Meshtastic region.
 */
struct Region
{
    const char *name;
    uint32_t freqStartHz;
    uint32_t freqEndHz;
    uint8_t dutyCyclePercent;
    int8_t powerLimitDbm;
};

/**
 * LoRa modem settings for one Meshtastic modem preset.
 *
 * name is the firmware display name. The firmware hashes this name to pick
 * the frequency slot when the primary channel has no name of its own.
 */
struct ModemPreset
{
    const char *name;
    uint8_t spreadingFactor;
    uint32_t bandwidthHz;
    uint8_t codingRateDenominator; // 5 means 4/5, 8 means 4/8
};

namespace Regions
{
    // The 433 MHz board (SX1278) can only use EU_433.
    inline constexpr Region EU_433{"EU_433", 433000000, 434000000, 10, 10};
    // The 868 MHz board (SX1276) uses EU_868.
    inline constexpr Region EU_868{"EU_868", 869400000, 869650000, 10, 27};
}

namespace Presets
{
    inline constexpr ModemPreset SHORT_TURBO{"ShortTurbo", 7, 500000, 5};
    inline constexpr ModemPreset SHORT_FAST{"ShortFast", 7, 250000, 5};
    inline constexpr ModemPreset SHORT_SLOW{"ShortSlow", 8, 250000, 5};
    inline constexpr ModemPreset MEDIUM_FAST{"MediumFast", 9, 250000, 5};
    inline constexpr ModemPreset MEDIUM_SLOW{"MediumSlow", 10, 250000, 5};
    inline constexpr ModemPreset LONG_TURBO{"LongTurbo", 11, 500000, 8};
    inline constexpr ModemPreset LONG_FAST{"LongFast", 11, 250000, 5};
    inline constexpr ModemPreset LONG_MODERATE{"LongMod", 11, 125000, 8};
    inline constexpr ModemPreset LONG_SLOW{"LongSlow", 12, 125000, 8};
}

/**
 * Hash a string with djb2, the hash Meshtastic uses to pick a frequency slot.
 *
 * @param text string to hash
 * @return 32-bit hash
 */
uint32_t djb2Hash(const std::string &text);

/**
 * Find the default frequency slot for a primary channel.
 *
 * @param region regional band limits
 * @param preset modem preset
 * @param primaryChannelName name of the primary channel; empty means the
 *        default channel, which the firmware names after the preset
 * @return zero-based slot; the Meshtastic app shows it one-based
 * @throws std::invalid_argument if the preset bandwidth does not fit the region
 */
uint32_t frequencySlot(const Region &region, const ModemPreset &preset, const std::string &primaryChannelName);

/**
 * Compute the centre frequency of a slot.
 *
 * @param region regional band limits
 * @param preset modem preset
 * @param slot zero-based slot
 * @return centre frequency in Hz
 * @throws std::invalid_argument if the slot is outside the region
 */
uint32_t slotFrequencyHz(const Region &region, const ModemPreset &preset, uint32_t slot);

/**
 * Cap a TX power level at the region limit.
 *
 * @param requestedDbm requested power in dBm
 * @param region regional band limits
 * @return the requested power, or the region limit if the request is higher
 */
int clampTxPower(int requestedDbm, const Region &region);

#endif
