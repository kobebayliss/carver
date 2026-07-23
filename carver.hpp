#include <cstdio>
#include <sys/mman.h>

struct Node {
	Node* next;
};

struct Carver {
	void* heap = nullptr;
	const size_t heap_size;
	const size_t obj_size;
	Node* head;  // next free block of memory (freed before)
	char* bump_ptr;
public:
	Carver(size_t obj_size, size_t heap_size = 16777216) : obj_size(obj_size), heap_size(heap_size) {
		heap = mmap(nullptr, heap_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);  // ask OS for block of memory for custom heap (default = 16MB)
		head = nullptr;
		bump_ptr = static_cast<char*>(heap);
	}
	~Carver() {
		munmap(heap, heap_size);
	}
	void* allocate() {
		if (head) {
			Node* node = head;
			head = head->next;
			return node;
		}
		// no freed memory in linked list; get pointer to untouched memory
		void* ptr = bump_ptr;
		bump_ptr += obj_size;
		return ptr;
	}
	void release(void* addr) {
		Node* node = static_cast<Node*>(addr);
		node->next = head;
		head = node;
	}
};
