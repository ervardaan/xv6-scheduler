#ifndef __MUTEX_H__
#define __MUTEX_H__
#include "spinlock.h"

// Some values are copied from sleeplock.h
typedef struct {
  uint locked;       // Is the lock held?
  struct spinlock lk; // spinlock protecting this sleep lock
} mutex;

#endif
