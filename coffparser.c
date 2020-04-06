#include "coffparser.h"

FILE *fp;
FILE *fp_lib;
uint32_t txt_i;

void add_item(struct likelihood *list, struct likelihood *item) {
	while(list->next != NULL) list = list->next;
	list->next = item;
}

void print_list(struct likelihood *list, uint32_t max) {
  uint32_t i;
  for (i=0; list != NULL && i < max; i++) {
     printf("score:(%u), addr: 0x%x\n", list->score, list->raw_offset);
     list = list->next;
   }
 }

struct likelihood *max_item(struct likelihood *list) {
    struct likelihood *lh = list;
    uint32_t score = 0;
    while (list != NULL) {
      if (list->score > score) {
        lh = list;
        score = list->score;
      }
      list = list->next;
    }
    return lh;
}

int main(int argc, char const *argv[]) {
  if (argc < 2) {
		printf("usage: coffloader [file] [lib]\n");
		exit(1);
	}
	if ((fp_lib = fopen(argv[2], "r")) == NULL) {
		printf("Could not open %s\n", argv[1]);
		exit(1);
	}
  //Read file header
  struct coff_file_header fh;
  fread(&fh, FILE_HEADER, 1, fp_lib);
  printf("ver:0x%x\nsection:%u\nopt header:%d\nmagic:0x%x\n", fh.version, fh.sh_size, fh.oh_size, fh.magic);
  printf("symbol table offset:0x%08x\nsymbol table numbers:%u\n", fh.st_offset, fh.st_size);
  //Read optionnal file header
  if(fh.oh_size != 0) {
    struct coff_opt_file_header ofh;
    fread(&ofh, OPT_FILE_HEADER, 1, fp_lib);
  }
  //Read string table
  uint32_t saved_fp_lib = ftell(fp_lib);
  fseek(fp_lib, fh.st_offset+fh.st_size*SYMBOL_TABLE, SEEK_SET);
  uint32_t stt_size;
  fread(&stt_size, 4, 1, fp_lib);
  char *str_table = (char *)malloc(stt_size);
  fread(str_table, 1, stt_size, fp_lib);
  fseek(fp_lib, saved_fp_lib, SEEK_SET);
  //Read section headers
  uint32_t i;
  struct coff_section_header *sh = (struct coff_section_header *)malloc(sizeof(struct coff_section_header)*fh.sh_size);
  printf("Sections:\n");
  for(i=0; i<fh.sh_size; i++) {
    fread(&sh[i], SECTION_HEADER, 1, fp_lib);
    char *name;
    if (sh[i].index[0] == 0) {
      name = str_table+sh[i].index[1]-4; //seek name into the string table
    } else {
      strcpy(name, sh[i].name);
      if (strcmp(sh[i].name,".text") == 0) txt_i = i;
    }
    printf("name: %s addr: (0x%06x)[0x%06x - 0x%06x](%d)\n", name, sh[i].raw_offset, sh[i].p_addr, sh[i].p_addr+sh[i].s_size-1, sh[i].s_size);
  }
	// likelihood with first octet in common
  if ((fp = fopen(argv[1], "r")) == NULL) {
		printf("Could not open %s\n", argv[1]);
		exit(1);
	}
  fseek(fp, 0, SEEK_END);
  uint32_t f_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  uint8_t buf[2];
  struct likelihood *lh_sample = NULL;
  struct likelihood *lh;
  fseek(fp_lib, sh[txt_i].raw_offset, SEEK_SET);
  fread(buf, 1, 1, fp_lib);
  while(feof(fp) == 0) {
    fread(buf+1, 1, 1, fp);
    if (buf[0] == buf[1] && ftell(fp)-1+sh[txt_i].s_size < f_size) {
      lh = (struct likelihood *)malloc(sizeof(struct likelihood));
      memset(lh, 0, sizeof (struct likelihood));
      lh->raw_offset = ftell(fp)-1;
      if (lh_sample == NULL) {
        lh_sample = lh;
      } else {
        add_item(lh_sample, lh);
      }
    }
  }
  //For each candidate put a likelihood score
  lh = lh_sample;
  while (lh != NULL) {
    fseek(fp_lib, sh[txt_i].raw_offset, SEEK_SET);
    fseek(fp, lh->raw_offset, SEEK_SET);
    for(i=0; i<sh[txt_i].raw_offset; i++) {
      fread(buf, 1, 1, fp_lib);
      fread(buf+1, 1, 1, fp);
      if (buf[0] == buf[1]) lh->score++;
    }
    lh = lh->next;
  }
  print_list(max_item(lh_sample), 1);
  //Close files
  fclose(fp_lib);
  fclose(fp);
  return 0;
}
