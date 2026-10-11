// license:BSD-3-Clause
// copyright-holders:Richard Lucente, Antigravity
/***************************************************************************

    Wildbits Jr2 (FNX6809 Core) Emulation

    The Wildbits Jr2 (formerly Foenix F256 Jr2) is an FPGA-based retro
    system powered by an Artix-7 FPGA running the FNX6809 soft core,
    TinyVicky II graphics, 512KB SRAM, 512KB Flash ROM, and integrated
    peripherals. Aligned with hardware baseline wildbits_jr2_6809_v8_rc12.

****************************************************************************/

#include "emu.h"

#include "cpu/m6809/m6809.h"
#include "machine/spi_sdcard.h"
#include "screen.h"
#include "sound/ymopl.h"
#include "sound/sn76496.h"
#include "sound/mos6581.h"
#include "speaker.h"
#include "osdepend.h"
#include <queue>
#include <deque>
#include <vector>
#include <cmath>
#include <cstring>
#include <limits>

namespace {

#define MINIMP3_IMPLEMENTATION
#define MAX_FRAME_SYNC_MATCHES 2
#include "minimp3/minimp3.h"

static const uint8_t s_os9_bannerfont[2048] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
    0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xff, 0x07, 0x1f, 0x3f, 0x7f, 0x7f, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0x7f, 0x7f, 0x3f, 0x1f, 0x07, 0xe0, 0xf8, 0xfc, 0xfe, 0xfe, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xfe, 0xfe, 0xfc, 0xf8, 0xe0, 0x0f, 0x1f, 0x3f, 0x7f, 0x7f, 0x3f, 0x1f, 0x0f,
    0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01,
    0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff, 0xff, 0xff, 0xfe, 0xfc, 0xfc, 0xfe, 0xff, 0xff,
    0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xf0, 0xf0, 0xf0, 0x0f, 0x0f, 0x0f, 0x0f, 0x1f, 0x3f, 0x7f, 0xff,
    0xf0, 0xf0, 0xf0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff, 0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x0f, 0x0f, 0x0f,
    0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x18, 0x7e, 0x18, 0x18, 0x7e, 0x18, 0x00, 0xff,
    0x18, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7e, 0x18, 0x00,
    0x00, 0x04, 0x06, 0xff, 0x06, 0x04, 0x00, 0x00, 0x00, 0x20, 0x60, 0xff, 0x60, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xc0, 0xc0, 0xc0, 0xff, 0x00, 0x00, 0x24, 0x66, 0xff, 0x66, 0x24, 0x00, 0x00,
    0x00, 0x00, 0x10, 0x38, 0x7c, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x7c, 0x38, 0x10, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x30, 0x30, 0x30, 0x00, 0x30, 0x00,
    0x66, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6c, 0x6c, 0xfe, 0x6c, 0xfe, 0x6c, 0x6c, 0x00,
    0x10, 0x7c, 0xd2, 0x7c, 0x86, 0x7c, 0x10, 0x00, 0xf0, 0x96, 0xfc, 0x18, 0x3e, 0x72, 0xde, 0x00,
    0x30, 0x48, 0x30, 0x78, 0xce, 0xcc, 0x78, 0x00, 0x0c, 0x0c, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x10, 0x60, 0xc0, 0xc0, 0xc0, 0x60, 0x10, 0x00, 0x10, 0x0c, 0x06, 0x06, 0x06, 0x0c, 0x10, 0x00,
    0x00, 0x54, 0x38, 0xfe, 0x38, 0x54, 0x00, 0x00, 0x00, 0x18, 0x18, 0x7e, 0x18, 0x18, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x70, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x02, 0x06, 0x0c, 0x18, 0x30, 0x60, 0xc0, 0x00,
    0x7c, 0xce, 0xde, 0xf6, 0xe6, 0xe6, 0x7c, 0x00, 0x18, 0x38, 0x78, 0x18, 0x18, 0x18, 0x3c, 0x00,
    0x7c, 0xc6, 0x06, 0x0c, 0x30, 0x60, 0xfe, 0x00, 0x7c, 0xc6, 0x06, 0x3c, 0x06, 0xc6, 0x7c, 0x00,
    0x0e, 0x1e, 0x36, 0x66, 0xfe, 0x06, 0x06, 0x00, 0xfe, 0xc0, 0xc0, 0xfc, 0x06, 0x06, 0xfc, 0x00,
    0x7c, 0xc6, 0xc0, 0xfc, 0xc6, 0xc6, 0x7c, 0x00, 0xfe, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x60, 0x00,
    0x7c, 0xc6, 0xc6, 0x7c, 0xc6, 0xc6, 0x7c, 0x00, 0x7c, 0xc6, 0xc6, 0x7e, 0x06, 0xc6, 0x7c, 0x00,
    0x00, 0x30, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x30, 0x20, 0x00,
    0x00, 0x1c, 0x30, 0x60, 0x30, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x7e, 0x00, 0x00, 0x00,
    0x00, 0x70, 0x18, 0x0c, 0x18, 0x70, 0x00, 0x00, 0x7c, 0xc6, 0x0c, 0x18, 0x30, 0x00, 0x30, 0x00,
    0x7c, 0x82, 0x9a, 0xaa, 0xaa, 0x9e, 0x7c, 0x00, 0x7c, 0xc6, 0xc6, 0xfe, 0xc6, 0xc6, 0xc6, 0x00,
    0xfc, 0x66, 0x66, 0x7c, 0x66, 0x66, 0xfc, 0x00, 0x7c, 0xc6, 0xc0, 0xc0, 0xc0, 0xc6, 0x7c, 0x00,
    0xfc, 0x66, 0x66, 0x66, 0x66, 0x66, 0xfc, 0x00, 0xfe, 0x62, 0x68, 0x78, 0x68, 0x62, 0xfe, 0x00,
    0xfe, 0x62, 0x68, 0x78, 0x68, 0x60, 0xf0, 0x00, 0x7c, 0xc6, 0xc6, 0xc0, 0xde, 0xc6, 0x7c, 0x00,
    0xc6, 0xc6, 0xc6, 0xfe, 0xc6, 0xc6, 0xc6, 0x00, 0x3c, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00,
    0x1e, 0x0c, 0x0c, 0x0c, 0x0c, 0xcc, 0x78, 0x00, 0xc6, 0xcc, 0xd8, 0xf0, 0xd8, 0xcc, 0xc6, 0x00,
    0xf0, 0x60, 0x60, 0x60, 0x60, 0x62, 0xfe, 0x00, 0xc6, 0xee, 0xfe, 0xd6, 0xc6, 0xc6, 0xc6, 0x00,
    0xc6, 0xe6, 0xf6, 0xde, 0xce, 0xc6, 0xc6, 0x00, 0x7c, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x7c, 0x00,
    0xfc, 0x66, 0x66, 0x7c, 0x60, 0x60, 0xf0, 0x00, 0x7c, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x7c, 0x0c,
    0xfc, 0x66, 0x66, 0x7c, 0x66, 0x66, 0xe6, 0x00, 0x7c, 0xc6, 0xc0, 0x7c, 0x06, 0xc6, 0x7c, 0x00,
    0x7e, 0x5a, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x7c, 0x00,
    0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x6c, 0x38, 0x00, 0xc6, 0xc6, 0xc6, 0xc6, 0xd6, 0xee, 0xc6, 0x00,
    0xc6, 0x6c, 0x38, 0x38, 0x38, 0x6c, 0xc6, 0x00, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x18, 0x3c, 0x00,
    0xfe, 0xc6, 0x0c, 0x18, 0x30, 0x66, 0xfe, 0x00, 0x1c, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1c, 0x00,
    0xc0, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x02, 0x00, 0x70, 0x30, 0x30, 0x30, 0x30, 0x30, 0x70, 0x00,
    0x00, 0x00, 0x10, 0x38, 0x6c, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
    0x30, 0x30, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x06, 0x7e, 0xc6, 0x7e, 0x00,
    0xc0, 0xc0, 0xfc, 0xc6, 0xc6, 0xc6, 0xfc, 0x00, 0x00, 0x00, 0x7c, 0xc6, 0xc0, 0xc6, 0x7c, 0x00,
    0x06, 0x06, 0x7e, 0xc6, 0xc6, 0xc6, 0x7e, 0x00, 0x00, 0x00, 0x7c, 0xc6, 0xfe, 0xc0, 0x7c, 0x00,
    0x3c, 0x66, 0x60, 0xf0, 0x60, 0x60, 0x60, 0x00, 0x00, 0x00, 0x7e, 0xc6, 0xc6, 0x7e, 0x06, 0x7c,
    0xc0, 0xc0, 0xfc, 0xc6, 0xc6, 0xc6, 0xc6, 0x00, 0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x3c, 0x00,
    0x00, 0x0c, 0x00, 0x1c, 0x0c, 0x0c, 0xcc, 0x78, 0xc0, 0xc0, 0xc6, 0xd8, 0xf0, 0xd8, 0xc6, 0x00,
    0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00, 0x00, 0x00, 0xee, 0xfe, 0xd6, 0xc6, 0xc6, 0x00,
    0x00, 0x00, 0xfc, 0xc6, 0xc6, 0xc6, 0xc6, 0x00, 0x00, 0x00, 0x7c, 0xc6, 0xc6, 0xc6, 0x7c, 0x00,
    0x00, 0x00, 0xfc, 0xc6, 0xc6, 0xfc, 0xc0, 0xc0, 0x00, 0x00, 0x7e, 0xc6, 0xc6, 0x7e, 0x06, 0x06,
    0x00, 0x00, 0xde, 0x76, 0x60, 0x60, 0x60, 0x00, 0x00, 0x00, 0x7c, 0xc0, 0x7c, 0x06, 0x7c, 0x00,
    0x18, 0x18, 0x7e, 0x18, 0x18, 0x18, 0x1e, 0x00, 0x00, 0x00, 0xc6, 0xc6, 0xc6, 0xc6, 0x7e, 0x00,
    0x00, 0x00, 0xc6, 0xc6, 0xc6, 0x6c, 0x38, 0x00, 0x00, 0x00, 0xc6, 0xc6, 0xd6, 0xfe, 0xc6, 0x00,
    0x00, 0x00, 0xc6, 0x6c, 0x38, 0x6c, 0xc6, 0x00, 0x00, 0x00, 0xc6, 0xc6, 0xc6, 0x7e, 0x06, 0x7c,
    0x00, 0x00, 0xfe, 0x0c, 0x18, 0x60, 0xfe, 0x00, 0x0e, 0x18, 0x18, 0x70, 0x18, 0x18, 0x0e, 0x00,
    0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00, 0xe0, 0x30, 0x30, 0x1c, 0x30, 0x30, 0xe0, 0x00,
    0x00, 0x00, 0x70, 0x9a, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x3c, 0x66, 0xff, 0x00, 0x00,
    0x7c, 0xc6, 0xc0, 0xc0, 0xc6, 0x7c, 0x18, 0x70, 0x66, 0x00, 0xc6, 0xc6, 0xc6, 0xc6, 0x7e, 0x00,
    0x0e, 0x18, 0x7c, 0xc6, 0xfe, 0xc0, 0x7c, 0x00, 0x18, 0x24, 0x7c, 0x06, 0x7e, 0xc6, 0x7e, 0x00,
    0x66, 0x00, 0x7c, 0x06, 0x7e, 0xc6, 0x7e, 0x00, 0x38, 0x0c, 0x7c, 0x06, 0x7e, 0xc6, 0x7e, 0x00,
    0x18, 0x00, 0x7c, 0x06, 0x7e, 0xc6, 0x7e, 0x00, 0x00, 0x00, 0x7c, 0xc0, 0xc0, 0x7c, 0x18, 0x70,
    0x18, 0x24, 0x7c, 0xc6, 0xfe, 0xc0, 0x7c, 0x00, 0x66, 0x00, 0x7c, 0xc6, 0xfe, 0xc0, 0x7c, 0x00,
    0x70, 0x18, 0x7c, 0xc6, 0xfe, 0xc0, 0x7c, 0x00, 0x66, 0x00, 0x38, 0x18, 0x18, 0x18, 0x3c, 0x00,
    0x18, 0x24, 0x38, 0x18, 0x18, 0x18, 0x3c, 0x00, 0x38, 0x0c, 0x38, 0x18, 0x18, 0x18, 0x3c, 0x00,
    0x66, 0x00, 0x7c, 0xc6, 0xfe, 0xc6, 0xc6, 0x00, 0x18, 0x00, 0x7c, 0xc6, 0xfe, 0xc6, 0xc6, 0x00,
    0x0e, 0x18, 0xfe, 0x60, 0x78, 0x60, 0xfe, 0x00, 0x00, 0x00, 0x7c, 0x1a, 0x7e, 0xd8, 0x7e, 0x00,
    0x7e, 0xd8, 0xd8, 0xde, 0xf8, 0xd8, 0xde, 0x00, 0x18, 0x24, 0x7c, 0xc6, 0xc6, 0xc6, 0x7c, 0x00,
    0x66, 0x00, 0x7c, 0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x38, 0x0c, 0x7c, 0xc6, 0xc6, 0xc6, 0x7c, 0x00,
    0x18, 0x24, 0xc6, 0xc6, 0xc6, 0xc6, 0x7e, 0x00, 0x38, 0x0c, 0xc6, 0xc6, 0xc6, 0xc6, 0x7e, 0x00,
    0x66, 0x00, 0xc6, 0xc6, 0xc6, 0x7e, 0x06, 0x7c, 0x66, 0x7c, 0xc6, 0xc6, 0xc6, 0xc6, 0x7c, 0x00,
    0xc6, 0x00, 0xc6, 0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x18, 0x7c, 0xc6, 0xc0, 0xc6, 0x7c, 0x18, 0x00,
    0x1e, 0x32, 0x30, 0x78, 0x30, 0x70, 0xfe, 0x00, 0x66, 0x3c, 0x18, 0x7e, 0x18, 0x3c, 0x18, 0x00,
    0xfc, 0xc6, 0xfc, 0xc0, 0xcc, 0xde, 0xcc, 0x0e, 0x00, 0x1c, 0x32, 0x30, 0xfc, 0x30, 0xf0, 0x00,
    0x0e, 0x18, 0x7c, 0x06, 0x7e, 0xc6, 0x7e, 0x00, 0x1a, 0x30, 0x38, 0x18, 0x18, 0x18, 0x3c, 0x00,
    0x0e, 0x18, 0x7c, 0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x0e, 0x18, 0xc6, 0xc6, 0xc6, 0xc6, 0x7e, 0x00,
    0x66, 0x98, 0xfc, 0xc6, 0xc6, 0xc6, 0xc6, 0x00, 0x66, 0x98, 0xe6, 0xf6, 0xde, 0xce, 0xc6, 0x00,
    0x7c, 0x06, 0x7e, 0xc6, 0x7e, 0x00, 0xfe, 0x00, 0x7c, 0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0xfe, 0x00,
    0x18, 0x00, 0x18, 0x30, 0x60, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0xfe, 0xc0, 0xc0, 0xc0, 0xc0, 0x00,
    0x00, 0x00, 0xfe, 0x06, 0x06, 0x06, 0x06, 0x00, 0xc0, 0xc0, 0xc0, 0xde, 0x06, 0x0c, 0x1e, 0x00,
    0xc0, 0xc0, 0xc0, 0xcc, 0x1c, 0x3e, 0x0c, 0x00, 0x30, 0x00, 0x30, 0x30, 0x30, 0x30, 0x30, 0x00,
    0x00, 0x36, 0x6c, 0xd8, 0x6c, 0x36, 0x00, 0x00, 0x00, 0xd8, 0x6c, 0x36, 0x6c, 0xd8, 0x00, 0x00,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55,
    0x44, 0x22, 0x44, 0x22, 0x44, 0x22, 0x44, 0x22, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
    0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xff, 0x07, 0x1f, 0x3f, 0x7f, 0x7f, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0x7f, 0x7f, 0x3f, 0x1f, 0x07, 0xe0, 0xf8, 0xfc, 0xfe, 0xfe, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xfe, 0xfe, 0xfc, 0xf8, 0xe0, 0x0f, 0x1f, 0x3f, 0x7f, 0x7f, 0x3f, 0x1f, 0x0f,
    0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01,
    0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff, 0xff, 0xff, 0xfe, 0xfc, 0xfc, 0xfe, 0xff, 0xff,
    0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xf0, 0xf0, 0xf0, 0x0f, 0x0f, 0x0f, 0x0f, 0x1f, 0x3f, 0x7f, 0xff,
    0xf0, 0xf0, 0xf0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff, 0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x0f, 0x0f, 0x0f,
    0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff, 0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80,
    0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x18, 0xff, 0x18, 0x18, 0x18, 0x18,
    0x18, 0x18, 0x18, 0x1f, 0x18, 0x1f, 0x18, 0x18, 0x36, 0x36, 0x36, 0x37, 0x36, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x37, 0x30, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x30, 0x37, 0x36, 0x36,
    0x36, 0x36, 0x36, 0xf7, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0xf7, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x37, 0x30, 0x37, 0x36, 0x36, 0x00, 0x00, 0x00, 0xff, 0x00, 0xff, 0x00, 0x00,
    0x36, 0x36, 0x36, 0xf7, 0x00, 0xf7, 0x36, 0x36, 0x18, 0x18, 0x18, 0xff, 0x00, 0xff, 0x00, 0x00,
    0x36, 0x36, 0x36, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0xff, 0x18, 0x18,
    0x00, 0x00, 0x00, 0xff, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x3f, 0x00, 0x00, 0x00, 0x00,
    0x18, 0x18, 0x18, 0x1f, 0x18, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x18, 0x1f, 0x18, 0x18,
    0x00, 0x00, 0x00, 0x3f, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0xff, 0x36, 0x36, 0x36, 0x36,
    0x18, 0x18, 0x18, 0xff, 0x18, 0xff, 0x18, 0x18, 0x18, 0x18, 0x18, 0xf8, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x1f, 0x18, 0x18, 0x18, 0x18, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0,
    0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x77, 0x98, 0x98, 0x77, 0x00, 0x00, 0x1c, 0x36, 0x66, 0xfc, 0xc6, 0xc6, 0xfc, 0xc0,
    0xfe, 0x62, 0x60, 0x60, 0x60, 0x60, 0x60, 0x00, 0x00, 0x00, 0xff, 0x66, 0x66, 0x66, 0x66, 0x00,
    0xfe, 0x62, 0x30, 0x18, 0x30, 0x62, 0xfe, 0x00, 0x00, 0x00, 0x3f, 0x66, 0xc6, 0xcc, 0x78, 0x00,
    0x00, 0x00, 0x33, 0x33, 0x33, 0x3e, 0x30, 0xf0, 0x00, 0x00, 0xff, 0x18, 0x18, 0x18, 0x18, 0x00,
    0x3c, 0x18, 0x3c, 0x66, 0x66, 0x3c, 0x18, 0x3c, 0x00, 0x7c, 0xc6, 0xfe, 0xc6, 0x7c, 0x00, 0x00,
    0x00, 0x7e, 0xc3, 0xc3, 0xc3, 0x66, 0xe7, 0x00, 0x1e, 0x19, 0x3c, 0x66, 0xc6, 0xcc, 0x78, 0x00,
    0x00, 0x00, 0x66, 0x99, 0x99, 0x66, 0x00, 0x00, 0x00, 0x03, 0x7c, 0xce, 0xe6, 0x7c, 0xc0, 0x00,
    0x00, 0x3e, 0xc0, 0xfe, 0xc0, 0x3e, 0x00, 0x00, 0x00, 0x7e, 0xc3, 0xc3, 0xc3, 0xc3, 0x00, 0x00,
    0x00, 0xfe, 0x00, 0xfe, 0x00, 0xfe, 0x00, 0x00, 0x18, 0x18, 0x7e, 0x18, 0x18, 0x7e, 0x00, 0x00,
    0x70, 0x18, 0x0c, 0x18, 0x70, 0x00, 0xfe, 0x00, 0x1c, 0x30, 0x60, 0x30, 0x1c, 0x00, 0xfe, 0x00,
    0x00, 0x0e, 0x1b, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0xd8, 0x70, 0x00,
    0x00, 0x18, 0x00, 0x7e, 0x00, 0x18, 0x00, 0x00, 0x00, 0x76, 0xdc, 0x00, 0x76, 0xdc, 0x00, 0x00,
    0x3c, 0x66, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x3c, 0x18, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x0f, 0x0c, 0x0c, 0x0c, 0xec, 0x6c, 0x38, 0x00,
    0xd8, 0xec, 0xcc, 0xcc, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x30, 0xc0, 0xf0, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x3c, 0x3c, 0x3c, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t s_os9_palette[64] = {
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00,
	0x00, 0x00, 0x88, 0x00, 0xee, 0xff, 0xaa, 0x00,
	0xcc, 0x4c, 0xcc, 0x00, 0x55, 0xcc, 0x00, 0x00,
	0xaa, 0x00, 0x00, 0x00, 0x77, 0xdd, 0xdd, 0x00,
	0x55, 0x88, 0xdd, 0x00, 0x00, 0x44, 0x66, 0x00,
	0x77, 0x77, 0xff, 0x00, 0x33, 0x33, 0x33, 0x00,
	0x77, 0x77, 0x77, 0x00, 0x66, 0xff, 0xaa, 0x00,
	0xff, 0x88, 0x00, 0x00, 0xbb, 0xbb, 0xbb, 0x00,
};

class wildbits_jr2_state : public driver_device, public device_sound_interface
{
public:
	wildbits_jr2_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, device_sound_interface(mconfig, *this)
		, m_maincpu(*this, "maincpu")
		, m_opl3(*this, "opl3")
		, m_psg_l(*this, "psg_l")
		, m_psg_r(*this, "psg_r")
		, m_sid_l(*this, "sid_l")
		, m_sid_r(*this, "sid_r")
		, m_sdcard(*this, "sdcard")
		, m_screen(*this, "screen")
		, m_flash(*this, "flash")
		, m_bank_r(*this, "bank_r%u", 0U)
		, m_io_key(*this, "KEY%u", 0U)
		, m_dipsw(*this, "DIPSW")
		, m_mouse_x_axis(*this, "MOUSEX")
		, m_mouse_y_axis(*this, "MOUSEY")
		, m_mouse_btn(*this, "MOUSE_BUTTONS")
		, m_last_mouse_x(0)
		, m_last_mouse_y(0)
		, m_last_mouse_btn(0)
		, m_frame_count(0)
	{ }

	void wbjr2(machine_config &config) ATTR_COLD;

protected:
	virtual void machine_start() override ATTR_COLD;
	virtual void machine_reset() override ATTR_COLD;
	virtual void device_stop() override ATTR_COLD;
	virtual void sound_stream_update(sound_stream &stream) override;

private:
	void wbjr2_mem(address_map &map) ATTR_COLD;
	uint32_t screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	void poll_keyboard();
	void poll_mouse();

	// MMU handlers
	uint8_t mmu_mem_ctrl_r();
	void mmu_mem_ctrl_w(uint8_t data);
	uint8_t mmu_io_ctrl_r();
	void mmu_io_ctrl_w(uint8_t data);
	uint8_t mmu_slot_r(offs_t offset);
	void mmu_slot_w(offs_t offset, uint8_t data);

	// System Control handlers
	uint8_t sys0_r();
	void sys0_w(uint8_t data);
	uint8_t sys1_r();
	void sys1_w(uint8_t data);
	void rst0_w(uint8_t data);
	void rst1_w(uint8_t data);
	uint8_t mid_r();
	uint8_t pcbid_r(offs_t offset);

	// LFSR Random Number Generator ($FE04 - $FE06)
	void lfsr_update();
	uint8_t lfsr_r(offs_t offset);
	void lfsr_w(offs_t offset, uint8_t data);

	// Optical Keyboard Decode ($FE10 - $FE1F)
	uint8_t optkbd_r(offs_t offset);

	// Consolidated Fixed I/O Sound Registers ($FF91 - $FF99)
	uint8_t sound_fixed_r(offs_t offset);
	void sound_fixed_w(offs_t offset, uint8_t data);

	// Hardware Mouse Cursor ($FEA0 - $FEA8)
	uint8_t mouse_r(offs_t offset);
	void mouse_w(offs_t offset, uint8_t data);

	// SAM2695 MIDI Synth ($FF30 - $FF39)
	uint8_t sam2695_r(offs_t offset);
	void sam2695_w(offs_t offset, uint8_t data);

	// VS1053b Audio Decoder ($FF50 - $FF5F)
	uint8_t vs1053_r(offs_t offset);
	void vs1053_w(offs_t offset, uint8_t data);
	TIMER_CALLBACK_MEMBER(vs_midi_tick);
	void vs_stop_smf();
	void vs_stop_audio();
	void vs_decode_audio();

	// Real-Time Clock ($FE40 - $FE4F)
	uint8_t rtc_r(offs_t offset);
	void rtc_w(offs_t offset, uint8_t data);

	// Interrupt Controller ($FE20 - $FE2F)
	uint8_t intc_r(offs_t offset);
	void intc_w(offs_t offset, uint8_t data);
	void check_irqs();
	void set_irq(int group, uint8_t mask);

	// System Timers ($FE30 - $FE3F)
	uint8_t timer_r(offs_t offset);
	void timer_w(offs_t offset, uint8_t data);
	TIMER_CALLBACK_MEMBER(timer0_tick);
	TIMER_CALLBACK_MEMBER(timer1_tick);

	// PS/2 Keyboard & Mouse ($FE50 - $FE54)
	uint8_t ps2_r(offs_t offset);
	void ps2_w(offs_t offset, uint8_t data);
	void queue_kbd_scancode(uint8_t scancode);

	// 16550 UART ($FE60 - $FE67)
	uint8_t uart_r(offs_t offset);
	void uart_w(offs_t offset, uint8_t data);
	void poll_uart_socket();

	// Audio CODEC ($FE70 - $FE72: WM8776)
	uint8_t codec_r(offs_t offset);
	void codec_w(offs_t offset, uint8_t data);

	// SPI SD Card ($FE90 - $FE91)
	uint8_t sdc_stat_r();
	void sdc_stat_w(uint8_t data);
	uint8_t sdc_data_r();
	void sdc_data_w(uint8_t data);
	void sdcard_miso_w(int state);

	// Hardware Configuration DIP Switches ($FF90)
	uint8_t dipsw_r();

	// WizFi360 WiFi / SPI ($FF20 - $FF2F)
	uint8_t wizfi_r(offs_t offset);
	void wizfi_w(offs_t offset, uint8_t data);
	void push_wizfi_response(const std::string &resp);
	void process_wizfi_cmd(const std::string &cmd_raw);
	void handle_cipstart(const std::string &cmd);
	void handle_cipsend(const std::string &cmd);
	void poll_wizfi_socket();
	void reset_wizfi();

	// Hardware Integer Math Coprocessor ($FEE0 - $FEFB)
	uint8_t math_r(offs_t offset);
	void math_w(offs_t offset, uint8_t data);

	// Hardware Floating-Point Unit ($FFE0 - $FFEF: FP_Math_Module)
	uint8_t fpu_r(offs_t offset);
	void fpu_w(offs_t offset, uint8_t data);

	// TinyVicky Direct Memory Access (DMA) Controller ($FEC0 - $FEDF)
	uint8_t dma_r(offs_t offset);
	void dma_w(offs_t offset, uint8_t data);
	void dma_execute();
	uint8_t dma_read_byte(uint32_t phys_addr);
	void dma_write_byte(uint32_t phys_addr, uint8_t data);

	// TinyVicky Video handlers ($FFC0 - $FFDF)
	uint8_t vky_r(offs_t offset);
	void vky_w(offs_t offset, uint8_t data);
	void vblank_w(int state);
	void update_line_timer();
	TIMER_CALLBACK_MEMBER(scanline_tick);

	// Hardware Line-Draw Accelerator ($1080 - $1087 in Page $C0, $FFCA)
	void linedraw_execute();

	void io_wait_read();
	void io_wait_write();
	bool m_is_turbo;
	uint8_t m_ram_write_frac;
	uint8_t m_io_read_frac;
	uint8_t m_io_write_frac;
	uint8_t *m_slot_w_ptr[8];

	void ram_w(offs_t offset, uint8_t data);
	void slot7_ram_w(offs_t offset, uint8_t data);

	void update_banks();
	uint8_t *get_physical_block_ptr(uint8_t block_num);

	required_device<cpu_device> m_maincpu;
	required_device<ymf262_device> m_opl3;
	required_device<sn76489_device> m_psg_l;
	required_device<sn76489_device> m_psg_r;
	required_device<mos6581_device> m_sid_l;
	required_device<mos6581_device> m_sid_r;
	required_device<spi_sdcard_device> m_sdcard;
	required_device<screen_device> m_screen;
	required_region_ptr<uint8_t> m_flash;
	memory_bank_array_creator<8> m_bank_r;

	// Memory structures
	std::unique_ptr<uint8_t[]> m_ram;        // 2MB Physical SRAM (2,016 KB contiguous for Blocks $00-$FB)
	std::unique_ptr<uint8_t[]> m_cart;       // 256KB Cartridge port decode ($80 - $9F)
	std::unique_ptr<uint8_t[]> m_unmapped;   // 8KB dummy unmapped page
	std::unique_ptr<uint8_t[]> m_vram_fc;    // Block $FC: VICKY control, Gamma, LineDraw, Tilemaps, MemText regs, Sprites, Text CLUTs
	std::unique_ptr<uint8_t[]> m_vram_fd;    // Block $FD: Font RAM ($0000-$0FFF) & Graphics CLUTs ($1000-$1FFF)
	std::unique_ptr<uint8_t[]> m_vram_fe;    // Block $FE: Text Character Matrix (4,800 bytes)
	std::unique_ptr<uint8_t[]> m_vram_ff;    // Block $FF: Text Color Matrix (4,800 bytes)
	std::unique_ptr<uint8_t[]> m_memtext_clut; // 4KB MemText FG/BG CLUTs
	uint8_t m_constant_ram[256];
	uint8_t m_vector_ram[16];

	// MMU state
	uint8_t m_mlut[4][8];
	uint8_t m_mmu_mem_ctrl;
	uint8_t m_mmu_io_ctrl;

	// System registers
	uint8_t m_sys0;
	uint8_t m_sys1;
	uint8_t m_rst0;
	uint8_t m_rst1;

	// LFSR state ($FE04 - $FE06)
	uint16_t m_lfsr;
	uint16_t m_lfsr_seed;
	uint8_t m_lfsr_ctrl;
	uint64_t m_lfsr_last_cycle;

