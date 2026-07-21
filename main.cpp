#include <cstdio>
#include <iostream>
#include <sys/mman.h>
#include <deque>

struct Foo {
	size_t id;
	Foo(size_t i) : id(i) {
		printf("Foo %d constructed\n", id);
	}
	~Foo() {
		printf("Foo %d destroyed\n", id);
	}
};

class Carver {
	void* heap = nullptr;
	const size_t heap_size;
	const size_t obj_size;
	size_t next_free_slot = 0;  // relative to beginning of local heap
	std::deque<void*> free_list;
public:
	Carver(size_t obj_size, size_t heap_size = 16777216) : obj_size(obj_size), heap_size(heap_size) {
		heap = mmap(nullptr, heap_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		free_list = {};
		std::cout << "Created Carver's heap at " << heap << " to " << static_cast<void*>(static_cast<char*>(heap) + heap_size) << std::endl;
	}
	~Carver() {
		munmap(heap, heap_size);
		std::cout << "Released heap memory" << std::endl;
	}
	void* allocate() {
		void* slot = nullptr;
		if (free_list.empty()) {  // if free list queue is empty, allocate next free segment in contiguous manner
			slot = static_cast<char*>(heap) + (next_free_slot * obj_size);
			next_free_slot++;
		} else {
			slot = free_list.front();
			free_list.pop_front();
		}
		return slot;
	}
	void release(void* addr) {
		if (addr != (static_cast<char*>(heap) + ((next_free_slot - 1) * obj_size))) {  // add to free list queue if releasing slot not at the end of the contiguous blocks
			free_list.push_back(addr);
		} else {
			next_free_slot--;
		}
		std::cout << "Released memory at " << addr << std::endl;
	}
};

int main() {
	Carver carver(sizeof(Foo));
	void* addr1 = carver.allocate();
	Foo* foo1 = new (addr1) Foo('A');
	void* addr2 = carver.allocate();
	Foo* foo2 = new (addr2) Foo('B');
	std::cout << "Carver allocation 1: " << foo1 << std::endl;
	std::cout << "Carver allocation 2: " << foo2 << std::endl;
	foo1->~Foo();
	foo2->~Foo();
	carver.release(addr1);
	carver.release(addr2);
}
