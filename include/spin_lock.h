#pragma once

#include "stdatomic.h"

struct spin_lock
{
  atomic_int front, back;
};

typedef struct spin_lock spin_lock_t;

void spin_lock (spin_lock_t *lock);
void spin_unlock (spin_lock_t *lock);

struct rw_lock {
  spin_lock_t gate;
  atomic_int readers;
};

typedef struct rw_lock rw_lock_t;

void read_lock(rw_lock_t *l);
void read_unlock(rw_lock_t *l);
void write_lock(rw_lock_t *l);
void write_unlock(rw_lock_t *l);
