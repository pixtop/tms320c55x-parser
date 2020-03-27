#include "bootparser.h"

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

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("usage: bootloader [file]\n");
		exit(1);
	}
	FILE *fp;
	if ((fp = fopen(argv[1], "r")) == NULL) {
		printf("Could not open %s\n", argv[1]);
		exit(1);
	}
	struct boot_table btable;
	uint8_t buf[4];
	fread(buf, 1, sizeof buf, fp);
	btable.entry_point = be32_to_cpu(buf);
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
	btable.l_section = NULL;
	while(1) {
		struct section *psec = (struct section *)malloc(sizeof(struct section));
		fread(buf, 1, sizeof buf, fp);
		psec->size = be32_to_cpu(buf);
		if (psec->size == 0) {
			free(psec);
			break;
		}
		fread(buf, 1, sizeof buf, fp);
		psec->addr = be32_to_cpu(buf);
		psec->next = NULL;
		if (btable.l_section != NULL)	
			last_item(btable.l_section)->next = psec;
		else
			btable.l_section = psec;
		if ((ftell(fp)+psec->size)%2 != 0)
			fseek(fp, psec->size+1, SEEK_CUR);
		else
			fseek(fp, psec->size, SEEK_CUR);
		printf("section address: [0x%06x - 0x%06x], size: 0x%x \t(%s)\n", psec->addr, psec->addr+psec->size-1, psec->size, mem_map(psec->addr));
	}
	fclose(fp);
	exit(0);
}	