	// Sound state ($FF91 - $FF99)
	uint8_t m_sid_select;

	// Interrupt Controller
	uint8_t m_int_pending[4];
	uint8_t m_int_pol[4];
	uint8_t m_int_edge[4];
	uint8_t m_int_mask[4];

	// Timers
	emu_timer *m_timer0;
	emu_timer *m_timer1;
	uint8_t m_t0_ctr;
	uint8_t m_t0_stat;
	uint32_t m_t0_val;
	uint32_t m_t0_cmp;
	uint8_t m_t0_cmp_ctr;

	uint8_t m_t1_ctr;
	uint8_t m_t1_stat;
	uint32_t m_t1_val;
	uint32_t m_t1_cmp;
	uint8_t m_t1_cmp_ctr;

	// PS/2 Controller
	uint8_t m_ps2_ctrl;
	uint8_t m_ps2_out;
	std::queue<uint8_t> m_kbd_fifo;
	std::queue<uint8_t> m_mouse_fifo;

	// 16550 UART state
	uint8_t m_uart_dll;
	uint8_t m_uart_dlh;
	uint8_t m_uart_ier;
	uint8_t m_uart_fcr;
	uint8_t m_uart_lcr;
	uint8_t m_uart_mcr;
	uint8_t m_uart_scr;
	std::queue<uint8_t> m_uart_rx_fifo;
	osd_file::ptr m_uart_socket;

	// RTC state
	uint8_t m_rtc_ctrl;

	// Audio CODEC state
	uint8_t m_codec_lo;
	uint8_t m_codec_hi;
	uint8_t m_codec_regs[32];
	float m_codec_vs_gain_l;
	float m_codec_vs_gain_r;
	void reset_codec();
	void update_codec();

	// Hardware Mouse Cursor state
	uint8_t m_mouse_men;
	uint16_t m_mouse_x;
	uint16_t m_mouse_y;
	uint8_t m_mouse_bytes[3];

	// SAM2695 MIDI state
	uint8_t m_sam2695_ctrl;
	std::queue<uint8_t> m_sam2695_rx_fifo;
	std::unique_ptr<osd::midi_output_port> m_midi_out;

	// VS1053b Audio Decoder state ($FF50 - $FF5F)
	uint8_t m_vs_ctrl;
	uint8_t m_vs_scireg;
	uint8_t m_vs_data_hi;
	uint8_t m_vs_data_lo;
	uint16_t m_vs_sci[16];
	uint16_t m_vs_wram_addr;
	uint16_t m_vs_wram[0x4000];
	int m_sdi_memtest_idx;
	std::vector<uint8_t> m_vs_sdi_buf;
	bool m_vs_playing_smf = false;
	bool m_vs_smf_waiting_data = false;
	int m_vs_smf_retry_count = 0;
	uint32_t m_vs_smf_pos = 0;
	uint32_t m_vs_smf_division = 480;
	uint32_t m_vs_smf_tempo_us = 500000;
	uint8_t m_vs_smf_running_status = 0;
	emu_timer *m_vs_midi_timer = nullptr;
	double m_vs_decode_seconds = 0.0;
	sound_stream *m_stream = nullptr;
	mp3dec_t m_mp3dec;
	bool m_mp3_inited = false;
	bool m_vs_decoding_audio = false;
	std::deque<int16_t> m_pcm_fifo_l;
	std::deque<int16_t> m_pcm_fifo_r;
	uint64_t m_total_samples_played = 0;
	uint32_t m_current_hz = 44100;

	enum audio_format_t
	{
		FMT_NONE = 0,
		FMT_MP3,
		FMT_WAV
	};

	audio_format_t m_audio_format = FMT_NONE;
	bool m_wav_header_parsed = false;
	uint16_t m_wav_channels = 2;
	uint16_t m_wav_bits = 16;
	uint32_t m_wav_rate = 44100;
	uint32_t m_wav_data_left = 0;

	// SPI SD Card state
	uint8_t m_sdc_stat;
	uint8_t m_sdc_data_in;
	uint8_t m_sdc_data_out;
	int m_sdcard_miso;

	// WizFi360 State
	std::queue<uint8_t> m_wizfi_rx_fifo;
	std::string m_wizfi_tx_buf;
	uint8_t m_wizfi_baud_code;
	uint8_t m_wizfi_reset_reg;
	uint8_t m_wizfi_ctrl;
	bool m_wizfi_transparent;
	int m_wizfi_plus_count;
	int m_wizfi_cipsend_remaining;
	int m_wizfi_cipsend_total;
	bool m_wizfi_cipmux;
	bool m_wizfi_cipmode;
	bool m_wizfi_echo;
	int m_wizfi_cwmode;
	bool m_wizfi_wifi_connected;
	std::string m_wizfi_ssid;
	int m_wizfi_link_id;
	std::string m_wizfi_remote_host;
	int m_wizfi_remote_port;
	osd_file::ptr m_wizfi_socket;

	// Hardware Integer Math Coprocessor ($FEE0 - $FEFB)
	uint16_t m_math_mulu_a;
	uint16_t m_math_mulu_b;
	uint16_t m_math_divu_den;
	uint16_t m_math_divu_num;
	uint32_t m_math_add_a;
	uint32_t m_math_add_b;

	// Hardware Floating-Point Unit state ($FFE0 - $FFEF: FP_Math_Module)
	uint8_t m_fpu_ctrl[4];
	uint8_t m_fpu_in0[4];
	uint8_t m_fpu_in1[4];

	// TinyVicky Direct Memory Access (DMA) Controller ($FEC0 - $FEDF)
	uint8_t m_dma_reg[24];
	uint8_t m_dma_status;

	// TinyVicky Master Registers
	uint8_t m_vky_mstr_ctrl_0;
	uint8_t m_vky_mstr_ctrl_1;
	uint8_t m_vky_layer_ctrl_0;
	uint8_t m_vky_layer_ctrl_1;
	uint8_t m_vky_brdr_ctrl;
	uint8_t m_vky_brdr_b;
	uint8_t m_vky_brdr_g;
	uint8_t m_vky_brdr_r;
	uint8_t m_vky_brdr_w;
	uint8_t m_vky_brdr_h;
	uint8_t m_vky_gfx_mode;
	uint8_t m_vky_bg_b;
	uint8_t m_vky_bg_g;
	uint8_t m_vky_bg_r;
	uint8_t m_vky_crsr_ctrl;
	uint8_t m_vky_crsr_char;
	uint8_t m_vky_crsr_color;
	uint16_t m_vky_crsr_x;
	uint16_t m_vky_crsr_y;
	uint16_t m_vky_line_cmp;
	uint8_t m_vky_lint_ctrl;
	emu_timer *m_scanline_timer;

	// TinyVicky Hardware Line-Draw Accelerator ($1080 - $1087 in Page $C0, $FFCA)
	uint8_t m_vky_drawline_ctrl;
	uint8_t m_ld_ctrl;
	uint8_t m_ld_color;
	uint16_t m_ld_x0;
	uint16_t m_ld_x1;
	uint8_t m_ld_y0;
	uint8_t m_ld_y1;
	bool m_ld_done;
	uint16_t m_ld_fifo_count;

