#include "bootparser.h"

struct boot_table btable = {.l_section = NULL};
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

uint32_t shift(uint32_t p_addr) {
	struct section *psec = btable.l_section;
	while (psec != NULL) {
		if (p_addr >= psec->paddr && p_addr < psec->paddr+psec->size) {
			return psec->paddr - psec->offset;
		}
		psec = psec->next;
	}
	return 0;
}

void cinit_finder(struct section s) {
	uint8_t buf[4];
	uint32_t fp_offset = ftell(fp);
	uint32_t boot_addr = btable.entry_point - shift(btable.entry_point);
	fseek(fp, boot_addr, SEEK_SET);
	while (1) {
		fread(buf, 1, 1, fp);
		if (buf[0] == 0x86) {
			fread(buf, 1, 4, fp);
			buf[0] = 0;
			fseek(fp, be32_to_cpu(buf) - shift(be32_to_cpu(buf)) /* @autoinit */, SEEK_SET);
			while (1) {
				fread(buf, 1, 1, fp);
				if (buf[0] == 0xec /* amar opcode */) {
					fread(buf, 1, 2, fp);
					fread(&buf[1], 1, 3, fp);
					buf[0] = 0;
					cinit_paddr = be32_to_cpu(buf)*2; //word addr
					fseek(fp, fp_offset, SEEK_SET);
					return;
				}
			}
		}
	}
}

int sname_props(struct section s, enum E_section e) {
	int status = 0;
	//uint8_t buf[2];
	//uint32_t saved_fp = ftell(fp);
	switch(e) {
		case TEXT:
			//entry point is located inside .text section
			status = btable.entry_point > s.paddr && btable.entry_point < s.paddr+s.size ? 1:0;
			break;
		case VECTORS:
			//IVT have a 0x100 size and is aligned
			status = s.size == 0x100 && (s.paddr & 0xFF) == 0 ? 1:0;
			break;
		case CINIT:
			status = s.paddr == cinit_paddr ? 1:0;
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
	uint8_t buf[4];
	//Read entry point
	fread(buf, 1, sizeof buf, fp);
	btable.entry_point = be32_to_cpu(buf);
	//Read Register configuration
	fread(buf, 1, sizeof buf, fp);
	btable.register_count = be32_to_cpu(buf);
	int i;
	if (btable.register_count > 0) {
		btable.register_conf = (struct reg_conf *)malloc(sizeof(struct reg_conf)*btable.register_count);
		for(i=0; i<btable.register_count; i++) {
			fread(buf, 1, sizeof buf, fp);
			btable.register_conf[i].addr=be16_to_cpu(buf);
			btable.register_conf[i].val=be16_to_cpu(buf+2);
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
		printf("register address: 0x%x, value: 0x%x\n", btable.register_conf[i].addr, btable.register_conf[i].val);
		UNDERLINE;
	}
	//Looking for sections
	struct section *psec;
	while(1) {
		psec = (struct section *)malloc(sizeof(struct section));
		memset(psec, 0, sizeof(struct section));
		fread(buf, 1, sizeof buf, fp);
		psec->size = be32_to_cpu(buf);
		if (psec->size == 0) {
			free(psec);
			break;
		}
		fread(buf, 1, sizeof buf, fp);
		psec->paddr = be32_to_cpu(buf);
		psec->offset = ftell(fp);
		if (btable.l_section != NULL)
			last_item(btable.l_section)->next = psec;
		else
			btable.l_section = psec;
		if ((ftell(fp)+psec->size)%2 != 0)
			fseek(fp, psec->size+1, SEEK_CUR);
		else
			fseek(fp, psec->size, SEEK_CUR);
	}
	psec = btable.l_section;
	//Finding section name then print
	while(psec != NULL) {
		sname_finder(psec);
		printf("section [%s] \taddress: (0x%06x)[0x%06x - 0x%06x], size: 0x%x \t(%s)\n", psec->name, psec->offset, psec->paddr, psec->paddr+psec->size-1, psec->size, mem_map(psec->paddr));
		psec = psec->next;
	}
	fclose(fp);
	exit(0);
}
