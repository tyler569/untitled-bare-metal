#include "spin_lock.h"
#include "kern/arch.h"
#include "stdatomic.h"

void
spin_lock (spin_lock_t *lock)
{
  const int back
      = atomic_fetch_add_explicit (&lock->back, 1, memory_order_relaxed);

  while (atomic_load_explicit (&lock->front, memory_order_acquire) != back)
    relax_busy_loop ();
}

void
spin_unlock (spin_lock_t *lock)
{
  atomic_fetch_add_explicit (&lock->front, 1, memory_order_release);
}

void
read_lock (rw_lock_t *l)
{
  spin_lock (&l->gate);
  atomic_fetch_add_explicit (&l->readers, 1, memory_order_relaxed);
  spin_unlock (&l->gate);
}

void
read_unlock (rw_lock_t *l)
{
  atomic_fetch_sub_explicit (&l->readers, 1, memory_order_release);
}

void
write_lock (rw_lock_t *l)
{
  spin_lock (&l->gate);
  while (atomic_load_explicit (&l->readers, memory_order_acquire) != 0)
    relax_busy_loop ();
}

void
write_unlock (rw_lock_t *l)
{
  spin_unlock (&l->gate);
}
