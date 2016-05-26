/******************************************************************************
**                                                                          **
** Copyright (c) 2007 Browan Communications Inc.                            **
** All rights reserved.                                                     **
**                                                                          **
******************************************************************************

Version History:
----------------

Version     : 1.0
Date        : Dec 5, 2007
Revised By  : jone.li
Description : Original Version: A set of macros to manage forward-linked lists. 
version     : 2.0
Date        : MAY 12, 2010
Revised By  : jone.li
Description : patch to kernel and used only in linux kernel

******************************************************************************
***/

#ifndef _LISTS_H_
#define _LISTS_H_
#include <linux/mutex.h> /*larger than 2.6.16*/

/*
  Locks a list.
  
  This macro attempts to place an exclusive lock in the
  list head structure pointed to by head.
  Returns 0 on success, non-zero on failure
*/
#define LIST_LOCK(head)                     \
    mutex_lock(&(head)->lock) 

/*
  Attempts to unlock a list.

  This macro attempts to remove an exclusive lock from the
  list head structure pointed to by head. If the list
  was not locked by this thread, this macro has no effect.
*/
#define LIST_UNLOCK(head)                       \
    mutex_unlock(&(head)->lock)

/*
  Defines a structure to be used to hold a list of specified type.
  
  This macro creates a structure definition that can be used
  to hold a list of the entries of type type. It does not actually
  declare (allocate) a structure; to do that, either follow this
  macro with the desired name of the instance you wish to declare,
  or use the specified name to declare instances elsewhere.

  Example usage:
  static LIST_HEAD(entry_list, entry) entries;

  This would define struct entry_list, and declare an instance of it named
  entries, all intended to hold a list of type struct entry.
*/
#define LIST_HEAD(name, type)                   \
struct name {                               \
    struct type *first;                     \
    struct type *last;                      \
    struct mutex lock;                       \
}

#define LIST_DECARE(name) \
struct name	

/*
  Defines a structure to be used to hold a list of specified type (with no lock).

  This macro creates a structure definition that can be used
  to hold a list of the entries of type type. It does not actually
  declare (allocate) a structure; to do that, either follow this
  macro with the desired name of the instance you wish to declare,
  or use the specified name to declare instances elsewhere.

  Example usage:
  static LIST_HEAD_NOLOCK(entry_list, entry) entries;

  This would define struct entry_list, and declare an instance of it named
  entries, all intended to hold a list of type struct entry.
*/
#define LIST_HEAD_NOLOCK(name, type)                \
struct name {                               \
    struct type *first;                     \
    struct type *last;                      \
}


/*
  Defines initial values for a declaration of LIST_HEAD_NOLOCK
*/
#if defined(__cplusplus) || defined(c_plusplus)
#define LIST_HEAD_NOLOCK_INIT_VALUE {   \
    NULL,                  \
    NULL,                   \
    }
#else
#define LIST_HEAD_NOLOCK_INIT_VALUE {   \
    .first = NULL,                  \
    .last = NULL,                   \
    }
#endif 

/*
  Defines a structure to be used to hold a list of specified type, statically initialized.

  This is the same as LIST_HEAD_STATIC, except without the lock included.
*/
#define LIST_HEAD_NOLOCK_STATIC(name, type)             \
struct name {                               \
    struct type *first;                     \
    struct type *last;                      \
} name = LIST_HEAD_NOLOCK_INIT_VALUE

static inline int mutexInit (struct mutex *pmutex)
{
    mutex_init (pmutex);
	return 0;
}

static inline int mutexDestroy (struct mutex *pmutex)
{
	return 0;
}

static inline int mutexTryLock(struct mutex *pmutex)
{
    return mutex_trylock(pmutex);
}
static inline int mutexLock(struct mutex *pmutex)
{
	mutex_lock(pmutex);
	return 0;
}
static inline int mutexUnlock(struct mutex *pmutex)
{
    mutex_unlock(pmutex);
	return 0;
}

/*
  Initializes a list head structure with a specified first entry.

  This macro initializes a list head structure by setting the head
  entry to the supplied value and recreating the embedded lock.
*/
#define LIST_HEAD_SET(head, entry) do {             \
    (head)->first = (entry);                    \
    (head)->last = (entry);                     \
    mutexInit(&(head)->lock);                   \
} while (0)

/*
  Initializes a list head structure with a specified first entry.

  This macro initializes a list head structure by setting the head
  entry to the supplied value.
*/
#define LIST_HEAD_SET_NOLOCK(head, entry) do {          \
    (head)->first = (entry);                    \
    (head)->last = (entry);                     \
} while (0)

