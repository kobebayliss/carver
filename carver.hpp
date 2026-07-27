#include <cstdio>
#include <sys/mman.h>

template <size_t obj_size, size_t heap_size = 16777216, size_t max_free = 65536>  // max_free is maximum number of freed objects (not bytes)
class Carver {
	char* heap = nullptr;
	char* bump_ptr = nullptr;
	void** free_stack;  // array of freed addresses, not intrusive
	size_t free_count = 0;
	
	char* get_heap() {
		return static_cast<char*>(mmap(nullptr, heap_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
	}
public:
	Carver() {
		heap = get_heap();
		bump_ptr = heap;
		free_stack = static_cast<void**>(mmap(nullptr, max_free * sizeof(void*), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
	}
	~Carver() {
		munmap(heap, heap_size);
		munmap(free_stack, max_free * sizeof(void*));
	}
	void* allocate() {
		if (free_count > 0) [[likely]] {  // get address from free stack
			return free_stack[--free_count];
		}
		if (bump_ptr == heap + heap_size) [[unlikely]] {  // reached end of current heap
			bump_ptr = get_heap();
		}
		void* ptr = bump_ptr;
		bump_ptr += obj_size;
		return ptr;
	}
	void release(void* addr) {
		if (free_count < max_free) [[likely]] {  // place address on free stack
			free_stack[free_count++] = addr;
		}
	}
};