	// Keyboard, DIP Switch & Mouse Input Ports
	required_ioport_array<4> m_io_key;
	required_ioport m_dipsw;
	required_ioport m_mouse_x_axis;
	required_ioport m_mouse_y_axis;
	required_ioport m_mouse_btn;
	uint16_t m_key_state[4];
	int16_t m_last_mouse_x;
	int16_t m_last_mouse_y;
	uint8_t m_last_mouse_btn;
	uint32_t m_frame_count;
};

// Machine ID for F256 Jr2 (6809 core) is 0x1A
constexpr uint8_t WBJR2_MACHINE_ID = 0x1a;

uint8_t *wildbits_jr2_state::get_physical_block_ptr(uint8_t block_num)
{
	// 21-bit physical space: 8KB per block (256 blocks, 2MB total space)
	// Blocks $00 - $FB: Contiguous Physical SRAM (252 blocks = 2,016 KB)
	// Blocks $40 - $7F: Flash ROM (512KB) when FLASHDIS=0; Physical SRAM when FLASHDIS=1
	// Blocks $80 - $9F: Cartridge Port (256KB) when FLASHDIS=0; Physical SRAM when FLASHDIS=1
	// Blocks $FC - $FF: Sectored I/O Pages:
	//   $FC: VICKY_BLK (registers, line draw, tilemaps, MemText regs, Sprites, Text CLUTs)
	//   $FD: FONT_BLK (dual font banks $0000-$0FFF, graphics CLUTs $1000-$1FFF)
	//   $FE: TEXT_RAM_BLK (Text Character Matrix, 4,800 bytes)
	//   $FF: COLOR_RAM_BLK (Text Color Matrix, 4,800 bytes)
	if (block_num < 0xfc)
	{
		if (m_mmu_io_ctrl & 0x04)
		{
			// Max-RAM FLASHDIS mode: 2,016 KB contiguous physical SRAM
			return &m_ram[block_num * 0x2000];
		}
		else
		{
			if (block_num >= 0x40 && block_num < 0x80)
				return &m_flash[(block_num - 0x40) * 0x2000];
			else if (block_num >= 0x80 && block_num < 0xa0)
				return &m_cart[(block_num - 0x80) * 0x2000];
			else
				return &m_ram[block_num * 0x2000];
		}
	}
	else if (block_num == 0xfc)
	{
		return m_vram_fc.get();
	}
	else if (block_num == 0xfd)
	{
		return m_vram_fd.get();
	}
	else if (block_num == 0xfe)
	{
		return m_vram_fe.get();
	}
	else // 0xff
	{
		return m_vram_ff.get();
	}
}

void wildbits_jr2_state::update_banks()
{
	uint8_t active_lut = m_mmu_mem_ctrl & 0x03;
	for (int slot = 0; slot < 8; slot++)
	{
		uint8_t block = m_mlut[active_lut][slot];
		m_bank_r[slot]->set_base(get_physical_block_ptr(block));
		if ((block >= 0x40 && block < 0x80 && !(m_mmu_io_ctrl & 0x04)) ||
		    (block >= 0x80 && block < 0xa0 && !(m_mmu_io_ctrl & 0x04)))
		{
			// Flash ROM and Cartridge ROM are read-only when FLASHDIS=0; discard CPU writes
			m_slot_w_ptr[slot] = m_unmapped.get();
		}
		else
		{
			m_slot_w_ptr[slot] = get_physical_block_ptr(block);
		}
	}
}

void wildbits_jr2_state::ram_w(offs_t offset, uint8_t data)
{
	if (m_is_turbo)
	{
		// LONGVIEW_MEMFAST Turbo Mode: Base clock = 20.0 MHz (50 ns).
		// RAM writes complete in 14 master ticks (70 ns) = 14.28 MHz.
		// Factor = 70 / 50 = 1.4 cycles (eat 2 wait cycles across 5 writes).
		m_ram_write_frac = (m_ram_write_frac + 2) % 5;
		if (m_ram_write_frac < 2)
			m_maincpu->eat_cycles(1);
	}
	else
	{
		// LONGVIEW_MEMFAST Stock Mode: Base clock = 16.67 MHz (60 ns).
		// RAM writes complete in 16 master ticks (80 ns) = 12.50 MHz.
		// Factor = 80 / 60 = 1.333 cycles (eat 1 wait cycle every 3 writes).
		m_ram_write_frac = (m_ram_write_frac + 1) % 3;
		if (m_ram_write_frac == 0)
			m_maincpu->eat_cycles(1);
	}
	m_slot_w_ptr[offset >> 13][offset & 0x1fff] = data;
}

void wildbits_jr2_state::slot7_ram_w(offs_t offset, uint8_t data)
{
	if (m_is_turbo)
	{
		m_ram_write_frac = (m_ram_write_frac + 2) % 5;
		if (m_ram_write_frac < 2)
			m_maincpu->eat_cycles(1);
	}
	else
	{
		m_ram_write_frac = (m_ram_write_frac + 1) % 3;
		if (m_ram_write_frac == 0)
			m_maincpu->eat_cycles(1);
	}
	m_slot_w_ptr[7][offset] = data;
}

void wildbits_jr2_state::io_wait_read()
{
	if (m_is_turbo)
	{
		// LONGVIEW_MEMFAST: 32 master ticks (160 ns = 6.25 MHz) on 20 MHz (50 ns) base clock:
		// 160 ns / 50 ns = 3.2 cycles (eat 2 or 3 cycles, average 2.2 wait cycles).
		m_io_read_frac = (m_io_read_frac + 1) % 5;
		m_maincpu->eat_cycles(m_io_read_frac == 0 ? 3 : 2);
	}
	else
	{
		// 32 master ticks (160 ns) on 16.67 MHz (60 ns) base clock:
		// 160 ns / 60 ns = 2.667 cycles (eat 1 or 2 cycles, average 1.67 wait cycles).
		m_io_read_frac = (m_io_read_frac + 2) % 3;
		m_maincpu->eat_cycles(m_io_read_frac == 0 ? 2 : 1);
	}
}

void wildbits_jr2_state::io_wait_write()
{
	if (m_is_turbo)
	{
		m_io_write_frac = (m_io_write_frac + 1) % 5;
		m_maincpu->eat_cycles(m_io_write_frac == 0 ? 3 : 2);
	}
	else
	{
		m_io_write_frac = (m_io_write_frac + 2) % 3;
		m_maincpu->eat_cycles(m_io_write_frac == 0 ? 2 : 1);
	}
}

uint8_t wildbits_jr2_state::mmu_mem_ctrl_r()
{
	io_wait_read();
	return m_mmu_mem_ctrl;
}

void wildbits_jr2_state::mmu_mem_ctrl_w(uint8_t data)
{
	io_wait_write();
	m_mmu_mem_ctrl = data;
	update_banks();
}

uint8_t wildbits_jr2_state::mmu_io_ctrl_r()
{
	io_wait_read();
	return (m_mmu_io_ctrl & 0x7f) | 0x80;
}

void wildbits_jr2_state::mmu_io_ctrl_w(uint8_t data)
{
	io_wait_write();
	uint8_t old = m_mmu_io_ctrl;
	m_mmu_io_ctrl = data;
	if ((old ^ data) & 0x04)
	{
		update_banks();
	}
}

uint8_t wildbits_jr2_state::mmu_slot_r(offs_t offset)
{
	io_wait_read();
	uint8_t lut = (m_mmu_mem_ctrl >> 4) & 0x03;
	return m_mlut[lut][offset & 0x07];
}

void wildbits_jr2_state::mmu_slot_w(offs_t offset, uint8_t data)
{
	io_wait_write();
	uint8_t lut = (m_mmu_mem_ctrl >> 4) & 0x03;
	m_mlut[lut][offset & 0x07] = data;
	if (lut == (m_mmu_mem_ctrl & 0x03))
	{
		update_banks();
	}
}

uint8_t wildbits_jr2_state::sys0_r()
{
	io_wait_read();
	uint8_t val = m_sys0 & ~0xc0;
	if (!m_sdcard->get_card_present())
	{
		val |= 0x40; // SYS_SD_CD: 1 = no card, 0 = card inserted
	}
	return val;
}

void wildbits_jr2_state::sys0_w(uint8_t data)
{
	io_wait_write();
	m_sys0 = data;
	if ((data & 0x80) && (m_rst0 == 0xde) && (m_rst1 == 0xad))
	{
		machine().schedule_hard_reset();
	}
}

uint8_t wildbits_jr2_state::sys1_r()
{
	io_wait_read();
	return m_sys1;
}

void wildbits_jr2_state::sys1_w(uint8_t data)
{
	io_wait_write();
	m_sys1 = data;
}

void wildbits_jr2_state::rst0_w(uint8_t data)
{
	io_wait_write();
	m_rst0 = data;
}

void wildbits_jr2_state::rst1_w(uint8_t data)
{
	io_wait_write();
	m_rst1 = data;
}

uint8_t wildbits_jr2_state::mid_r()
{
	io_wait_read();
	return WBJR2_MACHINE_ID;
}

// LFSR Random Number Generator ($FE04 - $FE06)
void wildbits_jr2_state::lfsr_update()
{
	if (!(m_lfsr_ctrl & 0x01))
	{
		m_lfsr_last_cycle = m_maincpu->total_cycles();
		return;
	}

	uint64_t current_cycles = m_maincpu->total_cycles();
	uint64_t elapsed = current_cycles - m_lfsr_last_cycle;
	m_lfsr_last_cycle = current_cycles;

	if (m_lfsr_ctrl & 0x02)
	{
		// Seed Data Valid is active: held at seed value
		m_lfsr = m_lfsr_seed;
		return;
	}

	elapsed %= 65535;
	while (elapsed--)
	{
		uint8_t xnor = !(((m_lfsr >> 15) ^ (m_lfsr >> 14) ^ (m_lfsr >> 12) ^ (m_lfsr >> 3)) & 1);
		m_lfsr = ((m_lfsr << 1) & 0xfffe) | xnor;
	}
}

uint8_t wildbits_jr2_state::lfsr_r(offs_t offset)
{
	io_wait_read();
	lfsr_update();
	switch (offset)
	{
	case 0: // $FE04: LFSR Data Low
		return m_lfsr & 0xff;
	case 1: // $FE05: LFSR Data High
		return (m_lfsr >> 8) & 0xff;
	case 2: // $FE06: LFSR Status / Control
		return ((m_lfsr == m_lfsr_seed) ? 0x80 : 0x00) | (m_lfsr_ctrl & 0x7f);
	default:
		return 0x00;
	}
}

void wildbits_jr2_state::lfsr_w(offs_t offset, uint8_t data)
{
	io_wait_write();
	lfsr_update();
	switch (offset)
	{
	case 0: // $FE04: Seed Low
		m_lfsr_seed = (m_lfsr_seed & 0xff00) | data;
		break;
	case 1: // $FE05: Seed High
		m_lfsr_seed = (m_lfsr_seed & 0x00ff) | ((uint16_t)data << 8);
		break;
	case 2: // $FE06: Control Register
		m_lfsr_ctrl = data;
		if (data & 0x02)
			m_lfsr = m_lfsr_seed;
		break;
	}
}

// Real-Time Clock ($FE40 - $FE4F: bq4802)
static inline uint8_t to_bcd(uint8_t val)
{
	return ((val / 10) << 4) | (val % 10);
}

uint8_t wildbits_jr2_state::rtc_r(offs_t offset)
{
	io_wait_read();
	system_time systime;
	machine().current_datetime(systime);
	switch (offset)
	{
	case 0x00: return to_bcd(systime.local_time.second);
	case 0x02: return to_bcd(systime.local_time.minute);
	case 0x04: return to_bcd(systime.local_time.hour);
	case 0x06: return to_bcd(systime.local_time.mday);
	case 0x08: return to_bcd(systime.local_time.weekday + 1);
	case 0x09: return to_bcd(systime.local_time.month + 1);
	case 0x0a: return to_bcd(systime.local_time.year % 100);
	case 0x0e: return m_rtc_ctrl;
	case 0x0f: return to_bcd(systime.local_time.year / 100);
	default: return 0;
	}
}

void wildbits_jr2_state::rtc_w(offs_t offset, uint8_t data)
{
	io_wait_write();
	if (offset == 0x0e)
		m_rtc_ctrl = data;
}

void wildbits_jr2_state::reset_codec()
{
	memset(m_codec_regs, 0, sizeof(m_codec_regs));
	m_codec_regs[0] = 0x79; // HPLOUT (0 dB)
	m_codec_regs[1] = 0x79; // HPROUT (0 dB)
	m_codec_regs[2] = 0x79; // HP Master
	m_codec_regs[3] = 0xff; // DACL (0 dB)
	m_codec_regs[4] = 0xff; // DACR (0 dB)
	m_codec_regs[5] = 0xff; // DAC Master
	m_codec_regs[10] = 0x02; // 16-bit I2S
	m_codec_regs[13] = 0x00; // All power on
	m_codec_regs[17] = 0x01; // ALC Control 2
	m_codec_regs[21] = 0x03; // ADC Mux
	m_codec_regs[22] = 0x07; // Output Mux: Bypass + Aux + DAC
	m_codec_vs_gain_l = 1.0f;
	m_codec_vs_gain_r = 1.0f;
	update_codec();
}

void wildbits_jr2_state::update_codec()
{
	// WM8776 CODEC Gain Calculation:
	// R00/R01: Headphone attenuation ($79 = 0 dB, 1 dB/step down to $30 = -73 dB, <$30 = mute)
	// R03/R04: DAC attenuation ($FF = 0 dB, 0.5 dB/step down to $01 = -127 dB, $00 = mute)
	// R13: Power-down & mute (bit 0 = chip PD, bit 2 = DAC PD, bit 3 = HP PD)
	// R22: Output Mux (bit 0 = DAC enable, bit 2 = Bypass/AIN enable)

	auto get_hp_gain = [](uint8_t code) -> float {
		if (code < 0x30)
			return 0.0f;
		float db = (float)((int)code - 121);
		return std::pow(10.0f, db / 20.0f);
	};

	auto get_dac_gain = [](uint8_t code) -> float {
		if (code == 0)
			return 0.0f;
		float db = (float)((int)code - 255) * 0.5f;
		return std::pow(10.0f, db / 20.0f);
	};

	uint8_t r13 = m_codec_regs[13];
	bool chip_pd = (r13 & 0x01) != 0;
	bool dac_pd  = (r13 & 0x04) != 0;
	bool hp_pd   = (r13 & 0x08) != 0;

	bool dac_en    = (m_codec_regs[22] & 0x01) != 0;
	bool bypass_en = (m_codec_regs[22] & 0x04) != 0;

	float dac_gain_l = 0.0f;
	float dac_gain_r = 0.0f;
	m_codec_vs_gain_l = 0.0f;
	m_codec_vs_gain_r = 0.0f;

	if (!chip_pd && !hp_pd)
	{
		float hp_l = get_hp_gain(m_codec_regs[0]);
		float hp_r = get_hp_gain(m_codec_regs[1]);

		if (bypass_en)
		{
			m_codec_vs_gain_l = hp_l;
			m_codec_vs_gain_r = hp_r;
		}

		if (!dac_pd && dac_en)
		{
			float dac_l = get_dac_gain(m_codec_regs[3]);
			float dac_r = get_dac_gain(m_codec_regs[4]);
			dac_gain_l = hp_l * dac_l;
			dac_gain_r = hp_r * dac_r;
		}
	}

	m_opl3->set_output_gain(0, dac_gain_l);
	m_opl3->set_output_gain(1, dac_gain_r);
	m_opl3->set_output_gain(2, dac_gain_l);
	m_opl3->set_output_gain(3, dac_gain_r);
	m_psg_l->set_output_gain(ALL_OUTPUTS, dac_gain_l);
	m_psg_r->set_output_gain(ALL_OUTPUTS, dac_gain_r);
	m_sid_l->set_output_gain(ALL_OUTPUTS, dac_gain_l);
	m_sid_r->set_output_gain(ALL_OUTPUTS, dac_gain_r);
}

// Audio CODEC ($FE70 - $FE72: WM8776)
uint8_t wildbits_jr2_state::codec_r(offs_t offset)
{
	io_wait_read();
	if (offset == 2)
		return 0x00; // Bit 0 = 0 (Ready / idle)
	return (offset == 0) ? m_codec_lo : m_codec_hi;
}

void wildbits_jr2_state::codec_w(offs_t offset, uint8_t data)
{
	io_wait_write();
	if (offset == 0)
		m_codec_lo = data;
	else if (offset == 1)
		m_codec_hi = data;
	else if (offset == 2)
	{
		if (data & 0x01)
		{
			uint16_t word = ((uint16_t)m_codec_hi << 8) | m_codec_lo;
			uint8_t reg = (word >> 9) & 0x7f;
			uint8_t val = word & 0xff;

			if (reg == 0x17) // R23: Software reset
			{
				reset_codec();
			}
			else
			{
				if (reg < 32)
					m_codec_regs[reg] = val;
				if (reg == 0x02) // Master headphone volume
				{
					m_codec_regs[0] = val;
					m_codec_regs[1] = val;
				}
				else if (reg == 0x05) // Master DAC volume
				{
					m_codec_regs[3] = val;
					m_codec_regs[4] = val;
				}
				update_codec();
			}
		}
	}
}

// Hardware Configuration DIP Switches ($FF90)
uint8_t wildbits_jr2_state::dipsw_r()
{
	io_wait_read();
	uint8_t val = m_dipsw->read();
	// Command-line -bios option:
	//   "-bios turbo" (default): Bit 0 = 0 (Active-low switch ON / 20 MHz Turbo Mode enabled)
	//   "-bios stock":           Bit 0 = 1 (Active-low switch OFF / Stock 16.67 MHz clock, Turbo disabled)
	if (system_bios() == 2)
	{
		val |= 0x01; // Bit 0 = 1 (Stock 16.67 MHz)
	}
	else if (system_bios() == 1)
	{
		val &= ~0x01; // Bit 0 = 0 (20 MHz Turbo Mode)
	}
	return val;
}

// Interrupt Controller ($FE20 - $FE2F)
void wildbits_jr2_state::check_irqs()
{
	bool assert_irq = false;
	for (int g = 0; g < 4; g++)
	{
		if ((m_int_pending[g] & ~m_int_mask[g]) != 0)
		{
			assert_irq = true;
			break;
		}
	}
	m_maincpu->set_input_line(M6809_IRQ_LINE, assert_irq ? ASSERT_LINE : CLEAR_LINE);
}

void wildbits_jr2_state::set_irq(int group, uint8_t mask)
{
	m_int_pending[group] |= mask;
	check_irqs();
}

uint8_t wildbits_jr2_state::intc_r(offs_t offset)
{
	io_wait_read();
	switch (offset >> 2)
	{
	case 0: return m_int_pending[offset & 3];
	case 1: return m_int_pol[offset & 3];
	case 2: return m_int_edge[offset & 3];
	case 3: return m_int_mask[offset & 3];
	default: return 0;
	}
}

void wildbits_jr2_state::intc_w(offs_t offset, uint8_t data)
{
	io_wait_write();
	switch (offset >> 2)
	{
	case 0: // Write 1 to clear pending bit
		m_int_pending[offset & 3] &= ~data;
		check_irqs();
		break;
	case 1:
		m_int_pol[offset & 3] = data;
		break;
	case 2:
		m_int_edge[offset & 3] = data;
		break;
	case 3: // Mask register (0=unmasked, 1=masked)
		m_int_mask[offset & 3] = data;
		check_irqs();
		break;
	}
}

// System Timers ($FE30 - $FE3F)
TIMER_CALLBACK_MEMBER(wildbits_jr2_state::timer0_tick)
{
	m_t0_stat |= 0x01; // Compare match status
	set_irq(0, 0x10);  // INT_TIMER_0 (Bit 4 of Group 0)

	poll_wizfi_socket();

	if (m_t0_cmp > 0 && (m_t0_ctr & 0x01))
	{
		attotime period = attotime::from_hz(25'175'000) * m_t0_cmp;
		if (period < attotime::from_hz(1000))
			period = attotime::from_hz(1000);
		m_timer0->adjust(period);
	}
}

TIMER_CALLBACK_MEMBER(wildbits_jr2_state::timer1_tick)
{
	m_t1_stat |= 0x01;
	set_irq(0, 0x20);  // INT_TIMER_1 (Bit 5 of Group 0)

	poll_wizfi_socket();

	if (m_t1_cmp > 0 && (m_t1_ctr & 0x01))
	{
		m_timer1->adjust(attotime::from_hz(60) * m_t1_cmp);
	}
}

uint8_t wildbits_jr2_state::timer_r(offs_t offset)
{
	io_wait_read();
	uint32_t t0_current = (uint32_t)(machine().time().as_ticks(25'175'000) & 0xffffff);

	switch (offset)
	{
	case 0x00: {
		uint8_t stat = m_t0_stat;
		m_t0_stat = 0; // Read clears status
		return stat;
	}
	case 0x01: return t0_current & 0xff;
	case 0x02: return (t0_current >> 8) & 0xff;
	case 0x03: return (t0_current >> 16) & 0xff;
	case 0x04: return m_t0_cmp_ctr;
	case 0x05: return m_t0_cmp & 0xff;
	case 0x06: return (m_t0_cmp >> 8) & 0xff;
	case 0x07: return (m_t0_cmp >> 16) & 0xff;
	case 0x08: {
		uint8_t stat = m_t1_stat;
		m_t1_stat = 0;
		return stat;
	}
	case 0x09: return m_t1_val & 0xff;
	case 0x0a: return (m_t1_val >> 8) & 0xff;
	case 0x0b: return (m_t1_val >> 16) & 0xff;
	case 0x0c: return m_t1_cmp_ctr;
	case 0x0d: return m_t1_cmp & 0xff;
	case 0x0e: return (m_t1_cmp >> 8) & 0xff;
	case 0x0f: return (m_t1_cmp >> 16) & 0xff;
	default: return 0;
	}
}

void wildbits_jr2_state::timer_w(offs_t offset, uint8_t data)
{
	io_wait_write();
	switch (offset)
	{
	case 0x00: // T0_CTR
		m_t0_ctr = data;
		if (data & 0x01) // Enable
		{
			if (m_t0_cmp > 0)
			{
				attotime period = attotime::from_hz(25'175'000) * m_t0_cmp;
				if (period < attotime::from_hz(1000))
					period = attotime::from_hz(1000);
				m_timer0->adjust(period);
			}
		}
		else
		{
			m_timer0->adjust(attotime::never);
		}
		break;
	case 0x01: m_t0_val = (m_t0_val & 0xffff00) | data; break;
	case 0x02: m_t0_val = (m_t0_val & 0xff00ff) | (data << 8); break;
	case 0x03: m_t0_val = (m_t0_val & 0x00ffff) | (data << 16); break;
	case 0x04: m_t0_cmp_ctr = data; break;
	case 0x05: m_t0_cmp = (m_t0_cmp & 0xffff00) | data; break;
	case 0x06: m_t0_cmp = (m_t0_cmp & 0xff00ff) | (data << 8); break;
	case 0x07: m_t0_cmp = (m_t0_cmp & 0x00ffff) | (data << 16); break;
	case 0x08: // T1_CTR
		m_t1_ctr = data;
		if (data & 0x01)
		{
			if (m_t1_cmp > 0)
				m_timer1->adjust(attotime::from_hz(60) * m_t1_cmp);
		}
		else
		{
			m_timer1->adjust(attotime::never);
		}
		break;
	case 0x09: m_t1_val = (m_t1_val & 0xffff00) | data; break;
	case 0x0a: m_t1_val = (m_t1_val & 0xff00ff) | (data << 8); break;
	case 0x0b: m_t1_val = (m_t1_val & 0x00ffff) | (data << 16); break;
	case 0x0c: m_t1_cmp_ctr = data; break;
	case 0x0d: m_t1_cmp = (m_t1_cmp & 0xffff00) | data; break;
	case 0x0e: m_t1_cmp = (m_t1_cmp & 0xff00ff) | (data << 8); break;
	case 0x0f: m_t1_cmp = (m_t1_cmp & 0x00ffff) | (data << 16); break;
	}
}

// PS/2 Controller ($FE50 - $FE54)
void wildbits_jr2_state::queue_kbd_scancode(uint8_t scancode)
{
	m_kbd_fifo.push(scancode);
	set_irq(0, 0x04); // INT_PS2_KBD (Bit 2 of Group 0)
}

uint8_t wildbits_jr2_state::ps2_r(offs_t offset)
{
	io_wait_read();
	switch (offset)
	{
	case 0x00: return m_ps2_ctrl;
	case 0x01: return m_ps2_out;
	case 0x02: // KBD_IN: Pop from FIFO
		if (!m_kbd_fifo.empty())
		{
			uint8_t code = m_kbd_fifo.front();
			m_kbd_fifo.pop();
			return code;
		}
		return 0;
	case 0x03: // MS_IN
		if (!m_mouse_fifo.empty())
		{
			uint8_t code = m_mouse_fifo.front();
			m_mouse_fifo.pop();
			return code;
		}
		return 0;
	case 0x04: { // PS2_STAT
		uint8_t stat = 0;
		if (m_kbd_fifo.empty()) stat |= 0x01;   // KEMP
		if (m_mouse_fifo.empty()) stat |= 0x02; // MEMP
		return stat;
	}
	default: return 0;
	}
}

void wildbits_jr2_state::ps2_w(offs_t offset, uint8_t data)
{
	io_wait_write();
	switch (offset)
	{
	case 0x00: // PS2_CTRL
		m_ps2_ctrl = data;
		if (data & 0x10) // KCLR: Clear keyboard FIFO and IRQ
		{
			while (!m_kbd_fifo.empty()) m_kbd_fifo.pop();
			m_int_pending[0] &= ~0x04;
			check_irqs();
		}
		if (data & 0x20) // MCLR: Clear mouse FIFO and IRQ
		{
			while (!m_mouse_fifo.empty()) m_mouse_fifo.pop();
			m_int_pending[0] &= ~0x08;
			check_irqs();
		}
		if (data & 0x02) // Keyboard transmit trigger
		{
			if (m_ps2_out == 0xff) // Reset command
			{
				queue_kbd_scancode(0xfa); // ACK
				queue_kbd_scancode(0xaa); // BAT OK
			}
			else
			{
				queue_kbd_scancode(0xfa); // Default ACK
			}
		}
		if (data & 0x08) // Mouse transmit trigger
		{
			if (m_ps2_out == 0xff) // Reset command
			{
				m_mouse_fifo.push(0xfa); // ACK
				m_mouse_fifo.push(0xaa); // BAT OK
				m_mouse_fifo.push(0x00); // Mouse ID 0
				set_irq(0, 0x08);
			}
			else if (m_ps2_out == 0xf2) // Get ID
			{
				m_mouse_fifo.push(0xfa); // ACK
				m_mouse_fifo.push(0x00); // Standard PS/2 mouse ID
				set_irq(0, 0x08);
			}
			else
			{
				m_mouse_fifo.push(0xfa); // Default ACK
				set_irq(0, 0x08);
			}
		}
		break;
	case 0x01: // PS2_OUT
		m_ps2_out = data;
		break;
	}
}

// SPI SD Card Controller ($FE90 - $FE91)
void wildbits_jr2_state::sdcard_miso_w(int state)
{
	m_sdcard_miso = state;
}

uint8_t wildbits_jr2_state::sdc_stat_r()
{
	io_wait_read();
	// Bit 0: CS_EN, Bit 1: SPI_CLK, Bit 7: 0 (not busy)
	return m_sdc_stat & 0x03;
}

void wildbits_jr2_state::sdc_stat_w(uint8_t data)
{
	io_wait_write();
	m_sdc_stat = data;
	// Bit 0: CS_EN (1 = chip select active, 0 = inactive)
	m_sdcard->spi_ss_w((data & 0x01) ? 1 : 0);
}

uint8_t wildbits_jr2_state::sdc_data_r()
{
	io_wait_read();
	return m_sdc_data_in;
}

void wildbits_jr2_state::sdc_data_w(uint8_t data)
{
	io_wait_write();
	m_sdc_data_out = data;
	uint8_t in_byte = 0;
	for (int bit = 7; bit >= 0; bit--)
	{
		m_sdcard->spi_mosi_w((data >> bit) & 1);
		m_sdcard->spi_clock_w(1);
		in_byte = (in_byte << 1) | (m_sdcard_miso ? 1 : 0);
		m_sdcard->spi_clock_w(0);
	}
	m_sdc_data_in = in_byte;
}

// 16550 UART Serial Controller ($FE60 - $FE67)
void wildbits_jr2_state::poll_uart_socket()
{
	if (!m_uart_socket)
	{
		uint64_t filesize = 0;
		osd_file::open("socket.127.0.0.1:65504", OPEN_FLAG_READ | OPEN_FLAG_WRITE, m_uart_socket, filesize);
	}
	if (!m_uart_socket)
		return;

	uint8_t buf[512];
	uint32_t actual = 0;
	std::error_condition err = m_uart_socket->read(buf, 0, sizeof(buf), actual);
	if (!err && actual > 0)
	{
		for (uint32_t i = 0; i < actual; i++)
		{
			if (m_uart_rx_fifo.size() < 4096)
				m_uart_rx_fifo.push(buf[i]);
		}
		if (m_uart_ier & 0x01)
		{
			set_irq(1, 0x01); // INT_UART (Group 1, bit 0)
		}
	}
}

uint8_t wildbits_jr2_state::uart_r(offs_t offset)
{
	io_wait_read();
	poll_uart_socket();
	switch (offset & 7)
	{
	case 0: // RBR (or DLL if DLAB=1)
		if (m_uart_lcr & 0x80)
			return m_uart_dll;
		if (!m_uart_rx_fifo.empty())
		{
			uint8_t val = m_uart_rx_fifo.front();
			m_uart_rx_fifo.pop();
			return val;
		}
		return 0;
	case 1: // IER (or DLH if DLAB=1)
		if (m_uart_lcr & 0x80)
			return m_uart_dlh;
		return m_uart_ier;
	case 2: // IIR
		return m_uart_rx_fifo.empty() ? 0x01 : 0x04;
	case 3: // LCR
		return m_uart_lcr;
	case 4: // MCR
		return m_uart_mcr;
	case 5: // LSR
		return 0x60 | (m_uart_rx_fifo.empty() ? 0 : 0x01);
	case 6: // MSR
		return 0xb0; // DSR, CTS, DCD set
	case 7: // SCR
		return m_uart_scr;
	}
	return 0xff;
}

void wildbits_jr2_state::uart_w(offs_t offset, uint8_t data)
{
	io_wait_write();
	poll_uart_socket();
	switch (offset & 7)
	{
	case 0: // THR (or DLL if DLAB=1)
		if (m_uart_lcr & 0x80)
		{
			m_uart_dll = data;
		}
		else
		{
			if (m_uart_socket)
			{
				uint32_t written = 0;
				m_uart_socket->write(&data, 0, 1, written);
			}
		}
		break;
	case 1: // IER (or DLH if DLAB=1)
		if (m_uart_lcr & 0x80)
		{
			m_uart_dlh = data;
		}
		else
		{
			m_uart_ier = data;
			if ((m_uart_ier & 0x01) && !m_uart_rx_fifo.empty())
			{
				set_irq(1, 0x01); // INT_UART (Group 1, bit 0)
			}
		}
		break;
	case 2: // FCR
		m_uart_fcr = data;
		if (data & 0x02) // Clear RX FIFO
		{
			while (!m_uart_rx_fifo.empty())
				m_uart_rx_fifo.pop();
		}
		break;
	case 3: // LCR
		m_uart_lcr = data;
		break;
	case 4: // MCR
		m_uart_mcr = data;
		break;
	case 5: // LSR
		break;
	case 6: // MSR
		break;
	case 7: // SCR
		m_uart_scr = data;
		break;
	}
}

// WizFi360 WiFi / SPI Controller ($FF20 - $FF2F)
void wildbits_jr2_state::push_wizfi_response(const std::string &resp)
{
	if (resp.empty())
		return;
	for (char c : resp)
	{
		if (m_wizfi_rx_fifo.size() < 2048)
			m_wizfi_rx_fifo.push((uint8_t)c);
	}
	set_irq(3, 0x01); // INT_WIZFI_RX (Group 3, bit 0)
}

void wildbits_jr2_state::reset_wizfi()
{
	while (!m_wizfi_rx_fifo.empty())
		m_wizfi_rx_fifo.pop();
	m_wizfi_tx_buf.clear();
	m_wizfi_baud_code = 0x0a; // Baud code 10 = 115,200
	m_wizfi_reset_reg = 0x00;
	m_wizfi_ctrl = 0;
	m_wizfi_transparent = false;
	m_wizfi_plus_count = 0;
	m_wizfi_cipsend_remaining = 0;
	m_wizfi_cipsend_total = 0;
	m_wizfi_cipmux = false;
	m_wizfi_cipmode = false;
	m_wizfi_echo = false;
	m_wizfi_cwmode = 1;
	m_wizfi_wifi_connected = true;
	m_wizfi_ssid = "WildbitsNet";
	m_wizfi_link_id = 0;
	m_wizfi_remote_host.clear();
	m_wizfi_remote_port = 0;
	if (m_wizfi_socket)
		m_wizfi_socket.reset();
}

void wildbits_jr2_state::poll_wizfi_socket()
{
	if (!m_wizfi_socket)
		return;

	uint8_t rx_buf[512];
	uint32_t actual = 0;
	std::error_condition err = m_wizfi_socket->read(rx_buf, 0, sizeof(rx_buf), actual);
	if (!err && actual > 0)
	{
		if (m_wizfi_transparent)
		{
			for (uint32_t i = 0; i < actual; i++)
			{
				if (m_wizfi_rx_fifo.size() < 2048)
					m_wizfi_rx_fifo.push(rx_buf[i]);
			}
		}
		else
		{
			std::string header = m_wizfi_cipmux ?
				util::string_format("\r\n+IPD,%d,%d:", m_wizfi_link_id, actual) :
				util::string_format("\r\n+IPD,%d:", actual);
			for (char c : header)
			{
				if (m_wizfi_rx_fifo.size() < 2048)
					m_wizfi_rx_fifo.push((uint8_t)c);
			}
			for (uint32_t i = 0; i < actual; i++)
			{
				if (m_wizfi_rx_fifo.size() < 2048)
					m_wizfi_rx_fifo.push(rx_buf[i]);
			}
		}
		set_irq(3, 0x01); // INT_WIZFI_RX (Group 3, bit 0)
	}
}

void wildbits_jr2_state::handle_cipstart(const std::string &cmd)
{
	size_t eq = cmd.find('=');
	if (eq == std::string::npos)
	{
		push_wizfi_response("\r\nERROR\r\n");
		return;
	}

	std::string args = cmd.substr(eq + 1);
	std::vector<std::string> parts;
	std::string current;
	bool in_quote = false;
	for (char c : args)
	{
		if (c == '"')
		{
			in_quote = !in_quote;
		}
		else if (c == ',' && !in_quote)
		{
			parts.push_back(current);
			current.clear();
		}
		else
		{
			current += c;
		}
	}
	if (!current.empty())
		parts.push_back(current);

	int link_id = 0;
	std::string type, host;
	int port = 0;
	int idx = 0;

	if (parts.size() >= 3 && parts[0].find_first_not_of("0123456789") == std::string::npos && !parts[0].empty())
	{
		link_id = std::stoi(parts[0]);
		idx = 1;
	}

	if (idx < (int)parts.size()) type = parts[idx++];
	if (idx < (int)parts.size()) host = parts[idx++];
	if (idx < (int)parts.size()) port = std::stoi(parts[idx++]);

	while (!host.empty() && (host.front() == '"' || host.front() == ' ')) host.erase(0, 1);
	while (!host.empty() && (host.back() == '"' || host.back() == ' ')) host.pop_back();

	if (m_wizfi_socket)
	{
		push_wizfi_response("\r\nALREADY CONNECTED\r\n\r\nERROR\r\n");
		return;
	}

	if (!m_wizfi_wifi_connected)
	{
		if (m_wizfi_cipmux)
			push_wizfi_response(util::string_format("\r\n%d,CLOSED\r\n\r\nCONNECT FAIL\r\n\r\nERROR\r\n", link_id));
		else
			push_wizfi_response("\r\nCLOSED\r\n\r\nCONNECT FAIL\r\n\r\nERROR\r\n");
		return;
	}

	m_wizfi_link_id = link_id;
	m_wizfi_remote_host = host;
	m_wizfi_remote_port = port;

	uint64_t filesize = 0;
	std::string socket_path = util::string_format("socket.%s:%d", host.c_str(), port);
	std::error_condition err = osd_file::open(socket_path, OPEN_FLAG_READ | OPEN_FLAG_WRITE, m_wizfi_socket, filesize);

	if (err && (host == "192.168.1.100" || host == "localhost"))
	{
		std::string fallback_path = util::string_format("socket.127.0.0.1:%d", port);
		err = osd_file::open(fallback_path, OPEN_FLAG_READ | OPEN_FLAG_WRITE, m_wizfi_socket, filesize);
	}

	if (!err && m_wizfi_socket)
	{
		if (m_wizfi_cipmux)
			push_wizfi_response(util::string_format("\r\n%d,CONNECT\r\n\r\nOK\r\n", link_id));
		else
			push_wizfi_response("\r\nCONNECT\r\n\r\nOK\r\n");
	}
	else
	{
		if (m_wizfi_cipmux)
			push_wizfi_response(util::string_format("\r\n%d,CLOSED\r\n\r\nCONNECT FAIL\r\n\r\nERROR\r\n", link_id));
		else
			push_wizfi_response("\r\nCLOSED\r\n\r\nCONNECT FAIL\r\n\r\nERROR\r\n");
	}
}

void wildbits_jr2_state::handle_cipsend(const std::string &cmd)
{
	if (m_wizfi_cipmode)
	{
		push_wizfi_response("\r\nOK\r\n\r\n> ");
		m_wizfi_transparent = true;
		m_wizfi_plus_count = 0;
	}
	else
	{
		size_t eq = cmd.find('=');
		if (eq != std::string::npos)
		{
			std::string arg = cmd.substr(eq + 1);
			size_t comma = arg.find(',');
			int len = 0;
			if (comma != std::string::npos)
				len = std::stoi(arg.substr(comma + 1));
			else
				len = std::stoi(arg);

			m_wizfi_cipsend_remaining = len;
			m_wizfi_cipsend_total = len;
			push_wizfi_response("\r\nOK\r\n> ");
		}
		else
		{
			push_wizfi_response("\r\nERROR\r\n");
		}
	}
}

void wildbits_jr2_state::process_wizfi_cmd(const std::string &cmd_raw)
{
	if (m_wizfi_echo)
	{
		push_wizfi_response(cmd_raw + "\r\n");
	}

	std::string cmd = cmd_raw;
	while (!cmd.empty() && (cmd.back() == '\r' || cmd.back() == '\n' || cmd.back() == ' '))
		cmd.pop_back();

	if (cmd.empty())
		return;

	std::string cmd_upper = cmd;
	for (char &c : cmd_upper)
		c = toupper((unsigned char)c);

	if (cmd_upper == "AT")
	{
		push_wizfi_response("\r\nOK\r\n");
	}
	else if (cmd_upper == "ATE0")
	{
		m_wizfi_echo = false;
		push_wizfi_response("\r\nOK\r\n");
	}
	else if (cmd_upper == "ATE1")
	{
		m_wizfi_echo = true;
		push_wizfi_response("\r\nOK\r\n");
	}
	else if (cmd_upper.rfind("AT+GMR", 0) == 0)
	{
		push_wizfi_response("\r\nAT version:1.1.2.0(Apr 12 2023 08:08:36)\r\nSDK version:3.2.0(a0ffff9f)\r\ncompile time:Apr 12 2023 08:08:36\r\n\r\nOK\r\n");
	}
	else if (cmd_upper.rfind("AT+RST", 0) == 0)
	{
		reset_wizfi();
		push_wizfi_response("\r\nOK\r\n\r\nready\r\nWIFI CONNECTED\r\nWIFI GOT IP\r\n\r\nOK\r\n");
	}
	else if (cmd_upper.rfind("AT+UART", 0) == 0)
	{
		if (cmd.find('?') != std::string::npos)
		{
			std::string tag = (cmd_upper.find("_DEF") != std::string::npos) ? "+UART_DEF" : "+UART_CUR";
			push_wizfi_response(util::string_format("\r\n%s:115200,8,1,0,0\r\nOK\r\n", tag.c_str()));
		}
		else
		{
			push_wizfi_response("\r\nOK\r\n");
		}
	}

	else if (cmd_upper.rfind("AT+CWMODE", 0) == 0)
	{
		if (cmd.find('?') != std::string::npos)
		{
			std::string tag = (cmd_upper.find("_DEF") != std::string::npos) ? "+CWMODE_DEF" :
			                  (cmd_upper.find("_CUR") != std::string::npos) ? "+CWMODE_CUR" : "+CWMODE";
			push_wizfi_response(util::string_format("\r\n%s:%d\r\n\r\nOK\r\n", tag.c_str(), m_wizfi_cwmode));
		}
		else
		{
			size_t eq = cmd.find('=');
			if (eq != std::string::npos && eq + 1 < cmd.length())
				m_wizfi_cwmode = cmd[eq + 1] - '0';
			push_wizfi_response("\r\nOK\r\n");
		}
	}
	else if (cmd_upper.rfind("AT+CWDHCP", 0) == 0)
	{
		if (cmd.find('?') != std::string::npos)
		{
			std::string tag = (cmd_upper.find("_DEF") != std::string::npos) ? "+CWDHCP_DEF" :
			                  (cmd_upper.find("_CUR") != std::string::npos) ? "+CWDHCP_CUR" : "+CWDHCP";
			push_wizfi_response(util::string_format("\r\n%s:3\r\nOK\r\n", tag.c_str()));
		}
		else
		{
			push_wizfi_response("\r\nOK\r\n");
		}
	}
	else if (cmd_upper.rfind("AT+CWJAP", 0) == 0)
	{
		if (cmd.find('?') != std::string::npos)
		{
			std::string tag = (cmd_upper.find("_DEF") != std::string::npos) ? "+CWJAP_DEF" :
			                  (cmd_upper.find("_CUR") != std::string::npos) ? "+CWJAP_CUR" : "+CWJAP";
			push_wizfi_response(util::string_format("\r\n%s:\"%s\",\"00:08:dc:6b:e3:36\",1,-50\r\n\r\nOK\r\n", tag.c_str(), m_wizfi_ssid.c_str()));
		}
		else
		{
			size_t eq = cmd.find('=');
			if (eq != std::string::npos)
			{
				std::string arg = cmd.substr(eq + 1);
				size_t comma = arg.find(',');
				std::string ssid = (comma != std::string::npos) ? arg.substr(0, comma) : arg;
				while (!ssid.empty() && (ssid.front() == '"' || ssid.front() == ' ')) ssid.erase(0, 1);
				while (!ssid.empty() && (ssid.back() == '"' || ssid.back() == ' ')) ssid.pop_back();
				if (!ssid.empty())
					m_wizfi_ssid = ssid;
			}
			m_wizfi_wifi_connected = true;
			push_wizfi_response("\r\nWIFI CONNECTED\r\nWIFI GOT IP\r\n\r\nOK\r\n");
		}
	}
	else if (cmd_upper.rfind("AT+CWQAP", 0) == 0)
	{
		m_wizfi_wifi_connected = false;
		push_wizfi_response("\r\nOK\r\nWIFI DISCONNECT\r\n");
	}
	else if (cmd_upper.rfind("AT+CWLAP", 0) == 0)
	{
		push_wizfi_response("\r\n+CWLAP:(4,\"WildbitsNet\",-50,\"00:08:dc:6b:e3:36\",1,0)\r\n\r\nOK\r\n");
	}
	else if (cmd_upper.rfind("AT+CIPSTA", 0) == 0 && cmd_upper.find("MAC") == std::string::npos)
	{
		if (cmd.find('?') != std::string::npos)
		{
			std::string tag = (cmd_upper.find("_DEF") != std::string::npos) ? "+CIPSTA_DEF" :
			                  (cmd_upper.find("_CUR") != std::string::npos) ? "+CIPSTA_CUR" : "+CIPSTA";
			push_wizfi_response(util::string_format("\r\n%s:ip:\"192.168.1.100\"\r\n%s:gateway:\"192.168.1.1\"\r\n%s:netmask:\"255.255.255.0\"\r\n\r\nOK\r\n",
				tag.c_str(), tag.c_str(), tag.c_str()));
		}
		else
		{
			push_wizfi_response("\r\nOK\r\n");
		}
	}
	else if (cmd_upper.rfind("AT+CIPSTAMAC", 0) == 0)
	{
		if (cmd.find('?') != std::string::npos)
		{
			std::string tag = (cmd_upper.find("_DEF") != std::string::npos) ? "+CIPSTAMAC_DEF" :
			                  (cmd_upper.find("_CUR") != std::string::npos) ? "+CIPSTAMAC_CUR" : "+CIPSTAMAC";
			push_wizfi_response(util::string_format("\r\n%s:\"00:08:dc:6b:e3:36\"\r\n\r\nOK\r\n", tag.c_str()));
		}
		else
		{
			push_wizfi_response("\r\nOK\r\n");
		}
	}
	else if (cmd_upper.rfind("AT+CIPAPMAC", 0) == 0)
	{
		if (cmd.find('?') != std::string::npos)
		{
			std::string tag = (cmd_upper.find("_DEF") != std::string::npos) ? "+CIPAPMAC_DEF" :
			                  (cmd_upper.find("_CUR") != std::string::npos) ? "+CIPAPMAC_CUR" : "+CIPAPMAC";
			push_wizfi_response(util::string_format("\r\n%s:\"02:08:dc:6b:e3:36\"\r\n\r\nOK\r\n", tag.c_str()));
		}
		else
		{
			push_wizfi_response("\r\nOK\r\n");
		}
	}
	else if (cmd_upper.rfind("AT+CIFSR", 0) == 0)
	{
		push_wizfi_response("\r\n+CIFSR:STAIP,\"192.168.1.100\"\r\n+CIFSR:STAMAC,\"00:08:dc:6b:e3:36\"\r\n\r\nOK\r\n");
	}
	else if (cmd_upper.rfind("AT+CIPMUX", 0) == 0)
	{
		if (cmd.find('?') != std::string::npos)
		{
			push_wizfi_response(util::string_format("\r\n+CIPMUX:%d\r\n\r\nOK\r\n", m_wizfi_cipmux ? 1 : 0));
		}
		else
		{
			if (m_wizfi_socket)
			{
				push_wizfi_response("\r\nlink is builded\r\n\r\nERROR\r\n");
			}
			else
			{
				size_t eq = cmd.find('=');
				if (eq != std::string::npos && eq + 1 < cmd.length())
					m_wizfi_cipmux = (cmd[eq + 1] != '0');
				push_wizfi_response("\r\nOK\r\n");
			}
		}
	}
	else if (cmd_upper.rfind("AT+CIPMODE", 0) == 0)
	{
		if (cmd.find('?') != std::string::npos)
		{
			push_wizfi_response(util::string_format("\r\n+CIPMODE:%d\r\n\r\nOK\r\n", m_wizfi_cipmode ? 1 : 0));
		}
		else
		{
			size_t eq = cmd.find('=');
			if (eq != std::string::npos && eq + 1 < cmd.length())
			{
				bool req_mode = (cmd[eq + 1] != '0');
				if (req_mode && m_wizfi_cipmux)
				{
					push_wizfi_response("\r\nERROR\r\n");
				}
				else
				{
					m_wizfi_cipmode = req_mode;
					push_wizfi_response("\r\nOK\r\n");
				}
			}
			else
			{
				push_wizfi_response("\r\nERROR\r\n");
			}
		}
	}
	else if (cmd_upper.rfind("AT+CIPSTATUS", 0) == 0)
	{
		if (m_wizfi_socket)
		{
			push_wizfi_response(util::string_format("\r\nSTATUS:3\r\n+CIPSTATUS:%d,\"TCP\",\"%s\",%d,5000,0\r\n\r\nOK\r\n",
				m_wizfi_link_id, m_wizfi_remote_host.c_str(), m_wizfi_remote_port));
		}
		else if (m_wizfi_wifi_connected)
		{
			push_wizfi_response("\r\nSTATUS:2\r\n\r\nOK\r\n");
		}
		else
		{
			push_wizfi_response("\r\nSTATUS:5\r\n\r\nOK\r\n");
		}
	}
	else if (cmd_upper.rfind("AT+CIPSERVER", 0) == 0 || cmd_upper.rfind("AT+CIPSTO", 0) == 0)
	{
		push_wizfi_response("\r\nOK\r\n");
	}
	else if (cmd_upper.rfind("AT+CIPSTART", 0) == 0)
	{
		handle_cipstart(cmd);
	}
	else if (cmd_upper.rfind("AT+CIPCLOSE", 0) == 0)
	{
		if (m_wizfi_socket)
		{
			m_wizfi_socket.reset();
			m_wizfi_transparent = false;
			if (m_wizfi_cipmux)
				push_wizfi_response(util::string_format("\r\n%d,CLOSED\r\n\r\nOK\r\n", m_wizfi_link_id));
			else
				push_wizfi_response("\r\nCLOSED\r\n\r\nOK\r\n");
		}
		else
		{
			if (m_wizfi_cipmux)
			{
				size_t eq = cmd.find('=');
				int id = (eq != std::string::npos && eq + 1 < cmd.length()) ? (cmd[eq + 1] - '0') : m_wizfi_link_id;
				push_wizfi_response(util::string_format("\r\n%d,CLOSED\r\n\r\nERROR\r\n", id));
			}
			else
			{
				push_wizfi_response("\r\nERROR\r\n");
			}
		}
	}
	else if (cmd_upper.rfind("AT+CIPSEND", 0) == 0)
	{
		handle_cipsend(cmd);
	}
	else if (cmd_upper.rfind("AT+MQTTSET", 0) == 0 || cmd_upper.rfind("AT+MQTTTOPIC", 0) == 0)
	{
		push_wizfi_response("\r\nOK\r\n");
	}
	else if (cmd_upper.rfind("AT+MQTTCON", 0) == 0)
	{
		push_wizfi_response("\r\n+MQTTCON:OK\r\n\r\nOK\r\n");
	}
	else if (cmd_upper.rfind("AT+MQTTPUB", 0) == 0)
	{
		push_wizfi_response("\r\n+MQTTPUB:OK\r\n\r\nOK\r\n");
	}
	else if (cmd_upper.rfind("AT+MQTTSUB", 0) == 0)
	{
		push_wizfi_response("\r\n+MQTTSUB:OK\r\n\r\nOK\r\n");
	}
	else if (cmd_upper.rfind("AT+MQTTDIS", 0) == 0)
	{
		push_wizfi_response("\r\n+MQTTDIS:OK\r\n\r\nOK\r\n");
	}
	else if (cmd_upper.rfind("AT+", 0) == 0)
	{
		push_wizfi_response("\r\nOK\r\n");
	}
	else
	{
		push_wizfi_response("\r\nERROR\r\n");
	}
}

uint8_t wildbits_jr2_state::wizfi_r(offs_t offset)
{
	io_wait_read();
	switch (offset)
	{
	case 0x00: {
		// Control register ($FF20)
		// Bit 7: TxEmpty
		// Bit 6: RxEmpty
		// Bit 5: 0 (reserved)
		// Bits 4..0: Baud rate code (0..16, reset value 10 = 115,200)
		uint8_t ctrl = m_wizfi_baud_code & 0x1f;
		if (m_wizfi_tx_buf.empty() && m_wizfi_cipsend_remaining == 0)
			ctrl |= 0x80; // TxEmpty
		if (m_wizfi_rx_fifo.empty())
			ctrl |= 0x40; // RxEmpty
		return ctrl;
	}
	case 0x01:
		// Reset register ($FF21): Bit 7 reads 1 (HasReset flag), bits 6..3 read 0, bits 2..0 = WIFI_Reset_Register
		return 0x80 | (m_wizfi_reset_reg & 0x07);
	case 0x02: {
		// Data port ($FF22): Rx FIFO pop
		if (!m_wizfi_rx_fifo.empty())
		{
			uint8_t b = m_wizfi_rx_fifo.front();
			m_wizfi_rx_fifo.pop();
			return b;
		}
		return 0x00;
	}
	case 0x03: return 0x00; // Rx RD cnt hi (11-bit)
	case 0x04: return 0x00; // Rx RD cnt lo
	case 0x05: return (m_wizfi_rx_fifo.size() >> 8) & 0x07; // Rx WR cnt hi
	case 0x06: return m_wizfi_rx_fifo.size() & 0xff;        // Rx WR cnt lo
	case 0x07: return 0x00; // Tx RD cnt hi
	case 0x08: return 0x00; // Tx RD cnt lo
	case 0x09: return (m_wizfi_tx_buf.length() >> 8) & 0x07; // Tx WR cnt hi
	case 0x0a: return m_wizfi_tx_buf.length() & 0xff;        // Tx WR cnt lo
	default: return 0x55;
	}
}

void wildbits_jr2_state::wizfi_w(offs_t offset, uint8_t data)
{
	io_wait_write();
	switch (offset)
	{
	case 0x00:
		m_wizfi_baud_code = data & 0x1f;
		break;
	case 0x01: {
		uint8_t old_reset = m_wizfi_reset_reg;
		m_wizfi_reset_reg = data & 0x07;
		if (data & 0x01) // Bit 0: Chip reset
		{
			reset_wizfi();
			// Pin T14 coupling: Setting bit 0 of $FF21 simultaneously resets SAM2695
			m_sam2695_ctrl = 0;
			while (!m_sam2695_rx_fifo.empty())
				m_sam2695_rx_fifo.pop();
		}
		else if (old_reset & 0x01)
		{
			push_wizfi_response("\r\nready\r\nWIFI CONNECTED\r\nWIFI GOT IP\r\n");
		}
		if (data & 0x02) // Bit 1: FIFO reset
		{
			while (!m_wizfi_rx_fifo.empty())
				m_wizfi_rx_fifo.pop();
			m_wizfi_tx_buf.clear();
		}
		break;
	}
	case 0x02:
		if (m_wizfi_transparent)
		{
			if (data == '+')
			{
				m_wizfi_plus_count++;
				if (m_wizfi_plus_count == 3)
				{
					m_wizfi_transparent = false;
					m_wizfi_plus_count = 0;
					return;
				}
			}
			else
			{
				if (m_wizfi_plus_count > 0)
				{
					for (int i = 0; i < m_wizfi_plus_count; i++)
					{
						char p = '+';
						uint32_t written = 0;
						if (m_wizfi_socket)
							m_wizfi_socket->write(&p, 0, 1, written);
					}
					m_wizfi_plus_count = 0;
				}
				if (m_wizfi_socket)
				{
					char c = (char)data;
					uint32_t written = 0;
					m_wizfi_socket->write(&c, 0, 1, written);
				}
			}
		}
		else if (m_wizfi_cipsend_remaining > 0)
		{
			if (m_wizfi_socket)
			{
				char c = (char)data;
				uint32_t written = 0;
				m_wizfi_socket->write(&c, 0, 1, written);
			}
			m_wizfi_cipsend_remaining--;
			if (m_wizfi_cipsend_remaining == 0)
			{
				std::string resp = util::string_format("\r\nRecv %d bytes\r\n\r\nSEND OK\r\n", m_wizfi_cipsend_total);
				push_wizfi_response(resp);
				set_irq(3, 0x20); // INT_WIZFI_TX (Group 3, bit 5: TX FIFO empty)
			}
		}
		else
		{
			if (data == '\r' || data == '\n')
			{
				if (!m_wizfi_tx_buf.empty())
				{
					process_wizfi_cmd(m_wizfi_tx_buf);
					m_wizfi_tx_buf.clear();
					set_irq(3, 0x20); // INT_WIZFI_TX (Group 3, bit 5: TX FIFO empty)
				}
			}
			else
			{
				m_wizfi_tx_buf += (char)data;
			}
		}
		break;
	default:
		break;
	}
}

// Hardware Integer Math Coprocessor ($FEE0 - $FEFB)
uint8_t wildbits_jr2_state::math_r(offs_t offset)
{
	io_wait_read();
	switch (offset)
	{
	// Input registers readback ($FEE0 - $FEEF)
	case 0x00: return (m_math_mulu_a >> 8) & 0xff;
	case 0x01: return m_math_mulu_a & 0xff;
	case 0x02: return (m_math_mulu_b >> 8) & 0xff;
	case 0x03: return m_math_mulu_b & 0xff;
	case 0x04: return (m_math_divu_den >> 8) & 0xff;
	case 0x05: return m_math_divu_den & 0xff;
	case 0x06: return (m_math_divu_num >> 8) & 0xff;
	case 0x07: return m_math_divu_num & 0xff;
	case 0x08: return (m_math_add_a >> 24) & 0xff;
	case 0x09: return (m_math_add_a >> 16) & 0xff;
	case 0x0a: return (m_math_add_a >> 8) & 0xff;
	case 0x0b: return m_math_add_a & 0xff;
	case 0x0c: return (m_math_add_b >> 24) & 0xff;
	case 0x0d: return (m_math_add_b >> 16) & 0xff;
	case 0x0e: return (m_math_add_b >> 8) & 0xff;
	case 0x0f: return m_math_add_b & 0xff;

	// Output result registers ($FEF0 - $FEFB)
	case 0x10: { // MULU_RES HH
		uint32_t prod = (uint32_t)m_math_mulu_a * (uint32_t)m_math_mulu_b;
		return (prod >> 24) & 0xff;
	}
	case 0x11: { // MULU_RES HL
		uint32_t prod = (uint32_t)m_math_mulu_a * (uint32_t)m_math_mulu_b;
		return (prod >> 16) & 0xff;
	}
	case 0x12: { // MULU_RES LH
		uint32_t prod = (uint32_t)m_math_mulu_a * (uint32_t)m_math_mulu_b;
		return (prod >> 8) & 0xff;
	}
	case 0x13: { // MULU_RES LL
		uint32_t prod = (uint32_t)m_math_mulu_a * (uint32_t)m_math_mulu_b;
		return prod & 0xff;
	}
	case 0x14: { // QUOU_RES H
		if (m_math_divu_den == 0) return 0xff;
		return ((m_math_divu_num / m_math_divu_den) >> 8) & 0xff;
	}
	case 0x15: { // QUOU_RES L
		if (m_math_divu_den == 0) return 0xff;
		return (m_math_divu_num / m_math_divu_den) & 0xff;
	}
	case 0x16: { // REMU_RES H
		if (m_math_divu_den == 0) return (m_math_divu_num >> 8) & 0xff;
		return ((m_math_divu_num % m_math_divu_den) >> 8) & 0xff;
	}
	case 0x17: { // REMU_RES L
		if (m_math_divu_den == 0) return m_math_divu_num & 0xff;
		return (m_math_divu_num % m_math_divu_den) & 0xff;
	}
	case 0x18: { // ADD_RES HH
		uint32_t sum = m_math_add_a + m_math_add_b;
		return (sum >> 24) & 0xff;
	}
	case 0x19: { // ADD_RES HL
		uint32_t sum = m_math_add_a + m_math_add_b;
		return (sum >> 16) & 0xff;
	}
	case 0x1a: { // ADD_RES LH
		uint32_t sum = m_math_add_a + m_math_add_b;
		return (sum >> 8) & 0xff;
	}
	case 0x1b: { // ADD_RES LL
		uint32_t sum = m_math_add_a + m_math_add_b;
		return sum & 0xff;
	}
	default:
		return 0;
	}
}

void wildbits_jr2_state::math_w(offs_t offset, uint8_t data)
{
	io_wait_write();
	offset &= 0x0f; // JR_Math_Block.v Address[3:0] decode: result writes alias to operand inputs
	switch (offset)
	{
	case 0x00: m_math_mulu_a = (m_math_mulu_a & 0x00ff) | (data << 8); break;
	case 0x01: m_math_mulu_a = (m_math_mulu_a & 0xff00) | data; break;
	case 0x02: m_math_mulu_b = (m_math_mulu_b & 0x00ff) | (data << 8); break;
	case 0x03: m_math_mulu_b = (m_math_mulu_b & 0xff00) | data; break;
	case 0x04: m_math_divu_den = (m_math_divu_den & 0x00ff) | (data << 8); break;
	case 0x05: m_math_divu_den = (m_math_divu_den & 0xff00) | data; break;
	case 0x06: m_math_divu_num = (m_math_divu_num & 0x00ff) | (data << 8); break;
	case 0x07: m_math_divu_num = (m_math_divu_num & 0xff00) | data; break;
	case 0x08: m_math_add_a = (m_math_add_a & 0x00ffffff) | ((uint32_t)data << 24); break;
	case 0x09: m_math_add_a = (m_math_add_a & 0xff00ffff) | ((uint32_t)data << 16); break;
	case 0x0a: m_math_add_a = (m_math_add_a & 0xffff00ff) | ((uint32_t)data << 8); break;
	case 0x0b: m_math_add_a = (m_math_add_a & 0xffffff00) | (uint32_t)data; break;
	case 0x0c: m_math_add_b = (m_math_add_b & 0x00ffffff) | ((uint32_t)data << 24); break;
	case 0x0d: m_math_add_b = (m_math_add_b & 0xff00ffff) | ((uint32_t)data << 16); break;
	case 0x0e: m_math_add_b = (m_math_add_b & 0xffff00ff) | ((uint32_t)data << 8); break;
	case 0x0f: m_math_add_b = (m_math_add_b & 0xffffff00) | (uint32_t)data; break;
	default: break;
	}
}

// Hardware Floating-Point Unit ($FFE0 - $FFEF: FP_Math_Module)
uint8_t wildbits_jr2_state::fpu_r(offs_t offset)
{
	io_wait_read();

	// $FFE0 - $FFE3: Control registers (CTRL3 is R/W scratch)
	if (offset < 4)
		return m_fpu_ctrl[offset];

	// Extract input 0 (raw float or 20.12 fixed-to-float)
	uint32_t in0_raw = (uint32_t(m_fpu_in0[0]) << 24) |
	                   (uint32_t(m_fpu_in0[1]) << 16) |
	                   (uint32_t(m_fpu_in0[2]) << 8) |
	                   uint32_t(m_fpu_in0[3]);
	float in0_f;
	if (m_fpu_ctrl[0] & 0x01)
	{
		int32_t fix0 = int32_t(in0_raw);
		in0_f = float(fix0) / 4096.0f;
	}
	else
	{
		std::memcpy(&in0_f, &in0_raw, sizeof(float));
	}

	// Extract input 1 (raw float or 20.12 fixed-to-float)
	uint32_t in1_raw = (uint32_t(m_fpu_in1[0]) << 24) |
	                   (uint32_t(m_fpu_in1[1]) << 16) |
	                   (uint32_t(m_fpu_in1[2]) << 8) |
	                   uint32_t(m_fpu_in1[3]);
	float in1_f;
	if (m_fpu_ctrl[0] & 0x02)
	{
		int32_t fix1 = int32_t(in1_raw);
		in1_f = float(fix1) / 4096.0f;
	}
	else
	{
		std::memcpy(&in1_f, &in1_raw, sizeof(float));
	}

	// Multiplier: in0 * in1
	float mul_f = in0_f * in1_f;
	uint8_t mul_st = 0x10; // bit 4: tvalid; bit 3: zero (undriven in RTL, stays 0)
	if (std::isnan(mul_f))
		mul_st |= 0x01;
	else if (std::isinf(mul_f))
		mul_st |= 0x02;

	// Divider: in0 / in1
	float div_f;
	uint8_t div_st = 0x20; // bit 5: tvalid
	if (in1_f == 0.0f)
	{
		div_st |= 0x08; // RTL quirk: AXI core divide-by-zero lands at bit 3 (zero)
		if (in0_f > 0.0f)
			div_f = std::numeric_limits<float>::infinity();
		else if (in0_f < 0.0f)
			div_f = -std::numeric_limits<float>::infinity();
		else
			div_f = std::numeric_limits<float>::quiet_NaN();
	}
	else
	{
		div_f = in0_f / in1_f;
		if (std::isnan(div_f))
			div_st |= 0x01;
		else if (std::isinf(div_f))
			div_st |= 0x02;
	}

	// Adder/Subtractor
	float add_a = in0_f;
	switch ((m_fpu_ctrl[0] >> 4) & 0x03)
	{
	case 0: add_a = in0_f; break;
	case 1: add_a = in1_f; break;
	case 2: add_a = mul_f; break;
	case 3: add_a = div_f; break;
	}

	float add_b = in0_f;
	switch ((m_fpu_ctrl[0] >> 6) & 0x03)
	{
	case 0: add_b = in0_f; break;
	case 1: add_b = in1_f; break;
	case 2: add_b = mul_f; break;
	case 3: add_b = div_f; break;
	}

	float add_f = (m_fpu_ctrl[0] & 0x08) ? (add_a - add_b) : (add_a + add_b);
	uint8_t add_st = 0x10; // bit 4: tvalid
	if (add_f == 0.0f)
		add_st |= 0x08;
	if (std::isnan(add_f))
		add_st |= 0x01;
	else if (std::isinf(add_f))
		add_st |= 0x02;

	// Output Mux (CTRL1 bits 1:0)
	float out_f;
	switch (m_fpu_ctrl[1] & 0x03)
	{
	case 0: out_f = mul_f; break;
	case 1: out_f = div_f; break;
	case 2: out_f = add_f; break;
	case 3:
	default:
		out_f = 1.0f; // Hardwired constant 1.0f ($3F800000)
		break;
	}

	// Status registers & results
	switch (offset)
	{
	case 0x04: return mul_st; // FPMATH_MUL_ST
	case 0x05: return div_st; // FPMATH_DIV_ST
	case 0x06: return add_st; // FPMATH_ADD_ST
	case 0x07: return 0x08;   // FPMATH_CNV_ST (bit 3: tvalid)
	case 0x08:
	case 0x09:
	case 0x0a:
	case 0x0b: // FPMATH_OUT: 32-bit selected float result, big-endian
	{
		uint32_t out_u32;
		std::memcpy(&out_u32, &out_f, sizeof(uint32_t));
		int shift = (3 - (offset - 0x08)) * 8;
		return (out_u32 >> shift) & 0xff;
	}
	case 0x0c:
	case 0x0d:
	case 0x0e:
	case 0x0f: // FPMATH_FIXED: 32-bit 20.12 signed fixed-point conversion, big-endian
	{
		int32_t fix_val = 0;
		if (!std::isnan(out_f) && !std::isinf(out_f))
			fix_val = int32_t(std::round(double(out_f) * 4096.0));
		uint32_t fix_u32 = uint32_t(fix_val);
		int shift = (3 - (offset - 0x0c)) * 8;
		return (fix_u32 >> shift) & 0xff;
	}
	default: break;
	}

	return 0xff;
}

void wildbits_jr2_state::fpu_w(offs_t offset, uint8_t data)
{
	io_wait_write();
	if (offset < 4)
	{
		m_fpu_ctrl[offset] = data;
	}
	else if (offset >= 8 && offset <= 11)
	{
		m_fpu_in0[offset - 8] = data;
	}
	else if (offset >= 12 && offset <= 15)
	{
		m_fpu_in1[offset - 12] = data;
	}
}

// TinyVicky Direct Memory Access (DMA) Controller ($FEC0 - $FEDF)
// On physical hardware, the DMA engine is an external SRAM bus master.
// It directly drives the external 2MB SRAM chip and does not access internal
// FPGA Block RAM (VICKY control, CLUTs, text/color matrices: $C0-$C4).
uint8_t wildbits_jr2_state::dma_read_byte(uint32_t phys_addr)
{
	phys_addr &= 0x1fffff;
	if (phys_addr < 0x1f8000)
		return m_ram[phys_addr];
	return 0xff; // Blocks $FC-$FF (internal FPGA BRAM / Sectored I/O, not on external SRAM bus)
}

void wildbits_jr2_state::dma_write_byte(uint32_t phys_addr, uint8_t data)
{
	phys_addr &= 0x1fffff;
	if (phys_addr < 0x1f8000)
		m_ram[phys_addr] = data;
}

void wildbits_jr2_state::dma_execute()
{
	bool is_2d = (m_dma_reg[0] & 0x02) != 0;
	bool is_fill = (m_dma_reg[0] & 0x04) != 0;
	bool int_en = (m_dma_reg[0] & 0x08) != 0;
	bool double_speed = (m_dma_reg[0] & 0x40) != 0;

	// Byte-lane mask: bits 5:4 (bit 4 = even lane / LSB, bit 5 = odd lane / MSB)
	bool mask_even = (m_dma_reg[0] & 0x10) != 0;
	bool mask_odd = (m_dma_reg[0] & 0x20) != 0;

	if (mask_even && mask_odd)
	{
		m_dma_status = 0x00;
		if (int_en)
			set_irq(0, 0x40);
		return;
	}

	// Big-Endian 24-bit physical addresses:
	// Source: $FEC5 (H), $FEC6 (M), $FEC7 (L) [$FEC4 is unused]
	// Dest:   $FEC9 (H), $FECA (M), $FECB (L) [$FEC8 is unused]
	uint32_t src = ((uint32_t)m_dma_reg[5] << 16) | ((uint32_t)m_dma_reg[6] << 8) | m_dma_reg[7];
	uint32_t dst = ((uint32_t)m_dma_reg[9] << 16) | ((uint32_t)m_dma_reg[10] << 8) | m_dma_reg[11];
	uint8_t fill_byte = m_dma_reg[1];

	// RC17/RC20 DMA logic operations ($FED4)
	uint8_t op_reg = m_dma_reg[20];
	uint8_t op = op_reg & 0x07;
	bool op_not = (op_reg & 0x08) != 0;
	bool has_logic_op = (op != 0) || op_not;

	auto apply_op = [op, op_not](uint8_t s, uint8_t d) -> uint8_t {
		uint8_t r;
		switch (op)
		{
		case 0: // COPY
			r = s;
			break;
		case 1: // OR
			r = s | d;
			break;
		case 2: // AND
			r = s & d;
			break;
		case 3: // XOR
			r = s ^ d;
			break;
		case 4: // MASK (per-nibble: source nibble != 0 retains source nibble, else destination nibble)
		{
			uint8_t hi = (s & 0xf0) ? (s & 0xf0) : (d & 0xf0);
			uint8_t lo = (s & 0x0f) ? (s & 0x0f) : (d & 0x0f);
			r = hi | lo;
			break;
		}
		default:
			r = s;
			break;
		}
		if (op_not)
			r = ~r;
		return r;
	};

	m_dma_status = 0x80; // Transfer in progress

	int cycles = 0;

	if (!is_2d)
	{
		// 1D Linear Transfer: length {m_dma_reg[15], m_dma_reg[12], m_dma_reg[13]} masked to 17 bits
		uint32_t count = (((uint32_t)m_dma_reg[15] << 16) | ((uint32_t)m_dma_reg[12] << 8) | m_dma_reg[13]) & 0x1ffff;
		if (count > 0)
		{
			if (is_fill)
			{
				if (double_speed)
				{
					for (uint32_t i = 0; i < count; i += 2)
					{
						uint32_t d_addr = (dst + i) & 0x1ffffe;
						if (!mask_even)
						{
							uint8_t fb = m_dma_reg[2];
							if (has_logic_op)
								dma_write_byte(d_addr, apply_op(fb, dma_read_byte(d_addr)));
							else
								dma_write_byte(d_addr, fb);
						}
						if (!mask_odd && (i + 1 < count))
						{
							uint8_t fb = m_dma_reg[3];
							if (has_logic_op)
								dma_write_byte(d_addr + 1, apply_op(fb, dma_read_byte(d_addr + 1)));
							else
								dma_write_byte(d_addr + 1, fb);
						}
					}
					cycles = has_logic_op ? (count / 12) : (count / 32);
				}
				else
				{
					for (uint32_t i = 0; i < count; i++)
					{
						uint32_t d_addr = (dst + i) & 0x1fffff;
						bool masked = (d_addr & 1) ? mask_odd : mask_even;
						if (!masked)
						{
							if (has_logic_op)
							{
								uint8_t d_val = dma_read_byte(d_addr);
								dma_write_byte(d_addr, apply_op(fill_byte, d_val));
							}
							else
							{
								dma_write_byte(d_addr, fill_byte);
							}
						}
					}
					cycles = has_logic_op ? (count / 6) : (count / 16);
				}
			}
			else
			{
				if (double_speed)
				{
					for (uint32_t i = 0; i < count; i += 2)
					{
						uint32_t s_addr = (src + i) & 0x1ffffe;
						uint32_t d_addr = (dst + i) & 0x1ffffe;
						if (!mask_even)
						{
							uint8_t s_val = dma_read_byte(s_addr);
							if (has_logic_op)
								dma_write_byte(d_addr, apply_op(s_val, dma_read_byte(d_addr)));
							else
								dma_write_byte(d_addr, s_val);
						}
						if (!mask_odd && (i + 1 < count))
						{
							uint8_t s_val = dma_read_byte(s_addr + 1);
							if (has_logic_op)
								dma_write_byte(d_addr + 1, apply_op(s_val, dma_read_byte(d_addr + 1)));
							else
								dma_write_byte(d_addr + 1, s_val);
						}
					}
					cycles = has_logic_op ? (count / 4) : (count / 10);
				}
				else
				{
					for (uint32_t i = 0; i < count; i++)
					{
						uint32_t s_addr = (src + i) & 0x1fffff;
						uint32_t d_addr = (dst + i) & 0x1fffff;
						bool masked = (d_addr & 1) ? mask_odd : mask_even;
						if (!masked)
						{
							uint8_t s_val = dma_read_byte(s_addr);
							if (has_logic_op)
							{
								uint8_t d_val = dma_read_byte(d_addr);
								dma_write_byte(d_addr, apply_op(s_val, d_val));
							}
							else
							{
								dma_write_byte(d_addr, s_val);
							}
						}
					}
					cycles = has_logic_op ? (count / 2) : (count / 5);
				}
			}
		}
	}
	else
	{
		// 2D Rectangular Block Transfer
		uint16_t width = ((uint16_t)m_dma_reg[12] << 8) | m_dma_reg[13];
		uint16_t height = ((uint16_t)m_dma_reg[14] << 8) | m_dma_reg[15];
		uint16_t src_stride = ((uint16_t)m_dma_reg[16] << 8) | m_dma_reg[17];
		uint16_t dst_stride = ((uint16_t)m_dma_reg[18] << 8) | m_dma_reg[19];

		for (uint16_t y = 0; y < height; y++)
		{
			uint32_t row_src = (src + (uint32_t)y * src_stride) & 0x1fffff;
			uint32_t row_dst = (dst + (uint32_t)y * dst_stride) & 0x1fffff;

			if (is_fill)
			{
				uint16_t step = double_speed ? 2 : 1;
				for (uint16_t x = 0; x < width; x += step)
				{
					if (double_speed)
					{
						uint32_t d_addr = (row_dst + x) & 0x1ffffe;
						if (!mask_even)
						{
							uint8_t fb = m_dma_reg[2];
							if (has_logic_op)
								dma_write_byte(d_addr, apply_op(fb, dma_read_byte(d_addr)));
							else
								dma_write_byte(d_addr, fb);
						}
						if (!mask_odd && (x + 1 < width))
						{
							uint8_t fb = m_dma_reg[3];
							if (has_logic_op)
								dma_write_byte(d_addr + 1, apply_op(fb, dma_read_byte(d_addr + 1)));
							else
								dma_write_byte(d_addr + 1, fb);
						}
					}
					else
					{
						uint32_t d_addr = (row_dst + x) & 0x1fffff;
						bool masked = (d_addr & 1) ? mask_odd : mask_even;
						if (!masked)
						{
							if (has_logic_op)
								dma_write_byte(d_addr, apply_op(fill_byte, dma_read_byte(d_addr)));
							else
								dma_write_byte(d_addr, fill_byte);
						}
					}
				}
			}
			else
			{
				uint16_t step = double_speed ? 2 : 1;
				for (uint16_t x = 0; x < width; x += step)
				{
					if (double_speed)
					{
						uint32_t s_addr = (row_src + x) & 0x1ffffe;
						uint32_t d_addr = (row_dst + x) & 0x1ffffe;
						if (!mask_even)
						{
							uint8_t s_val = dma_read_byte(s_addr);
							if (has_logic_op)
								dma_write_byte(d_addr, apply_op(s_val, dma_read_byte(d_addr)));
							else
								dma_write_byte(d_addr, s_val);
						}
						if (!mask_odd && (x + 1 < width))
						{
							uint8_t s_val = dma_read_byte(s_addr + 1);
							if (has_logic_op)
								dma_write_byte(d_addr + 1, apply_op(s_val, dma_read_byte(d_addr + 1)));
							else
								dma_write_byte(d_addr + 1, s_val);
						}
					}
					else
					{
						uint32_t s_addr = (row_src + x) & 0x1fffff;
						uint32_t d_addr = (row_dst + x) & 0x1fffff;
						bool masked = (d_addr & 1) ? mask_odd : mask_even;
						if (!masked)
						{
							uint8_t s_val = dma_read_byte(s_addr);
							if (has_logic_op)
								dma_write_byte(d_addr, apply_op(s_val, dma_read_byte(d_addr)));
							else
								dma_write_byte(d_addr, s_val);
						}
					}
				}
			}
		}

		uint32_t total_bytes = (uint32_t)width * height;
		if (double_speed)
			cycles = is_fill ? (has_logic_op ? (total_bytes / 12) : (total_bytes / 32)) : (has_logic_op ? (total_bytes / 4) : (total_bytes / 10));
		else
			cycles = is_fill ? (has_logic_op ? (total_bytes / 6) : (total_bytes / 16)) : (has_logic_op ? (total_bytes / 2) : (total_bytes / 5));
	}

	if (cycles > 0)
	{
		m_maincpu->eat_cycles(cycles);
	}

	m_dma_status = 0x00; // Transfer complete

	if (int_en)
	{
		set_irq(0, 0x40); // INT_DMA0 (Group 0, bit 6)
	}
}

uint8_t wildbits_jr2_state::dma_r(offs_t offset)
{
	io_wait_read();
	switch (offset)
	{
	case 0: return m_dma_reg[0] & 0x7f; // Bit 7 Start_Trf is write-only, reads as 0
	case 1: return m_dma_status;        // Bit 7: TRF_IP, bits 6:0: 0
	case 2: return m_dma_reg[2];
	case 3: return m_dma_reg[3];
	case 4: return m_dma_reg[7];
	case 5: return m_dma_reg[6];
	case 6: return m_dma_reg[5];
	case 7: return m_dma_reg[4];
	case 8: return m_dma_reg[11];
	case 9: return m_dma_reg[10];
	case 10: return m_dma_reg[9];
	case 11: return m_dma_reg[8];
	case 12: return m_dma_reg[13];
	case 13: return m_dma_reg[12];
	case 14: return m_dma_reg[15];
	case 15: return m_dma_reg[14];
	case 16: return m_dma_reg[17];
	case 17: return m_dma_reg[16];
	case 18: return m_dma_reg[19];
	case 19: return m_dma_reg[18];
	case 20: return (m_dma_reg[20] & 0x7f) | 0x80; // Bit 7: DMA_OP_Implemented
	case 21: return m_dma_reg[21];
	case 22: return m_dma_reg[22];
	case 23: return m_dma_reg[23];
	default: return 0xff; // $FED8 - $FEDF dead hole returns $FF
	}
}

void wildbits_jr2_state::dma_w(offs_t offset, uint8_t data)
{
	io_wait_write();
	if (offset == 0)
	{
		// Hardware state machine:
		// Starting a transfer requires a rising edge on bit 7 (Start_Trf: 0 -> 1)
		// and bit 0 (DMA_Enable) must be set.
		// Software must clear bit 7 back to 0 before starting another transfer.
		bool rising_start = (data & 0x80) && !(m_dma_reg[0] & 0x80) && (data & 0x01);
		m_dma_reg[0] = data;
		if (rising_start)
		{
			dma_execute();
		}
	}
	else if (offset == 20)
	{
		// DMA_OP_REG: all bits writable (bit 7 forced to 1 on readback in hardware)
		m_dma_reg[20] = data;
	}
	else if (offset < 24)
	{
		m_dma_reg[offset] = data;
	}
}

// TinyVicky Master Registers ($FFC0 - $FFDF)
uint8_t wildbits_jr2_state::vky_r(offs_t offset)
{
	io_wait_read();
	switch (offset)
	{
	case 0x00: return m_vky_mstr_ctrl_0;
	case 0x01: return m_vky_mstr_ctrl_1;
	case 0x02: return m_vky_layer_ctrl_0;
	case 0x03: return m_vky_layer_ctrl_1;
	case 0x04: return m_vky_brdr_ctrl;
	case 0x05: return m_vky_brdr_b;
	case 0x06: return m_vky_brdr_g;
	case 0x07: return m_vky_brdr_r;
	case 0x08: return m_vky_brdr_w;
	case 0x09: return m_vky_brdr_h;
	case 0x0a: return m_vky_drawline_ctrl;
	case 0x0b: return m_vky_gfx_mode;
	case 0x0d: return m_vky_bg_b;
	case 0x0e: return m_vky_bg_g;
	case 0x0f: return m_vky_bg_r;
	case 0x10: return m_vky_crsr_ctrl;
	case 0x12: return m_vky_crsr_char;
	case 0x13: return m_vky_crsr_color;
	case 0x14: return m_vky_crsr_x & 0xff; // Hardware readback: Low byte at 0x14
	case 0x15: return m_vky_crsr_x >> 8;   // Hardware readback: High byte at 0x15
	case 0x16: return m_vky_crsr_y & 0xff; // Hardware readback: Low byte at 0x16
	case 0x17: return m_vky_crsr_y >> 8;   // Hardware readback: High byte at 0x17
	case 0x18: return m_screen->hpos() >> 8;   // RAST_COL_H
	case 0x19: return m_screen->hpos() & 0xff; // RAST_COL_L
	case 0x1a: return m_screen->vpos() >> 8;   // RAST_ROW_H
	case 0x1b: return m_screen->vpos() & 0xff; // RAST_ROW_L
	default: return 0;
	}
}

void wildbits_jr2_state::vky_w(offs_t offset, uint8_t data)
{
	// TURBO_FASTIOWRITE: 24-tick fast I/O write frame (~8.39 MHz)
	io_wait_write();
	if (offset < 0x20)
		m_vram_fc[offset] = data;
	switch (offset)
	{
	case 0x00:
		if (m_vky_mstr_ctrl_0 != data)
		{
			m_screen->update_partial(m_screen->vpos());
			m_vky_mstr_ctrl_0 = data;
		}
		break;
	case 0x01:
		if (m_vky_mstr_ctrl_1 != data)
		{
			m_screen->update_partial(m_screen->vpos());
			m_vky_mstr_ctrl_1 = data;
		}
		break;
	case 0x02: m_vky_layer_ctrl_0 = data; break;
	case 0x03: m_vky_layer_ctrl_1 = data; break;
	case 0x04: m_vky_brdr_ctrl = data; break;
	case 0x05: m_vky_brdr_b = data; break;
	case 0x06: m_vky_brdr_g = data; break;
	case 0x07: m_vky_brdr_r = data; break;
	case 0x08: m_vky_brdr_w = data & 0x1f; break;
	case 0x09: m_vky_brdr_h = data & 0x1f; break;
	case 0x0a: m_vky_drawline_ctrl = data; break;
	case 0x0b: m_vky_gfx_mode = data & 0x0f; break; // HIRES4 mode & CLUT group
	case 0x0d: m_vky_bg_b = data; break;
	case 0x0e: m_vky_bg_g = data; break;
	case 0x0f: m_vky_bg_r = data; break;
	case 0x10: m_vky_crsr_ctrl = data; break;
	case 0x12: m_vky_crsr_char = data; break;
	case 0x13: m_vky_crsr_color = data; break;
	case 0x14: m_vky_crsr_x = (m_vky_crsr_x & 0x00ff) | (data << 8); break;
	case 0x15: m_vky_crsr_x = (m_vky_crsr_x & 0xff00) | data; break;
	case 0x16: m_vky_crsr_y = (m_vky_crsr_y & 0x00ff) | (data << 8); break;
	case 0x17: m_vky_crsr_y = (m_vky_crsr_y & 0xff00) | data; break;
	case 0x18:
		m_vky_lint_ctrl = data;
		update_line_timer();
		break;
	case 0x19:
		m_vky_line_cmp = (m_vky_line_cmp & 0x00ff) | (data << 8); // LINE_CMP_H
		update_line_timer();
		break;
	case 0x1a:
		m_vky_line_cmp = (m_vky_line_cmp & 0xff00) | data;        // LINE_CMP_L
		update_line_timer();
		break;
	default: break;
	}
}

void wildbits_jr2_state::linedraw_execute()
{
	int bm_sel = (m_ld_ctrl >> 2) & 0x03;
	uint8_t bm_ctrl = (bm_sel == 0) ? m_vram_fc[0x1000] : (bm_sel == 1) ? m_vram_fc[0x1008] : (bm_sel == 2) ? m_vram_fc[0x1010] : 0;
	bool hires4 = (m_vky_gfx_mode & 0x01) || (bm_ctrl & 0x10);
	uint16_t xmax = hires4 ? 640 : 320;

	if (m_ld_x0 >= xmax || m_ld_x1 >= xmax || m_ld_y0 >= 240 || m_ld_y1 >= 240)
	{
		// Coordinate checks: X < 320 (or < 640 in HIRES4), Y < 240.
		// If out of bounds, trigger ignored and DONE never asserts.
		return;
	}

	uint32_t bm_base = 0;
	if (bm_sel == 0)
		bm_base = ((uint32_t)m_vram_fc[0x1001] << 16) | ((uint32_t)m_vram_fc[0x1002] << 8) | m_vram_fc[0x1003];
	else if (bm_sel == 1)
		bm_base = ((uint32_t)m_vram_fc[0x1009] << 16) | ((uint32_t)m_vram_fc[0x100a] << 8) | m_vram_fc[0x100b];
	else if (bm_sel == 2)
		bm_base = ((uint32_t)m_vram_fc[0x1011] << 16) | ((uint32_t)m_vram_fc[0x1012] << 8) | m_vram_fc[0x1013];
	else
		bm_base = 0x001000;

	bool enabled = (m_vky_drawline_ctrl & 0x01) || (m_ld_ctrl & 0x01);

	auto plot_pixel = [&](int x, int y) {
		if (!enabled)
			return;
		if (hires4)
		{
			uint32_t addr = (bm_base + y * 320 + (x / 2)) & 0x1fffff;
			if (x & 1)
				m_ram[addr] = (m_ram[addr] & 0xf0) | (m_ld_color & 0x0f);
			else
				m_ram[addr] = (m_ram[addr] & 0x0f) | ((m_ld_color & 0x0f) << 4);
		}
		else
		{
			uint32_t addr = (bm_base + y * 320 + x) & 0x1fffff;
			m_ram[addr] = m_ld_color;
		}
	};

	int x0 = m_ld_x0;
	int y0 = m_ld_y0;
	int x1 = m_ld_x1;
	int y1 = m_ld_y1;

	int dx = std::abs(x1 - x0);
	int dy = std::abs(y1 - y0);
	int sx = (x0 < x1) ? 1 : -1;
	int sy = (y0 < y1) ? 1 : -1;
	int err = dx - dy;

	int pixels = 0;
	while (true)
	{
		plot_pixel(x0, y0);
		pixels++;
		if (x0 == x1 && y0 == y1)
			break;
		int e2 = 2 * err;
		if (e2 > -dy)
		{
			err -= dy;
			x0 += sx;
		}
		if (e2 < dx)
		{
			err += dx;
			y0 += sy;
		}
	}

	// Model HoldLine CPU arbitration during line-draw bursts
	if (pixels > 0)
		m_maincpu->eat_cycles(pixels);

	m_ld_done = true;
	m_ld_fifo_count = 0;
}

void wildbits_jr2_state::update_line_timer()
{
	if ((m_vky_lint_ctrl & 0x01) && m_vky_line_cmp < 525)
	{
		m_scanline_timer->adjust(m_screen->time_until_pos(m_vky_line_cmp, 0));
	}
	else
	{
		m_scanline_timer->adjust(attotime::never);
	}
}

TIMER_CALLBACK_MEMBER(wildbits_jr2_state::scanline_tick)
{
	set_irq(0, 0x02); // INT_VKY_SOL (Group 0, Bit 1)
	if ((m_vky_lint_ctrl & 0x01) && m_vky_line_cmp < 525)
	{
		m_scanline_timer->adjust(m_screen->time_until_pos(m_vky_line_cmp, 0));
	}
	else
	{
		m_scanline_timer->adjust(attotime::never);
	}
}

// PCB ID and TinyVicky Chip Version ($FE08 - $FE0F)
uint8_t wildbits_jr2_state::pcbid_r(offs_t offset)
{
	io_wait_read();
	static const uint8_t s_id_ver[8] = {
		'A', '0',       // $FE08-$FE09: PCBID ("A0")
		0x11, 0x00,     // $FE0A-$FE0B: CHIP_SUBVERSION ($0011 = 17)
		0x00, 0x02,     // $FE0C-$FE0D: CHIP_VERSION ($0200)
		0x09, 0x95      // $FE0E-$FE0F: CHIP_NUMBER ($9509)
	};
	return s_id_ver[offset & 7];
}

// Optical Keyboard Decode ($FE10 - $FE1F)
uint8_t wildbits_jr2_state::optkbd_r(offs_t offset)
{
	io_wait_read();
	return 0x55;
}

// Consolidated Fixed I/O Sound Registers ($FF91 - $FF99)
uint8_t wildbits_jr2_state::sound_fixed_r(offs_t offset)
{
	io_wait_read();
	if (offset == 7) // $FF98: SID selector
		return m_sid_select & 0x7f;
	return 0xff;
}

void wildbits_jr2_state::sound_fixed_w(offs_t offset, uint8_t data)
{
	io_wait_write();
	switch (offset)
	{
	case 0: // $FF91: Left PSG
		m_psg_l->write(data);
		break;
	case 1: // $FF92: Both / Mono PSG
		m_psg_l->write(data);
		m_psg_r->write(data);
		break;
	case 2: // $FF93: Right PSG
		m_psg_r->write(data);
		break;
	case 3: // $FF94: OPL3 Bank 0 Address
		m_opl3->write(0, data);
		break;
	case 4: // $FF95: OPL3 Bank 0 Data
		m_opl3->write(1, data);
		break;
	case 5: // $FF96: OPL3 Bank 1 Address
		m_opl3->write(2, data);
		break;
	case 6: // $FF97: OPL3 Bank 1 Data
		m_opl3->write(3, data);
		break;
	case 7: // $FF98: SID Selector
		m_sid_select = data & 0x7f;
		break;
	case 8: // $FF99: SID Data Write
	{
		uint8_t reg = m_sid_select & 0x1f;
		uint8_t chip = (m_sid_select >> 5) & 0x03;
		if (chip == 0) // Left
			m_sid_l->write(reg, data);
		else if (chip == 1) // Right
			m_sid_r->write(reg, data);
		else if (chip == 2) // Both
		{
			m_sid_l->write(reg, data);
			m_sid_r->write(reg, data);
		}
		break;
	}
	}
}

// Hardware Mouse Cursor ($FEA0 - $FEA8)
uint8_t wildbits_jr2_state::mouse_r(offs_t offset)
{
	io_wait_read();
	switch (offset)
	{
	case 0x00: return m_mouse_men;
	case 0x01: return 0x00;
	case 0x02: return (m_mouse_x >> 8) & 0xff;
	case 0x03: return m_mouse_x & 0xff;
	case 0x04: return (m_mouse_y >> 8) & 0xff;
	case 0x05: return m_mouse_y & 0xff;
	case 0x06: return m_mouse_bytes[0];
	case 0x07: return m_mouse_bytes[1];
	case 0x08: return m_mouse_bytes[2];
	default: return 0x00;
	}
}

void wildbits_jr2_state::mouse_w(offs_t offset, uint8_t data)
{
	io_wait_write();
	switch (offset)
	{
	case 0x00: m_mouse_men = data; break;
	case 0x01: break;
	case 0x02: m_mouse_x = (m_mouse_x & 0x00ff) | (data << 8); break;
	case 0x03: m_mouse_x = (m_mouse_x & 0xff00) | data; break;
	case 0x04: m_mouse_y = (m_mouse_y & 0x00ff) | (data << 8); break;
	case 0x05: m_mouse_y = (m_mouse_y & 0xff00) | data; break;
	case 0x06: m_mouse_bytes[0] = data; break;
	case 0x07: m_mouse_bytes[1] = data; break;
	case 0x08: m_mouse_bytes[2] = data; break;
	}
}

// SAM2695 MIDI Synth ($FF30 - $FF39)
uint8_t wildbits_jr2_state::sam2695_r(offs_t offset)
{
	io_wait_read();
	switch (offset)
	{
	case 0x00: {
		// Edition 3 Control Register ($FF30):
		// Bit 3 = Tx_empty, Bit 2 = Rx_empty
		uint8_t ctrl = m_sam2695_ctrl & ~0x0c;
		ctrl |= 0x08; // Tx_empty (writes dispatch immediately to m_midi_out)
		if (m_sam2695_rx_fifo.empty())
			ctrl |= 0x04; // Rx_empty
		return ctrl;
	}
	case 0x01: {
		// MIDI FIFO data port ($FF31): pop Rx FIFO
		if (!m_sam2695_rx_fifo.empty())
		{
			uint8_t val = m_sam2695_rx_fifo.front();
			m_sam2695_rx_fifo.pop();
			return val;
		}
		return 0x00;
	}
	case 0x02: return 0x00; // Rx RD cnt hi
	case 0x03: return 0x00; // Rx RD cnt lo
	case 0x04: return (m_sam2695_rx_fifo.size() >> 8) & 0x07; // Rx WR cnt hi
	case 0x05: return m_sam2695_rx_fifo.size() & 0xff;        // Rx WR cnt lo
	case 0x06: return 0x00; // Tx RD cnt hi
	case 0x07: return 0x00; // Tx RD cnt lo
	case 0x08: return 0x00; // Tx WR cnt hi
	case 0x09: return 0x00; // Tx WR cnt lo
	default: return 0x55;
	}
}

void wildbits_jr2_state::sam2695_w(offs_t offset, uint8_t data)
{
	io_wait_write();
	switch (offset)
	{
	case 0x00:
		m_sam2695_ctrl = data;
		if (data & 0x02) // Bit 1 = FIFO_Reset
		{
			while (!m_sam2695_rx_fifo.empty())
				m_sam2695_rx_fifo.pop();
		}
		break;
	case 0x01:
		if (m_midi_out)
			m_midi_out->write(data);
		break;
	default:
		break;
	}
}

// VS1053b Audio Decoder & SPI Bridge ($FF50 - $FF5F)
uint8_t wildbits_jr2_state::vs1053_r(offs_t offset)
{
	io_wait_read();
	switch (offset & 7)
	{
	case 0:
		// $FF50: VS_CTRL (bit 7 = BUSY, bits 3..0 = CTRL state)
		return m_vs_ctrl & 0x0f;
	case 1:
		// $FF51: VS_SCIREG
		return m_vs_scireg & 0x0f;
	case 2:
		// $FF52: VS_DATAHI
		return m_vs_data_hi;
	case 3:
		// $FF53: VS_DATALO
		return m_vs_data_lo;
	case 4:
	{
		// $FF54: VS_FIFOSTAT (bit 7 = Empty, bit 6 = Full, bits 2..0 = count 10..8)
		uint16_t count = 0;
		if (m_vs_playing_smf)
		{
			uint32_t buffered = (m_vs_sdi_buf.size() > m_vs_smf_pos) ? (m_vs_sdi_buf.size() - m_vs_smf_pos) : 0;
			if (buffered == 0)
				count = 0;
			else if (buffered >= 2048)
				count = 2040 - (m_vs_smf_pos % 128);
			else
				count = (uint16_t)buffered;
		}
		else if (m_vs_decoding_audio)
		{
			if (m_pcm_fifo_l.empty() && m_vs_sdi_buf.empty())
			{
				count = 0;
			}
			else if (!m_vs_sdi_buf.empty())
			{
				uint32_t queued = m_vs_sdi_buf.size();
				count = (queued >= 2048) ? 2048 : (uint16_t)queued;
			}
			else
			{
				count = 1;
			}
		}
		uint8_t stat = 0;
		if (count == 0) stat |= 0x80; // Empty
		if (count >= 2048) stat |= 0x40; // Full
		stat |= ((count >> 8) & 0x07);
		return stat;
	}
	case 5:
	{
		// $FF55: VS_FIFOCNTL (count 7..0)
		uint16_t count = 0;
		if (m_vs_playing_smf)
		{
			uint32_t buffered = (m_vs_sdi_buf.size() > m_vs_smf_pos) ? (m_vs_sdi_buf.size() - m_vs_smf_pos) : 0;
			if (buffered == 0)
				count = 0;
			else if (buffered >= 2048)
				count = 2040 - (m_vs_smf_pos % 128);
			else
				count = (uint16_t)buffered;
		}
		else if (m_vs_decoding_audio)
		{
			if (m_pcm_fifo_l.empty() && m_vs_sdi_buf.empty())
			{
				count = 0;
			}
			else if (!m_vs_sdi_buf.empty())
			{
				uint32_t queued = m_vs_sdi_buf.size();
				count = (queued >= 2048) ? 2048 : (uint16_t)queued;
			}
			else
			{
				count = 1;
			}
		}
		return count & 0xff;
	}
	case 6:
		return 0x00;
	case 7:
		// $FF57: VS_FIFO (SDI stream data port write-only)
		return 0x00;
	default:
		return 0x00;
	}
}

void wildbits_jr2_state::vs1053_w(offs_t offset, uint8_t data)
{
	io_wait_write();
	switch (offset & 7)
	{
	case 0: // $FF50: VS_CTRL
	{
		uint8_t old_ctrl = m_vs_ctrl;
		m_vs_ctrl = data & 0x0f;
		if (m_vs_ctrl & 0x08) // VS_RESET: hold chip XRESET low, restore boot defaults
		{
			vs_stop_smf();
			vs_stop_audio();
			memset(m_vs_sci, 0, sizeof(m_vs_sci));
			m_vs_sci[0] = 0x0800; // MODE: SM_SDINEW
			m_vs_sci[1] = 0x0048; // STATUS: VS1053b (version 4) + SS_APDOWN2 (analog powerdown)
			m_vs_sci[5] = 0x1f40; // AUDATA: 8000 Hz, mono
			m_vs_wram_addr = 0;
			m_sdi_memtest_idx = 0;
		}
		else if (!(old_ctrl & 0x01) && (data & 0x01)) // Rising edge on VS_START
		{
			uint8_t reg = m_vs_scireg & 0x0f;
			if (data & 0x02) // SCI READ
			{
				uint16_t val = 0;
				if (reg == 6) // VS_WRAM
				{
					if (m_vs_wram_addr == 0xc017) val = 0x0000; // GPIO_DDR
					else if (m_vs_wram_addr == 0xc018) val = 0x0000; // GPIO_IDATA (unstrapped: GPIO0=0, GPIO1=0)
					else if (m_vs_wram_addr == 0xc019) val = 0x0000; // GPIO_ODATA
					else if (m_vs_wram_addr < 0x4000) val = m_vs_wram[m_vs_wram_addr];
					else val = 0;
					m_vs_wram_addr++;
				}
				else
				{
					val = m_vs_sci[reg];
				}
				m_vs_data_hi = (val >> 8) & 0xff;
				m_vs_data_lo = val & 0xff;
			}
			else // SCI WRITE
			{
				uint16_t val = ((uint16_t)m_vs_data_hi << 8) | m_vs_data_lo;
				if (reg == 7) // VS_WRAMADDR
				{
					m_vs_wram_addr = val;
				}
				else if (reg == 6) // VS_WRAM
				{
					if (m_vs_wram_addr < 0x4000)
						m_vs_wram[m_vs_wram_addr] = val;
					m_vs_wram_addr++;
				}
				else
				{
					m_vs_sci[reg] = val;
					if (reg == 0) // VS_MODE
					{
						if (val & 0x04) // SM_RESET
						{
							vs_stop_smf();
							vs_stop_audio();
							m_vs_sci[0] = 0x0800;
							m_vs_sci[1] = 0x0048;
							m_vs_sci[5] = 0x1f40;
							m_vs_sci[8] = 0;
							m_vs_sci[10] = 0;
							m_vs_wram_addr = 0;
						}
						if (val & 0x08) // SM_CANCEL
						{
							vs_stop_smf();
							vs_stop_audio();
							m_vs_sci[0] &= ~0x08; // clear cancel flag when done
						}
					}
					else if (reg == 10) // VS_AIADDR
					{
						if (val == 0x0050) // RT-MIDI start plugin
						{
							m_vs_sci[5] = 0xac45; // 44100 Hz stereo MIDI synth mode
						}
					}
				}
			}
		}
		break;
	}
	case 1: // $FF51: VS_SCIREG
		m_vs_scireg = data & 0x0f;
		break;
	case 2: // $FF52: VS_DATAHI
		m_vs_data_hi = data;
		break;
	case 3: // $FF53: VS_DATALO
		m_vs_data_lo = data;
		break;
	case 7: // $FF57: VS_FIFO (SDI stream data)
	{
		static const uint8_t s_memtest[8] = { 0x4D, 0xEA, 0x6D, 0x54, 0x00, 0x00, 0x00, 0x00 };
		if (data == s_memtest[m_sdi_memtest_idx])
		{
			m_sdi_memtest_idx++;
			if (m_sdi_memtest_idx == 8)
			{
				m_vs_sci[8] = 0x83ff; // HDAT0 memory test passed
				m_sdi_memtest_idx = 0;
			}
		}
		else
		{
			m_sdi_memtest_idx = (data == s_memtest[0]) ? 1 : 0;
		}

		if (m_vs_sci[10] == 0x0050) // RT-MIDI mode (VS_AIADDR == 0x0050)
		{
			if (m_midi_out)
				m_midi_out->write(data);
		}
		else
		{
			// Stream data handling
			m_vs_sdi_buf.push_back(data);

			if (m_vs_playing_smf)
			{
				if (m_vs_smf_waiting_data)
				{
					if (m_vs_smf_pos == 22)
					{
						uint32_t save_pos = m_vs_smf_pos;
						uint32_t delta = 0;
						bool ok = false;
						while (m_vs_smf_pos < m_vs_sdi_buf.size())
						{
							uint8_t b = m_vs_sdi_buf[m_vs_smf_pos++];
							delta = (delta << 7) | (b & 0x7f);
							if (!(b & 0x80))
							{
								ok = true;
								break;
							}
						}
						if (ok)
						{
							m_vs_smf_waiting_data = false;
							m_vs_smf_retry_count = 0;
							double dt = (double)delta * ((double)m_vs_smf_tempo_us / 1000000.0) / (double)m_vs_smf_division;
							m_vs_decode_seconds += dt;
							m_vs_sci[4] = (uint16_t)m_vs_decode_seconds;
							if (m_vs_midi_timer)
								m_vs_midi_timer->adjust(attotime::from_double(dt));
						}
						else
						{
							m_vs_smf_pos = save_pos;
						}
					}
					else
					{
						m_vs_smf_waiting_data = false;
						m_vs_smf_retry_count = 0;
						if (m_vs_midi_timer)
							m_vs_midi_timer->adjust(attotime::zero);
					}
				}
			}
			else if (m_vs_decoding_audio)
			{
				if ((m_audio_format == FMT_WAV && !m_wav_header_parsed) || m_vs_sdi_buf.size() >= 512)
					vs_decode_audio();
			}
			else
			{
				// Format detection
				if (m_vs_sdi_buf.size() >= 22 &&
				    m_vs_sdi_buf[0] == 'M' && m_vs_sdi_buf[1] == 'T' &&
				    m_vs_sdi_buf[2] == 'h' && m_vs_sdi_buf[3] == 'd' &&
				    m_vs_sdi_buf[14] == 'M' && m_vs_sdi_buf[15] == 'T' &&
				    m_vs_sdi_buf[16] == 'r' && m_vs_sdi_buf[17] == 'k')
				{
					m_vs_smf_division = ((uint32_t)m_vs_sdi_buf[12] << 8) | m_vs_sdi_buf[13];
					if (m_vs_smf_division == 0)
						m_vs_smf_division = 480;
					m_vs_smf_tempo_us = 500000;
					m_vs_smf_pos = 22; // Start of track events
					m_vs_smf_running_status = 0;
					m_vs_playing_smf = true;
					m_vs_smf_waiting_data = false;
					m_vs_smf_retry_count = 0;
					m_vs_sci[9] = 0xffe0; // HDAT1 active
					m_vs_sci[4] = 0;      // DECODE_TIME = 0
					m_vs_decode_seconds = 0.0;

					// Read initial delta-time
					uint32_t save_pos = m_vs_smf_pos;
					uint32_t delta = 0;
					bool ok = false;
					while (m_vs_smf_pos < m_vs_sdi_buf.size())
					{
						uint8_t b = m_vs_sdi_buf[m_vs_smf_pos++];
						delta = (delta << 7) | (b & 0x7f);
						if (!(b & 0x80))
						{
							ok = true;
							break;
						}
					}
					if (!ok)
					{
						m_vs_smf_pos = save_pos;
						m_vs_smf_waiting_data = true;
						if (m_vs_midi_timer)
							m_vs_midi_timer->adjust(attotime::from_msec(2));
					}
					else
					{
						double dt = (double)delta * ((double)m_vs_smf_tempo_us / 1000000.0) / (double)m_vs_smf_division;
						m_vs_decode_seconds += dt;
						m_vs_sci[4] = (uint16_t)m_vs_decode_seconds;
						if (m_vs_midi_timer)
							m_vs_midi_timer->adjust(attotime::from_double(dt));
					}
				}
				else if (m_vs_sdi_buf.size() >= 12 &&
				         m_vs_sdi_buf[0] == 'R' && m_vs_sdi_buf[1] == 'I' &&
				         m_vs_sdi_buf[2] == 'F' && m_vs_sdi_buf[3] == 'F' &&
				         m_vs_sdi_buf[8] == 'W' && m_vs_sdi_buf[9] == 'A' &&
				         m_vs_sdi_buf[10] == 'V' && m_vs_sdi_buf[11] == 'E')
				{
					m_audio_format = FMT_WAV;
					m_vs_decoding_audio = true;
					m_vs_sci[9] = 0x7665;
					m_vs_sci[4] = 0;
					vs_decode_audio();
				}
				else if (m_vs_sdi_buf.size() >= 3 &&
				         m_vs_sdi_buf[0] == 'I' && m_vs_sdi_buf[1] == 'D' &&
				         m_vs_sdi_buf[2] == '3')
				{
					m_audio_format = FMT_MP3;
					m_vs_decoding_audio = true;
					m_vs_sci[9] = 0xffe0;
					m_vs_sci[4] = 0;
					vs_decode_audio();
				}
				else if (m_vs_sdi_buf.size() >= 2 &&
				         m_vs_sdi_buf[0] == 0xff && (m_vs_sdi_buf[1] & 0xe0) == 0xe0)
				{
					m_audio_format = FMT_MP3;
					m_vs_decoding_audio = true;
					m_vs_sci[9] = 0xffe0;
					m_vs_sci[4] = 0;
					vs_decode_audio();
				}
				else if (m_vs_sdi_buf.size() > 64 &&
				         (m_vs_sdi_buf[0] != 'M' || m_vs_sdi_buf[1] != 'T' ||
				          m_vs_sdi_buf[2] != 'h' || m_vs_sdi_buf[3] != 'd'))
				{
					bool found = false;
					for (size_t i = 1; i + 4 <= m_vs_sdi_buf.size(); i++)
					{
						if ((m_vs_sdi_buf[i] == 0xff && (m_vs_sdi_buf[i + 1] & 0xe0) == 0xe0) ||
						    (m_vs_sdi_buf[i] == 'I' && m_vs_sdi_buf[i + 1] == 'D' && m_vs_sdi_buf[i + 2] == '3') ||
						    (m_vs_sdi_buf[i] == 'R' && m_vs_sdi_buf[i + 1] == 'I' && m_vs_sdi_buf[i + 2] == 'F' && m_vs_sdi_buf[i + 3] == 'F'))
						{
							m_vs_sdi_buf.erase(m_vs_sdi_buf.begin(), m_vs_sdi_buf.begin() + i);
							found = true;
							break;
						}
					}
					if (!found && m_vs_sdi_buf.size() > 128)
					{
						m_vs_sdi_buf.erase(m_vs_sdi_buf.begin());
					}
				}
			}
		}
		break;
	}
	default:
		break;
	}
}

void wildbits_jr2_state::vs_stop_smf()
{
	if (m_vs_midi_timer)
		m_vs_midi_timer->adjust(attotime::never);
	m_vs_playing_smf = false;
	m_vs_smf_waiting_data = false;
	m_vs_smf_retry_count = 0;
	m_vs_smf_pos = 0;
	m_vs_sdi_buf.clear();
	m_vs_sci[9] = 0x0000; // HDAT1 = 0 (clean end)

	if (m_midi_out)
	{
		for (uint8_t ch = 0; ch < 16; ch++)
		{
			m_midi_out->write(0xb0 | ch);
			m_midi_out->write(0x7b); // All Notes Off
			m_midi_out->write(0x00);
		}
	}
}

void wildbits_jr2_state::vs_stop_audio()
{
	m_vs_decoding_audio = false;
	m_audio_format = FMT_NONE;
	m_wav_header_parsed = false;
	m_wav_channels = 2;
	m_wav_bits = 16;
	m_wav_rate = 44100;
	m_wav_data_left = 0;
	m_mp3_inited = false;
	m_pcm_fifo_l.clear();
	m_pcm_fifo_r.clear();
	m_vs_sdi_buf.clear();
	m_total_samples_played = 0;
	m_current_hz = 44100;
	m_vs_sci[8] = 0;
	m_vs_sci[9] = 0;
	if (m_stream)
		m_stream->set_sample_rate(44100);
}

void wildbits_jr2_state::vs_decode_audio()
{
	if (!m_vs_decoding_audio && m_audio_format == FMT_NONE)
	{
		// Format detection
		if (m_vs_sdi_buf.size() >= 3 && m_vs_sdi_buf[0] == 'I' && m_vs_sdi_buf[1] == 'D' && m_vs_sdi_buf[2] == '3')
		{
			m_audio_format = FMT_MP3;
			m_vs_decoding_audio = true;
			m_vs_sci[9] = 0xffe0;
			m_vs_sci[4] = 0;
		}
		else if (m_vs_sdi_buf.size() >= 4 && m_vs_sdi_buf[0] == 'R' && m_vs_sdi_buf[1] == 'I' && m_vs_sdi_buf[2] == 'F' && m_vs_sdi_buf[3] == 'F')
		{
			m_audio_format = FMT_WAV;
			m_vs_decoding_audio = true;
			m_vs_sci[9] = 0x7665;
			m_vs_sci[4] = 0;
		}
		else if (m_vs_sdi_buf.size() >= 2 && m_vs_sdi_buf[0] == 0xff && (m_vs_sdi_buf[1] & 0xe0) == 0xe0)
		{
			m_audio_format = FMT_MP3;
			m_vs_decoding_audio = true;
			m_vs_sci[9] = 0xffe0;
			m_vs_sci[4] = 0;
		}
	}

	if (!m_vs_decoding_audio)
		return;

	if (m_audio_format == FMT_MP3)
	{
		// Strip ID3v2 tag if present at start
		if (m_vs_sdi_buf.size() >= 10 && m_vs_sdi_buf[0] == 'I' && m_vs_sdi_buf[1] == 'D' && m_vs_sdi_buf[2] == '3')
		{
			uint32_t tag_len = 10 + (((uint32_t)(m_vs_sdi_buf[6] & 0x7f) << 21) |
			                         ((uint32_t)(m_vs_sdi_buf[7] & 0x7f) << 14) |
			                         ((uint32_t)(m_vs_sdi_buf[8] & 0x7f) << 7) |
			                         (uint32_t)(m_vs_sdi_buf[9] & 0x7f));
			if (m_vs_sdi_buf[5] & 0x10) // footer present
				tag_len += 10;
			if (m_vs_sdi_buf.size() >= tag_len)
			{
				m_vs_sdi_buf.erase(m_vs_sdi_buf.begin(), m_vs_sdi_buf.begin() + tag_len);
			}
			else
			{
				return;
			}
		}

		if (!m_mp3_inited)
		{
			mp3dec_init(&m_mp3dec);
			m_mp3_inited = true;
		}

		mp3d_sample_t pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
		mp3dec_frame_info_t info;

		while (m_pcm_fifo_l.size() < 16384 && m_vs_sdi_buf.size() >= 4)
		{
			int samples = mp3dec_decode_frame(&m_mp3dec, m_vs_sdi_buf.data(), m_vs_sdi_buf.size(), pcm, &info);
			if (samples > 0)
			{
				if (info.hz > 0 && (uint32_t)info.hz != m_current_hz)
				{
					m_current_hz = info.hz;
					if (m_stream)
						m_stream->set_sample_rate(m_current_hz);
				}
				m_vs_sci[5] = (m_current_hz & 0xfffe) | ((info.channels > 1) ? 1 : 0);
				m_vs_sci[9] = 0xffe0;
				m_vs_sci[8] = (info.bitrate_kbps << 8);

				if (info.channels == 2)
				{
					for (int s = 0; s < samples; s++)
					{
						m_pcm_fifo_l.push_back(pcm[s * 2]);
						m_pcm_fifo_r.push_back(pcm[s * 2 + 1]);
					}
				}
				else
				{
					for (int s = 0; s < samples; s++)
					{
						m_pcm_fifo_l.push_back(pcm[s]);
						m_pcm_fifo_r.push_back(pcm[s]);
					}
				}

				size_t erase_len = (size_t)info.frame_bytes;
				if (erase_len > m_vs_sdi_buf.size())
					erase_len = m_vs_sdi_buf.size();
				m_vs_sdi_buf.erase(m_vs_sdi_buf.begin(), m_vs_sdi_buf.begin() + erase_len);
			}
			else
			{
				break;
			}
		}
	}
	else if (m_audio_format == FMT_WAV)
	{
		if (!m_wav_header_parsed)
		{
			if (m_vs_sdi_buf.size() < 12)
				return;

			size_t pos = 12;
			bool found_fmt = false;
			bool found_data = false;
			size_t data_start = 0;
			uint32_t data_size = 0;

			while (pos + 8 <= m_vs_sdi_buf.size())
			{
				uint32_t chunk_id = ((uint32_t)m_vs_sdi_buf[pos] << 24) |
				                    ((uint32_t)m_vs_sdi_buf[pos + 1] << 16) |
				                    ((uint32_t)m_vs_sdi_buf[pos + 2] << 8) |
				                    (uint32_t)m_vs_sdi_buf[pos + 3];
				uint32_t chunk_sz = (uint32_t)m_vs_sdi_buf[pos + 4] |
				                    ((uint32_t)m_vs_sdi_buf[pos + 5] << 8) |
				                    ((uint32_t)m_vs_sdi_buf[pos + 6] << 16) |
				                    ((uint32_t)m_vs_sdi_buf[pos + 7] << 24);

				if (chunk_id == 0x666d7420) // 'fmt '
				{
					if (pos + 8 + 16 <= m_vs_sdi_buf.size())
					{
						m_wav_channels = (uint16_t)m_vs_sdi_buf[pos + 10] | ((uint16_t)m_vs_sdi_buf[pos + 11] << 8);
						m_wav_rate = (uint32_t)m_vs_sdi_buf[pos + 12] |
						             ((uint32_t)m_vs_sdi_buf[pos + 13] << 8) |
						             ((uint32_t)m_vs_sdi_buf[pos + 14] << 16) |
						             ((uint32_t)m_vs_sdi_buf[pos + 15] << 24);
						m_wav_bits = (uint16_t)m_vs_sdi_buf[pos + 22] | ((uint16_t)m_vs_sdi_buf[pos + 23] << 8);
						found_fmt = true;
					}
					else
					{
						return;
					}
				}
				else if (chunk_id == 0x64617461) // 'data'
				{
					found_data = true;
					data_start = pos + 8;
					data_size = chunk_sz;
					break;
				}

				pos += 8 + chunk_sz;
				if (chunk_sz & 1) pos++;
			}

			if (found_fmt && found_data && m_vs_sdi_buf.size() >= data_start)
			{
				m_wav_header_parsed = true;
				m_wav_data_left = data_size;
				if (m_wav_rate > 0 && m_wav_rate != m_current_hz)
				{
					m_current_hz = m_wav_rate;
					if (m_stream)
						m_stream->set_sample_rate(m_current_hz);
				}
				m_vs_sci[5] = (m_current_hz & 0xfffe) | ((m_wav_channels > 1) ? 1 : 0);
				m_vs_sci[9] = 0x7665;
				m_vs_sdi_buf.erase(m_vs_sdi_buf.begin(), m_vs_sdi_buf.begin() + data_start);
			}
			else
			{
				return;
			}
		}

		size_t frame_bytes = (m_wav_bits / 8) * m_wav_channels;
		if (frame_bytes == 0) frame_bytes = 2;

		while (m_pcm_fifo_l.size() < 16384 && m_vs_sdi_buf.size() >= frame_bytes)
		{
			int16_t l = 0, r = 0;
			if (m_wav_bits == 16)
			{
				if (m_wav_channels == 2)
				{
					l = (int16_t)((uint16_t)m_vs_sdi_buf[0] | ((uint16_t)m_vs_sdi_buf[1] << 8));
					r = (int16_t)((uint16_t)m_vs_sdi_buf[2] | ((uint16_t)m_vs_sdi_buf[3] << 8));
				}
				else
				{
					l = r = (int16_t)((uint16_t)m_vs_sdi_buf[0] | ((uint16_t)m_vs_sdi_buf[1] << 8));
				}
			}
			else if (m_wav_bits == 8)
			{
				if (m_wav_channels == 2)
				{
					l = ((int16_t)m_vs_sdi_buf[0] - 128) << 8;
					r = ((int16_t)m_vs_sdi_buf[1] - 128) << 8;
				}
				else
				{
					l = r = ((int16_t)m_vs_sdi_buf[0] - 128) << 8;
				}
			}
			m_pcm_fifo_l.push_back(l);
			m_pcm_fifo_r.push_back(r);
			m_vs_sdi_buf.erase(m_vs_sdi_buf.begin(), m_vs_sdi_buf.begin() + frame_bytes);
		}
	}
}

void wildbits_jr2_state::sound_stream_update(sound_stream &stream)
{
	if (m_vs_decoding_audio)
	{
		if (m_pcm_fifo_l.size() < 16384)
			vs_decode_audio();

		uint8_t atten_l = (m_vs_sci[11] >> 8) & 0xff;
		uint8_t atten_r = m_vs_sci[11] & 0xff;
		double vol_l = (atten_l >= 254) ? 0.0 : (std::pow(10.0, -0.025 * atten_l) * m_codec_vs_gain_l);
		double vol_r = (atten_r >= 254) ? 0.0 : (std::pow(10.0, -0.025 * atten_r) * m_codec_vs_gain_r);

		for (int s = 0; s < stream.samples(); s++)
		{
			if (!m_pcm_fifo_l.empty())
			{
				int16_t l = m_pcm_fifo_l.front();
				m_pcm_fifo_l.pop_front();
				int16_t r = m_pcm_fifo_r.empty() ? l : m_pcm_fifo_r.front();
				if (!m_pcm_fifo_r.empty())
					m_pcm_fifo_r.pop_front();

				stream.put_clamp(0, s, (float)(l * vol_l / 32768.0));
				stream.put_clamp(1, s, (float)(r * vol_r / 32768.0));
				m_total_samples_played++;
			}
			else
			{
				stream.put(0, s, 0.0f);
				stream.put(1, s, 0.0f);
			}
		}

		if (m_current_hz > 0)
			m_vs_sci[4] = (uint16_t)(m_total_samples_played / m_current_hz);
	}
	else
	{
		stream.fill(0, 0.0f);
		stream.fill(1, 0.0f);
	}
}

TIMER_CALLBACK_MEMBER(wildbits_jr2_state::vs_midi_tick)
{
	if (!m_vs_playing_smf)
		return;

	while (m_vs_playing_smf)
	{
		uint32_t cur = m_vs_smf_pos;
		uint8_t cur_rs = m_vs_smf_running_status;

		auto read_vlq = [this](uint32_t &pos, bool &ok) -> uint32_t {
			uint32_t val = 0;
			ok = false;
			while (pos < m_vs_sdi_buf.size())
			{
				uint8_t b = m_vs_sdi_buf[pos++];
				val = (val << 7) | (b & 0x7f);
				if (!(b & 0x80))
				{
					ok = true;
					break;
				}
			}
			return val;
		};

		bool need_more = false;
		if (m_vs_smf_pos >= m_vs_sdi_buf.size())
		{
			need_more = true;
		}
		else
		{
			uint8_t b = m_vs_sdi_buf[m_vs_smf_pos++];
			if (b == 0xff) // Meta event
			{
				if (m_vs_smf_pos >= m_vs_sdi_buf.size())
				{
					need_more = true;
				}
				else
				{
					uint8_t type = m_vs_sdi_buf[m_vs_smf_pos++];
					bool ok_len = false;
					uint32_t len = read_vlq(m_vs_smf_pos, ok_len);
					if (!ok_len || m_vs_smf_pos + len > m_vs_sdi_buf.size())
					{
						need_more = true;
					}
					else
					{
						m_vs_smf_running_status = 0;
						if (type == 0x51 && len == 3)
						{
							m_vs_smf_tempo_us = ((uint32_t)m_vs_sdi_buf[m_vs_smf_pos] << 16) |
							                    ((uint32_t)m_vs_sdi_buf[m_vs_smf_pos + 1] << 8) |
							                    (uint32_t)m_vs_sdi_buf[m_vs_smf_pos + 2];
						}
						else if (type == 0x2f) // End of Track
						{
							vs_stop_smf();
							return;
						}
						m_vs_smf_pos += len;
					}
				}
			}
			else if (b == 0xf0 || b == 0xf7) // SysEx
			{
				bool ok_len = false;
				uint32_t len = read_vlq(m_vs_smf_pos, ok_len);
				if (!ok_len || m_vs_smf_pos + len > m_vs_sdi_buf.size())
				{
					need_more = true;
				}
				else
				{
					m_vs_smf_running_status = 0;
					m_vs_smf_pos += len;
				}
			}
			else // Channel event
			{
				uint8_t status = 0;
				uint8_t d1 = 0;
				if (b & 0x80)
				{
					status = b;
					if (status < 0xf0)
						m_vs_smf_running_status = status;
					else
						m_vs_smf_running_status = 0;

					if (m_vs_smf_pos >= m_vs_sdi_buf.size())
					{
						need_more = true;
					}
					else
					{
						d1 = m_vs_sdi_buf[m_vs_smf_pos++];
					}
				}
				else
				{
					status = m_vs_smf_running_status;
					d1 = b;
				}

				if (!need_more)
				{
					uint8_t type = status & 0xf0;
					bool has_d2 = (type != 0xc0 && type != 0xd0);
					uint8_t d2 = 0;
					if (has_d2)
					{
						if (m_vs_smf_pos >= m_vs_sdi_buf.size())
						{
							need_more = true;
						}
						else
						{
							d2 = m_vs_sdi_buf[m_vs_smf_pos++];
						}
					}

					if (!need_more && m_midi_out && (status & 0x80))
					{
						m_midi_out->write(status);
						m_midi_out->write(d1);
						if (has_d2)
							m_midi_out->write(d2);
					}
				}
			}
		}

		if (!need_more)
		{
			// Read the next delta-time
			bool ok_dt = false;
			uint32_t delta = read_vlq(m_vs_smf_pos, ok_dt);
			if (!ok_dt)
			{
				need_more = true;
			}
			else
			{
				double dt = (double)delta * ((double)m_vs_smf_tempo_us / 1000000.0) / (double)m_vs_smf_division;
				m_vs_decode_seconds += dt;
				m_vs_sci[4] = (uint16_t)m_vs_decode_seconds;

				if (delta > 0)
				{
					if (m_vs_midi_timer)
						m_vs_midi_timer->adjust(attotime::from_double(dt));
					return;
				}
				// delta == 0: continue while loop to process next event immediately!
			}
		}

		if (need_more)
		{
			m_vs_smf_pos = cur;
			m_vs_smf_running_status = cur_rs;
			m_vs_smf_waiting_data = true;
			m_vs_smf_retry_count++;
			if (m_vs_smf_retry_count > 500) // 1 second without data: stop
			{
				vs_stop_smf();
				return;
			}
			if (m_vs_midi_timer)
				m_vs_midi_timer->adjust(attotime::from_msec(2));
			return;
		}
	}
}

void wildbits_jr2_state::wbjr2_mem(address_map &map)
{
	// Eight 8KB dynamic slots covering the entire 64KB logical address space
	map(0x0000, 0x1fff).bankr("bank_r0");
	map(0x2000, 0x3fff).bankr("bank_r1");
	map(0x4000, 0x5fff).bankr("bank_r2");
	map(0x6000, 0x7fff).bankr("bank_r3");
	map(0x8000, 0x9fff).bankr("bank_r4");
	map(0xa000, 0xbfff).bankr("bank_r5");
	map(0xc000, 0xdfff).bankr("bank_r6");
	map(0x0000, 0xdfff).w(FUNC(wildbits_jr2_state::ram_w));
	map(0xe000, 0xffff).bankr("bank_r7");
	map(0xe000, 0xfcff).w(FUNC(wildbits_jr2_state::slot7_ram_w));

	// Overlays in Slot 7 ($E000-$FFFF):
	// Fixed I/O Windows ($FE00-$FEFF and $FF00-$FFEF):
	// On hardware, RAM_Access_Inhibit in the FPGA overrides Slot 7 MMU translation across these windows.
	// Any unmapped locations within fixed I/O space return open-bus ($FF), ignore writes,
	// and execute peripheral cycle-stretching (io_wait_read() / io_wait_write()), preventing unintended fall-through to bank7.
	map(0xfe00, 0xfeff).lr8(NAME([this](offs_t offset) -> uint8_t { io_wait_read(); return 0xff; }))
	                   .lw8(NAME([this](offs_t offset, uint8_t data) { io_wait_write(); }));
	map(0xff00, 0xffef).lr8(NAME([this](offs_t offset) -> uint8_t { io_wait_read(); return 0xff; }))
	                   .lw8(NAME([this](offs_t offset, uint8_t data) { io_wait_write(); }));

	// $FD00-$FDFF: Constant RAM for OS-9 Level 2 (when enabled in MMU_IO_CTRL bit 0)
	map(0xfd00, 0xfdff).lr8(NAME([this](offs_t offset) -> uint8_t {
		io_wait_read();
		if (m_mmu_io_ctrl & 0x01)
			return m_constant_ram[offset];
		uint8_t active_lut = m_mmu_mem_ctrl & 0x03;
		return get_physical_block_ptr(m_mlut[active_lut][7])[0x1d00 + offset];
	})).lw8(NAME([this](offs_t offset, uint8_t data) {
		io_wait_write();
		if (m_mmu_io_ctrl & 0x01)
			m_constant_ram[offset] = data;
		else
		{
			uint8_t active_lut = m_mmu_mem_ctrl & 0x03;
			get_physical_block_ptr(m_mlut[active_lut][7])[0x1d00 + offset] = data;
		}
	}));

	// $FE00-$FE03: System Control & Software Reset registers
	map(0xfe00, 0xfe00).rw(FUNC(wildbits_jr2_state::sys0_r), FUNC(wildbits_jr2_state::sys0_w));
	map(0xfe01, 0xfe01).rw(FUNC(wildbits_jr2_state::sys1_r), FUNC(wildbits_jr2_state::sys1_w));
	map(0xfe02, 0xfe02).w(FUNC(wildbits_jr2_state::rst0_w));
	map(0xfe03, 0xfe03).w(FUNC(wildbits_jr2_state::rst1_w));

	// $FE04-$FE06: LFSR Random Number Generator
	map(0xfe04, 0xfe06).rw(FUNC(wildbits_jr2_state::lfsr_r), FUNC(wildbits_jr2_state::lfsr_w));

	// $FE07: Machine ID register
	map(0xfe07, 0xfe07).r(FUNC(wildbits_jr2_state::mid_r));
	map(0xfe08, 0xfe0f).r(FUNC(wildbits_jr2_state::pcbid_r));

	// $FE10-$FE1F: Optical Keyboard Decode (fixed $55 on Jr2)
	map(0xfe10, 0xfe1f).r(FUNC(wildbits_jr2_state::optkbd_r));

	// $FE40-$FE4F: Real-Time Clock (bq4802)
	map(0xfe40, 0xfe4f).rw(FUNC(wildbits_jr2_state::rtc_r), FUNC(wildbits_jr2_state::rtc_w));

	// $FE20-$FE2F: Interrupt Controller
	map(0xfe20, 0xfe2f).rw(FUNC(wildbits_jr2_state::intc_r), FUNC(wildbits_jr2_state::intc_w));

	// $FE30-$FE3F: System Timers 0 & 1
	map(0xfe30, 0xfe3f).rw(FUNC(wildbits_jr2_state::timer_r), FUNC(wildbits_jr2_state::timer_w));

	// $FE50-$FE54: PS/2 Keyboard and Mouse
	map(0xfe50, 0xfe54).rw(FUNC(wildbits_jr2_state::ps2_r), FUNC(wildbits_jr2_state::ps2_w));

	// $FE60-$FE67: 16550 UART (Serial / DriveWire)
	map(0xfe60, 0xfe67).rw(FUNC(wildbits_jr2_state::uart_r), FUNC(wildbits_jr2_state::uart_w));

	// $FE70-$FE72: Audio CODEC (WM8776)
	map(0xfe70, 0xfe72).rw(FUNC(wildbits_jr2_state::codec_r), FUNC(wildbits_jr2_state::codec_w));

	// $FE90-$FE91: SPI SD Card Controller
	map(0xfe90, 0xfe90).rw(FUNC(wildbits_jr2_state::sdc_stat_r), FUNC(wildbits_jr2_state::sdc_stat_w));
	map(0xfe91, 0xfe91).rw(FUNC(wildbits_jr2_state::sdc_data_r), FUNC(wildbits_jr2_state::sdc_data_w));

	// $FEA0-$FEA8: Hardware Mouse Cursor
	map(0xfea0, 0xfea8).rw(FUNC(wildbits_jr2_state::mouse_r), FUNC(wildbits_jr2_state::mouse_w));

	// $FEC0-$FEDF: TinyVicky Direct Memory Access (DMA) Controller
	map(0xfec0, 0xfedf).rw(FUNC(wildbits_jr2_state::dma_r), FUNC(wildbits_jr2_state::dma_w));

	// $FEE0-$FEFF: Hardware Integer Math Coprocessor
	map(0xfee0, 0xfeff).rw(FUNC(wildbits_jr2_state::math_r), FUNC(wildbits_jr2_state::math_w));

	// $FF20-$FF2F: WizFi360 WiFi / SPI Controller
	map(0xff20, 0xff2f).rw(FUNC(wildbits_jr2_state::wizfi_r), FUNC(wildbits_jr2_state::wizfi_w));

	// $FF30-$FF39: SAM2695 MIDI Interface (Edition 2)
	map(0xff30, 0xff39).rw(FUNC(wildbits_jr2_state::sam2695_r), FUNC(wildbits_jr2_state::sam2695_w));

	// $FF50-$FF5F: VS1053b Audio Decoder
	map(0xff50, 0xff5f).rw(FUNC(wildbits_jr2_state::vs1053_r), FUNC(wildbits_jr2_state::vs1053_w));

	// $FF90: Hardware Configuration DIP Switches
	map(0xff90, 0xff90).r(FUNC(wildbits_jr2_state::dipsw_r));

	// $FF91-$FF99: Consolidated Fixed I/O Sound Registers
	map(0xff91, 0xff99).rw(FUNC(wildbits_jr2_state::sound_fixed_r), FUNC(wildbits_jr2_state::sound_fixed_w));

	// $FFA0-$FFA1: MMU Control registers
	map(0xffa0, 0xffa0).rw(FUNC(wildbits_jr2_state::mmu_mem_ctrl_r), FUNC(wildbits_jr2_state::mmu_mem_ctrl_w));
	map(0xffa1, 0xffa1).rw(FUNC(wildbits_jr2_state::mmu_io_ctrl_r), FUNC(wildbits_jr2_state::mmu_io_ctrl_w));

	// $FFA8-$FFAF: MMU Slot Assignment registers (when EDIT_EN=1)
	map(0xffa8, 0xffaf).rw(FUNC(wildbits_jr2_state::mmu_slot_r), FUNC(wildbits_jr2_state::mmu_slot_w));

	// $FFC0-$FFDF: TinyVicky Video Master Registers
	map(0xffc0, 0xffdf).rw(FUNC(wildbits_jr2_state::vky_r), FUNC(wildbits_jr2_state::vky_w));

	// $FFE0-$FFEF: Hardware Floating-Point Unit (FP_Math_Module)
	map(0xffe0, 0xffef).rw(FUNC(wildbits_jr2_state::fpu_r), FUNC(wildbits_jr2_state::fpu_w));

	// $FFF0-$FFFF: Vector RAM overlay (when enabled in MMU_IO_CTRL bit 1)
	map(0xfff0, 0xffff).lr8(NAME([this](offs_t offset) -> uint8_t {
		if (m_mmu_io_ctrl & 0x02)
			return m_vector_ram[offset];
		uint8_t active_lut = m_mmu_mem_ctrl & 0x03;
		return get_physical_block_ptr(m_mlut[active_lut][7])[0x1ff0 + offset];
	})).lw8(NAME([this](offs_t offset, uint8_t data) {
		if (m_mmu_io_ctrl & 0x02)
			m_vector_ram[offset] = data;
		else
		{
			uint8_t active_lut = m_mmu_mem_ctrl & 0x03;
			get_physical_block_ptr(m_mlut[active_lut][7])[0x1ff0 + offset] = data;
		}
	}));
}

uint32_t wildbits_jr2_state::screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	// TinyVicky master video blank check
	if (m_vky_mstr_ctrl_0 & 0x80) // Disable_Vid: blank video output
	{
		bitmap.fill(rgb_t(0, 0, 0), cliprect);
		return 0;
	}

