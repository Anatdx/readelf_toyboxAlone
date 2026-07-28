#ifndef READELF_TOYBOX_API_H
#define READELF_TOYBOX_API_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define YUKISU_READELF_API_VERSION 1U
#define YUKISU_READELF_IDENT_SIZE 16U

typedef enum yukisu_readelf_status {
  YUKISU_READELF_OK = 0,
  YUKISU_READELF_INVALID_ARGUMENT = 1,
  YUKISU_READELF_NOT_ELF = 2,
  YUKISU_READELF_TRUNCATED = 3,
  YUKISU_READELF_BAD_CLASS = 4,
  YUKISU_READELF_BAD_ENDIAN = 5,
  YUKISU_READELF_BAD_IDENT_VERSION = 6,
  YUKISU_READELF_MALFORMED = 7,
} yukisu_readelf_status;

enum yukisu_readelf_header_flags {
  YUKISU_READELF_EXTENDED_PHNUM = 1U << 0,
  YUKISU_READELF_EXTENDED_SHNUM = 1U << 1,
  YUKISU_READELF_EXTENDED_SHSTRNDX = 1U << 2,
};

typedef struct yukisu_readelf_error {
  uint32_t status;
  uint32_t reserved;
  uint64_t offset;
} yukisu_readelf_error;

typedef struct yukisu_readelf_header {
  uint32_t api_version;
  uint32_t struct_size;
  uint64_t file_size;
  uint8_t ident[YUKISU_READELF_IDENT_SIZE];
  uint8_t elf_class;
  uint8_t data_encoding;
  uint8_t ident_version;
  uint8_t os_abi;
  uint8_t abi_version;
  uint8_t reserved[3];
  uint16_t type;
  uint16_t machine;
  uint32_t elf_version;
  uint64_t entry;
  uint64_t program_header_offset;
  uint64_t section_header_offset;
  uint32_t flags;
  uint32_t header_flags;
  uint16_t header_size;
  uint16_t program_header_entry_size;
  uint16_t program_header_count;
  uint16_t section_header_entry_size;
  uint16_t section_header_count;
  uint16_t section_name_index;
} yukisu_readelf_header;

// Decode an ELF header from its leading bytes. header_bytes must contain at
// least the complete class-specific ELF header (52 bytes for ELF32, 64 for
// ELF64); file_size is the size of the complete object containing it.
//
// This API is reentrant and never writes to stdio, exits, or uses Toybox
// global state. The Toybox readelf CLI uses this same decoder.
yukisu_readelf_status yukisu_readelf_parse_header(
    const void* header_bytes,
    size_t header_bytes_size,
    uint64_t file_size,
    yukisu_readelf_header* output,
    yukisu_readelf_error* error);

const char* yukisu_readelf_status_string(yukisu_readelf_status status);

#ifdef __cplusplus
}
#endif

#endif
