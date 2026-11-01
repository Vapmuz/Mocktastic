/*
 * Amalgamation translation unit for the vendored nanopb sources.
 *
 * build_opt.h adds nanopb_meshtastic/src to the include path, so the nanopb
 * *headers* resolve and meshtastic.cpp compiles. But the Arduino build only
 * *compiles* sources that live in the sketch root or its src/ subfolder, so
 * the .c files under nanopb_meshtastic/src are never built on their own and
 * their symbols (pb_encode, pb_decode, meshtastic_Data_msg, ...) end up
 * undefined at link time.
 *
 * This file lives in the sketch root, so Arduino compiles it; pulling the
 * needed nanopb .c files in here gives them a translation unit and gets them
 * linked. The set below mirrors NANOPB_OBJS in the Makefile: mesh.pb.c drags
 * in the other message descriptors at link time even though we only use Data.
 *
 * Note: we deliberately include only this subset, NOT the whole tree. Some
 * unused generated headers (e.g. deviceonly.pb.h) pull in C++ headers like
 * <vector>, which cannot be compiled as C -- so amalgamating everything, or
 * moving the tree into the sketch src/ folder, breaks the build.
 */
#include "nanopb_meshtastic/src/pb_common.c"
#include "nanopb_meshtastic/src/pb_encode.c"
#include "nanopb_meshtastic/src/pb_decode.c"
#include "nanopb_meshtastic/src/meshtastic/mesh.pb.c"
#include "nanopb_meshtastic/src/meshtastic/channel.pb.c"
#include "nanopb_meshtastic/src/meshtastic/config.pb.c"
#include "nanopb_meshtastic/src/meshtastic/device_ui.pb.c"
#include "nanopb_meshtastic/src/meshtastic/module_config.pb.c"
#include "nanopb_meshtastic/src/meshtastic/remote_hardware.pb.c"
#include "nanopb_meshtastic/src/meshtastic/xmodem.pb.c"
#include "nanopb_meshtastic/src/meshtastic/telemetry.pb.c"
