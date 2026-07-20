#include <cstdio>
#include <iostream>

struct Foo {
	size_t id;
	Foo(size_t i) : id(i) {
		printf("Foo %d constructed\n", id);
	}
	~Foo() {
		printf("Foo %d destroyed\n", id);
	}
};

namespace Carver {
	const size_t CAPACITY = 20;
	char storage[sizeof(Foo) * CAPACITY];
	size_t next_free_slot = 0;
	void* get_next_slot() {
		void* slot = storage + (next_free_slot * sizeof(Foo));
		next_free_slot++;
		return slot;
	}
};

int main() {
	void* slot = Carver::get_next_slot();
	Foo* test = new (slot) Foo('A');
	void* slot2 = Carver::get_next_slot();
	Foo* test2 = new (slot2) Foo('B');
	Foo* test3 = new Foo('C');
	Foo* test4 = new Foo('D');
	std::cout << "Carver allocation (contiguous):        " << test << '\n';
	std::cout << "Carver allocation (next contiguous):   " << test2 << '\n';
	std::cout << "Default heap allocation:               " << test3 << '\n';
	std::cout << "Default heap allocation (independent): " << test4 << '\n';
	test->~Foo();
	test2->~Foo();
	test3->~Foo();
	test4->~Foo();
}