	// TinyVicky master video enables
	bool text_en = (m_vky_mstr_ctrl_0 & 0x01) != 0;
	bool text_overlay = (m_vky_mstr_ctrl_0 & 0x02) != 0;
	bool graph_en = (m_vky_mstr_ctrl_0 & 0x04) != 0;
	bool bitmap_en = (m_vky_mstr_ctrl_0 & 0x08) != 0;
	bool tile_en = graph_en && ((m_vky_mstr_ctrl_0 & 0x10) != 0);
	bool sprite_en = graph_en && ((m_vky_mstr_ctrl_0 & 0x20) != 0);
	bool gamma_en = (m_vky_mstr_ctrl_0 & 0x40) != 0;

	bool clk_70 = (m_vky_mstr_ctrl_1 & 0x01) != 0;
	bool font_set1 = (m_vky_mstr_ctrl_1 & 0x20) != 0;
	uint16_t font_base = font_set1 ? 0x0800 : 0x0000;

	// Base background color
	rgb_t bg_clear(m_vky_bg_r, m_vky_bg_g, m_vky_bg_b);
	bitmap.fill(bg_clear, cliprect);

	int bm_w = 320;
	int bm_h = clk_70 ? 200 : 240;

	// Lambda to render a single 256-color Bitmap plane (0..2)
	auto render_bitmap_plane = [&](int p) {
		uint16_t reg_base = 0x1000 + p * 8;
		uint8_t bm_ctrl = m_vram_fc[reg_base + 0];
		if (!(bm_ctrl & 0x01))
			return;

		uint8_t clut_idx = (bm_ctrl >> 1) & 0x03;
		uint16_t clut_base = 0x1000 + clut_idx * 0x0400;

		uint32_t start_addr = ((uint32_t)m_vram_fc[reg_base + 1] << 16) |
		                      ((uint32_t)m_vram_fc[reg_base + 2] << 8) |
		                      m_vram_fc[reg_base + 3];

		bool is_hires4 = (m_vky_gfx_mode & 0x01) || (bm_ctrl & 0x10);
		uint8_t clut_grp = (bm_ctrl & 0x10) ? ((bm_ctrl >> 5) & 0x07) : ((m_vky_gfx_mode >> 1) & 0x07);

		for (int by = 0; by < bm_h; by++)
		{
			int sy0 = by * 2;
			int sy1 = by * 2 + 1;
			if (sy0 > cliprect.max_y || sy1 < cliprect.min_y)
				continue;

			uint32_t row_addr = start_addr + by * bm_w;
			if (row_addr + bm_w > 0x200000)
				continue;

			const uint8_t *src_row = &m_ram[row_addr];

			if (is_hires4)
			{
				for (int bx = 0; bx < bm_w; bx++)
				{
					uint8_t byte = src_row[bx];
					uint8_t nib0 = (byte >> 4) & 0x0f; // High nibble = left pixel
					uint8_t nib1 = byte & 0x0f;        // Low nibble = right pixel

					int sx0 = bx * 2;
					int sx1 = bx * 2 + 1;

					if (nib0 != 0)
					{
						uint8_t color_idx = (clut_grp << 4) | nib0;
						uint16_t entry_offset = clut_base + color_idx * 4;
						uint8_t b = m_vram_fd[entry_offset + 0];
						uint8_t g = m_vram_fd[entry_offset + 1];
						uint8_t r = m_vram_fd[entry_offset + 2];
						if (gamma_en)
						{
							b = m_vram_fc[0x0000 + b];
							g = m_vram_fc[0x0400 + g];
							r = m_vram_fc[0x0800 + r];
						}
						rgb_t pen(r, g, b);
						if (sx0 >= cliprect.min_x && sx0 <= cliprect.max_x)
						{
							if (sy0 >= cliprect.min_y && sy0 <= cliprect.max_y)
								bitmap.pix(sy0, sx0) = pen;
							if (sy1 >= cliprect.min_y && sy1 <= cliprect.max_y)
								bitmap.pix(sy1, sx0) = pen;
						}
					}

					if (nib1 != 0)
					{
						uint8_t color_idx = (clut_grp << 4) | nib1;
						uint16_t entry_offset = clut_base + color_idx * 4;
						uint8_t b = m_vram_fd[entry_offset + 0];
						uint8_t g = m_vram_fd[entry_offset + 1];
						uint8_t r = m_vram_fd[entry_offset + 2];
						if (gamma_en)
						{
							b = m_vram_fc[0x0000 + b];
							g = m_vram_fc[0x0400 + g];
							r = m_vram_fc[0x0800 + r];
						}
						rgb_t pen(r, g, b);
						if (sx1 >= cliprect.min_x && sx1 <= cliprect.max_x)
						{
							if (sy0 >= cliprect.min_y && sy0 <= cliprect.max_y)
								bitmap.pix(sy0, sx1) = pen;
							if (sy1 >= cliprect.min_y && sy1 <= cliprect.max_y)
								bitmap.pix(sy1, sx1) = pen;
						}
					}
				}
			}
			else
			{
				for (int bx = 0; bx < bm_w; bx++)
				{
					uint8_t color_idx = src_row[bx];
					if (color_idx == 0)
						continue; // Transparent pixel

					uint16_t entry_offset = clut_base + color_idx * 4;
					uint8_t b = m_vram_fd[entry_offset + 0];
					uint8_t g = m_vram_fd[entry_offset + 1];
					uint8_t r = m_vram_fd[entry_offset + 2];

					if (gamma_en)
					{
						b = m_vram_fc[0x0000 + b];
						g = m_vram_fc[0x0400 + g];
						r = m_vram_fc[0x0800 + r];
					}

					rgb_t pen(r, g, b);

					int sx0 = bx * 2;
					int sx1 = bx * 2 + 1;

					if (sy0 >= cliprect.min_y && sy0 <= cliprect.max_y)
					{
						if (sx0 >= cliprect.min_x && sx0 <= cliprect.max_x)
							bitmap.pix(sy0, sx0) = pen;
						if (sx1 >= cliprect.min_x && sx1 <= cliprect.max_x)
							bitmap.pix(sy0, sx1) = pen;
					}
					if (sy1 >= cliprect.min_y && sy1 <= cliprect.max_y)
					{
						if (sx0 >= cliprect.min_x && sx0 <= cliprect.max_x)
							bitmap.pix(sy1, sx0) = pen;
						if (sx1 >= cliprect.min_x && sx1 <= cliprect.max_x)
							bitmap.pix(sy1, sx1) = pen;
					}
				}
			}
		}
	};