/*
  Declare a forward link structure inside a list entry.

  This macro declares a structure to be used to link list entries together.
  It must be used inside the definition of the structure named in
  type, as follows:

  struct list_entry {
    ...
    LIST_ENTRY(list_entry) list;
  }

  The field name list here is arbitrary, and can be anything you wish.
*/
#define LIST_ENTRY(type)                        \
struct {                                \
    struct type *next;                      \
}

/*
  Returns the first entry contained in a list.
 */
#define LIST_FIRST(head)    ((head)->first)

/*
  Returns the last entry contained in a list.
 */
#define LIST_LAST(head) ((head)->last)

/*
  Returns the next entry in the list after the given entry.
*/
#define LIST_NEXT(elm, field)   ((elm)->field.next)

/*
  Checks whether the specified list contains any entries.

  Returns non-zero if the list has entries, zero if not.
 */
#define LIST_EMPTY(head)    (LIST_FIRST(head) == NULL)

/*
  Loops over (traverses) the entries in a list.

  This macro is use to loop over (traverse) the entries in a list. It uses a
  for loop, and supplies the enclosed code with a pointer to each list
  entry as it loops. It is typically used as follows:
  
  static LIST_HEAD(entry_list, list_entry) entries;
  ...
  struct list_entry {
    ...
    LIST_ENTRY(list_entry) list;
  }
  ...
  struct list_entry *current;
  ...
  LIST_TRAVERSE(&entries, current, list) {
     (do something with current here)
  }
  
  If you modify the forward-link pointer contained in the current entry while
  inside the loop, the behavior will be unpredictable. At a minimum, the following
  macros will modify the forward-link pointer, and should not be used inside
  LIST_TRAVERSE() against the entry pointed to by the current pointer without
  careful consideration of their consequences:
  LIST_NEXT() (when used as an lvalue)
  LIST_INSERT_AFTER()
  LIST_INSERT_HEAD()
  LIST_INSERT_TAIL()
*/
#define LIST_TRAVERSE(head,var,field)               \
    for((var) = (head)->first; (var); (var) = (var)->field.next)

/*
  Loops safely over (traverses) the entries in a list.

  This macro is used to safely loop over (traverse) the entries in a list. It
  uses a for loop, and supplies the enclosed code with a pointer to each list
  entry as it loops. It is typically used as follows:
  
  static LIST_HEAD(entry_list, list_entry) entries;
  ...
  struct list_entry {
    ...
    LIST_ENTRY(list_entry) list;
  }
  ...
  struct list_entry *current;
  ...
  LIST_TRAVERSE_SAFE_BEGIN(&entries, current, list) {
     (do something with current here)
  }
  LIST_TRAVERSE_SAFE_END;

  It differs from LIST_TRAVERSE() in that the code inside the loop can modify
  (or even free, after calling LIST_REMOVE_CURRENT()) the entry pointed to by
  the current pointer without affecting the loop traversal.
*/
#define LIST_TRAVERSE_SAFE_BEGIN(head, var, field) {                \
    typeof((head)->first) __list_next;                      \
    typeof((head)->first) __list_prev = NULL;                   \
    typeof((head)->first) __new_prev = NULL;                    \
    for ((var) = (head)->first, __new_prev = (var),                 \
          __list_next = (var) ? (var)->field.next : NULL;               \
         (var);                                 \
         __list_prev = __new_prev, (var) = __list_next,             \
         __new_prev = (var),                            \
         __list_next = (var) ? (var)->field.next : NULL             \
        )

/*
  Removes the current entry from a list during a traversal.

  This macro can only be used inside an LIST_TRAVERSE_SAFE_BEGIN()
  block; it is used to unlink the current entry from the list without affecting
  the list traversal (and without having to re-traverse the list to modify the
  previous entry, if any).
 */
#define LIST_REMOVE_CURRENT(head, field) do { \
    __new_prev->field.next = NULL;                          \
    __new_prev = __list_prev;                           \
    if (__list_prev)                                \
        __list_prev->field.next = __list_next;                  \
    else                                        \
        (head)->first = __list_next;                        \
    if (!__list_next)                               \
        (head)->last = __list_prev; \
    } while (0)

/*
  Inserts a list entry before the current entry during a traversal.

  This macro can only be used inside an LIST_TRAVERSE_SAFE_BEGIN()
  block.
 */
