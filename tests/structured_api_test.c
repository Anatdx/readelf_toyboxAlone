#include "readelf_toybox_api.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void put16(uint8_t* output, uint16_t value, int big_endian)
{
  if (big_endian) {
    output[0] = value >> 8;
    output[1] = value;
  } else {
    output[0] = value;
    output[1] = value >> 8;
  }
}

static void put32(uint8_t* output, uint32_t value, int big_endian)
{
  int i;
  for (i = 0; i < 4; ++i)
    output[big_endian ? 3-i : i] = value >> (i*8);
}

static void put64(uint8_t* output, uint64_t value, int big_endian)
{
  int i;
  for (i = 0; i < 8; ++i)
    output[big_endian ? 7-i : i] = value >> (i*8);
}

static void make_elf64(uint8_t* output, int big_endian)
{
  memset(output, 0, 256);
  memcpy(output, "\177ELF", 4);
  output[4] = 2;
  output[5] = big_endian ? 2 : 1;
  output[6] = 1;
  put16(output+16, 1, big_endian);
  put16(output+18, 183, big_endian);
  put32(output+20, 1, big_endian);
  put64(output+24, UINT64_C(0xfedcba9876543210), big_endian);
  put64(output+32, 64, big_endian);
  put64(output+40, 120, big_endian);
  put32(output+48, 0, big_endian);
  put16(output+52, 64, big_endian);
  put16(output+54, 56, big_endian);
  put16(output+56, 1, big_endian);
  put16(output+58, 64, big_endian);
  put16(output+60, 2, big_endian);
  put16(output+62, 1, big_endian);
}

static void make_elf32(uint8_t* output, int big_endian)
{
  memset(output, 0, 160);
  memcpy(output, "\177ELF", 4);
  output[4] = 1;
  output[5] = big_endian ? 2 : 1;
  output[6] = 1;
  put16(output+16, 1, big_endian);
  put16(output+18, 40, big_endian);
  put32(output+20, 1, big_endian);
  put32(output+24, 0x87654321U, big_endian);
  put32(output+28, 52, big_endian);
  put32(output+32, 84, big_endian);
  put32(output+36, 0x05000000U, big_endian);
  put16(output+40, 52, big_endian);
  put16(output+42, 32, big_endian);
  put16(output+44, 1, big_endian);
  put16(output+46, 40, big_endian);
  put16(output+48, 1, big_endian);
  put16(output+50, 0, big_endian);
}

static void test_elf64(int big_endian)
{
  uint8_t bytes[256];
  yukisu_readelf_header header;
  yukisu_readelf_error error;

  make_elf64(bytes, big_endian);
  assert(yukisu_readelf_parse_header(bytes, 64, sizeof(bytes), &header,
                                     &error) == YUKISU_READELF_OK);
  assert(error.status == YUKISU_READELF_OK);
  assert(header.api_version == YUKISU_READELF_API_VERSION);
  assert(header.elf_class == 2);
  assert(header.data_encoding == (big_endian ? 2 : 1));
  assert(header.machine == 183);
  assert(header.entry == UINT64_C(0xfedcba9876543210));
  assert(header.program_header_offset == 64);
  assert(header.section_header_offset == 120);
  assert(header.program_header_count == 1);
  assert(header.section_header_count == 2);
}

static void test_elf32(int big_endian)
{
  uint8_t bytes[160];
  yukisu_readelf_header header;

  make_elf32(bytes, big_endian);
  assert(yukisu_readelf_parse_header(bytes, 52, sizeof(bytes), &header, 0)
         == YUKISU_READELF_OK);
  assert(header.elf_class == 1);
  assert(header.data_encoding == (big_endian ? 2 : 1));
  assert(header.machine == 40);
  assert(header.entry == UINT32_C(0x87654321));
  assert(header.flags == UINT32_C(0x05000000));
  assert(header.program_header_entry_size == 32);
  assert(header.section_header_entry_size == 40);
}

int main(void)
{
  uint8_t bytes[256];
  yukisu_readelf_header header;
  yukisu_readelf_error error;

  test_elf64(0);
  test_elf64(1);
  test_elf32(0);
  test_elf32(1);

  make_elf64(bytes, 0);
  assert(yukisu_readelf_parse_header(bytes, 4, sizeof(bytes), &header,
                                     &error) == YUKISU_READELF_TRUNCATED);
  bytes[4] = 0;
  assert(yukisu_readelf_parse_header(bytes, 64, sizeof(bytes), &header,
                                     &error) == YUKISU_READELF_BAD_CLASS);
  make_elf64(bytes, 0);
  bytes[5] = 0;
  assert(yukisu_readelf_parse_header(bytes, 64, sizeof(bytes), &header,
                                     &error) == YUKISU_READELF_BAD_ENDIAN);
  make_elf64(bytes, 0);
  bytes[6] = 0;
  assert(yukisu_readelf_parse_header(bytes, 64, sizeof(bytes), &header,
                                     &error) ==
         YUKISU_READELF_BAD_IDENT_VERSION);
  make_elf64(bytes, 0);
  put64(bytes+40, 240, 0);
  assert(yukisu_readelf_parse_header(bytes, 64, sizeof(bytes), &header,
                                     &error) == YUKISU_READELF_MALFORMED);
  make_elf64(bytes, 0);
  put16(bytes+56, 0xffff, 0);
  put16(bytes+54, 0, 0);
  assert(yukisu_readelf_parse_header(bytes, 64, sizeof(bytes), &header,
                                     &error) == YUKISU_READELF_MALFORMED);
  make_elf64(bytes, 0);
  put16(bytes+56, 0xffff, 0);
  put32(bytes+120+44, 1, 0);
  assert(yukisu_readelf_parse_header(bytes, 64, sizeof(bytes), &header,
                                     &error) == YUKISU_READELF_OK);
  assert(header.header_flags & YUKISU_READELF_EXTENDED_PHNUM);

  puts("structured readelf API tests passed");
  return 0;
}
