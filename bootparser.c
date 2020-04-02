#include "bootparser.h"

struct boot_table btable;
struct section *sections;
struct reg_conf *registers;
FILE *fp;
uint32_t cinit_paddr;

char *mem_map(uint32_t addr) {
	if (addr >= 0 && addr < 0xC0)
		return "MMR (Reserved)";
	else if (addr >= 0xC0 && addr < 0x10000)
		return "DARAM";
	else if (addr >= 0x10000 && addr < 0x40000)
		return "SARAM";
	else if (addr >= 0x40000 && addr < 0xFF0000)
		return "External";
	else if (addr >= 0xFF0000 && addr < 0xFFC000)
	       return "ROM|External";
	else if (addr >= 0xFFC000 && addr < 0xFFFFFF)
		return "External";
	else
		return "Unkown mapping";
}

struct section *last_item(struct section *sec) {
	while(sec->next != NULL) sec = sec->next;
	return sec;
}

uint32_t shift(uint32_t p_addr) {
	struct section *psec = sections;
	while (psec != NULL) {
		if (p_addr >= psec->paddr && p_addr < psec->paddr+psec->size) {
			return psec->paddr - psec->offset;
		}
		psec = psec->next;
	}
	return 0;
}

void cinit_finder(struct section s) {
	uint8_t opcode;
	uint32_t fp_offset = ftell(fp);
	uint32_t boot_addr = btable.entry_point - shift(btable.entry_point);
	// Diving into boot.asm
	fseek(fp, boot_addr, SEEK_SET);
	uint32_t addr = 0;
	while (1) {
		fread(&opcode, 1, 1, fp);
		if (opcode == 0x86) {
			// Diving into autoinit.asm
			fread(&addr, 4, 1, fp);
			addr = (addr & 0xFFFFFF00); //remove opcode;
			fseek(fp, (__bswap_32(addr)) - shift(__bswap_32(addr)) /* @autoinit */, SEEK_SET);
			while (1) {
				fread(&opcode, 1, 1, fp);
				if (opcode == 0xec /* amar opcode */) {
					fseek(fp, 2, SEEK_CUR);
					fread(&addr, 3, 1, fp);
					addr = addr << 8;
					cinit_paddr = __bswap_32(addr)*2; //word addr
					fseek(fp, fp_offset, SEEK_SET);
					return;
				}
			}
		}
	}
}

int sname_props(struct section s, enum E_section e) {
	int status = 0;
	switch(e) {
		case TEXT:
			//entry point is located inside .text section
			status = btable.entry_point > s.paddr && btable.entry_point < s.paddr+s.size ? 1:0;
			break;
		case VECTORS:
			//IVT have a 0x100 size and is aligned
			status = (s.size == 0x100 && (s.paddr & 0xFF) == 0) ? 1:0;
			break;
		case CINIT:
			status = (s.paddr == cinit_paddr) ? 1:0;
			break;
		default:
			break;
	}
	return status;
}

void sname_finder(struct section *psec) {
	if (sname_props(*psec, TEXT)) {
		cinit_finder(*psec);
		strcpy(psec->name, ".text");
	} else if (sname_props(*psec, VECTORS)) {
		strcpy(psec->name, ".vectors");
	} else if (sname_props(*psec, CINIT)) {
		strcpy(psec->name, ".cinit");
	}
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("usage: bootloader [file]\n");
		exit(1);
	}
	if ((fp = fopen(argv[1], "r")) == NULL) {
		printf("Could not open %s\n", argv[1]);
		exit(1);
	}
	fread(&btable, BOOT_TABLE, 1, fp);
	//Read entry point
	btable.entry_point = __bswap_32(btable.entry_point);
	printf("0x%x\n", btable.entry_point);
	//Read Register configuration
	btable.register_count = __bswap_32(btable.register_count);
	int i;
	if (btable.register_count > 0) {
		registers = (struct reg_conf *)malloc(REG_CONF*btable.register_count);
		for(i=0; i<btable.register_count; i++) {
			fread(&registers[i], REG_CONF, 1, fp);
			registers[i].addr = __bswap_16(registers[i].addr);
			registers[i].val = __bswap_16(registers[i].val);
		}
	}
	//Print Boot table headers
	UNDERLINE;
	printf("Boot Table TMS320c55x\n");
	UNDERLINE;
	printf("entry point: 0x%06x\n", btable.entry_point);
	UNDERLINE;
	printf("register count: %u\n", btable.register_count);
	UNDERLINE;
	for(i=0; i<btable.register_count; i++) {
		printf("register address: 0x%x, value: 0x%x\n", registers[i].addr, registers[i].val);
		UNDERLINE;
	}
	//Looking for sections
	struct section *psec;
	while(1) {
		psec = (struct section *)malloc(sizeof(struct section));
		memset(psec, 0, sizeof(struct section));
		fread(psec, 8, 1, fp);
		psec->size = __bswap_32(psec->size);
		if (psec->size == 0) {
			free(psec);
			break;
		}
		psec->paddr = __bswap_32(psec->paddr);
		psec->offset = ftell(fp);

		if (sections != NULL)
			last_item(sections)->next = psec;
		else
			sections = psec;

		if ((ftell(fp)+psec->size)%2 != 0)
			fseek(fp, psec->size+1, SEEK_CUR);
		else
			fseek(fp, psec->size, SEEK_CUR);
	}
	psec = sections;
	//Finding section name then print
	while(psec != NULL) {
		sname_finder(psec);
		printf("section [%s] \taddress: (0x%06x)[0x%06x - 0x%06x], size: 0x%x \t(%s)\n", psec->name, psec->offset, psec->paddr, psec->paddr+psec->size-1, psec->size, mem_map(psec->paddr));
		psec = psec->next;
	}
	fclose(fp);
	exit(0);
}
