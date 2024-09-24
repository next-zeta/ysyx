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

#include "sdb.h"
#include "watchpoint.h"

#define NR_WP 32

//typedef struct watchpoint {
//  int NO;
//  struct watchpoint *next;
// bool success;
//  char expr[100000];
//  int value;
  /* TODO: Add more members if necessary */

//} WP;


WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

WP* new_wp(){
	for(WP* np = free_;np->next!=NULL;np=np->next){
		if(np->success==false){
			np->success=true;
			if(head==NULL){
				head=np;
			}
			return np;
			break;
		}
	}
		printf("No node available.\n");
		assert(0);
		return NULL;
}

void free_wp(WP *np){
	int t=0;
	if(head->NO == np->NO){
		head -> success = false;
		head = NULL;
		return ;
	}
	for(WP *tmp = head;tmp->next!=NULL;tmp=tmp->next){
		if(tmp->next->NO == np->NO){
			tmp->next->success = false;
			printf("Success delete No%d\n",np->NO);
			t++;
			return ;
		}
	}
	if(t==0){
		printf("Don't find the watchpoint.\n");
	}
}



void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
    wp_pool[i].success=false;
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

