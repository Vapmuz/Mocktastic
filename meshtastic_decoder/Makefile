CXX := g++
CC := gcc
CXXFLAGS := -std=c++20 -Wall -Wextra -pedantic

TEST_SRC := test/functions_test.cpp
TARGET := a.out

# Minimal mbedtls (used only by the test stub, never by the .ino sketch)
# Everest (3rdparty/everest) e' volutamente escluso: vedi config_minimal.h.
MBEDTLS_DIR := mbedtls
MBEDTLS_CFLAGS := -I$(MBEDTLS_DIR)/include -I$(MBEDTLS_DIR)/library -I$(MBEDTLS_DIR) \
	-DMBEDTLS_CONFIG_FILE='"config_minimal.h"'
MBEDTLS_OBJS := mbedtls_sha256.o mbedtls_platform_util.o mbedtls_aes.o \
	mbedtls_ccm.o mbedtls_block_cipher.o mbedtls_constant_time.o \
	mbedtls_ecdh.o mbedtls_ecp.o mbedtls_ecp_curves.o mbedtls_bignum.o mbedtls_bignum_core.o

# Minimal nanopb (used only by the test stub, never by the .ino sketch).
# mesh.pb.c contains meshtastic_User but is one translation unit together
# with FromRadio/ToRadio/NodeInfo, whose submessage descriptors pull in
# these other generated files at link time even though the tests never
# touch those message types directly.
NANOPB_DIR := nanopb_meshtastic/src
NANOPB_CFLAGS := -I$(NANOPB_DIR)
NANOPB_OBJS := nanopb_pb_common.o nanopb_pb_decode.o nanopb_pb_encode.o \
	nanopb_mesh.o nanopb_channel.o nanopb_config.o nanopb_device_ui.o \
	nanopb_module_config.o nanopb_remote_hardware.o nanopb_xmodem.o nanopb_telemetry.o

# Documentation
DOXYGEN := doxygen
DOXYFILE := Doxyfile
DOC_DIR := docs

.PHONY: all clean test build-test doxygen doxygen-clean

all: $(TARGET)

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) main.cpp -o $(TARGET)

test: build-test
	./$(TARGET)

build-test: $(MBEDTLS_OBJS) $(NANOPB_OBJS)
	$(CXX) $(CXXFLAGS) -g -DTESTS $(MBEDTLS_CFLAGS) $(NANOPB_CFLAGS) $(TEST_SRC) $(MBEDTLS_OBJS) $(NANOPB_OBJS) -o $(TARGET)

mbedtls_sha256.o: $(MBEDTLS_DIR)/library/sha256.c
	$(CC) -std=c11 $(MBEDTLS_CFLAGS) -c $< -o $@

mbedtls_platform_util.o: $(MBEDTLS_DIR)/library/platform_util.c
	$(CC) -std=c11 $(MBEDTLS_CFLAGS) -c $< -o $@

mbedtls_aes.o: $(MBEDTLS_DIR)/library/aes.c
	$(CC) -std=c11 $(MBEDTLS_CFLAGS) -c $< -o $@

mbedtls_ccm.o: $(MBEDTLS_DIR)/library/ccm.c
	$(CC) -std=c11 $(MBEDTLS_CFLAGS) -c $< -o $@

mbedtls_block_cipher.o: $(MBEDTLS_DIR)/library/block_cipher.c
	$(CC) -std=c11 $(MBEDTLS_CFLAGS) -c $< -o $@

mbedtls_constant_time.o: $(MBEDTLS_DIR)/library/constant_time.c
	$(CC) -std=c11 $(MBEDTLS_CFLAGS) -c $< -o $@

mbedtls_ecdh.o: $(MBEDTLS_DIR)/library/ecdh.c
	$(CC) -std=c11 $(MBEDTLS_CFLAGS) -c $< -o $@

mbedtls_ecp.o: $(MBEDTLS_DIR)/library/ecp.c
	$(CC) -std=c11 $(MBEDTLS_CFLAGS) -c $< -o $@

mbedtls_ecp_curves.o: $(MBEDTLS_DIR)/library/ecp_curves.c
	$(CC) -std=c11 $(MBEDTLS_CFLAGS) -c $< -o $@

mbedtls_bignum.o: $(MBEDTLS_DIR)/library/bignum.c
	$(CC) -std=c11 $(MBEDTLS_CFLAGS) -c $< -o $@

mbedtls_bignum_core.o: $(MBEDTLS_DIR)/library/bignum_core.c
	$(CC) -std=c11 $(MBEDTLS_CFLAGS) -c $< -o $@

nanopb_pb_common.o: $(NANOPB_DIR)/pb_common.c
	$(CC) -std=c11 $(NANOPB_CFLAGS) -c $< -o $@

nanopb_pb_decode.o: $(NANOPB_DIR)/pb_decode.c
	$(CC) -std=c11 $(NANOPB_CFLAGS) -c $< -o $@

nanopb_pb_encode.o: $(NANOPB_DIR)/pb_encode.c
	$(CC) -std=c11 $(NANOPB_CFLAGS) -c $< -o $@

nanopb_mesh.o: $(NANOPB_DIR)/meshtastic/mesh.pb.c
	$(CC) -std=c11 $(NANOPB_CFLAGS) -c $< -o $@

nanopb_channel.o: $(NANOPB_DIR)/meshtastic/channel.pb.c
	$(CC) -std=c11 $(NANOPB_CFLAGS) -c $< -o $@

nanopb_config.o: $(NANOPB_DIR)/meshtastic/config.pb.c
	$(CC) -std=c11 $(NANOPB_CFLAGS) -c $< -o $@

nanopb_device_ui.o: $(NANOPB_DIR)/meshtastic/device_ui.pb.c
	$(CC) -std=c11 $(NANOPB_CFLAGS) -c $< -o $@

nanopb_module_config.o: $(NANOPB_DIR)/meshtastic/module_config.pb.c
	$(CC) -std=c11 $(NANOPB_CFLAGS) -c $< -o $@

nanopb_remote_hardware.o: $(NANOPB_DIR)/meshtastic/remote_hardware.pb.c
	$(CC) -std=c11 $(NANOPB_CFLAGS) -c $< -o $@

nanopb_xmodem.o: $(NANOPB_DIR)/meshtastic/xmodem.pb.c
	$(CC) -std=c11 $(NANOPB_CFLAGS) -c $< -o $@

nanopb_telemetry.o: $(NANOPB_DIR)/meshtastic/telemetry.pb.c
	$(CC) -std=c11 $(NANOPB_CFLAGS) -c $< -o $@

# Documents the sketch sources only (*.cpp/*.h/*.ino at this level);
# test/, mbedtls/ and nanopb_meshtastic/ are excluded in the Doxyfile.
doxygen:
	@command -v $(DOXYGEN) >/dev/null 2>&1 || \
		{ echo "doxygen not found: brew install doxygen"; exit 1; }
	@command -v dot >/dev/null 2>&1 || \
		{ echo "graphviz not found (HAVE_DOT=YES needs it): brew install graphviz"; exit 1; }
	$(DOXYGEN) $(DOXYFILE)
	@echo "Docs generated in $(DOC_DIR)/html/index.html"

doxygen-clean:
	rm -rf $(DOC_DIR)

clean: doxygen-clean
	rm -f $(TARGET) *.o *.out
	rm -rf a.out.dSYM/

