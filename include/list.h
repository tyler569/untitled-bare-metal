#pragma once

#include <sys/cdefs.h>

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
list_init (struct list_head *head)
{
  head->next = head;
  head->prev = head;
}

static inline void
list_insert (struct list_head *new, struct list_head *prev,
             struct list_head *next)
{
  new->next = next;
  new->prev = prev;
  prev->next = new;
  next->prev = new;
}

static inline void
list_insert_after (struct list_head *new, struct list_head *prev)
{
  list_insert (new, prev, prev->next);
}

static inline void
list_insert_before (struct list_head *new, struct list_head *next)
{
  list_insert (new, next->prev, next);
}

static inline void
list_remove (struct list_head *entry)
{
  entry->prev->next = entry->next;
  entry->next->prev = entry->prev;
}

static inline struct list_head *
list_pop_front (struct list_head *head)
{
  struct list_head *next = head->next;
  list_remove (next);
  return next;
}

static inline bool
list_empty (struct list_head *head)
{
  return head->next == head;
}

#define list_first(head) (head)->next