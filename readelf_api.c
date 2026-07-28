/*
 * Structured ELF header API extracted from Toybox readelf's header decoder.
 *
 * Copyright 2019 The Android Open Source Project
 * SPDX-License-Identifier: 0BSD
 */

#include "readelf_toybox_api.h"

#include <string.h>

enum {
  ELFCLASS32_VALUE = 1,
  ELFCLASS64_VALUE = 2,
  ELFDATA2LSB_VALUE = 1,
  ELFDATA2MSB_VALUE = 2,
  EV_CURRENT_VALUE = 1,
  PN_XNUM_VALUE = 0xffff,
  SHN_XINDEX_VALUE = 0xffff,
};

static void set_error(yukisu_readelf_error* error,
                      yukisu_readelf_status status, uint64_t offset)
{
  if (!error) return;
  memset(error, 0, sizeof(*error));
  error->status = (uint32_t)status;
  error->offset = offset;
}

static uint16_t read_u16(const uint8_t* data, int big_endian)
{
  if (big_endian) return ((uint16_t)data[0] << 8) | data[1];
  return ((uint16_t)data[1] << 8) | data[0];
}

static uint32_t read_u32(const uint8_t* data, int big_endian)
{
  uint32_t value = 0;
  int i;

  if (big_endian) {
    for (i = 0; i < 4; ++i) value = (value << 8) | data[i];
  } else {
    for (i = 3; i >= 0; --i) value = (value << 8) | data[i];
  }
  return value;
}

static uint64_t read_u64(const uint8_t* data, int big_endian)
{
  uint64_t value = 0;
  int i;

  if (big_endian) {
    for (i = 0; i < 8; ++i) value = (value << 8) | data[i];
  } else {
    for (i = 7; i >= 0; --i) value = (value << 8) | data[i];
  }
  return value;
}

static int table_fits(uint64_t offset, uint16_t entry_size, uint16_t count,
                      uint64_t file_size)
{
  uint64_t table_size;

  if (!count) return 1;
  if (!offset || !entry_size || offset > file_size) return 0;
  table_size = (uint64_t)entry_size * count;
  return table_size <= file_size - offset;
}

