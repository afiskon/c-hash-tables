/*
  htable.c
  (c) Alexandr A Alexeev 2011 | http://eax.me/
*/

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "htable.h"

#define MIN_HTABLE_SIZE (1 << 3)

/******** PRIVATE ********/

/* http://en.wikipedia.org/wiki/Jenkins_hash_function */
uint32_t _htable_hash(const char *key, const size_t key_len) {
  uint32_t hash, i;
  for(hash = i = 0; i < key_len; ++i) {
    hash += key[i];
    hash += (hash << 10);
    hash ^= (hash >> 6);
  }
  hash += (hash << 3);
  hash ^= (hash >> 11);
  hash += (hash << 15);
  return hash;
}

/* если нужно, изменить размер таблицы
   false в случае ошибки */
bool _htable_resize(struct htable *tbl) {
  struct htable_item **new_items_table;
  struct htable_item *curr_item, *prev_item, *temp_item;
  size_t item_idx, new_size = tbl->size;
  uint32_t new_mask;

  if(tbl->items < MIN_HTABLE_SIZE) {
    tbl->last_error = HTABLE_ERROR_NONE;
    return true;
  }

  if(tbl->items > tbl->size) {
    new_size = tbl->size * 2;
    new_mask = new_size - 1;
    new_items_table = malloc(new_size * sizeof(struct htable_item*));
    if(new_items_table == NULL) {
      tbl->last_error = HTABLE_ERROR_MALLOC_FAILED;
      return false;
    }

    /* первую половину тупо копируем */
    memcpy(new_items_table, tbl->items_table, tbl->size * sizeof(struct htable_item*));
    /* вторую половину пока что заполняем нулями */
    memset(&new_items_table[tbl->size], 0, tbl->size * sizeof(struct htable_item*));

    for(item_idx = 0; item_idx < tbl->size; item_idx++) {
      prev_item = NULL;
      curr_item = new_items_table[item_idx];
      while(curr_item != NULL) {
        if((curr_item->hash & tbl->mask) != (curr_item->hash & new_mask)) {
          if(prev_item == NULL) {
            new_items_table[item_idx] = curr_item->next;
          } else {
            prev_item->next = curr_item->next;
          }
          temp_item = curr_item->next;
          curr_item->next = new_items_table[curr_item->hash & new_mask];
          new_items_table[curr_item->hash & new_mask] = curr_item;
          curr_item = temp_item;
        } else {
          prev_item = curr_item;
          curr_item = curr_item->next;
        }
      } /* while */
    } /* for */
  } else if(tbl->items <= (tbl->size / 2)) {
    new_size = tbl->size / 2;
    new_mask = new_size - 1;
    new_items_table = malloc(new_size * sizeof(struct htable_item*));
    if(new_items_table == NULL) {
      tbl->last_error = HTABLE_ERROR_MALLOC_FAILED;
      return false;
    }
    memcpy(new_items_table, tbl->items_table, new_size * sizeof(struct htable_item*));
    for(item_idx = new_size; item_idx < tbl->size; item_idx++) {
      if(tbl->items_table[item_idx] == NULL)
        continue;

      if(new_items_table[item_idx - new_size] == NULL) {
        new_items_table[item_idx - new_size] = tbl->items_table[item_idx];
      } else {
        curr_item = new_items_table[item_idx - new_size];
        while(curr_item->next != NULL) {
          curr_item = curr_item->next;
        }
        curr_item->next = tbl->items_table[item_idx];
      }
    }
  }

  if(new_size != tbl->size) {
    free(tbl->items_table);
    tbl->items_table = new_items_table;
    tbl->size = new_size;
    tbl->mask = new_mask;
  }

  tbl->last_error = HTABLE_ERROR_NONE;
  return true;
}

/******** PUBLIC ********/

