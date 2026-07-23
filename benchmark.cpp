#include "carver.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <vector>

constexpr size_t ITERATIONS = 10'000'000;

struct Foo {
	size_t id;
	Foo(size_t i = 0) : id(i) {}
};

template<typename Func>
double benchmark(const char* name, Func func) {
	auto start = std::chrono::high_resolution_clock::now();
	func();
	auto end = std::chrono::high_resolution_clock::now();
	auto duration =
		std::chrono::duration_cast<std::chrono::nanoseconds>(
			end - start
		).count();
	return duration / 1e6;
}

int main() {
    Carver carver(sizeof(Foo), 134217728);
    std::vector<Foo*> objects;
    objects.reserve(ITERATIONS);

    auto runBenchmark = [&](const char* name, auto&& func) {
        double total = 0.0;

        for (size_t j = 0; j < 20; j++) {
            double value = benchmark(name, func);
	    std::cout << value << "ms" << std::endl;

            if (j != 0)
                total += value;

            std::cout << name << " iteration #" << j << '\n';
        }

        std::cout << name << " avg (19 tests): " << total / 19 << " ms\n\n";
    };

    runBenchmark("new/delete", [&]() {
        for (size_t i = 0; i < ITERATIONS; i++)
            objects.push_back(new Foo(i));

        for (size_t i = 0; i < ITERATIONS; i += 2) {
            delete objects[i];
            objects[i] = nullptr;
        }

        for (size_t i = 0; i < ITERATIONS / 2; i++)
            objects.push_back(new Foo(i));

        for (Foo* obj : objects)
            if (obj)
                delete obj;

        objects.clear();
    });

    runBenchmark("malloc/free", [&]() {
        for (size_t i = 0; i < ITERATIONS; i++) {
            void* memory = malloc(sizeof(Foo));
            Foo* foo = new (memory) Foo(i);
            objects.push_back(foo);
        }

        for (size_t i = 0; i < ITERATIONS; i += 2) {
            objects[i]->~Foo();
            free(objects[i]);
            objects[i] = nullptr;
        }

        for (size_t i = 0; i < ITERATIONS / 2; i++) {
            void* memory = malloc(sizeof(Foo));
            Foo* foo = new (memory) Foo(i);
            objects.push_back(foo);
        }

        for (Foo* obj : objects) {
            if (obj) {
                obj->~Foo();
                free(obj);
            }
        }

        objects.clear();
    });

    runBenchmark("Carver", [&]() {
        for (size_t i = 0; i < ITERATIONS; i++) {
            void* memory = carver.allocate();
            Foo* foo = new (memory) Foo(i);
            objects.push_back(foo);
        }

        for (size_t i = 0; i < ITERATIONS; i += 2) {
            objects[i]->~Foo();
            carver.release(objects[i]);
            objects[i] = nullptr;
        }

        for (size_t i = 0; i < ITERATIONS / 2; i++) {
            void* memory = carver.allocate();
            Foo* foo = new (memory) Foo(i);
            objects.push_back(foo);
        }

        for (Foo* obj : objects) {
            if (obj) {
                obj->~Foo();
                carver.release(obj);
            }
        }

        objects.clear();
    });
}
