#include <cstdio>
#include <iostream>
#include <sys/mman.h>

struct Foo {
	size_t id;
	Foo(size_t i) : id(i) {
		printf("Foo %zu constructed\n", id);
	}
	~Foo() {
		printf("Foo %zu destroyed\n", id);
	}
};

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
		std::cout << "Created Carver's heap at " << heap << " to " << static_cast<void*>(static_cast<char*>(heap) + heap_size) << std::endl;
	}
	~Carver() {
		munmap(heap, heap_size);
		std::cout << "Released heap memory" << std::endl;
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
		std::cout << "Released memory at " << addr << std::endl;
	}
};

int main() {
    Carver carver(sizeof(Foo));

    Foo* foos[12];

    // Allocate 8 objects
    std::cout << "\n=== Initial allocations ===\n";
    for (size_t i = 0; i < 8; i++) {
        void* addr = carver.allocate();
        foos[i] = new (addr) Foo(i);
        std::cout << "Foo " << i << " @ " << foos[i] << '\n';
    }

    // Destroy and release a few of them
    std::cout << "\n=== Releasing Foo 2, 5, and 6 ===\n";
    for (int i : {2, 5, 6}) {
        foos[i]->~Foo();
        carver.release(foos[i]);
    }

    // Allocate 4 more
    std::cout << "\n=== Allocating 4 more ===\n";
    for (size_t i = 8; i < 12; i++) {
        void* addr = carver.allocate();
        foos[i] = new (addr) Foo(i);
        std::cout << "Foo " << i << " @ " << foos[i] << '\n';
    }

    // Clean up remaining objects
    std::cout << "\n=== Cleaning up ===\n";
    for (size_t i = 0; i < 12; i++) {
        if (foos[i]) {
            foos[i]->~Foo();
            carver.release(foos[i]);
        }
    }
}