/* создаем новую хэш-таблицу, NULL в случае ошибки */
struct htable* htable_new(void) {
  struct htable_item **items_table;
  struct htable *tbl = malloc(sizeof(struct htable));
  if(tbl == NULL) {
    return NULL;
  }

  tbl->items = 0;
  tbl->size = MIN_HTABLE_SIZE;
  tbl->mask = tbl->size - 1;
  tbl->last_error = HTABLE_ERROR_NONE;
  items_table = malloc(sizeof(struct htable_item*) * tbl->size);
  if(items_table == NULL) {
    free(tbl);
    return NULL;
  }
  tbl->items_table = items_table;
  memset(items_table, 0, sizeof(struct htable_item*) * tbl->size);
  return tbl;
}

/* уничтожаем хэш-таблицу */
void htable_free(struct htable *tbl) {
  size_t item_idx;
  struct htable_item *curr_item, *next_item;

  for(item_idx = 0; item_idx < tbl->size; item_idx++) {
    curr_item = tbl->items_table[item_idx];
    while(curr_item) {
      next_item = curr_item->next;
      free(curr_item->key);
      free(curr_item);
      curr_item = next_item;
    }
  }

  free(tbl->items_table);
  free(tbl);
}

/* поиск элемента, false если не найден */
bool htable_get(struct htable *tbl, const char *key, const size_t key_len, int *value) {
  uint32_t hash = _htable_hash(key, key_len);
  struct htable_item *curr_item = tbl->items_table[hash & tbl->mask];

  while(curr_item) {
    if(curr_item->key_len == key_len) {
      if(memcmp(curr_item->key, key, key_len) == 0) {
        tbl->last_error = HTABLE_ERROR_NONE;
        *value = curr_item->value;
        return true;
      }
    }
    curr_item = curr_item->next;
  }
  tbl->last_error = HTABLE_ERROR_NOT_FOUND;
  return false;
}

/* добавляем элемент или присваиваем значение
   false в случае ошибки */
bool htable_set(struct htable *tbl, const char *key, const size_t key_len, const int value) {
  uint32_t hash = _htable_hash(key, key_len);
  struct htable_item *item = tbl->items_table[hash & tbl->mask];

  /* если такой ключ уже используется, меняем значение */
  while(item) {
    if(item->key_len == key_len) {
      if(memcmp(item->key, key, key_len) == 0) {
        item->value = value;
        tbl->last_error = HTABLE_ERROR_NONE;
        return true;
      }
    }
    item = item->next;
  }
 
  /* ключ еще не используется, добавляем новый элемент */
  item = malloc(sizeof(struct htable_item));
  if(item == NULL) {
    tbl->last_error = HTABLE_ERROR_MALLOC_FAILED;
    return false;
  }

  item->key = malloc(key_len);
  if(item->key == NULL) {
    tbl->last_error = HTABLE_ERROR_MALLOC_FAILED;
    free(item);
    return false;
  }

  memcpy(item->key, key, key_len);
  item->hash = hash;
  item->key_len = key_len;
  item->value = value;
  item->next = tbl->items_table[hash & tbl->mask];
  tbl->items_table[hash & tbl->mask] = item;
  tbl->items++;

  return _htable_resize(tbl);
}

/* удаление элемента, false в случае ошибки */
bool htable_del(struct htable *tbl, const char *key, const size_t key_len) {
  uint32_t hash = _htable_hash(key, key_len);
  struct htable_item *prev = NULL;
  struct htable_item *item = tbl->items_table[hash & tbl->mask];

  while(item) {
    if(item->key_len == key_len) {
      if(memcmp(item->key, key, key_len) == 0) {
        if(prev) {
          prev->next = item->next;
        } else {
          tbl->items_table[hash & tbl->mask] = item->next;
        } 

        free(item->key);
        free(item);

        tbl->items--;
        return _htable_resize(tbl);
      }
    }
    prev = item;
    item = item->next;
  }

  tbl->last_error = HTABLE_ERROR_NOT_FOUND;
  return false;
}

