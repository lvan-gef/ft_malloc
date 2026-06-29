# ft_malloc Implementation Plan

## 1. Validate the Free-List Allocator

- Test `fl_init()` with a small mmap-backed arena.
- Test one allocation and verify the returned pointer is not `NULL`.
- Test multiple allocations from the same free list.
- Test freeing a block and allocating again to confirm reuse.
- Test freeing adjacent blocks and verify coalescing works.
- Test returned pointer alignment against `max_align_t`.

## 2. Define Allocation Sizes

- Pick thresholds for allocation classes.
- Suggested starting values:

```c
#define TINY_MAX   512
#define SMALL_MAX  4096
```

- Compute zone sizes so each zone can hold at least 100 allocations of the largest block size.
- Page-align zone sizes before calling `mmap()`.

## 3. Create a Zone Layer

- Add a zone type enum:

```c
typedef enum e_zone_type {
	ZONE_TINY,
	ZONE_SMALL,
	ZONE_LARGE
} t_zone_type;
```

- Add a zone structure that tracks the mapped region and its free list:

```c
typedef struct s_zone {
	t_zone_type		type;
	size_t			size;
	void			*start;
	fl_list_t		*free_list;
	struct s_zone	*next;
} t_zone;
```

## 4. Implement Allocation Routing

- Route allocations by size:

```c
if (size <= TINY_MAX)
	allocate from a TINY zone;
else if (size <= SMALL_MAX)
	allocate from a SMALL zone;
else
	allocate as LARGE;
```

- For TINY and SMALL allocations, search existing zones first.
- If no existing zone has space, create a new zone with `mmap()`.
- For LARGE allocations, create a dedicated mapping for that allocation.

## 5. Implement `malloc()`

- Handle `size == 0` consistently.
- Route the allocation to the correct zone type.
- Return the pointer from the free-list allocator for TINY and SMALL.
- Return the mapped payload pointer for LARGE.

## 6. Implement `free()`

- Ignore `NULL` pointers.
- Find the zone that owns the pointer.
- For TINY and SMALL zones, call `fl_free()`.
- For LARGE zones, remove the zone from the list and call `munmap()`.
- Do not unmap TINY or SMALL zones immediately unless you explicitly add that behavior later.

## 7. Implement `calloc()`

- Check for multiplication overflow.
- Allocate `count * size` bytes with `malloc()`.
- Zero the allocation with `ft_bzero()` or equivalent.

## 8. Implement `realloc()`

- If `ptr == NULL`, behave like `malloc(size)`.
- If `size == 0`, free the pointer and return `NULL`.
- Find the old allocation size from its header.
- If the existing block is large enough, optionally keep it in place.
- Otherwise allocate a new block, copy the smaller of old and new sizes, then free the old block.

## 9. Add Debug/Inspection Helpers

- Implement `show_alloc_mem()`.
- Print zones grouped by TINY, SMALL, and LARGE.
- Print allocated block ranges and total allocated bytes.

## 10. Add Integration Tests

- Test small allocations.
- Test medium allocations.
- Test large allocations.
- Test repeated allocate/free loops.
- Test fragmentation and reuse.
- Test `calloc()` zeroing.
- Test `realloc()` growth and shrink behavior.

## 11. Build the Shared Library

- Compile allocator objects with `-fPIC`.
- Link with `-shared`.
- Produce:

```text
libft_malloc_$(HOSTTYPE).so
```

- Keep a symlink or copy named `libft_malloc.so` if the subject requires it.