	// Lambda to render TinyVicky 128 hardware sprites for a specific depth level
	// Depth: 0 = Total front, 1 = Between L0 and L1, 2 = Between L1 and L2, 3 = Total back
	// Priority: Sprites evaluated 127 down to 0; sprite 0 rendered last, winning overlaps.
	auto render_sprites = [&](int target_depth) {
		if (!sprite_en)
			return;

		for (int s = 127; s >= 0; s--)
		{
			uint16_t rec_base = 0x1300 + s * 8;
			uint8_t ctrl = m_vram_fc[rec_base + 0];
			if (!(ctrl & 0x01)) // Sprite disabled
				continue;

			int depth = (ctrl >> 3) & 0x03;
			if (depth != target_depth)
				continue;

			uint8_t clut_idx = (ctrl >> 1) & 0x03;
			uint16_t clut_base = 0x1000 + clut_idx * 0x0400;

			int spr_w = 32, spr_h = 32;
			switch ((ctrl >> 5) & 0x03)
			{
			case 0: spr_w = 32; spr_h = 32; break; // 32x32
			case 1: spr_w = 24; spr_h = 24; break; // 24x24
			case 2: spr_w = 16; spr_h = 16; break; // 16x16
			case 3: spr_w =  8; spr_h =  8; break; // 8x8
			}

			uint32_t start_addr = ((uint32_t)m_vram_fc[rec_base + 1] << 16) |
			                      ((uint32_t)m_vram_fc[rec_base + 2] << 8) |
			                      m_vram_fc[rec_base + 3];

			int16_t spr_x = (int16_t)(((uint16_t)m_vram_fc[rec_base + 4] << 8) | m_vram_fc[rec_base + 5]);
			int16_t spr_y = (int16_t)(((uint16_t)m_vram_fc[rec_base + 6] << 8) | m_vram_fc[rec_base + 7]);

			// 32-pixel off-screen coordinate margin
			int vis_x = spr_x - 32;
			int vis_y = spr_y - 32;

			if (vis_x + spr_w <= 0 || vis_x >= bm_w || vis_y + spr_h <= 0 || vis_y >= bm_h)
				continue;

			for (int sy = 0; sy < spr_h; sy++)
			{
				int by = vis_y + sy;
				if (by < 0 || by >= bm_h)
					continue;

				int sy0 = by * 2;
				int sy1 = by * 2 + 1;
				if (sy0 > cliprect.max_y || sy1 < cliprect.min_y)
					continue;

				uint32_t row_addr = start_addr + sy * spr_w;
				if (row_addr + spr_w > 0x200000)
					continue;

				const uint8_t *src_row = &m_ram[row_addr];

				for (int sx = 0; sx < spr_w; sx++)
				{
					int bx = vis_x + sx;
					if (bx < 0 || bx >= bm_w)
						continue;

					uint8_t color_idx = src_row[sx];
					if (color_idx == 0)
						continue; // Transparent pixel

					uint16_t entry_offset = clut_base + color_idx * 4;
					uint8_t b = m_vram_fd[entry_offset + 0];
					uint8_t g = m_vram_fd[entry_offset + 1];
					uint8_t r = m_vram_fd[entry_offset + 2];

					if (gamma_en)
					{
						b = m_vram_fc[0x0000 + b];
						g = m_vram_fc[0x0400 + g];
						r = m_vram_fc[0x0800 + r];
					}

					rgb_t pen(r, g, b);

					int sx0 = bx * 2;
					int sx1 = bx * 2 + 1;

					if (sy0 >= cliprect.min_y && sy0 <= cliprect.max_y)
					{
						if (sx0 >= cliprect.min_x && sx0 <= cliprect.max_x)
							bitmap.pix(sy0, sx0) = pen;
						if (sx1 >= cliprect.min_x && sx1 <= cliprect.max_x)
							bitmap.pix(sy0, sx1) = pen;
					}
					if (sy1 >= cliprect.min_y && sy1 <= cliprect.max_y)
					{
						if (sx0 >= cliprect.min_x && sx0 <= cliprect.max_x)
							bitmap.pix(sy1, sx0) = pen;
						if (sx1 >= cliprect.min_x && sx1 <= cliprect.max_x)
							bitmap.pix(sy1, sx1) = pen;
					}
				}
			}
		}
	};

