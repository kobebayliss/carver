#include <cstdio>
#include <cstdlib>
#include <sys/mman.h>

template <size_t obj_size, size_t heap_size = 16777216, size_t max_free = 65536>  // max_free is maximum number of freed objects (not bytes)
class Carver {
	char* current_heap = nullptr;
	char* bump_ptr = nullptr;
	void** free_stack;  // array of freed addresses, not intrusive
	size_t free_count = 0;
	
	char* get_heap(char* prev_heap = nullptr) {
		char* new_heap = static_cast<char*>(mmap(nullptr, heap_size + 8, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));  // allocate heap_size + 8 bytes for pointer to previous heap
		*reinterpret_cast<char**>(new_heap) = prev_heap;
		return new_heap + 8;
	}
public:
	Carver() {
		current_heap = get_heap();
		bump_ptr = current_heap;
		free_stack = static_cast<void**>(mmap(nullptr, max_free * sizeof(void*), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
	}
	~Carver() {
		if (current_heap) munmap(current_heap, heap_size);
		if (free_stack) munmap(free_stack, max_free * sizeof(void*));
	}
	// copy operations (unsupported)
	Carver(const Carver&) = delete;
	Carver& operator=(const Carver&) = delete;
	// move operations
	Carver(Carver&& other) noexcept : current_heap(other.current_heap), bump_ptr(other.bump_ptr), free_stack(other.free_stack), free_count(other.free_count) {
		other.current_heap = nullptr;
		other.bump_ptr = nullptr;
		other.free_stack = nullptr;
		other.free_count = 0;
	}
	Carver& operator=(Carver&& other) noexcept {
		if (this != &other) {
			if (current_heap) munmap(current_heap, heap_size);
			if (free_stack) munmap(free_stack, max_free * sizeof(void*));
			current_heap = other.current_heap;
			bump_ptr = other.bump_ptr;
			free_stack = other.free_stack;
			free_count = other.free_count;
			other.current_heap = nullptr;
			other.bump_ptr = nullptr;
			other.free_stack = nullptr;
			other.free_count = 0;
		}
		return *this;
	}

	void* allocate() {
		if (free_count > 0) [[likely]] {  // get address from free stack
			return free_stack[--free_count];
		}
		if (bump_ptr == current_heap + heap_size) [[unlikely]] {  // reached end of current heap
			current_heap = get_heap(current_heap);
			bump_ptr = current_heap;
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