#define LIST_INSERT_BEFORE_CURRENT(head, elm, field) do {       \
    if (__list_prev) {                      \
        (elm)->field.next = __list_prev->field.next;        \
        __list_prev->field.next = elm;              \
    } else {                            \
        (elm)->field.next = (head)->first;          \
        (head)->first = (elm);                  \
    }                               \
    __new_prev = (elm);                     \
} while (0)


/*
  Closes a safe loop traversal block.
 */
#define LIST_TRAVERSE_SAFE_END  }

/*
  Initializes a list head structure.

  This macro initializes a list head structure by setting the head
  entry to NULL (empty list) and recreating the embedded lock.
*/
#define LIST_HEAD_INIT(head) {                  \
    (head)->first = NULL;                       \
    (head)->last = NULL;                        \
    mutexInit(&(head)->lock);                   \
}

/*
  Destroys a list head structure.

  This macro destroys a list head structure by setting the head
  entry to NULL (empty list) and destroying the embedded lock.
  It does not free the structure from memory.
*/
#define LIST_HEAD_DESTROY(head) {                   \
    (head)->first = NULL;                       \
    (head)->last = NULL;                        \
    mutexDestroy(&(head)->lock);                \
}


/*
  Initializes a list head structure.

  This macro initializes a list head structure by setting the head
  entry to NULL (empty list). There is no embedded lock handling
  with this macro.
*/
#define LIST_HEAD_INIT_NOLOCK(head) {               \
    (head)->first = NULL;                       \
    (head)->last = NULL;                        \
}

/*
  Inserts a list entry after a given entry.
 */
#define LIST_INSERT_AFTER(head, listelm, elm, field) do {       \
    (elm)->field.next = (listelm)->field.next;          \
    (listelm)->field.next = (elm);                  \
    if ((head)->last == (listelm))                  \
        (head)->last = (elm);                   \
} while (0)

/*
  Inserts a list entry at the head of a list.
 */
#define LIST_INSERT_HEAD(head, elm, field) do {         \
        (elm)->field.next = (head)->first;          \
        (head)->first = (elm);                  \
        if (!(head)->last)                  \
            (head)->last = (elm);               \
} while (0)


/*
  Appends a list entry to the tail of a list.

  Note: The link field in the appended entry is not modified, so if it is
  actually the head of a list itself, the entire list will be appended
  temporarily (until the next LIST_INSERT_TAIL is performed).
 */
#define LIST_INSERT_TAIL(head, elm, field) do {         \
      if (!(head)->first) {                     \
        (head)->first = (elm);                  \
        (head)->last = (elm);                   \
      } else {                              \
        (head)->last->field.next = (elm);           \
        (head)->last = (elm);                   \
      }                                 \
} while (0)

/*
  Appends a whole list to the tail of a list.
 */
#define LIST_APPEND_LIST(head, list, field) do {            \
      if (!(head)->first) {                     \
        (head)->first = (list)->first;              \
        (head)->last = (list)->last;                \
      } else {                              \
        (head)->last->field.next = (list)->first;       \
        (head)->last = (list)->last;                \
      }                                 \
} while (0)

/*
  Removes and returns the head entry from a list.

  Removes the head entry from the list, and returns a pointer to it.
  This macro is safe to call on an empty list.
 */
#define LIST_REMOVE_HEAD(head, field) ({                \
        typeof((head)->first) cur = (head)->first;      \
        if (cur) {                      \
            (head)->first = cur->field.next;        \
            cur->field.next = NULL;             \
            if ((head)->last == cur)            \
                (head)->last = NULL;            \
        }                           \
        cur;                            \
    })

/*
  Removes a specific entry from a list.
 */
#define LIST_REMOVE(head, elm, field) ({                    \
    __typeof(elm) __res = NULL; \
    if ((head)->first == (elm)) {                   \
        __res = (head)->first;                      \
        (head)->first = (elm)->field.next;          \
        if ((head)->last == (elm))          \
            (head)->last = NULL;            \
    } else {                                \
        typeof(elm) curelm = (head)->first;         \
        while (curelm && (curelm->field.next != (elm)))         \
            curelm = curelm->field.next;            \
        if (curelm) { \
            __res = (elm); \
            curelm->field.next = (elm)->field.next;         \
            if ((head)->last == (elm))              \
                (head)->last = curelm;              \
        } \
    }                               \
    (elm)->field.next = NULL;                                       \
    (__res); \
})


#endif /* _LISTS_H_ */

/****************************************************************************/
/**                                                                        **/
/**                               EOF                                      **/
/**                                                                        **/
/****************************************************************************/

