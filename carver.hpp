#include <cstdio>
#include <sys/mman.h>

struct Node {
	Node* next;
};

struct Carver {
	void* heap = nullptr;
	void* bump_ptr = nullptr;
	const size_t heap_size;
	const size_t obj_size;
	
	void* get_heap(size_t heap_size) {
		return mmap(nullptr, heap_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_POPULATE, -1, 0);
	}

public:
	Carver(size_t obj_size, size_t heap_size = 16777216) : obj_size(obj_size), heap_size(heap_size) {
		heap = get_heap(heap_size);
		bump_ptr = static_cast<char*>(heap);
	}
	~Carver() {
		munmap(heap, heap_size);
	}

	void* allocate() {
		// get pointer to untouched memory
		if (bump_ptr == static_cast<char*>(heap) + heap_size) {  // end of current heap, get new heap and move bump ptr there
			bump_ptr = get_heap(heap_size);
		}
		void* ptr = bump_ptr;
		bump_ptr = static_cast<void*>(static_cast<char*>(bump_ptr) + obj_size);
		return ptr;
	}
	void release(void* addr) {
		char* slot = static_cast<char*>(addr);
		if (slot + obj_size == bump_ptr) {  // was last object allocated, bump ptr moves back
			bump_ptr = addr;
		}
	}
};
