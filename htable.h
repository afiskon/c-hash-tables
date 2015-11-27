/*
  htable.h
  (c) Alexandr A Alexeev 2011 | http://eax.me/
*/

#include <stdint.h>
#include <stdbool.h>

enum htable_error {
  HTABLE_ERROR_NONE,
  HTABLE_ERROR_NOT_FOUND,
  HTABLE_ERROR_MALLOC_FAILED,
};

/* элемент хэш-таблицы */
struct htable_item {
  uint32_t hash; /* значение хэш-функции */
  char *key; /* ключ */
  size_t key_len; /* длина ключа */
  int value; /* значение */
  struct htable_item *next; /* следующий элемент в цепочке, если есть */
};

/* хэш-таблица */
struct htable {
  uint32_t mask; /* маска, применяемая к хэш-функции */
  size_t size; /* текущий размер хэш-таблицы */
  size_t items; /* сколько элементов хранится в хэш-таблице */
  enum htable_error last_error; /* ошибка в последней операции */
  struct htable_item **items_table; /* элементы */
};

struct htable* htable_new(void);
void htable_free(struct htable *tbl);
bool htable_get(struct htable *tbl, const char *key, const size_t key_len, int *value);
bool htable_set(struct htable *tbl, const char *key, const size_t key_len, const int value);
bool htable_del(struct htable *tbl, const char *key, const size_t key_len);

