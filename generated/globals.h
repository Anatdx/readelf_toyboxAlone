struct readelf_data {
  char *x, *p;

  char *elf, *shstrtab, *f;
  unsigned long long shoff, phoff, size, shstrtabsz;
  int bits, endian, shnum, shentsize, phentsize;
};
extern union global_union {
	struct readelf_data readelf;
} this;
