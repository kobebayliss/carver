# Carver
A lightweight, low-latency fixed-size memory allocator written in C++. <br><br>
Carver pre-allocates memory in `mmap`'d segments and carves them into fixed-size blocks, avoiding repeated `new`/`malloc` calls. 
Freed blocks are recycled through a flat free-stack array.
## Features
- **Fixed-size pool allocation** — `mmap` regions split into object-sized blocks
- **Dynamic heap growth** — when a segment fills up, a new `heap_size` segment is `mmap`'d and linked to the previous one, so the allocator isn't capped at a single region
- **Compile-time sizing** — `obj_size`, `heap_size`, `max_free` are template parameters, enabling inlining and constant folding
- **O(1) allocation** — bump pointer for fresh memory, flat array-backed free stack for recycled blocks
- **Non-intrusive free list** — freed addresses live in a separate contiguous array holding `max_free` pointers
- **Low overhead** — no per-object heap allocation, no metadata inside freed objects
## How it works
`allocate()` will check if a free block is available (using it in this case), otherwise return `bump_ptr` and increment by `obj_size`. If the current segment is exhausted, a new `heap_size` segment is `mmap`'d and linked to the previous one (each segment reserves 8 extra bytes to store a pointer back to the prior segment), then allocation continues from there.<br><br>
`release()` will push the recently freed address onto the free stack.<br><br>
On destruction (or move-assignment), all linked heap segments are walked and `munmap`'d in turn, along with the free-stack array.
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
<img width="361" height="115" alt="Screenshot 2026-07-27 at 3 56 54 PM" src="https://github.com/user-attachments/assets/154315b2-5de0-49d0-b912-50c9c291579e" />

## Why not malloc?
General-purpose allocators handle arbitrary sizes, fragmentation, thread safety, and coalescing. Carver trades that for speed by assuming a single fixed object size and controlled, single-threaded ownership.
## Current known limitations
- **`max_free` is a hard cap** — releases beyond it are silently dropped rather than returned to the heap chain, so a workload that frees more than `max_free` objects without reallocating them will grow additional heap segments unnecessarily
- **No bound on total heap growth** — segments are added on demand with no upper limit, so a leak-like usage pattern (allocating far more live objects than freed) can grow memory usage indefinitely
- **Not thread-safe** — no synchronization around `bump_ptr`, `current_heap`, or `free_stack`

## Disclaimers
Carver is a low-level allocator. Memory safety is the user's responsibility. `mmap` failures are not currently checked — size `heap_size`, `obj_size`, and `max_free` appropriately for your workload. Ensure each heap segment's size is divisible by the selected object's size as well as page size (typically 4KB).
<br>
<br>
When choosing `max_free`, consider your project's requirements; a large `max_free` will cause more efficient reuse of freed memory at the expense of latency (due to more pointer arithmetic and TLB misses due to poor locality of memory access), a small `max_free` will greatly speed up allocations at the cost of growing additional heap segments sooner. Examples of this tradeoff (ran on the given benchmark.cpp file) are shown below.
<br>
<br>

**Default `max_free` (65,536)**

  <img width="2558" height="1437" alt="swappy-20260731_211944" src="https://github.com/user-attachments/assets/401d4aa3-b731-415b-92c0-2334b057c0a6" />

<br>

**Choosing `max_free` large enough to always recycle the free list (12,000,000)**

  <img width="2557" height="1438" alt="swappy-20260731_183315" src="https://github.com/user-attachments/assets/a013811f-1370-4e00-98ac-f69014afd8ba" />
