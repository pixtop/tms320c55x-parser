#ifndef __COFF_H__
#define __COFF_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_HEADER       22
#define OPT_FILE_HEADER   28
#define SECTION_HEADER    48
#define SYMBOL_TABLE      18

#define le16_to_cpu(buf) ((uint16_t)*(buf) | (uint16_t)*(buf+1) << 8)
#define le32_to_cpu(buf) ((uint32_t)le16_to_cpu(buf) | (uint32_t)le16_to_cpu(buf+2) << 16)

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

struct coff_file_header {
  uint16_t  version;
  uint16_t  sh_size;  /* Number of section headers */
  int       date;     /* Time and date stamp */
  int       st_offset;  /* Symbol table's starting address */
  int       st_size;  /* Number of entries in the symbol table */
  uint16_t  oh_size;  /* Number of byte in the optionnal header */
  uint16_t  flags;
  uint16_t  magic;    /* Magic number: 0x9C for the TMS320c5500*/
} __attribute__((packed));

struct coff_opt_file_header {
  uint16_t  magic;
  uint16_t  version;
  uint32_t  exec_size;
  uint32_t  init_size;
  uint32_t  uninit_size;
  uint32_t  entry_point;
  uint32_t  exec_addr;
  uint32_t  init_addr;
} __attribute__((packed));

struct coff_section_header {
  union {
    char name[8];
    uint32_t index[2];
  };
  int       p_addr;
  int       v_addr;
  int       s_size;
  int       raw_offset;
  int       reloc_offset;
  int       reserved_0;
  uint32_t  reloc_nb;
  uint32_t  reserved_1;
  uint32_t  flags;
  uint16_t  reserved_2;
  uint16_t  page_nb;
} __attribute__((packed));

struct coff_symbol_table {
  char      name[8];
  int       value;
  short     section_n;
  uint16_t  reserved;
  uint8_t   st_class;
  uint8_t   aux_e;
} __attribute__((packed));

struct likelihood {
  uint32_t raw_offset;
  uint32_t score;
  struct likelihood *next;
} __attribute__((packed));

#endif
