#pragma once

#include "sys/cdefs.h"

struct list_head
{
  struct list_head *next;
  struct list_head *prev;
};

#define LIST_HEAD_INIT(name)                                                  \
  {                                                                           \
    &(name), &(name)                                                          \
  }

#define LIST_HEAD(name) struct list_head name = LIST_HEAD_INIT (name)

static inline void
init_list (struct list_head *head)
{
  head->next = head;
  head->prev = head;
}

static inline void
insert_list_internal (struct list_head *new, struct list_head *prev,
                      struct list_head *next)
{
  new->next = next;
  new->prev = prev;
  prev->next = new;
  next->prev = new;
}

static inline void
append_to_list (struct list_head *new, struct list_head *prev)
{
  insert_list_internal (new, prev, prev->next);
}

static inline void
prepend_to_list (struct list_head *new, struct list_head *next)
{
  insert_list_internal (new, next->prev, next);
}

static inline void
remove_from_list (struct list_head *entry)
{
  entry->prev->next = entry->next;
  entry->next->prev = entry->prev;
}

static inline struct list_head *
pop_from_list (struct list_head *head)
{
  struct list_head *next = head->next;
  remove_from_list (next);
  return next;
}

static inline bool
is_list_empty (struct list_head *head)
{
  return head->next == head;
}

#define first_item(head) (head)->next