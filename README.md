# Carver

A lightweight, low-latency fixed-size memory allocator written in C++.

Carver pre-allocates a large memory region via `mmap` and carves it into fixed-size blocks, avoiding repeated `new`/`malloc` calls. 
Freed blocks are recycled through a flat free-stack array.

## Features

- **Fixed-size pool allocation** — one large `mmap` region, split into object-sized blocks
- **Compile-time sizing** — `obj_size`, `heap_size`, `max_free` are template parameters, enabling inlining and constant folding
- **O(1) allocation** — bump pointer for fresh memory, flat array-backed free stack for recycled blocks
- **Non-intrusive free list** — freed addresses live in a separate contiguous array holding `max_free` pointers
- **Placement construction** — objects built in-place with placement `new`
- **Low overhead** — no per-object heap allocation, no metadata inside freed objects

## How it works
`allocate()` will check if a free block is available (using it in this case), otherwise return bump_ptr and increment by obj_size<br>
`release()` will push the recently freed address onto the free stack

## Example

```cpp
#include "carver.hpp"

struct Foo {
    size_t id;
    Foo(size_t i) : id(i) {}
};

int main() {
    Carver<sizeof(Foo)> carver;
    void* memory = carver.allocate();
    Foo* foo = new(memory) Foo(42);
    foo->~Foo();
    carver.release(foo);
}
```

## Benchmark

Tested with 10,000,000 allocations, released ~1/3 randomly, then 2,000,000 more allocations, followed by full cleanup.

- ~3.03x as fast as new/delete
- ~2.88x as fast as malloc/free

<img width="361" height="115" alt="Screenshot 2026-07-27 at 3 56 54 PM" src="https://github.com/user-attachments/assets/154315b2-5de0-49d0-b912-50c9c291579e" />

## Why not malloc?

General-purpose allocators handle arbitrary sizes, fragmentation, thread safety, and coalescing. Carver trades that for speed by assuming a single fixed object size and controlled, single-threaded ownership.

## Current known limitations

- **`max_free` is a hard cap** — releases beyond it are silently dropped (can lead to seg faults if total space of heaps exceeds 2-3GB)
- **Not thread-safe** — no synchronization around `bump_ptr` or `free_stack`

<br>

Disclaimer: Carver is a low-level allocator. Memory safety is the user's responsibility. `mmap` failures are not currently checked — size `heap_size`, `obj_size`, and `max_free` appropriately for your workload. Ensure your selected heap size is large enough, and is divisible by the selected object's size as well as page size (typically 4KB).
<br>
<br>
When choosing `max_free`, consider your project's requirements; a large `max_free` will cause more efficient reuse of freed memory at the expense of latency (due to more pointer arithmetic and TLB misses), a small `max_free` will greatly speed up allocations at the cost of eating large chunks of memory.
