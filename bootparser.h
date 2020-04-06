#ifndef __BOOTPARSER_H__
#define __BOOTPARSER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <byteswap.h>

#define BOOT_TABLE 8
#define REG_CONF 4

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

enum E_section { TEXT, VECTORS, CINIT, DATA };

struct reg_conf {
  uint16_t addr;
  uint16_t val;
} __attribute__((packed));;

struct section {
  uint32_t size;
  uint32_t paddr;
	uint32_t offset;
  char name[128];
  struct section *next;
} __attribute__((packed));;

struct boot_table {
  uint32_t entry_point;
  uint32_t register_count;
} __attribute__((packed));

#endif
