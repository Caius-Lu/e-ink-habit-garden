// ESP8266 专用兼容头（由 build_flags 的 -include 强制注入所有编译单元，包括第三方库）
// 作用：让 u8g2 的大中文字体放进 flash（.irom.text）而不是 RAM，
//       并把字体字节读取换成对齐安全的 pgm_read_byte（flash 映射区非对齐直读会崩溃）
#pragma once

#ifdef ESP8266

// 每个字体一个独立段（.irom.text.font.字体名），这样链接器能按字体裁剪，
// 只保留实际引用的字体；段名以 .irom.text 开头会被放进 flash 区
#define U8X8_FONT_SECTION(name) __attribute__((section(".irom.text.font." #name)))
#define u8x8_pgm_read(adr) pgm_read_byte(adr)

#include <sys/pgmspace.h>

#endif
