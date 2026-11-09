# Implementation plan: EU 433 MHz on the LilyGO T3 V1.6.1 (SX1278)

## Context

Upstream Mocktastic targets the LilyGO T3 V1.6.1 with an SX1276 radio on
EU 868 MHz. This fork targets the same board in its 433 MHz version, which
has an SX1278 radio.

Findings from the code review on 2026-11-09:

- The SX1276 and SX1278 share one register map and one silicon version
  (`0x12`). The SX1278 tunes 137-525 MHz only, so it cannot reach 868 MHz.
  At 433 MHz it supports every Meshtastic bandwidth (125, 250, 500 kHz) and
  spreading factor (7-12).
- The `LoRa` library by Sandeep Mistry supports SX1276/77/78/79 and accepts
  any frequency. It checks for version `0x12`, which the SX1278 returns.
- The 433 MHz board uses the same GPIO pins as the 868 MHz board.
- All radio settings live in `meshtastic_decoder.ino`. The protocol code
  (packets, crypto, channel hash, protobuf) does not depend on frequency.
- Hard-coded 868 MHz assumptions, all in `meshtastic_decoder.ino`:
  - `LoRa.begin(869525024)` in both `setup()` functions.
  - The MediumFast modem settings, duplicated in both `setup()` functions.
  - No `LoRa.setTxPower()` call, so the library default of 17 dBm applies.
    The Meshtastic EU_433 limit is 10 dBm.
  - `/pwr17` accepts any power level.

## Stage 1: Region and preset module
**Goal**: A host-tested `radio_config.h/.cpp` that computes the Meshtastic
default frequency for a region, preset, and primary channel name, and caps
TX power at the region limit.
**Success Criteria**: Known Meshtastic frequencies come out exactly.
**Tests**:
- US LongFast is slot 20, 906.875 MHz (documented Meshtastic default).
- EU_868 MediumFast is 869.525 MHz (the upstream hard-coded value).
- EU_433 LongFast is 433.875 MHz; EU_433 MediumFast is 433.125 MHz.
- EU_433 LongModerate hashes the name "LongMod", so it lands on 433.6875 MHz.
- An empty channel name falls back to the preset name.
- A preset wider than the region throws.
- A slot past the end of the region throws.
- TX power above the region limit is capped; power below it is kept.
**Status**: Complete

## Stage 2: Use the module in the sketch
**Goal**: One `setupRadio()` for both modes that reads region and preset from
`keys.h`, sets TX power to the region limit, and prints what it chose.
`/pwr17` caps the value at the region limit.
**Success Criteria**: DECODER and SENSOR builds compile for the T3 board.
**Tests**: ESP32 compile of both modes (PlatformIO, espressif32 5.2.0).
**Status**: Complete

## Stage 3: EU_433 LongFast default
**Goal**: `keys.h` selects `Regions::EU_433` and `Presets::LONG_FAST`, and the
public channel name follows the preset.
**Success Criteria**: All existing host tests still pass.
**Tests**: `make test`.
**Status**: In Progress

## Stage 4: Documentation
**Goal**: README explains the 433 MHz board, region and preset selection,
the frequency table, and the power and duty cycle limits.
**Success Criteria**: A new user can configure the board from the README.
**Status**: Not Started

## Decisions

- **Compute the frequency, do not hard-code it.** Meshtastic picks the
  frequency slot by hashing the primary channel name (djb2) modulo the slot
  count. A hand-copied number breaks silently when the preset or channel
  name changes. Computing it the same way as the firmware keeps the two in
  step.
- **Integer Hz arithmetic.** Meshtastic computes in `float` MHz, which gives
  869525024 Hz for 869.525 MHz (the upstream value). The SX127x frequency
  step is 61 Hz, so the 24 Hz float error does not matter. Integers make
  the tests exact.
- **Slot count uses floor division.** Release firmware v2.7 uses
  `floor(span / bandwidth)`. The `develop` branch now uses `round()` and adds
  spacing and padding per region profile. For EU_433 and EU_868 with
  standard presets the span divides exactly, so both give the same answer.
  Recheck this if a region with an uneven span (for example ANZ_433) is
  added.
- **Only EU_433 and EU_868 regions.** EU_433 is the target. EU_868 keeps the
  upstream behaviour for the 868 MHz board and anchors a regression test.
  Other regions are one line each and can be added from the Meshtastic
  `RDEF` table when needed.
- **Preset names are the Meshtastic display names.** These are the strings
  the firmware hashes. LongModerate is `"LongMod"` in the firmware, not
  `"LongModerate"`. Using the long name would put the radio on the wrong
  slot.
- **LongFast is the default, not MediumFast.** LongFast is the Meshtastic
  default preset for EU_433, so a fresh 433 MHz node uses it. It lands on
  433.875 MHz (433.750-434.000 MHz occupied), inside the EU SRD band
  433.05-434.79 MHz. MediumFast lands on 433.125 MHz, and its 250 kHz
  bandwidth reaches down to 433.000 MHz, below the band edge.
- **Region and preset live in `keys.h`.** The public channel name must equal
  the preset name for the default channel. Keeping both in one file means
  one edit changes both.
- **Tests keep the MediumFast public channel.** Six tests use packets
  recorded on MediumFast, and the protocol code reads `Config::PublicChannel`
  directly. Changing those functions to take the channel as a parameter is a
  larger refactor than this fork needs. `keys.h` already keeps a separate
  test identity for `Device` under `#ifdef TESTS`, so `PublicChannel` follows
  the same pattern.
- **TX power defaults to the region limit (10 dBm on EU_433).** The `LoRa`
  library defaults to 17 dBm, which is above the EU 433 MHz limit of 10 mW
  ERP. The same cap applies to `/pwr17`.
- **No duty cycle enforcement.** EU_433 allows 10% airtime. The firmware
  only sends when a person types a command or a `/dht` request arrives,
  so this fork documents the limit instead of tracking airtime.
