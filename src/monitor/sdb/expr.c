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
#include <stdlib.h>
#include <string.h>
/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include "paddr.h"

uint32_t eval(int p,int q);
bool check_parentheses(int p,int q);
word_t expr(char *e, bool *success);
enum {
  TK_NOTYPE = 256, 
  TK_EQ=0,
  NUMBER=1,
  HEX=2,
  REG=3,
  NOT_EQ=4,
  AND=5,

  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
  {"\\=\\=", TK_EQ},        // equal
  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"\\-", '-'},		// minus
  {"\\*", '*'},		// multiply
  {"\\/", '/'},
  {"\\=", '='},
  {"\\(", '('},
  {"\\)", ')'},
  {"\\!\\=", NOT_EQ},
  {"\\&\\&",AND},
  {"\\$[a-zA-Z]*[0-9]*", REG},
  {"0[xX][0-9a-fA-F]+", HEX},
  {"[0-9]+",NUMBER},
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
  char tmp[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
		case '+':
			tokens[nr_token].type='+';
			nr_token++;
			break;
		case '-':
			tokens[nr_token].type='-';
			nr_token++;
			break;
		case '*':
			tokens[nr_token].type='*';
			nr_token++;
			break;
		case '/':
			tokens[nr_token].type='/';
			nr_token++;
			break;
		case '(':
			tokens[nr_token].type='(';
			nr_token++;
			break;
		case ')':
			tokens[nr_token].type=')';
			nr_token++;
			break;
		case 256:
			break;
		case 1:
			strncpy(tokens[nr_token].str,&e[position-substr_len],substr_len);
			tokens[nr_token].type=1;
			nr_token++;
			break;
		case 2: 
			tokens[nr_token].type = 2;
			strncpy(tokens[nr_token].str, &e[position - substr_len], substr_len);
			nr_token ++;
			break;
		case 3:
                         strncpy(tokens[nr_token].str,&e[position-substr_len],substr_len);
                         tokens[nr_token].type=3;
                         nr_token++;
                         break;
		case 0:
			tokens[nr_token].type = 0;
			strcpy(tokens[nr_token].str, "==");
			nr_token++;
			break;
		case 4:
			tokens[nr_token].type = 4;
			strcpy(tokens[nr_token].str, "!=");
			nr_token++;
			break;
		case 5:
                         tokens[nr_token].type = 5;
                         strcpy(tokens[nr_token].str, "&&");
			 nr_token++;
                         break;  
        	default: 
			printf("There is no rule to matching");
			break;

        }
        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

uint32_t eval(int p, int q) {
  if (p > q) {
	  printf("Incorrect input value\n");
	  return 0;
    /* Bad expression */
  }
  else if (p == q) {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
	  int result;
	  result = atoi(tokens[p].str);
	  return result;

  }
  else if (check_parentheses(p, q) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1);
  }
  else {
    /* We should do more things here. */
	int op=0;
	int i=p;
	int level=5,tmp=100;
	int z;
	while(i<=q){
		if(tokens[i].type=='('){
			int k=1;
			while(k!=0){
				i++;
				if(tokens[i].type=='('){
					k++;
				}
				if(tokens[i].type==')'){
					k--;
				}
			}
		}

		if(tokens[i].type=='+'||tokens[i].type=='-'){
			level=1;
			z=i;
		}
		else if(tokens[i].type=='*'||tokens[i].type=='/'){
			level=2;
			z=i;
		}
		else if(tokens[i].type==0||tokens[i].type==4||tokens[i].type==5){
			level=0;
			z=i;
		}
		else if(tokens[i].type==6){
			level=3;
			z=i;
		}
		else if(tokens[i].type==7){
			level=4;
			z=i;
		}
		else if(tokens[i].type==8){
			level=0;
			z=i;
		}
		else{
			level=5;
			z=i;
		}

		if(tmp>=level){
			tmp=level;
			op=z;
		}
		i++;

	}


	int op_type=tokens[op].type;


    	uint32_t val1;
    	uint32_t val2;
	if(op_type==6||op_type==7||op_type==8){
		val1=0;
		val2=eval(op+1,q);
	}
	else{
		val1=eval(p,op-1);
		val2=eval(op+1,q);
	}
	//printf("switch timnei, op.type = %d\n", op_type);
    	switch (op_type) {
      		case '+': 
		//	printf("return val1 + val2\n");
			return val1 + val2;
      		case '-': 
		//	printf("return val1 - val2\n");
			return val1 - val2; 
      		case '*': 
		//	printf("return val1 * val2\n");
			return val1 * val2; 
      		case '/':
		//	printf("return val1 / val2\n");
			if(val2==0){
				printf("The val2 is zero!!\n");
				assert(0);
			}
			return val1 / val2; 
		case 0:
			return val1 == val2;
		case 4:
			return val1 != val2;
		case 5:
			return val1 && val2;
		case 6:
			return val1-val2;
		case 7:
			return paddr_read(val2,1);
		case 8:
			return val1-val2;
      		default: printf("test eval default\n");
			  assert(0);
  }
}
}
bool check_parentheses(int p, int q){
	if(tokens[p].type!='('||tokens[q].type!=')'){
		return false;
	}
	else{
		int l=p+1;
		int first=2;
		while(l<q){
			if(tokens[l].type==')'){
				first=1;
				break;
			}
			if(tokens[l].type=='('){
				break;
			}
			l++;
		}
		if(first==1){
			return false;
		}
		else{
			l=p;
			int d=0;
			while(l<=q){
				if(tokens[l].type=='('){
					d++;
				}
				if(tokens[l].type==')'){
					d--;
				}
				l++;
			}
			if(d==0){
				return true;
			}
			else{
				return false;
			}
		}
	}
};

void hex(int n){
	int a;
	char tmp[32];
	strcpy(tokens[n].tmp,tokens[n].str);
	sscanf(tokens[n].str,"%x",&a);
	sprintf(tmp,"%d",a);
	strcpy(tokens[n].str,tmp);
}


word_t expr(char *e, bool *success) {
  
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  for(int i=0;i<nr_token;i++){
	  if(tokens[i].type==2){
		  hex(i);
	  }
	  if(tokens[i].type == 3){
	    bool success = true;
	    strcpy(tokens[i].tmp,tokens[i].str);
	    word_t tmp = isa_reg_str2val(tokens[i].str, &success);
	    if(success){
		    char s[32];
		    sprintf(s,"%d",tmp);
		    strcpy(tokens[i].str,s);
	    }

	    else{
		printf("Don't find. \n");
		assert(0);
	    }
	  }
	  if (tokens[i].type == '-' && (i == 0 || tokens[i - 1].type =='(')){
			  tokens[i].type=8;
			  }
 	  if (tokens[i].type == '-' && (i != 0 && (tokens[i - 1].type != 1 && tokens[i-1].type != ')')) ) {
    		tokens[i].type = 6;
  	  }
	  if (tokens[i].type == '*' && (i == 0 ||( tokens[i - 1].type != 1 && tokens[i-1].type != ')') )) {
    		tokens[i].type = 7;
	  }
  }
  int result;
  result=eval(0,nr_token-1);
  printf("The result:%d\n",result);
  for(int i=0;i<nr_token;i++){
	  if(tokens[i].type == 3 || tokens[i].type==2){
		  strcpy(tokens[i].str,tokens[i].tmp);
	  }
  }
  return result;
}
