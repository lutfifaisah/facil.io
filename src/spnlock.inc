#ifndef H_SPNLOCK_H
#define H_SPNLOCK_H
/* *****************************************************************************
spinlock / sync for tasks
***************************************************************************** */
#if defined(__unix__) || defined(__APPLE__) || defined(__linux__)
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <time.h>
#endif /* __unix__ */
#include <stdlib.h>

/* manage the way threads "wait" for the lock to release */
#if defined(__unix__) || defined(__APPLE__) || defined(__linux__)
/* nanosleep seems to be the most effective and efficient reschedule */
#define reschedule_thread()                                                    \
  {                                                                            \
    static const struct timespec tm = {.tv_nsec = 1};                          \
    nanosleep(&tm, NULL);                                                      \
  }
#define throttle_thread(micosec)                                               \
  {                                                                            \
    const struct timespec tm = {.tv_nsec = (micosec),                          \
                                .tv_sec = /* (micosec >> 20)*/ 1};             \
    nanosleep(&tm, NULL);                                                      \
  }
#else /* no effective rescheduling, just spin... */
#define reschedule_thread()
#define throttle_thread(micosec)
#endif

/** locks use a single byte */
typedef volatile unsigned char spn_lock_i;
/** The initail value of an unlocked spinlock. */
#define SPN_LOCK_INIT 0

/* Select the correct compiler builtin method. */
#if defined(__has_builtin)
#if __has_builtin(__sync_swap)
#define SPN_LOCK_BUILTIN(...) __sync_swap(__VA_ARGS__)
#elif __has_builtin(__sync_fetch_and_or)
#define SPN_LOCK_BUILTIN(...) __sync_fetch_and_or(__VA_ARGS__)
#else
#error Required builtin "__sync_swap" or "__sync_fetch_and_or" missing from compiler.
#endif /* defined(__has_builtin) */
#elif __GNUC__ > 3
#define SPN_LOCK_BUILTIN(...) __sync_fetch_and_or(__VA_ARGS__)
#else
#error Required builtin "__sync_swap" or "__sync_fetch_and_or" not found.
#endif

/** returns 1 and 0 if the lock was successfully aquired (TRUE == FAIL). */
static inline int spn_trylock(spn_lock_i *lock) {
  return SPN_LOCK_BUILTIN(lock, 1);
}

/** Releases a lock. */
static inline __attribute__((unused)) void spn_unlock(spn_lock_i *lock) {
  __asm__ volatile("" ::: "memory");
  *lock = 0;
}
/** returns a lock's state (non 0 == Busy). */
static inline __attribute__((unused)) int spn_is_locked(spn_lock_i *lock) {
  __asm__ volatile("" ::: "memory");
  return *lock;
}
/** Busy waits for the lock. */
static inline __attribute__((unused)) void spn_lock(spn_lock_i *lock) {
  while (spn_trylock(lock)) {
    reschedule_thread();
  }
}

#endif /* H_SPNLOCK_H */