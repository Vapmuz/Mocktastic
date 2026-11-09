#include "radio_config.h"

#include <algorithm>

uint32_t djb2Hash(const std::string &text)
{
    uint32_t hash = 5381;
    for (unsigned char c : text) {
        hash = (hash << 5) + hash + c; // hash * 33 + c
    }
    return hash;
}

/**
 * Count the frequency slots that fit in a region.
 *
 * Release firmware v2.7 uses floor(span / bandwidth). See IMPLEMENTATION_PLAN.md
 * for why this matches the newer round() formula for the regions defined here.
 */
static uint32_t frequencySlotCount(const Region &region, const ModemPreset &preset)
{
    return (region.freqEndHz - region.freqStartHz) / preset.bandwidthHz;
}

uint32_t frequencySlot(const Region &region, const ModemPreset &preset, const std::string &primaryChannelName)
{
    uint32_t slotCount = frequencySlotCount(region, preset);
    if (slotCount == 0) {
        throw std::invalid_argument(std::string("preset ") + preset.name + " is wider than region " + region.name);
    }

    std::string hashedName = primaryChannelName.empty() ? preset.name : primaryChannelName;
    return djb2Hash(hashedName) % slotCount;
}

uint32_t slotFrequencyHz(const Region &region, const ModemPreset &preset, uint32_t slot)
{
    if (slot >= frequencySlotCount(region, preset)) {
        throw std::invalid_argument("slot " + std::to_string(slot) + " is outside region " + region.name +
                                    " for preset " + preset.name);
    }
    return region.freqStartHz + preset.bandwidthHz / 2 + slot * preset.bandwidthHz;
}

int clampTxPower(int requestedDbm, const Region &region)
{
    return std::min(requestedDbm, static_cast<int>(region.powerLimitDbm));
}
