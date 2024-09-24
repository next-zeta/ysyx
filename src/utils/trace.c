#include <common.h>
#include <elf.h>
#include <stdio.h>
#include <isa.h>
#include <device/map.h>

#define MAX_IRINGBUF 16

typedef struct {
    char name[64];
    paddr_t addr;     
    Elf32_Xword size;
} Symbol;
#ifdef CONFIG_ITRACE
void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);

typedef struct {
  word_t pc;
  uint32_t inst;
} ItraceNode;

ItraceNode iringbuf[MAX_IRINGBUF];
int iring_num = 0;

void get_iringbuf(word_t pc, uint32_t inst){
  iringbuf[iring_num].pc=pc;
  iringbuf[iring_num].inst=inst;
  iring_num++;
  if(iring_num == MAX_IRINGBUF + 1){
    iring_num=0;
  }
}

void display_iringbuf(){
  int end = iring_num;
  char *p;
  char buf[128];
  int i= iring_num + 1;

  if(i == MAX_IRINGBUF + 1){
    i = 0;
  }

  do{
    int t;
    t=i+1;
    if(t==MAX_IRINGBUF+1){
      t=0;
    }
    p = buf;
    p += sprintf(buf, "%s 0x%08x:   %08x   ", t== end ? "-->" : "   " , iringbuf[i].pc, iringbuf[i].inst);
    disassemble(p, buf + sizeof(buf) - p, iringbuf[i].pc, (uint8_t *)&iringbuf[i].inst, 4);
    puts(buf);
    i++;
    if(i == MAX_IRINGBUF + 1){
      i=0;
    }
  }while(i != end);
}
#endif

void display_memory_read(paddr_t addr, int len)
{
    printf(ANSI_FMT("read memory: ", ANSI_FG_GREEN) FMT_PADDR ", the len is %d\n", addr, len);
}

void display_memory_write(paddr_t addr, int len, word_t data)
{
    printf(ANSI_FMT("write memory: ", ANSI_FG_YELLOW) FMT_PADDR ", the len is %d, the written data is " FMT_WORD "\n", addr, len, data);
}

int func_num=0;
Symbol *symbol=NULL;
void init_elf(const char *elf_file){
  if(elf_file == NULL) return;

  FILE *fp; 
  fp = fopen(elf_file,"rb");

  if (fp == NULL)
	{
		printf("fail to open the file");
		exit(0);
	}

  Elf32_Ehdr ehdr;
  int a;
  a = fread(&ehdr,sizeof(Elf32_Ehdr),1,fp);
  if(a == 0){
    printf("fail to read the elf_file head\n");
    exit(0);
  }

  if (ehdr.e_ident[0] != 0x7F || ehdr.e_ident[1] != 'E' || ehdr.e_ident[2] != 'L' || ehdr.e_ident[3] != 'F')
	{
		printf("Not a ELF file\n");
		exit(0);
	}

  Elf32_Shdr *shdr = (Elf32_Shdr*)malloc(sizeof(Elf32_Shdr));
  char *strtab = NULL;

  fseek(fp,ehdr.e_shoff,SEEK_SET);
  int i;
  for(i=0;i<ehdr.e_shnum;i++){
    a=fread(shdr,sizeof(Elf32_Shdr),1,fp);
    if(a==0){
    printf("fail to read the elf_file section\n");
    }
    if(shdr->sh_type == SHT_STRTAB)
        {    
            strtab = malloc(shdr->sh_size);
            fseek(fp, shdr->sh_offset, SEEK_SET);
            if(fread(strtab, shdr->sh_size, 1, fp) <= 0)
            {
                printf("fail to read the strtab\n");
                exit(0);
            }
        }
  }

  fseek(fp,ehdr.e_shoff,SEEK_SET);
  for(i=0;i<ehdr.e_shnum;i++){
    a=fread(shdr,sizeof(Elf32_Shdr),1,fp);
    if(a==0){
    printf("fail to read the elf_file section\n");
    }
    if(shdr->sh_type == SHT_SYMTAB){

  fseek(fp,shdr->sh_offset,SEEK_SET);
  Elf32_Sym sym;
  size_t sym_count = shdr->sh_size / shdr->sh_entsize;
  symbol = malloc(sizeof(Symbol) * sym_count);
  size_t j=0;
  for(j=0;j<sym_count;j++){
    a=fread(&sym,sizeof(Elf32_Sym), 1, fp);
    if(a == 0){
      printf("fail to read elf_file SYMTAB\n");
    }
    if(ELF32_ST_TYPE(sym.st_info) == STT_FUNC){
      const char *name = strtab + sym.st_name;
      strncpy(symbol[func_num].name, name, sizeof(symbol[func_num].name) - 1);
      symbol[func_num].addr = sym.st_value;
      symbol[func_num].size = sym.st_size;
      func_num++;
    }
  }}}
  fclose(fp);
  free(strtab);
}
int dis_num = 0;
void display_call(word_t pc,word_t func_pc){
  int i;
  for(i=0;i<func_num;i++){
    if(func_pc >= symbol[i].addr && func_pc < (symbol[i].addr + symbol[i].size)){
      break;
    }
  }
  printf("0x%08x     call [%s @ 0x%08x]\n",pc,symbol[i].name,func_pc);
  dis_num++;
}

void display_ret(word_t pc){
  int i;
  for(i=0;i<func_num;i++){
    if(pc >= symbol[i].addr && pc < (symbol[i].addr + symbol[i].size)){
      break;
    }
  }

  printf("0x%08x     ret [%s]\n",pc,symbol[i].name);
} 

void device_read(paddr_t addr,int len,IOMap *map){
  printf(ANSI_FMT("READ DEVICE:  ", ANSI_FG_MAGENTA));
  printf("addr:0x%.8x,device:%.10s,len:%d\n",addr,map->name,len);
}

void device_write(paddr_t addr, int len, word_t data, IOMap *map){
  printf(ANSI_FMT("WRITE DEVICE:  ", ANSI_FG_BLACK));
  printf("addr:0x%.8x,device:%.10s,len:%d,data:%x\n",addr,map->name,len,data);
}

void etrace(){
  printf(ANSI_FMT("mtvec:  ", ANSI_FG_BLACK));
  printf("0x%x\n",cpu.csr.mtvec);
  printf(ANSI_FMT("mepc:  ", ANSI_FG_BLACK));
  printf("mepc: 0x%x\n",cpu.csr.mepc);
  printf(ANSI_FMT("mstatus:  ", ANSI_FG_BLACK));
  printf("mstatus: 0x%x\n",cpu.csr.mstatus);
  printf(ANSI_FMT("mcause:  ", ANSI_FG_BLACK));
  printf("mcause: %d\n",cpu.csr.mcause);
  printf(ANSI_FMT("a0:  ", ANSI_FG_BLACK));
  printf("a0: 0x%x\n",cpu.gpr[10]);
  printf(ANSI_FMT("sp:  ", ANSI_FG_BLACK));
  printf("sp: 0x%x\n",cpu.gpr[2]);
}