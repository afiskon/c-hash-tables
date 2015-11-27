/*
  htable_test.c
  (c) Alexandr A Alexeev 2011 | http://eax.me/
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include "htable.h"

int main(void) {
  int value;
  bool done, result;
  char *cmd, *key, *end, *str_value;
  char input[256];
  struct htable* tbl = htable_new();
  if(tbl == NULL) {
    printf("ERROR: tbl == NULL\n");
    return 1;
  }

  while(fgets(input, sizeof(input), stdin)) {
    end = strstr(input, "\n");
    if(!end) break;
    *end = '\0';

    if(strcmp(input, "quit") == 0) break;
    if(strcmp(input, "exit") == 0) break;

    done = false;
    value = 0;
    cmd = input;
    key = strstr(input, " ");
    if(key) {
      *key++ = '\0';
      str_value = strstr(key, " ");
      if(str_value) {
        *str_value++ = '\0';
        value = atoi(str_value);

        if(strcmp(cmd, "set") == 0) {
          done = true;
          result = htable_set(tbl, key, strlen(key), value);
        }
      } else {
        if(strcmp(cmd, "get") == 0) {
          done = true; 
          result = htable_get(tbl, key, strlen(key), &value);
        } else if(strcmp(cmd, "del") == 0) {
          done = true;
          result = htable_del(tbl, key, strlen(key));
        }
      }
    }
    
    if(done == true) {
      printf("+OK, cmd = %s, key = %s, value = %d, result = %d, last_error = %d, size = %d, items = %d\n",
        cmd, key, value, result, tbl->last_error, tbl->size, tbl->items);
    } else {
      printf("-INVALID COMMAND, 'set key value', 'get key', 'del key' or 'quit' expected\n");
    }
  }
  printf("+GOODBYE\n");
  htable_free(tbl);
  return 0;
}
