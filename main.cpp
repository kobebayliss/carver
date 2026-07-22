#include "Carver.h"
#include <iostream>

struct Foo {
    size_t id;

    Foo(size_t i) : id(i) {
        std::cout << "Foo " << id << " constructed\n";
    }

    ~Foo() {
        std::cout << "Foo " << id << " destroyed\n";
    }
};


int main() {
    Carver carver(sizeof(Foo));

    Foo* foos[12] = {};

    std::cout << "\n=== Initial allocations ===\n";

    for (size_t i = 0; i < 8; i++) {
        void* addr = carver.allocate();
        foos[i] = new(addr) Foo(i);

        std::cout << "Foo " << i 
                  << " @ " << foos[i] 
                  << "\n";
    }


    std::cout << "\n=== Releasing Foo 2, 5, 6 ===\n";

    for (int i : {2, 5, 6}) {
        foos[i]->~Foo();
        carver.release(foos[i]);
    }


    std::cout << "\n=== Allocating 4 more ===\n";

    for (size_t i = 8; i < 12; i++) {
        void* addr = carver.allocate();
        foos[i] = new(addr) Foo(i);

        std::cout << "Foo " << i 
                  << " @ " << foos[i]
                  << "\n";
    }


    std::cout << "\n=== Cleanup ===\n";

    for (size_t i = 0; i < 12; i++) {
        if (foos[i]) {
            foos[i]->~Foo();
            carver.release(foos[i]);
        }
    }
}
