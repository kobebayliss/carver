# Carver

A lightweight, low-latency fixed-size memory allocator written in C++.

Carver pre-allocates a large memory region via `mmap` and carves it into fixed-size blocks, avoiding repeated `new`/`malloc` calls. Freed blocks are recycled through an intrusive free list. Built to explore how custom allocators achieve fast, predictable-latency allocation.

## Features

- **Fixed-size pool allocation** — one large `mmap` region, split into object-sized blocks
- **O(1) allocation** — bump pointer for fresh memory, intrusive free list for recycled blocks
- **Memory reuse** — released blocks are reused immediately, no extra allocation calls
- **Placement construction** — objects built in-place with placement `new`
- **Low overhead** — no per-object heap allocation, no external metadata

## How it works

```
mmap() a large heap  →  bump-allocate blocks sequentially
                     →  on release, block is pushed onto an intrusive free list
                     →  future allocations pop from the free list before carving new memory
```

Freed blocks store their own "next" pointer inline, so no separate free-list nodes are needed.

## Example

```cpp
#include "carver.hpp"

struct Foo {
    size_t id;
    Foo(size_t i) : id(i) {}
};

int main() {
    Carver carver(sizeof(Foo));

    void* memory = carver.allocate();
    Foo* foo = new(memory) Foo(42);

    foo->~Foo();
    carver.release(foo);
}
```

## Benchmark

Tested with 10,000,000 allocations, released half, then 5,000,000 more allocations.
- ~11.6x as fast as new/delete
- ~10.9x as fast as malloc/free

<img width="709" height="200" alt="image" src="https://github.com/user-attachments/assets/18362ee6-3243-407c-aa40-69e9f98ed7c7" />

## Why not malloc?

General-purpose allocators handle arbitrary sizes, fragmentation, thread safety, and coalescing. Carver trades that generality for speed by assuming fixed-size objects and controlled ownership.
<br>
<br>
Disclaimer: Carver is a low-level allocator. Memory safety (particularly seg faulting) is the user's responsibility. Ensure your selected heap size is large enough, and is divisible by the selected object's size as well as page size (typically 4KB)
