/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include "paddr.h"
#include "watchpoint.h"

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
	nemu_state.state = NEMU_QUIT;   
	return -1;
}
static int cmd_x(char *args);
static int cmd_help(char *args);
static int cmd_si(char *args);
static int cmd_info(char *args);
static int cmd_p(char *args);
static int cmd_w(char *args);
static int cmd_d(char *args);
void add_w(char *args);
void d_w(int n);
void sdb_w_display();

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si","Single step execution",cmd_si },
  { "info","Print program status",cmd_info },
  { "x","Print area",cmd_x},
  { "p","Expression evaluation",cmd_p},
  { "d","Delete the watchpoint",cmd_d},
  { "w","Add watchpoint",cmd_w},
  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)
static int cmd_si(char *args){
	int step = 0;
	if(args==NULL){
		step = 1;
	}
	else{
		sscanf(args,"%d",&step);
	}
	cpu_exec(step);
	return 0;
}

static int cmd_info(char *args){
	if(strcmp("r",args)==0){
		isa_reg_display();
	}
	if(strcmp("w",args)==0){
		sdb_w_display();
	}
	return 0;
}

static int cmd_w(char *args){
	add_w(args);
	return 0;
}

static int cmd_d(char *args){
	int n = atoi(args);
	d_w(n);
	return 0;
}

static int cmd_x(char *args){
	int len=0;
	args=strtok(args," ");
	sscanf(args,"%d",&len);
	args=strtok(NULL," ");
	paddr_t addr=0;
	sscanf(args,"%x",&addr);
	for(int i=0;i<len;i++){
		printf("0X%.8x           0X%.8x\n",addr,paddr_read(addr,4));
		addr+=4;
	}
	return 0;
}

static int cmd_p(char *args){
	if(args==NULL){
		printf("NO expression\n");
		return 0;
	}
	bool success = true;
	expr(args,&success);
	return 0;
}

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}



void sdb_w_display(){
	int t=0;
	for(int i=0;i<NR_WP;i++){
		if(wp_pool[i].success == true){
			printf("%d  %s  %d\n",wp_pool[i].NO,wp_pool[i].expr,wp_pool[i].value);
			t++;
		}
	}
	if(t==0){
		printf("No watchpoint.\n");
	}
}

void d_w(int n){
	for(int i=0;i<NR_WP;i++){
		if(wp_pool[i].NO == n){
			free_wp(&wp_pool[i]);
			return;
		}
	}
}

void add_w(char *args){
	WP* np = new_wp();
	strcpy(np -> expr,args);
	int tmp;
	bool success = true;
	tmp=expr(np -> expr,&success);
	if(success){
		np -> value = tmp;
		printf("new watchpoint:%d\n",np -> NO);
	}
	else{
		printf("expr error!\n");
	}
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
