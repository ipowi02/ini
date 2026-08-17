/* da.h - 1.1 - Public Domain 
 *
 * This single header file library provides a simple implementation of Dynamic Arrays in C.
 * Although mostly inspired by [Tsoding](https://github.com/tsoding)'s implementation of dynamic arrays,
 * it has some semantic differences in the names of the provided macros, and is not designed to be intercompatible
 * with Tsoding's implementation.
 */
#ifndef DA_H
#define DA_H
#define DA_H_VERSION "1.1"

#define DA_ALLOC malloc
#define DA_FREE free
#define DA_REALLOC realloc
#define DA_ASSERT assert
#define DA_DEFAULT_CAP 32

#ifndef DA_NOSTDLIB
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#endif

/* INITIALIZERS */
/* Generic dynamic array type, use it to create arrays of custom types. */
#define da_member(T) T *data; size_t length; size_t capacity 
#define da_t(T) struct {da_member(T);}


#define da_init(xs) memset((xs), 0, sizeof(*(xs)))
#define da_free(xs) DA_FREE((xs).data)
#define da_deinit(xs) do { da_free(xs); da_init(xs); } while(0)

/* ACCESSORS */
/* Gets an element of array xs, after asserting its length is greater than 0, and that the index 'idx' is smaller
 *  than the length. */
#define da_at(xs, idx) (xs)->data[(idx)]

#define da_checked_at(xs, idx)						\
  (xs)->data[(DA_ASSERT((xs)->length > 0 && "Tried to access items of 0 length array"), \
	      DA_ASSERT((xs)->length > (idx) && "The index is greater than the length of the array"),\
	      (idx))]
/* Pops off the last element of array xs, and returns it. */
#define da_pop(xs) da_checked_at(xs, --(xs)->length)

/* Gets the first element of array xs. */
#define da_first(xs) da_checked_at(xs, 0)

/* Gets the last element of array xs. */
#define da_last(xs) da_checked_at(xs, (xs)->length-1)

/* Writes the first index of at which 'val' is found in xs into 'out_idx', which ought to be declared before usage,
 * or -1 if val hasn't been found.
 */
#define da_find(xs, val, out_idx)					\
     do {								\
	  for ((out_idx) = 0; (out_idx) < (xs)->length; (out_idx)++) {	\
	       if ((xs)->data[(out_idx)] == (val)) break;		\
	  }								\
	  if ((out_idx) == (xs)->length) (out_idx) = -1;		\
     } while (0)


/* MODIFIERS */
/* Reserves capacity for array xs, via reallocating its memory. */
#define da_reserve(xs, cap)					\
     do {								\
     if ((cap) > (xs)->capacity) {					\
	  if ((xs)->capacity == 0)	(xs)->capacity = DA_DEFAULT_CAP; \
	  while ((cap) > (xs)->capacity) (xs)->capacity *= 1.5;	\
	  (xs)->data = DA_REALLOC((xs)->data, (xs)->capacity * sizeof(*(xs)->data)); \
	  DA_ASSERT((xs)->data != NULL && "Failed to reallocate memory"); \
     }									\
} while(0)
/* Reserves capacity and adjusts the length of array xs. */
#define da_resize(xs, new_size) \
    do {                                \
        da_reserve((xs), new_size); \
        (xs)->length = (new_size);       \
    } while (0)

/* Pushes an element into array xs. */
#define da_push(xs, ...)		       \
    do {                                       \
	 da_reserve((xs), (xs)->length + 1);    \
	 (xs)->data[(xs)->length++] = (__VA_ARGS__);	\
    } while (0)

/* Pushes N elements from the memory region pointed to by 'items' to array xs,
   where each element matches the array's element size.
 */
#define da_npush(xs, items, N)					\
     do {								\
	  da_reserve((xs), (xs)->length + (N));				\
	  memcpy((xs)->data + (xs)->length, (items), (N)*sizeof(*(xs)->data)); \
	  (xs)->length += (N);						\
     } while (0)

/* Trivial. */
#define da_pusharr(xs, xs2) da_npush(xs, xs2, xs2->length)


/* Syntactic sugar for a for loop, with an iterator (pointer) pointing to the arrays first element,
   iterating until its last one.
   'it' is a pointer here, you have to dereference it to access its value.
*/
#define da_foreach(T, it, xs) for (T *it = (xs)->data; it < (xs)->data + (xs)->length; ++it)


/* Sorts an array's elements with qsort */
#define da_sort(xs, fn) qsort((xs)->data, (xs)->length, sizeof(*(xs)->data), fn)

/* Inserts an element into the dynamic array xs, at index idx. */
#define da_insert(xs, idx, item)				        \
     do {							        \
          da_reserve(xs, xs->length+1);				        \
          memmove((xs)->data + (idx + 1) * sizeof(*(xs)->data),	        \
	          (xs)->data + idx * sizeof(*(xs)->data),		\
	          ((xs)->length - idx) * sizeof(*(xs)->data));	        \
          da_checked_at(xs, idx) = (item);				\
          (xs)->length++;						\
     while (0)

/* Typedefs for trivial types */
typedef da_t(void*) da_ptr_t;
typedef da_t(char*) da_str_t;
typedef da_t(int) da_int_t;
typedef da_t(char) da_char_t;
typedef da_char_t da_string;
typedef da_t(float) da_float_t;
typedef da_t(double) da_double_t;
#endif /* DA_H */
/* Update log:
 * 1.0.1 (2026-03-19) Now da_checked_at asserts the index is less than the length of the array too.
 * 1.1   (2026-08-09) Added da_member
                      Fixed da_at
 */
