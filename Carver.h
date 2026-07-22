#include <cstdio>
#include <sys/mman.h>

struct Node {
	Node* next;
};

class Carver {
	void* heap = nullptr;
	const size_t heap_size;
	const size_t obj_size;
	size_t next_free_slot = 0;  // relative to beginning of local heap
	Node* head;  // next free block of memory (not at end of assigned memory)
public:
	Carver(size_t obj_size, size_t heap_size = 16777216) : obj_size(obj_size), heap_size(heap_size) {
		heap = mmap(nullptr, heap_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);  // ask OS for block of memory for custom heap (default = 16MB)
		head = nullptr;
	}
	~Carver() {
		munmap(heap, heap_size);
	}
	void* allocate() {
		if (head) {  // if memory was released in middle of block, refill it first
			Node* node = head;
			head = node->next;
			return node;
		}
		void* slot = static_cast<char*>(heap) + (next_free_slot * obj_size);
		next_free_slot++;
		return slot;
	}
	void release(void* addr) {
		if (addr != (static_cast<char*>(heap) + ((next_free_slot - 1) * obj_size))) {  // add to free list if releasing slot not at the end of the contiguous blocks
			Node* node = static_cast<Node*>(addr);
			node->next = head;
			head = node;
		} else {
			next_free_slot--;
		}
	}
};