	// Lambda to render a single TinyVicky Tilemap plane (0..2)
	auto render_tilemap_plane = [&](int tm_idx) {
		if (!tile_en)
			return;

		uint16_t tm_base = 0x1100 + tm_idx * 12;
		uint8_t tm_ctrl = m_vram_fc[tm_base + 0];
		if (!(tm_ctrl & 0x01)) // Tilemap disabled
			return;

		int tile_size = (tm_ctrl & 0x10) ? 8 : 16; // 0 = 16x16, 1 = 8x8

		// 24-bit physical RAM pointer to tilemap matrix
		// Per defs/wildbits.d: +1: TLk_START_ADDY_H, +2: TLk_START_ADDY_M, +3: TLk_START_ADDY_L
		uint8_t b1 = m_vram_fc[tm_base + 1];
		uint8_t b2 = m_vram_fc[tm_base + 2];
		uint8_t b3 = m_vram_fc[tm_base + 3];
		uint32_t start_addr = ((uint32_t)b1 << 16) | ((uint32_t)b2 << 8) | b3;

		// Virtual map size in tiles (Big-Endian on FNX6809: +4: SIZE_H, +5: SIZE_L)
		uint16_t map_w = ((uint16_t)m_vram_fc[tm_base + 4] << 8) | m_vram_fc[tm_base + 5];
		if (map_w == 0)
			map_w = (bm_w / tile_size);

		uint16_t map_h = ((uint16_t)m_vram_fc[tm_base + 6] << 8) | m_vram_fc[tm_base + 7];
		if (map_h == 0)
			map_h = (bm_h / tile_size);

		if (map_w == 0 || map_h == 0)
			return;

		// Scroll offsets in pixels (FNX6809 hardware register pairs are Big-Endian:
		// +8: POS_H, +9: POS_L, +10: POS_H, +11: POS_L)
		uint16_t scroll_x = ((uint16_t)m_vram_fc[tm_base + 8] << 8) | m_vram_fc[tm_base + 9];
		uint16_t scroll_y = ((uint16_t)m_vram_fc[tm_base + 10] << 8) | m_vram_fc[tm_base + 11];

		int total_w_pix = (int)map_w * tile_size;
		int total_h_pix = (int)map_h * tile_size;
		if (total_w_pix <= 0 || total_h_pix <= 0)
			return;

		for (int by = 0; by < bm_h; by++)
		{
			int sy0 = by * 2;
			int sy1 = by * 2 + 1;
			if (sy0 > cliprect.max_y || sy1 < cliprect.min_y)
				continue;

			int virt_y = (by + scroll_y) % total_h_pix;
			if (virt_y < 0) virt_y += total_h_pix;

			int tile_y = virt_y / tile_size;
			int fine_y_orig = virt_y % tile_size;

			for (int bx = 0; bx < bm_w; bx++)
			{
				int virt_x = (bx + scroll_x) % total_w_pix;
				if (virt_x < 0) virt_x += total_w_pix;

				int tile_x = virt_x / tile_size;
				int fine_x_orig = virt_x % tile_size;

				uint32_t cell_addr = start_addr + ((uint32_t)tile_y * map_w + tile_x) * 2;
				if (cell_addr + 1 >= 0x200000)
					continue;

				uint8_t tile_idx = m_ram[cell_addr + 0];
				uint8_t tile_attr = m_ram[cell_addr + 1];

				// In TinyVicky II, tile index 0 in the cell matrix denotes an empty/transparent cell
				if (tile_idx == 0)
					continue;

				int fine_x = fine_x_orig;
				int fine_y = fine_y_orig;

				// Tile attribute Byte 1: [7:Reserved, 6:Collision, 5..3:CLUT (0..3), 2..0:TileSet (0..7)]
				uint8_t ts = tile_attr & 0x07;
				uint16_t ts_reg = 0x1180 + ts * 4;

				// FNX6809 / Wildbits Jr2 Tileset registers (Page $C0 offsets $1180..$119F, per defs/wildbits.d):
				// +0: TILE_MAP_ADDY_CFG (bit 3: 1 = square 256-pixel bitmap stride, 0 = 1D linear)
				// +1: TILE_MAP_ADDY_H (Physical RAM address 23:16)
				// +2: TILE_MAP_ADDY_M (Physical RAM address 15:8)
				// +3: TILE_MAP_ADDY_L (Physical RAM address 7:0)
				bool square;
				uint32_t ts_addr;
				if (m_vram_fc[ts_reg + 0] <= 0x0f && m_vram_fc[ts_reg + 1] <= 0x1f)
				{
					// Standard Wildbits Jr2 layout
					square = (m_vram_fc[ts_reg + 0] & 0x08) != 0;
					ts_addr = ((uint32_t)m_vram_fc[ts_reg + 1] << 16) | ((uint32_t)m_vram_fc[ts_reg + 2] << 8) | m_vram_fc[ts_reg + 3];
				}
				else
				{
					// Legacy C256 / F256 fallback (+0..+2 L, M, H, +3 CFG)
					square = (m_vram_fc[ts_reg + 3] & 0x08) != 0;
					ts_addr = ((uint32_t)m_vram_fc[ts_reg + 2] << 16) | ((uint32_t)m_vram_fc[ts_reg + 1] << 8) | m_vram_fc[ts_reg + 0];
				}

				uint32_t pix_addr;
				if (square)
				{
					int tile_col = tile_idx % 16;
					int tile_row = tile_idx / 16;
					int pitch = 16 * tile_size; // 128 for 8x8, 256 for 16x16
					pix_addr = ts_addr + (uint32_t)(tile_row * tile_size + fine_y) * pitch + (tile_col * tile_size + fine_x);
				}
				else
				{
					pix_addr = ts_addr + ((uint32_t)tile_idx * tile_size + fine_y) * tile_size + fine_x;
				}

				if (pix_addr >= 0x200000)
					continue;

				uint8_t color_idx = m_ram[pix_addr];
				if (color_idx == 0)
					continue; // Color index 0 is transparent

				uint8_t clut_idx = (tile_attr >> 3) & 0x03;

				uint16_t clut_base = 0x1000 + clut_idx * 0x0400;
				uint16_t entry_offset = clut_base + color_idx * 4;
				uint8_t b = m_vram_fd[entry_offset + 0];
				uint8_t g = m_vram_fd[entry_offset + 1];
				uint8_t r = m_vram_fd[entry_offset + 2];

				if (gamma_en)
				{
					b = m_vram_fc[0x0000 + b];
					g = m_vram_fc[0x0400 + g];
					r = m_vram_fc[0x0800 + r];
				}

				rgb_t pen(r, g, b);

				int sx0 = bx * 2;
				int sx1 = bx * 2 + 1;

				if (sy0 >= cliprect.min_y && sy0 <= cliprect.max_y)
				{
					if (sx0 >= cliprect.min_x && sx0 <= cliprect.max_x)
						bitmap.pix(sy0, sx0) = pen;
					if (sx1 >= cliprect.min_x && sx1 <= cliprect.max_x)
						bitmap.pix(sy0, sx1) = pen;
				}
				if (sy1 >= cliprect.min_y && sy1 <= cliprect.max_y)
				{
					if (sx0 >= cliprect.min_x && sx0 <= cliprect.max_x)
						bitmap.pix(sy1, sx0) = pen;
					if (sx1 >= cliprect.min_x && sx1 <= cliprect.max_x)
						bitmap.pix(sy1, sx1) = pen;
				}
			}
		}
	};

