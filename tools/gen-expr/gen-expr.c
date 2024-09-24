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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>
// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format=
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

int index_buf = 0;
int choose(int n);
void gen_num();
void gen_left();
void gen_right();
void gen_symbol();
static void gen_rand_expr();
int choose(int n){
	int a=rand()% n;
	return a;
}

void gen_num(){ 
	int num = rand()% 9+1;
	char a[10]={};
	sprintf(a,"%d",num);
	buf[index_buf ++]=a[0];

}
void gen_left(){
	buf[index_buf ++]='(';
}
void gen_right(){
	buf[index_buf ++]=')';
}
void gen_symbol(){
	char symbol[4]={'+','-','*','/'};
	int a= rand () % 4;
	buf[index_buf ++]=symbol[a];
}

static void gen_rand_expr() {
  //buf[0] = '\0';

  if(index_buf > 65530){
	  printf("Overinput\n");
  }
  else{
	  switch(choose(3)){
		  case 0:
			  gen_num();
			  break;
		  case 1:
			  gen_num();
			  gen_symbol();
			  gen_left();
			  gen_rand_expr();
			  gen_right();
			  gen_symbol();
			  gen_num();
			  break;
		  default:
			  gen_num();
			  gen_symbol();
			  gen_num();
			  break;
	  }
  }
}

int main(int argc, char *argv[]) {
  int loop = 1;
  int seed = time(0);
  srand(seed);
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    for (int k = 0; k < sizeof(buf); k++) {
    	    buf[k] = '\0'; 
}
    for (int z = 0;z <sizeof(code_buf);z++){
	    code_buf[z]='\0';
    }
    index_buf=0;

    gen_rand_expr();

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  return 0;
}
