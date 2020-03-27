#ifndef __BOOTPARSER_H__
#define __BOOTPARSER_H__

#include <stdio.h>
#include <stdlib.h>

#define be16_to_cpu(buf) ((uint16_t)*(buf+1) | (uint16_t)*(buf) << 8)
#define be32_to_cpu(buf) ((uint32_t)be16_to_cpu(buf+2) | (uint32_t)be16_to_cpu(buf) << 16)

#define UNDERLINE printf("-------------------------\n")

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

struct reg_conf {
        uint16_t addr;
        uint16_t val;
};

struct section {
        uint32_t size;
        uint32_t addr;
        struct section *next;
};

struct section *last_item(struct section *sec) {
        while(sec->next != NULL)
                sec = sec->next;
        return sec;
}

struct boot_table {
        uint32_t entry_point;
        uint32_t register_count;
        struct reg_conf *register_conf;
        struct section *l_section;
};

#endif