	// Helper to render a layer plane: 0..2 = Bitmap 0..2, 4..6 = Tilemap 0..2
	auto render_layer_plane = [&](int selector) {
		if (selector <= 2)
		{
			if (bitmap_en)
				render_bitmap_plane(selector);
		}
		else if (selector >= 4 && selector <= 6)
		{
			if (tile_en)
				render_tilemap_plane(selector - 4);
		}
	};

	// Helper to check if a bitmap/tilemap plane is active/enabled
	auto has_plane = [&](int p) -> bool {
		if (p <= 2)
			return bitmap_en && ((m_vram_fc[0x1000 + p * 8] & 0x01) != 0);
		else if (p >= 4 && p <= 6)
			return tile_en && ((m_vram_fc[0x1100 + (p - 4) * 12] & 0x01) != 0);
		return false;
	};

	// Render background sprites (depth 3 = behind Layer 2 / total back)
	render_sprites(3);

	// Render TinyVicky Graphics Layers (Bitmaps, Tilemaps & Sprites)
	if (graph_en && (bitmap_en || tile_en))
	{
		// Hardware layer compositing order per TinyVicky architecture:
		// Layer 2 = background, Layer 1 = middle, Layer 0 = foreground
		int l0_plane = m_vky_layer_ctrl_0 & 0x0f;        // Layer 0 (front)
		int l1_plane = (m_vky_layer_ctrl_0 >> 4) & 0x0f; // Layer 1 (middle)
		int l2_plane = m_vky_layer_ctrl_1 & 0x0f;        // Layer 2 (back)

		bool rendered[7] = { false };
		auto try_render_layer = [&](int p) {
			if (p >= 0 && p < 7 && !rendered[p] && has_plane(p))
			{
				render_layer_plane(p);
				rendered[p] = true;
			}
		};

		// 1. Layer 2 (Background, e.g. clouds)
		try_render_layer(l2_plane);
		render_sprites(2); // Between Layer 2 and Layer 1

		// 2. Layer 1 (Middle, e.g. mountains)
		try_render_layer(l1_plane);
		render_sprites(1); // Between Layer 1 and Layer 0

		// 3. Layer 0 (Foreground, e.g. platforms / primary playfield)
		try_render_layer(l0_plane);

		// Render any unassigned but enabled planes (fallback for unconfigured layers)
		for (int p = 0; p < 7; p++)
			try_render_layer(p);

		render_sprites(0); // In front of Layer 0 (total front)
	}
	else
	{
		render_sprites(2);
		render_sprites(1);
		render_sprites(0);
	}