yukisu_readelf_status yukisu_readelf_parse_header(
    const void* header_bytes, size_t header_bytes_size, uint64_t file_size,
    yukisu_readelf_header* output, yukisu_readelf_error* error)
{
  const uint8_t* bytes = (const uint8_t*)header_bytes;
  const uint8_t* cursor;
  size_t required_size;
  uint16_t expected_program_entry_size, expected_section_entry_size;
  int big_endian;

  if (!bytes || !output) {
    set_error(error, YUKISU_READELF_INVALID_ARGUMENT, 0);
    return YUKISU_READELF_INVALID_ARGUMENT;
  }

  memset(output, 0, sizeof(*output));
  output->api_version = YUKISU_READELF_API_VERSION;
  output->struct_size = (uint32_t)sizeof(*output);
  output->file_size = file_size;

  if (header_bytes_size < 4 || file_size < 4) {
    set_error(error, YUKISU_READELF_TRUNCATED, header_bytes_size);
    return YUKISU_READELF_TRUNCATED;
  }
  if (memcmp(bytes, "\177ELF", 4)) {
    set_error(error, YUKISU_READELF_NOT_ELF, 0);
    return YUKISU_READELF_NOT_ELF;
  }
  if (header_bytes_size < YUKISU_READELF_IDENT_SIZE ||
      file_size < YUKISU_READELF_IDENT_SIZE) {
    set_error(error, YUKISU_READELF_TRUNCATED, header_bytes_size);
    return YUKISU_READELF_TRUNCATED;
  }

  memcpy(output->ident, bytes, YUKISU_READELF_IDENT_SIZE);
  output->elf_class = bytes[4];
  output->data_encoding = bytes[5];
  output->ident_version = bytes[6];
  output->os_abi = bytes[7];
  output->abi_version = bytes[8];

  if (output->elf_class != ELFCLASS32_VALUE &&
      output->elf_class != ELFCLASS64_VALUE) {
    set_error(error, YUKISU_READELF_BAD_CLASS, 4);
    return YUKISU_READELF_BAD_CLASS;
  }
  if (output->data_encoding != ELFDATA2LSB_VALUE &&
      output->data_encoding != ELFDATA2MSB_VALUE) {
    set_error(error, YUKISU_READELF_BAD_ENDIAN, 5);
    return YUKISU_READELF_BAD_ENDIAN;
  }
  if (output->ident_version != EV_CURRENT_VALUE) {
    set_error(error, YUKISU_READELF_BAD_IDENT_VERSION, 6);
    return YUKISU_READELF_BAD_IDENT_VERSION;
  }

  required_size = output->elf_class == ELFCLASS64_VALUE ? 64U : 52U;
  if (header_bytes_size < required_size || file_size < required_size) {
    set_error(error, YUKISU_READELF_TRUNCATED,
              header_bytes_size < required_size ? header_bytes_size : file_size);
    return YUKISU_READELF_TRUNCATED;
  }

  big_endian = output->data_encoding == ELFDATA2MSB_VALUE;
  cursor = bytes + YUKISU_READELF_IDENT_SIZE;
  output->type = read_u16(cursor, big_endian);
  cursor += 2;
  output->machine = read_u16(cursor, big_endian);
  cursor += 2;
  output->elf_version = read_u32(cursor, big_endian);
  cursor += 4;

  if (output->elf_class == ELFCLASS64_VALUE) {
    output->entry = read_u64(cursor, big_endian);
    cursor += 8;
    output->program_header_offset = read_u64(cursor, big_endian);
    cursor += 8;
    output->section_header_offset = read_u64(cursor, big_endian);
    cursor += 8;
    expected_program_entry_size = 56;
    expected_section_entry_size = 64;
  } else {
    output->entry = read_u32(cursor, big_endian);
    cursor += 4;
    output->program_header_offset = read_u32(cursor, big_endian);
    cursor += 4;
    output->section_header_offset = read_u32(cursor, big_endian);
    cursor += 4;
    expected_program_entry_size = 32;
    expected_section_entry_size = 40;
  }

  output->flags = read_u32(cursor, big_endian);
  cursor += 4;
  output->header_size = read_u16(cursor, big_endian);
  cursor += 2;
  output->program_header_entry_size = read_u16(cursor, big_endian);
  cursor += 2;
  output->program_header_count = read_u16(cursor, big_endian);
  cursor += 2;
  output->section_header_entry_size = read_u16(cursor, big_endian);
  cursor += 2;
  output->section_header_count = read_u16(cursor, big_endian);
  cursor += 2;
  output->section_name_index = read_u16(cursor, big_endian);

  if (output->program_header_count == PN_XNUM_VALUE)
    output->header_flags |= YUKISU_READELF_EXTENDED_PHNUM;
  if (!output->section_header_count && output->section_header_offset)
    output->header_flags |= YUKISU_READELF_EXTENDED_SHNUM;
  if (output->section_name_index == SHN_XINDEX_VALUE)
    output->header_flags |= YUKISU_READELF_EXTENDED_SHSTRNDX;

  if (output->elf_version != EV_CURRENT_VALUE ||
      output->header_size < required_size ||
      output->program_header_offset > file_size ||
      output->section_header_offset > file_size ||
      (output->program_header_count &&
       output->program_header_entry_size < expected_program_entry_size) ||
      ((output->section_header_count || output->header_flags) &&
       output->section_header_entry_size < expected_section_entry_size) ||
      (output->header_flags &&
       !table_fits(output->section_header_offset,
                   output->section_header_entry_size, 1, file_size)) ||
      (output->program_header_count != PN_XNUM_VALUE &&
       !table_fits(output->program_header_offset,
                   output->program_header_entry_size,
                   output->program_header_count, file_size)) ||
      !table_fits(output->section_header_offset,
                  output->section_header_entry_size,
                  output->section_header_count, file_size)) {
    set_error(error, YUKISU_READELF_MALFORMED,
              output->program_header_offset > file_size
                  ? output->program_header_offset
                  : output->section_header_offset);
    return YUKISU_READELF_MALFORMED;
  }

  set_error(error, YUKISU_READELF_OK, 0);
  return YUKISU_READELF_OK;
}

const char* yukisu_readelf_status_string(yukisu_readelf_status status)
{
  switch (status) {
    case YUKISU_READELF_OK: return "ok";
    case YUKISU_READELF_INVALID_ARGUMENT: return "invalid argument";
    case YUKISU_READELF_NOT_ELF: return "not ELF";
    case YUKISU_READELF_TRUNCATED: return "truncated ELF header";
    case YUKISU_READELF_BAD_CLASS: return "unsupported ELF class";
    case YUKISU_READELF_BAD_ENDIAN: return "unsupported ELF data encoding";
    case YUKISU_READELF_BAD_IDENT_VERSION: return "bad ELF identification version";
    case YUKISU_READELF_MALFORMED: return "malformed ELF header";
  }
  return "unknown readelf error";
}