	// Render Text Mode
	if (text_en)
	{
		bool memtext_mode = ((m_vky_mstr_ctrl_1 & 0x40) != 0) && ((m_vram_fc[0x1200] & 0x01) != 0);
		bool memtext_show_bg = (m_vky_mstr_ctrl_1 & 0x80) != 0;

		if (memtext_mode)
		{
			bool font_8x16 = (m_vram_fc[0x1200] & 0x02) != 0;
			int font_h = font_8x16 ? 16 : 8;
			int cols = 80;
			int rows = font_8x16 ? (clk_70 ? 25 : 30) : (clk_70 ? 50 : 60);

			uint32_t text_base = ((uint32_t)m_vram_fc[0x1205] << 16) | ((uint32_t)m_vram_fc[0x1206] << 8) | m_vram_fc[0x1207];
			uint32_t color_base = ((uint32_t)m_vram_fc[0x1209] << 16) | ((uint32_t)m_vram_fc[0x120a] << 8) | m_vram_fc[0x120b];

			for (int row = 0; row < rows; row++)
			{
				for (int col = 0; col < cols; col++)
				{
					uint32_t cell_offset = ((uint32_t)row * 80 + col) * 2;
					uint32_t text_addr = (text_base + cell_offset) & 0x1fffff;
					uint32_t color_addr = (color_base + cell_offset) & 0x1fffff;

					uint8_t ch = m_ram[text_addr + 0];
					uint8_t attr = m_ram[text_addr + 1];
					uint8_t fg_idx = m_ram[color_addr + 0];
					uint8_t bg_idx = m_ram[color_addr + 1];

					uint8_t font_slot = font_8x16 ? ((attr >> 1) & 0x01) : (attr & 0x03);
					uint8_t fg_slot = (attr >> 2) & 0x01;
					uint8_t bg_slot = (attr >> 3) & 0x01;

					bool invert = (attr & 0x10) != 0;
					if ((attr & 0x20) && ((m_frame_count / 16) & 1))
						invert = !invert;

					uint16_t fg_entry = ((fg_slot * 256 + fg_idx) * 4) & 0x03ff;
					uint8_t fg_b = m_memtext_clut[0x0000 + fg_entry + 0];
					uint8_t fg_g = m_memtext_clut[0x0000 + fg_entry + 1];
					uint8_t fg_r = m_memtext_clut[0x0000 + fg_entry + 2];
					if (gamma_en)
					{
						fg_b = m_vram_fc[0x0000 + fg_b];
						fg_g = m_vram_fc[0x0400 + fg_g];
						fg_r = m_vram_fc[0x0800 + fg_r];
					}
					rgb_t fg_pen(fg_r, fg_g, fg_b);

					uint16_t bg_entry = ((bg_slot * 256 + bg_idx) * 4) & 0x03ff;
					uint8_t bg_b = m_memtext_clut[0x0800 + bg_entry + 0];
					uint8_t bg_g = m_memtext_clut[0x0800 + bg_entry + 1];
					uint8_t bg_r = m_memtext_clut[0x0800 + bg_entry + 2];
					if (gamma_en)
					{
						bg_b = m_vram_fc[0x0000 + bg_b];
						bg_g = m_vram_fc[0x0400 + bg_g];
						bg_r = m_vram_fc[0x0800 + bg_r];
					}
					rgb_t bg_pen(bg_r, bg_g, bg_b);

					bool bg_is_zero = (bg_idx == 0);

					for (int cy = 0; cy < font_h; cy++)
					{
						uint16_t font_addr = font_8x16
							? (uint16_t)((font_slot * 4096) + (ch * 16) + cy)
							: (uint16_t)((font_slot * 2048) + (ch * 8) + cy);
						uint8_t glyph_row = m_vram_fd[font_addr & 0x0fff];
						if (invert)
							glyph_row ^= 0xff;

						int py = row * font_h + cy;
						if (py > cliprect.max_y || py < cliprect.min_y)
							continue;

						uint32_t *dest = &bitmap.pix(py, col * 8);

						for (int cx = 0; cx < 8; cx++)
						{
							int px = col * 8 + cx;
							if (px < cliprect.min_x || px > cliprect.max_x)
								continue;

							bool is_fg = (glyph_row & (0x80 >> cx)) != 0;
							if (is_fg)
							{
								dest[cx] = fg_pen;
							}
							else
							{
								if (!graph_en)
								{
									dest[cx] = bg_pen;
								}
								else if (text_overlay)
								{
									if (memtext_show_bg && !bg_is_zero)
										dest[cx] = bg_pen;
								}
							}
						}
					}
				}
			}

			// Render MemText hardware cursor
			uint8_t crsr_ctrl = m_vram_fc[0x1201];
			if (crsr_ctrl & 0x01)
			{
				bool crsr_flash = (crsr_ctrl & 0x04) != 0;
				int flash_rate = (crsr_ctrl & 0x02) ? 16 : 32;
				bool crsr_visible = !crsr_flash || (((m_frame_count / flash_rate) & 1) == 0);
				if (crsr_visible)
				{
					int crsr_col = m_vram_fc[0x1202];
					int crsr_row = m_vram_fc[0x1203];
					if (crsr_col >= 0 && crsr_col < cols && crsr_row >= 0 && crsr_row < rows)
					{
						uint8_t cr_b = m_vram_fc[0x120d];
						uint8_t cr_g = m_vram_fc[0x120e];
						uint8_t cr_r = m_vram_fc[0x120f];
						if (gamma_en)
						{
							cr_b = m_vram_fc[0x0000 + cr_b];
							cr_g = m_vram_fc[0x0400 + cr_g];
							cr_r = m_vram_fc[0x0800 + cr_r];
						}
						rgb_t crsr_pen(cr_r, cr_g, cr_b);

						for (int cy = 0; cy < font_h; cy++)
						{
							uint8_t pat = m_vram_fc[0x1210 + cy];
							int py = crsr_row * font_h + cy;
							if (py >= cliprect.min_y && py <= cliprect.max_y)
							{
								uint32_t *dest = &bitmap.pix(py, crsr_col * 8);
								for (int cx = 0; cx < 8; cx++)
								{
									if (pat & (0x80 >> cx))
									{
										int px = crsr_col * 8 + cx;
										if (px >= cliprect.min_x && px <= cliprect.max_x)
										{
											dest[cx] = crsr_pen;
										}
									}
								}
							}
						}
					}
				}
			}
		}
		else
		{
			bool dbl_x = (m_vky_mstr_ctrl_1 & 0x02) != 0;
			bool dbl_y = (m_vky_mstr_ctrl_1 & 0x04) != 0;

			const int cell_w = dbl_x ? 16 : 8;
			const int cell_h = dbl_y ? 16 : 8;

			const int cols = dbl_x ? 40 : 80;
			const int rows = dbl_y ? (clk_70 ? 25 : 30) : (clk_70 ? 50 : 60);

			const int x_scale = dbl_x ? 2 : 1;
			const int y_scale = dbl_y ? 2 : 1;

			for (int row = 0; row < rows; row++)
			{
				for (int col = 0; col < cols; col++)
				{
					int cell_idx = row * cols + col;
					uint8_t ch = m_vram_fe[cell_idx];
					uint8_t attr = m_vram_ff[cell_idx];

					uint8_t fg_idx = (attr >> 4) & 0x0f;
					uint8_t bg_idx = attr & 0x0f;

					// Palette lookup from Block $FC (FG at $1700, BG at $1740)
					uint8_t fg_b = m_vram_fc[0x1700 + fg_idx * 4 + 0];
					uint8_t fg_g = m_vram_fc[0x1700 + fg_idx * 4 + 1];
					uint8_t fg_r = m_vram_fc[0x1700 + fg_idx * 4 + 2];
					if (gamma_en)
					{
						fg_b = m_vram_fc[0x0000 + fg_b];
						fg_g = m_vram_fc[0x0400 + fg_g];
						fg_r = m_vram_fc[0x0800 + fg_r];
					}
					rgb_t fg_pen(fg_r, fg_g, fg_b);

					uint8_t bg_b = m_vram_fc[0x1740 + bg_idx * 4 + 0];
					uint8_t bg_g = m_vram_fc[0x1740 + bg_idx * 4 + 1];
					uint8_t bg_r = m_vram_fc[0x1740 + bg_idx * 4 + 2];
					if (gamma_en)
					{
						bg_b = m_vram_fc[0x0000 + bg_b];
						bg_g = m_vram_fc[0x0400 + bg_g];
						bg_r = m_vram_fc[0x0800 + bg_r];
					}
					rgb_t bg_pen(bg_r, bg_g, bg_b);

					// Render character cell
					for (int cy = 0; cy < 8; cy++)
					{
						uint8_t glyph_row = m_vram_fd[font_base + ch * 8 + cy];

						for (int dy = 0; dy < y_scale; dy++)
						{
							int py = row * cell_h + cy * y_scale + dy;
							if (py > cliprect.max_y)
								break;
							if (py < cliprect.min_y)
								continue;

							uint32_t *dest = &bitmap.pix(py, col * cell_w);

							for (int cx = 0; cx < 8; cx++)
							{
								bool is_fg = (glyph_row & (0x80 >> cx)) != 0;
								if (is_fg)
								{
									for (int dx = 0; dx < x_scale; dx++)
									{
										int px = col * cell_w + cx * x_scale + dx;
										if (px >= cliprect.min_x && px <= cliprect.max_x)
										{
											dest[cx * x_scale + dx] = fg_pen;
										}
									}
								}
								else if (!graph_en || !text_overlay || ((m_vky_mstr_ctrl_1 & 0x10) && bg_idx != 0))
								{
									for (int dx = 0; dx < x_scale; dx++)
									{
										int px = col * cell_w + cx * x_scale + dx;
										if (px >= cliprect.min_x && px <= cliprect.max_x)
										{
											dest[cx * x_scale + dx] = bg_pen;
										}
									}
								}
							}
						}
					}
				}
			}

			// Render TinyVicky hardware text cursor
			if (m_vky_crsr_ctrl & 0x01)
			{
				bool blink = (m_vky_crsr_ctrl & 0x02) ? (((m_frame_count / 16) & 1) == 0) : true;
				if (blink)
				{
					int crsr_col = m_vky_crsr_x;
					int crsr_row = m_vky_crsr_y;
					if (crsr_col >= 0 && crsr_col < cols && crsr_row >= 0 && crsr_row < rows)
					{
						for (int cy = 0; cy < cell_h; cy++)
						{
							int py = crsr_row * cell_h + cy;
							if (py >= cliprect.min_y && py <= cliprect.max_y)
							{
								uint32_t *dest = &bitmap.pix(py, crsr_col * cell_w);
								for (int cx = 0; cx < cell_w; cx++)
								{
									int px = crsr_col * cell_w + cx;
									if (px >= cliprect.min_x && px <= cliprect.max_x)
									{
										dest[cx] ^= 0x00ffffff;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// Render TinyVicky hardware mouse cursor if enabled via $FEA0 bit 0
	if (m_mouse_men & 0x01)
	{
		for (int cy = 0; cy < 16; cy++)
		{
			int py = m_mouse_y + cy;
			if (py < cliprect.min_y || py > cliprect.max_y)
				continue;
			uint32_t *dest = &bitmap.pix(py, 0);
			for (int cx = 0; cx < 16; cx++)
			{
				int px = m_mouse_x + cx;
				if (px >= cliprect.min_x && px <= cliprect.max_x)
				{
					uint8_t pix = m_vram_fc[0x0c00 + cy * 16 + cx];
					if (pix == 255)
					{
						dest[px] = rgb_t(0xff, 0xff, 0xff); // White border
					}
					else if (pix != 0)
					{
						dest[px] = (pix == 1) ? rgb_t(0x00, 0x00, 0x00) : rgb_t(pix, pix, pix);
					}
				}
			}
		}
	}

		// Border rendering
	if (m_vky_brdr_ctrl & 0x01)
	{
		rgb_t brdr_pen(m_vky_brdr_r, m_vky_brdr_g, m_vky_brdr_b);
		int bw = m_vky_brdr_w;
		int bh = m_vky_brdr_h;

		if (bw > 0)
		{
			bitmap.plot_box(0, 0, bw, bitmap.height(), brdr_pen);
			bitmap.plot_box(bitmap.width() - bw, 0, bw, bitmap.height(), brdr_pen);
		}
		if (bh > 0)
		{
			bitmap.plot_box(0, 0, bitmap.width(), bh, brdr_pen);
			bitmap.plot_box(0, bitmap.height() - bh, bitmap.width(), bh, brdr_pen);
		}
	}

	return 0;
}

void wildbits_jr2_state::machine_start()
{
	m_ram = std::make_unique<uint8_t[]>(0x200000);   // 2MB Physical SRAM (1,792 KB decoded)
	std::fill_n(m_ram.get(), 0x200000, 0x00);
	m_cart = std::make_unique<uint8_t[]>(0x40000);       // 256KB Cartridge port decode ($80 - $9F)
	std::fill_n(m_cart.get(), 0x40000, 0xff);
	m_unmapped = std::make_unique<uint8_t[]>(0x2000);   // 8KB dummy unmapped page
	std::fill_n(m_unmapped.get(), 0x2000, 0xff);
	m_vram_fc = std::make_unique<uint8_t[]>(0x2000); // 8KB Block $FC
	m_vram_fd = std::make_unique<uint8_t[]>(0x2000); // 8KB Block $FD (Fonts & LUTs)
	m_vram_fe = std::make_unique<uint8_t[]>(0x2000); // 8KB Block $FE (Text Matrix)
	m_vram_ff = std::make_unique<uint8_t[]>(0x2000); // 8KB Block $FF (Color Matrix)
	m_memtext_clut = std::make_unique<uint8_t[]>(0x2000); // 8KB MemText FG/BG CLUTs

	m_timer0 = timer_alloc(FUNC(wildbits_jr2_state::timer0_tick), this);
	m_timer1 = timer_alloc(FUNC(wildbits_jr2_state::timer1_tick), this);
	m_vs_midi_timer = timer_alloc(FUNC(wildbits_jr2_state::vs_midi_tick), this);
	m_stream = stream_alloc(0, 2, 44100);

	// Probe host MIDI output ports and initialize port (preferring synthesizer/hardware over dummy Midi Through)
	bool has_midi_out = false;
	std::string selected_midi_port = "default";
	for (const auto &port : machine().osd().list_midi_ports())
	{
		if (port.output)
		{
			has_midi_out = true;
			if (port.name.find("Through") == std::string::npos &&
			    port.name.find("through") == std::string::npos)
			{
				selected_midi_port = port.name;
				break;
			}
		}
	}
	if (has_midi_out)
	{
		m_midi_out = machine().osd().create_midi_output(selected_midi_port);
		osd_printf_verbose("Wildbits Jr2: Connected MIDI output to '%s'\n", selected_midi_port);
	}

	save_pointer(NAME(m_ram), 0x200000);
	save_pointer(NAME(m_cart), 0x40000);
	save_pointer(NAME(m_vram_fc), 0x2000);
	save_pointer(NAME(m_vram_fd), 0x2000);
	save_pointer(NAME(m_vram_fe), 0x2000);
	save_pointer(NAME(m_vram_ff), 0x2000);
	save_pointer(NAME(m_memtext_clut), 0x2000);
	save_item(NAME(m_constant_ram));
	save_item(NAME(m_vector_ram));
	save_item(NAME(m_mlut));
	save_item(NAME(m_mmu_mem_ctrl));
	save_item(NAME(m_mmu_io_ctrl));
	save_item(NAME(m_sys0));
	save_item(NAME(m_sys1));
	save_item(NAME(m_rst0));
	save_item(NAME(m_rst1));
	save_item(NAME(m_int_pending));
	save_item(NAME(m_int_pol));
	save_item(NAME(m_int_edge));
	save_item(NAME(m_int_mask));
	save_item(NAME(m_t0_ctr));
	save_item(NAME(m_t0_stat));
	save_item(NAME(m_t0_val));
	save_item(NAME(m_t0_cmp));
	save_item(NAME(m_t0_cmp_ctr));
	save_item(NAME(m_t1_ctr));
	save_item(NAME(m_t1_stat));
	save_item(NAME(m_t1_val));
	save_item(NAME(m_t1_cmp));
	save_item(NAME(m_t1_cmp_ctr));
	save_item(NAME(m_ps2_ctrl));
	save_item(NAME(m_ps2_out));
	save_item(NAME(m_sdc_stat));
	save_item(NAME(m_sdc_data_in));
	save_item(NAME(m_sdc_data_out));
	save_item(NAME(m_sdcard_miso));
	save_item(NAME(m_vky_mstr_ctrl_0));
	save_item(NAME(m_vky_mstr_ctrl_1));
	save_item(NAME(m_vky_layer_ctrl_0));
	save_item(NAME(m_vky_layer_ctrl_1));
	save_item(NAME(m_vky_brdr_ctrl));
	save_item(NAME(m_vky_brdr_b));
	save_item(NAME(m_vky_brdr_g));
	save_item(NAME(m_vky_brdr_r));
	save_item(NAME(m_vky_brdr_w));
	save_item(NAME(m_vky_brdr_h));
	save_item(NAME(m_vky_bg_b));
	save_item(NAME(m_vky_bg_g));
	save_item(NAME(m_vky_bg_r));
	save_item(NAME(m_vky_crsr_ctrl));
	save_item(NAME(m_vky_crsr_char));
	save_item(NAME(m_vky_crsr_color));
	save_item(NAME(m_vky_crsr_x));
	save_item(NAME(m_vky_crsr_y));
	save_item(NAME(m_mouse_men));
	save_item(NAME(m_mouse_x));
	save_item(NAME(m_mouse_y));
	save_item(NAME(m_mouse_bytes));
	save_item(NAME(m_codec_lo));
	save_item(NAME(m_codec_hi));
	save_item(NAME(m_codec_regs));
	save_item(NAME(m_codec_vs_gain_l));
	save_item(NAME(m_codec_vs_gain_r));
	save_item(NAME(m_sam2695_ctrl));
	save_item(NAME(m_vs_ctrl));
	save_item(NAME(m_vs_scireg));
	save_item(NAME(m_vs_data_hi));
	save_item(NAME(m_vs_data_lo));
	save_item(NAME(m_vs_sci));
	save_item(NAME(m_vs_wram_addr));
	save_item(NAME(m_vs_wram));
	save_item(NAME(m_sdi_memtest_idx));
	save_item(NAME(m_vky_lint_ctrl));
	save_item(NAME(m_vky_line_cmp));
	save_item(NAME(m_vky_gfx_mode));
	save_item(NAME(m_math_mulu_a));
	save_item(NAME(m_math_mulu_b));
	save_item(NAME(m_math_divu_den));
	save_item(NAME(m_math_divu_num));
	save_item(NAME(m_math_add_a));
	save_item(NAME(m_math_add_b));
	save_item(NAME(m_fpu_ctrl));
	save_item(NAME(m_fpu_in0));
	save_item(NAME(m_fpu_in1));
	save_item(NAME(m_dma_reg));
	save_item(NAME(m_dma_status));
	save_item(NAME(m_vky_drawline_ctrl));
	save_item(NAME(m_ld_ctrl));
	save_item(NAME(m_ld_color));
	save_item(NAME(m_ld_x0));
	save_item(NAME(m_ld_x1));
	save_item(NAME(m_ld_y0));
	save_item(NAME(m_ld_y1));
	save_item(NAME(m_ld_done));
	save_item(NAME(m_ld_fifo_count));
	save_item(NAME(m_is_turbo));
	save_item(NAME(m_ram_write_frac));
	save_item(NAME(m_io_read_frac));
	save_item(NAME(m_io_write_frac));
	save_item(NAME(m_wizfi_baud_code));
	save_item(NAME(m_wizfi_reset_reg));
	save_item(NAME(m_last_mouse_x));
	save_item(NAME(m_last_mouse_y));
	save_item(NAME(m_last_mouse_btn));
	save_item(NAME(m_lfsr));
	save_item(NAME(m_lfsr_seed));
	save_item(NAME(m_lfsr_ctrl));
	save_item(NAME(m_lfsr_last_cycle));
	save_item(NAME(m_sid_select));

	// Hardware Line-Draw Accelerator taps on CPU address space for Page $FC ($1080 - $10FF)
	m_maincpu->space(AS_PROGRAM).install_read_tap(0x0000, 0xffff, "wbjr2_ld_r",
		[this](offs_t offset, u8 &data, u8 mem_mask)
		{
			uint8_t slot = (offset >> 13) & 7;
			uint8_t active_lut = m_mmu_mem_ctrl & 0x03;
			uint8_t block = m_mlut[active_lut][slot];
			if (block == 0xfc)
			{
				uint16_t blk_off = offset & 0x1fff;
				if (blk_off >= 0x1080 && blk_off <= 0x10ff)
				{
					uint8_t reg = blk_off & 7;
					switch (reg)
					{
					case 0:
						data = (m_ld_done ? 0x80 : 0x00) | (m_ld_ctrl & 0x7f);
						break;
					case 1:
						data = m_ld_color;
						break;
					case 2:
						data = (m_ld_fifo_count >> 8) & 0x1f;
						break;
					case 3:
						data = m_ld_fifo_count & 0xff;
						break;
					case 4:
						data = m_ld_x1 & 0xff;
						break;
					case 5:
						data = (m_ld_x1 >> 8) & 0x03;
						break;
					case 6:
						data = m_ld_y1;
						break;
					case 7:
						data = m_ld_y0;
						break;
					}
				}
				else if (blk_off >= 0x1000 && blk_off <= 0x107f)
				{
					// Hardware TinyVicky Bitmap register block readback (TinyVicky_BM_Registers.v):
					// Lower 5 bits Bus_A_i[4:0] decode BM0/BM1/BM2 registers; unmapped returns $66.
					// Start addresses are read back in reversed Little-Endian order (Low, Mid, High).
					uint8_t sub = blk_off & 0x1f;
					if (sub < 0x18)
					{
						uint8_t base = sub & ~7;
						uint8_t reg = sub & 7;
						if (reg == 1)
							data = m_vram_fc[0x1000 + base + 3]; // Addy Low
						else if (reg == 2)
							data = m_vram_fc[0x1000 + base + 2]; // Addy Mid
						else if (reg == 3)
							data = m_vram_fc[0x1000 + base + 1]; // Addy High
						else
							data = m_vram_fc[0x1000 + base + reg];
					}
					else
					{
						data = 0x66;
					}
				}
				else if (blk_off >= 0x1100 && blk_off <= 0x11ff)
				{
					// Hardware TinyVicky Tilemap register block readback (TinyVicky_TL_Registers.v):
					// Line 81: assign DataOut_Tile_MAP_o = Bus_A_i[7] ? TILE_DAT_REG[Bus_A_i[4:0]] : TILE_REG[Bus_A_i[5:0]];
					if (!(blk_off & 0x80))
						data = m_vram_fc[0x1100 + (blk_off & 0x3f)];
					else
						data = m_vram_fc[0x1180 + (blk_off & 0x1f)];
				}
			}
		});

	m_maincpu->space(AS_PROGRAM).install_write_tap(0x0000, 0xffff, "wbjr2_ld_w",
		[this](offs_t offset, u8 &data, u8 mem_mask)
		{
			uint8_t slot = (offset >> 13) & 7;
			uint8_t active_lut = m_mmu_mem_ctrl & 0x03;
			uint8_t block = m_mlut[active_lut][slot];
			if (block == 0xfc)
			{
				uint16_t blk_off = offset & 0x1fff;
				if (blk_off >= 0x1080 && blk_off <= 0x10ff)
				{
					uint8_t reg = blk_off & 7;
					switch (reg)
					{
					case 0:
					{
						bool old_go = (m_ld_ctrl & 0x02) != 0;
						bool new_go = (data & 0x02) != 0;
						m_ld_ctrl = data;
						if (data & 0x10)
						{
							m_ld_done = false;
							m_ld_fifo_count = 0;
						}
						if (!old_go && new_go)
						{
							linedraw_execute();
						}
						else if (!new_go)
						{
							m_ld_done = false;
						}
						break;
					}
					case 1:
						m_ld_color = data;
						break;
					case 2:
						m_ld_x0 = (m_ld_x0 & 0x00ff) | ((uint16_t)(data & 0x03) << 8);
						break;
					case 3:
						m_ld_x0 = (m_ld_x0 & 0xff00) | data;
						break;
					case 4:
						m_ld_x1 = (m_ld_x1 & 0x00ff) | ((uint16_t)(data & 0x03) << 8);
						break;
					case 5:
						m_ld_x1 = (m_ld_x1 & 0xff00) | data;
						break;
					case 6:
						m_ld_y0 = data;
						break;
					case 7:
						m_ld_y1 = data;
						break;
					}
				}
			}
		});

	m_is_turbo = false;
	m_ram_write_frac = 0;
	m_io_read_frac = 0;
	m_io_write_frac = 0;
	for (int i = 0; i < 8; i++)
		m_slot_w_ptr[i] = m_unmapped.get();

	m_scanline_timer = timer_alloc(FUNC(wildbits_jr2_state::scanline_tick), this);
}

void wildbits_jr2_state::machine_reset()
{
	if (system_bios() == 2)
	{
		m_maincpu->set_unscaled_clock(4 * 16'666'667); // Stock 16.67 MHz mode
		m_is_turbo = false;
	}
	else
	{
		m_maincpu->set_unscaled_clock(4 * 20'000'000); // 20 MHz Turbo Mode (Adaptive MMU Scheduler)
		m_is_turbo = true;
	}
	m_ram_write_frac = 0;
	m_io_read_frac = 0;
	m_io_write_frac = 0;
	// Default power-on Boot-from-Flash LUT configuration:
	// Slots 0..6 map to RAM blocks 0x00..0x06
	// Slot 7 maps to Flash block 0x7F (which contains reset vector $FFFE)
	for (int lut = 0; lut < 4; lut++)
	{
		for (int slot = 0; slot < 7; slot++)
		{
			m_mlut[lut][slot] = slot;
		}
		m_mlut[lut][7] = 0x7f; // High Flash ROM block
	}

	m_mmu_mem_ctrl = 0x00; // Active LUT 0, Edit LUT 0, EDIT_EN = 0
	m_mmu_io_ctrl = 0x00;  // Constant RAM and Vector RAM disabled
	m_sys0 = 0x00;
	m_sys1 = 0x00;
	m_rst0 = 0x00;
	m_rst1 = 0x00;
	m_lfsr = 0;
	m_lfsr_seed = 0;
	m_lfsr_ctrl = 0;
	m_lfsr_last_cycle = 0;
	m_sid_select = 0;

	// Reset Math Coprocessor
	m_math_mulu_a = 0;
	m_math_mulu_b = 0;
	m_math_divu_den = 0;
	m_math_divu_num = 0;
	m_math_add_a = 0;
	m_math_add_b = 0;

	// Reset Floating-Point Unit
	std::memset(m_fpu_ctrl, 0, sizeof(m_fpu_ctrl));
	std::memset(m_fpu_in0, 0, sizeof(m_fpu_in0));
	std::memset(m_fpu_in1, 0, sizeof(m_fpu_in1));

	// Reset Video line compare
	m_vky_lint_ctrl = 0;
	m_vky_line_cmp = 0xffff;
	m_scanline_timer->adjust(attotime::never);

	// Reset Interrupt Controller (EDGE=0xFF edge mode, POL=0x00 falling, all IRQs masked)
	for (int g = 0; g < 4; g++)
	{
		m_int_pending[g] = 0x00;
		m_int_pol[g] = 0x00;
		m_int_edge[g] = 0xff; // Default edge-triggered mode per IRQ_Controller_Jr.v
		m_int_mask[g] = 0xff; // All masked
	}
	check_irqs();

	// Reset Timers
	m_t0_ctr = 0;
	m_t0_stat = 0;
	m_t0_val = 0;
	m_t0_cmp = 0;
	m_t0_cmp_ctr = 0;
	m_timer0->adjust(attotime::never);

	m_t1_ctr = 0;
	m_t1_stat = 0;
	m_t1_val = 0;
	m_t1_cmp = 0;
	m_t1_cmp_ctr = 0;
	m_timer1->adjust(attotime::never);

	// Reset PS/2 Controller
	m_ps2_ctrl = 0;
	m_ps2_out = 0;
	while (!m_kbd_fifo.empty()) m_kbd_fifo.pop();
	while (!m_mouse_fifo.empty()) m_mouse_fifo.pop();
	for (int i = 0; i < 4; i++)
		m_key_state[i] = 0;
	m_last_mouse_x = m_mouse_x_axis->read();
	m_last_mouse_y = m_mouse_y_axis->read();
	m_last_mouse_btn = m_mouse_btn->read() & 0x07;
	m_frame_count = 0;

	// Reset SDC Controller
	m_sdc_stat = 0x00;
	m_sdc_data_in = 0xff;
	m_sdc_data_out = 0xff;
	m_sdcard_miso = 1;

	// Reset WizFi360
	reset_wizfi();

	// Reset 16550 UART
	m_uart_dll = 0;
	m_uart_dlh = 0;
	m_uart_ier = 0;
	m_uart_fcr = 0;
	m_uart_lcr = 0;
	m_uart_mcr = 0;
	m_uart_scr = 0;
	while (!m_uart_rx_fifo.empty()) m_uart_rx_fifo.pop();

	// Initialize Video Master defaults: Text Mode enabled (640x480, 80x30), Border default Black
	m_vky_mstr_ctrl_0 = 0x01; // TEXT enabled
	m_vky_mstr_ctrl_1 = 0x04; // 60Hz mode, DBL_Y enabled (80x30 default), Font 0
	m_vky_layer_ctrl_0 = 0x00;
	m_vky_layer_ctrl_1 = 0x00;
	m_vky_brdr_ctrl = 0x00;
	m_vky_brdr_b = 0x00;
	m_vky_brdr_g = 0x00;
	m_vky_brdr_r = 0x00;
	m_vky_brdr_w = 0;
	m_vky_brdr_h = 0;
	m_vky_gfx_mode = 0x00;
	m_vky_bg_b = 0x00;
	m_vky_bg_g = 0x00;
	m_vky_bg_r = 0x00;
	m_vky_crsr_ctrl = 0x00;
	m_vky_crsr_char = 0x20;
	m_vky_crsr_color = 0x7a;
	m_vky_crsr_x = 0;
	m_vky_crsr_y = 0;

	memset(m_constant_ram, 0, sizeof(m_constant_ram));
	memset(m_vector_ram, 0, sizeof(m_vector_ram));
	m_mouse_men = 0x00;
	m_mouse_x = 0;
	m_mouse_y = 0;
	m_mouse_bytes[0] = 0;
	m_mouse_bytes[1] = 0;
	m_mouse_bytes[2] = 0;
	m_sam2695_ctrl = 0x0c; // Tx_empty, Rx_empty
	while (!m_sam2695_rx_fifo.empty()) m_sam2695_rx_fifo.pop();
	m_codec_lo = 0;
	m_codec_hi = 0;
	reset_codec();

	// Reset VS1053b Audio Decoder
	vs_stop_smf();
	m_vs_ctrl = 0;
	m_vs_scireg = 0;
	m_vs_data_hi = 0;
	m_vs_data_lo = 0;
	m_vs_wram_addr = 0;
	m_sdi_memtest_idx = 0;
	memset(m_vs_sci, 0, sizeof(m_vs_sci));
	m_vs_sci[0] = 0x0800; // MODE: SM_SDINEW
	m_vs_sci[1] = 0x0048; // STATUS: VS1053b (version 4) + SS_APDOWN2 (analog powerdown)
	m_vs_sci[5] = 0x1f40; // AUDATA: 8000 Hz, mono
	memset(m_vs_wram, 0, sizeof(m_vs_wram));

	// Reset DMA Controller
	memset(m_dma_reg, 0, sizeof(m_dma_reg));
	m_dma_status = 0;

	// Reset Hardware Line-Draw Accelerator
	m_vky_drawline_ctrl = 0;
	m_ld_ctrl = 0;
	m_ld_color = 0;
	m_ld_x0 = 0;
	m_ld_x1 = 0;
	m_ld_y0 = 0;
	m_ld_y1 = 0;
	m_ld_done = false;
	m_ld_fifo_count = 0;

	memset(m_vram_fc.get(), 0, 0x2000);
	memset(m_vram_fd.get(), 0, 0x2000);
	memset(m_vram_fe.get(), 0x20, 0x2000); // Space filled text matrix
	memset(m_vram_ff.get(), 0x7a, 0x2000); // Yellow on Purple authentic NitrOS-9 color
	memset(m_memtext_clut.get(), 0, 0x2000);

	// Load Authentic OS-9 Bannerfont into Block $FD (Font Set 0: $0000-$07FF and Font Set 1: $0800-$0FFF)
	memcpy(&m_vram_fd[0x0000], s_os9_bannerfont, 2048);
	memcpy(&m_vram_fd[0x0800], s_os9_bannerfont, 2048);

	// Initialize Default Foreground and Background Text Palettes in Block $FC ($1700 and $1740)
	memcpy(&m_vram_fc[0x1700], s_os9_palette, 64);
	memcpy(&m_vram_fc[0x1740], s_os9_palette, 64);

	// Initialize MemText default registers ($1200-$121F) in Block $FC
	m_vram_fc[0x120c] = 0x0a; // Precharge
	m_vram_fc[0x120d] = 0x20; // Cursor B
	m_vram_fc[0x120e] = 0x00; // Cursor G
	m_vram_fc[0x120f] = 0x20; // Cursor R
	m_vram_fc[0x1216] = 0xff; // Cursor Graphics x8
	m_vram_fc[0x1217] = 0xff; // Cursor Graphics x8

	update_banks();
	vs_stop_smf();
	vs_stop_audio();
}

void wildbits_jr2_state::device_stop()
{
	vs_stop_smf();
	vs_stop_audio();
	if (!m_vram_fe || !m_vram_fc || !m_vram_fd || !m_ram)
		return;
	printf("Frame count: %d\n", m_frame_count);
	printf("\n=== VRAM TEXT MATRIX SNAPSHOT ===\n");
	for (int r = 0; r < 30; r++)
	{
		char line[81];
		for (int c = 0; c < 80; c++)
		{
			uint8_t ch = m_vram_fe[r * 80 + c];
			line[c] = (ch >= 32 && ch < 127) ? ch : (ch == 0x20 || ch == 0x00) ? ' ' : '#';
		}
		line[80] = 0;
		printf("%02d: |%s|\n", r, line);
	}
	printf("=================================\n");
	uint16_t pc = m_maincpu->pc();
	uint32_t bm0_addr = ((uint32_t)m_vram_fc[0x1001] << 16) | ((uint32_t)m_vram_fc[0x1002] << 8) | m_vram_fc[0x1003];
	printf("MSTR_CTRL: [%02X, %02X] LAYER_CTRL: [%02X, %02X]\n", m_vky_mstr_ctrl_0, m_vky_mstr_ctrl_1, m_vky_layer_ctrl_0, m_vky_layer_ctrl_1);
	printf("BM0: ctrl=%02X addr=%06X\n", m_vram_fc[0x1000], bm0_addr);
	printf("CLUT0[0..3]: RGB(%02X,%02X,%02X) RGB(%02X,%02X,%02X) RGB(%02X,%02X,%02X) RGB(%02X,%02X,%02X)\n",
		m_vram_fd[0x1002], m_vram_fd[0x1001], m_vram_fd[0x1000],
		m_vram_fd[0x1006], m_vram_fd[0x1005], m_vram_fd[0x1004],
		m_vram_fd[0x100a], m_vram_fd[0x1009], m_vram_fd[0x1008],
		m_vram_fd[0x100e], m_vram_fd[0x100d], m_vram_fd[0x100c]);
	printf("RAM[bm0_addr..+7]: %02X %02X %02X %02X %02X %02X %02X %02X\n",
		m_ram[bm0_addr & 0x1fffff], m_ram[(bm0_addr+1) & 0x1fffff], m_ram[(bm0_addr+2) & 0x1fffff], m_ram[(bm0_addr+3) & 0x1fffff],
		m_ram[(bm0_addr+4) & 0x1fffff], m_ram[(bm0_addr+5) & 0x1fffff], m_ram[(bm0_addr+6) & 0x1fffff], m_ram[(bm0_addr+7) & 0x1fffff]);
	printf("CPU PC: $%04X, INTC PEND: [%02X, %02X, %02X, %02X], MASK: [%02X, %02X, %02X, %02X], MMU_MEM_CTRL: $%02X\n",
		pc,
		m_int_pending[0], m_int_pending[1], m_int_pending[2], m_int_pending[3],
		m_int_mask[0], m_int_mask[1], m_int_mask[2], m_int_mask[3],
		m_mmu_mem_ctrl);

	uint8_t active_lut = m_mmu_mem_ctrl & 0x03;
	printf("Active LUT %d Blocks: [", active_lut);
	for (int s = 0; s < 8; s++)
		printf("%02X%s", m_mlut[active_lut][s], (s < 7) ? ", " : "]\n");

	uint16_t proc = (m_maincpu->space(AS_PROGRAM).read_byte(0x4b) << 8) | m_maincpu->space(AS_PROGRAM).read_byte(0x4c);
	uint16_t aprocq = (m_maincpu->space(AS_PROGRAM).read_byte(0x4d) << 8) | m_maincpu->space(AS_PROGRAM).read_byte(0x4e);
	uint16_t wprocq = (m_maincpu->space(AS_PROGRAM).read_byte(0x4f) << 8) | m_maincpu->space(AS_PROGRAM).read_byte(0x50);
	uint16_t sprocq = (m_maincpu->space(AS_PROGRAM).read_byte(0x51) << 8) | m_maincpu->space(AS_PROGRAM).read_byte(0x52);
	printf("D.Proc: $%04X, D.AProcQ: $%04X, D.WProcQ: $%04X, D.SProcQ: $%04X\n", proc, aprocq, wprocq, sprocq);

	printf("Code at PC: ");
	for (int i = 0; i < 16; i++)
		printf("%02X ", m_maincpu->space(AS_PROGRAM).read_byte(pc + i));
	printf("\n\n");
}

void wildbits_jr2_state::wbjr2(machine_config &config)
{
	// 6809 CPU clocked at 20 MHz in Turbo Mode (unscaled clock is 4x = 80 MHz)
	MC6809(config, m_maincpu, 4 * 20'000'000);
	m_maincpu->set_addrmap(AS_PROGRAM, &wildbits_jr2_state::wbjr2_mem);

	// SPI SD Card Controller
	SPI_SDCARD(config, m_sdcard);
	m_sdcard->set_prefer_sdhc();
	m_sdcard->spi_miso_callback().set(FUNC(wildbits_jr2_state::sdcard_miso_w));

	// Video Screen: 640x480 @ 60Hz
	SCREEN(config, m_screen, SCREEN_TYPE_RASTER);
	m_screen->set_raw(XTAL(25'175'000), 800, 0, 640, 525, 0, 480);
	m_screen->set_screen_update(FUNC(wildbits_jr2_state::screen_update));
	m_screen->screen_vblank().set(FUNC(wildbits_jr2_state::vblank_w));

	// Audio
	SPEAKER(config, "lspeaker").front_left();
	SPEAKER(config, "rspeaker").front_right();

	add_route(0, "lspeaker", 0.70);
	add_route(1, "rspeaker", 0.70);

	YMF262(config, m_opl3, XTAL(14'318'181));
	m_opl3->add_route(0, "lspeaker", 0.50);
	m_opl3->add_route(1, "rspeaker", 0.50);
	m_opl3->add_route(2, "lspeaker", 0.50);
	m_opl3->add_route(3, "rspeaker", 0.50);

	SN76489(config, m_psg_l, XTAL(14'318'181) / 4);
	m_psg_l->add_route(ALL_OUTPUTS, "lspeaker", 0.50);

	SN76489(config, m_psg_r, XTAL(14'318'181) / 4);
	m_psg_r->add_route(ALL_OUTPUTS, "rspeaker", 0.50);

	MOS6581(config, m_sid_l, XTAL(14'318'181) / 14);
	m_sid_l->add_route(ALL_OUTPUTS, "lspeaker", 0.50);

	MOS6581(config, m_sid_r, XTAL(14'318'181) / 14);
	m_sid_r->add_route(ALL_OUTPUTS, "rspeaker", 0.50);
}

struct key_map_entry {
	uint8_t port;
	uint16_t mask;
	uint8_t scancode;
	bool extended;
};

static const key_map_entry s_key_map[] = {
	// Port 0: Letters A-P
	{ 0, 0x0001, 0x1c, false }, // A
	{ 0, 0x0002, 0x32, false }, // B
	{ 0, 0x0004, 0x21, false }, // C
	{ 0, 0x0008, 0x23, false }, // D
	{ 0, 0x0010, 0x24, false }, // E
	{ 0, 0x0020, 0x2b, false }, // F
	{ 0, 0x0040, 0x34, false }, // G
	{ 0, 0x0080, 0x33, false }, // H
	{ 0, 0x0100, 0x43, false }, // I
	{ 0, 0x0200, 0x3b, false }, // J
	{ 0, 0x0400, 0x42, false }, // K
	{ 0, 0x0800, 0x4b, false }, // L
	{ 0, 0x1000, 0x3a, false }, // M
	{ 0, 0x2000, 0x31, false }, // N
	{ 0, 0x4000, 0x44, false }, // O
	{ 0, 0x8000, 0x4d, false }, // P

	// Port 1: Letters Q-Z, Numbers 0-5
	{ 1, 0x0001, 0x15, false }, // Q
	{ 1, 0x0002, 0x2d, false }, // R
	{ 1, 0x0004, 0x1b, false }, // S
	{ 1, 0x0008, 0x2c, false }, // T
	{ 1, 0x0010, 0x3c, false }, // U
	{ 1, 0x0020, 0x2a, false }, // V
	{ 1, 0x0040, 0x1d, false }, // W
	{ 1, 0x0080, 0x22, false }, // X
	{ 1, 0x0100, 0x35, false }, // Y
	{ 1, 0x0200, 0x1a, false }, // Z
	{ 1, 0x0400, 0x45, false }, // 0
	{ 1, 0x0800, 0x16, false }, // 1
	{ 1, 0x1000, 0x1e, false }, // 2
	{ 1, 0x2000, 0x26, false }, // 3
	{ 1, 0x4000, 0x25, false }, // 4
	{ 1, 0x8000, 0x2e, false }, // 5

	// Port 2: Numbers 6-9, Enter, Space, Backspace, Tab, Esc, -, =, [, ], ;, ', backslash
	{ 2, 0x0001, 0x36, false }, // 6
	{ 2, 0x0002, 0x3d, false }, // 7
	{ 2, 0x0004, 0x3e, false }, // 8
	{ 2, 0x0008, 0x46, false }, // 9
	{ 2, 0x0010, 0x5a, false }, // Enter
	{ 2, 0x0020, 0x29, false }, // Space
	{ 2, 0x0040, 0x66, false }, // Backspace
	{ 2, 0x0080, 0x0d, false }, // Tab
	{ 2, 0x0100, 0x76, false }, // Escape
	{ 2, 0x0200, 0x4e, false }, // -
	{ 2, 0x0400, 0x55, false }, // =
	{ 2, 0x0800, 0x54, false }, // [
	{ 2, 0x1000, 0x5b, false }, // ]
	{ 2, 0x2000, 0x4c, false }, // ;
	{ 2, 0x4000, 0x52, false }, // '
	{ 2, 0x8000, 0x5d, false }, // Backslash

	// Port 3: ,, ., /, `, Shift, Ctrl, Alt, CapsLock, Arrows, Del
	{ 3, 0x0001, 0x41, false }, // ,
	{ 3, 0x0002, 0x49, false }, // .
	{ 3, 0x0004, 0x4a, false }, // /
	{ 3, 0x0008, 0x0e, false }, // `
	{ 3, 0x0010, 0x12, false }, // Left Shift
	{ 3, 0x0020, 0x59, false }, // Right Shift
	{ 3, 0x0040, 0x14, false }, // Left Ctrl
	{ 3, 0x0080, 0x11, false }, // Left Alt
	{ 3, 0x0100, 0x58, false }, // Caps Lock
	{ 3, 0x0200, 0x75, true  }, // Up Arrow
	{ 3, 0x0400, 0x72, true  }, // Down Arrow
	{ 3, 0x0800, 0x6b, true  }, // Left Arrow
	{ 3, 0x1000, 0x74, true  }, // Right Arrow
	{ 3, 0x2000, 0x71, true  }, // Delete
	{ 3, 0x4000, 0x6c, true  }, // Home
	{ 3, 0x8000, 0x69, true  }, // End
};

void wildbits_jr2_state::poll_keyboard()
{
	uint16_t port_vals[4];
	for (int p = 0; p < 4; p++)
	{
		port_vals[p] = m_io_key[p]->read();
	}

	for (const auto &entry : s_key_map)
	{
		bool is_down = (port_vals[entry.port] & entry.mask) != 0;
		bool was_down = (m_key_state[entry.port] & entry.mask) != 0;

		if (is_down && !was_down)
		{
			if (entry.extended)
				queue_kbd_scancode(0xe0);
			queue_kbd_scancode(entry.scancode);
		}
		else if (!is_down && was_down)
		{
			if (entry.extended)
				queue_kbd_scancode(0xe0);
			queue_kbd_scancode(0xf0);
			queue_kbd_scancode(entry.scancode);
		}
	}

	for (int p = 0; p < 4; p++)
	{
		m_key_state[p] = port_vals[p];
	}
}

void wildbits_jr2_state::poll_mouse()
{
	int16_t cur_x = m_mouse_x_axis->read();
	int16_t cur_y = m_mouse_y_axis->read();
	uint8_t cur_btn = m_mouse_btn->read() & 0x07;

	int16_t dx = cur_x - m_last_mouse_x;
	int16_t dy = m_last_mouse_y - cur_y; // PS/2 standard: +Y is upward, -Y is downward

	if (dx != 0 || dy != 0 || cur_btn != m_last_mouse_btn)
	{
		m_last_mouse_x = cur_x;
		m_last_mouse_y = cur_y;
		m_last_mouse_btn = cur_btn;

		if (m_mouse_fifo.size() <= 12)
		{
			if (dx < -255) dx = -255;
			if (dx > 255) dx = 255;
			if (dy < -255) dy = -255;
			if (dy > 255) dy = 255;

			uint8_t b0 = 0x08 | (cur_btn & 0x07);
			if (dx < 0) b0 |= 0x10;
			if (dy < 0) b0 |= 0x20;

			uint8_t b1 = uint8_t(dx & 0xff);
			uint8_t b2 = uint8_t(dy & 0xff);

			m_mouse_fifo.push(b0);
			m_mouse_fifo.push(b1);
			m_mouse_fifo.push(b2);
			set_irq(0, 0x08); // INT_PS2_MOUSE (Bit 3 of Group 0)
		}
	}
}

void wildbits_jr2_state::vblank_w(int state)
{
	if (state)
	{
		m_frame_count++;
		poll_keyboard();
		poll_mouse();
		set_irq(0, 0x01); // INT_VKY_SOF (Start of Frame / VSYNC 60Hz tick)
	}
}

static INPUT_PORTS_START( wbjr2 )
	PORT_START("KEY0")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')

	PORT_START("KEY1")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR(')')
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')

	PORT_START("KEY2")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('^')
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('&')
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('*')
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR('(')
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB) PORT_CHAR(9)
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Escape") PORT_CODE(KEYCODE_ESC) PORT_CHAR(27)
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('+')
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR(':')
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('\'') PORT_CHAR('"')
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\') PORT_CHAR('|')

	PORT_START("KEY3")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_TILDE) PORT_CHAR('`') PORT_CHAR('~')
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("LShift") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("RShift") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL)
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Alt") PORT_CODE(KEYCODE_LALT) PORT_CODE(KEYCODE_RALT)
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Caps Lock") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Up") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Delete") PORT_CODE(KEYCODE_DEL) PORT_CHAR(UCHAR_MAMEKEY(DEL))
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Home") PORT_CODE(KEYCODE_HOME) PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("End") PORT_CODE(KEYCODE_END) PORT_CHAR(UCHAR_MAMEKEY(END))

	PORT_START("DIPSW")
	PORT_DIPNAME( 0x80, 0x80, "Gamma Correction" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x70, 0x70, "User DIP Switches" )
	PORT_DIPSETTING(    0x70, "000 (All Off)" )
	PORT_DIPSETTING(    0x60, "001" )
	PORT_DIPSETTING(    0x50, "010" )
	PORT_DIPSETTING(    0x40, "011" )
	PORT_DIPSETTING(    0x30, "100" )
	PORT_DIPSETTING(    0x20, "101" )
	PORT_DIPSETTING(    0x10, "110" )
	PORT_DIPSETTING(    0x00, "111 (All On)" )
	PORT_DIPNAME( 0x0e, 0x0e, "Boot Mode" )
	PORT_DIPSETTING(    0x0e, "Normal Boot" )
	PORT_DIPNAME( 0x01, 0x00, "Turbo Mode (Active-low: 0=Turbo, 1=Stock)" )
	PORT_DIPSETTING(    0x00, "Turbo Mode (20 MHz / LONGVIEW_MEMFAST)" )
	PORT_DIPSETTING(    0x01, "Stock Mode (16.67 MHz)" )

	PORT_START("MOUSEX")
	PORT_BIT( 0xffff, 0x0000, IPT_MOUSE_X ) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)

	PORT_START("MOUSEY")
	PORT_BIT( 0xffff, 0x0000, IPT_MOUSE_Y ) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)

	PORT_START("MOUSE_BUTTONS")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("Left Button") PORT_CODE(MOUSECODE_BUTTON1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("Right Button") PORT_CODE(MOUSECODE_BUTTON2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("Middle Button") PORT_CODE(MOUSECODE_BUTTON3)
INPUT_PORTS_END

ROM_START(wbjr2)
	ROM_REGION(0x80000, "flash", ROMREGION_ERASEFF)
	ROM_SYSTEM_BIOS(0, "turbo", "20 MHz Turbo Mode (Adaptive MMU Scheduler)")
	ROM_SYSTEM_BIOS(1, "stock", "Stock 16.67 MHz Mode (Adaptive MMU Scheduler)")
	ROM_DEFAULT_BIOS("turbo")
	ROM_LOAD("f0.dsk", 0x70000, 0x0a000, NO_DUMP)
	ROM_LOAD("booter", 0x7a000, 0x06000, NO_DUMP)
ROM_END

} // anonymous namespace

//    YEAR  NAME    PARENT  COMPAT  MACHINE  INPUT  CLASS               INIT        COMPANY     FULLNAME                       FLAGS
COMP( 2023, wbjr2,  0,      0,      wbjr2,   wbjr2, wildbits_jr2_state, empty_init, "Wildbits", "Wildbits Jr2 (FNX6809 Core)", MACHINE_SUPPORTS_SAVE )
