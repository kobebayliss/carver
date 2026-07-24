#include "carver.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <random>

constexpr size_t ITERATIONS = 10'000'000;
constexpr size_t REALLOC_COUNT = 2'000'000;
constexpr size_t num_tests = 20;

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

// Pre-generate ONE removal pattern (roughly 1/3 chance per object) and reuse
// it across all three allocators so each benchmark faces the identical
// workload instead of three different random draws.
std::vector<bool> makeRemovalMask(size_t n, double p, unsigned seed) {
	std::vector<bool> mask(n, false);
	std::mt19937_64 rng(seed);
	std::bernoulli_distribution dist(p);
	for (size_t i = 0; i < n; i++)
		mask[i] = dist(rng);
	return mask;
}

int main() {
    Carver carver(sizeof(Foo), 134217728 * 10);
	Foo** objects = new Foo*[ITERATIONS + REALLOC_COUNT]{};

    const unsigned SEED = 12345;
    std::vector<bool> removalMask = makeRemovalMask(ITERATIONS, 1.0 / 3.0, SEED);

    auto runBenchmark = [&](const char* name, auto&& func) {
        double total = 0.0;

        for (size_t j = 0; j < num_tests + 1; j++) {
	
            double value = benchmark(name, func);

	    std::cout << name << " took " << value << "ms for test " << j << std::endl;
            if (j != 0)
                total += value;

        }

        std::cout << name << " avg (" << num_tests << " tests): " << total / num_tests << " ms\n\n";
    };

	runBenchmark("new/delete", [&]() {
	    for (size_t i = 0; i < ITERATIONS; i++) {
		objects[i] = new Foo(i);
	    }

	    for (size_t i = 0; i < ITERATIONS; i++) {
		if (removalMask[i]) {
		    delete objects[i];
		    objects[i] = nullptr;
		}
	    }

	    for (size_t i = 0; i < REALLOC_COUNT; i++) {
		objects[ITERATIONS + i] = new Foo(i);
	    }

	    for (size_t i = 0; i < ITERATIONS + REALLOC_COUNT; i++) {
		if (objects[i]) {
		    delete objects[i];
		    objects[i] = nullptr;
		}
	    }
	});

	runBenchmark("malloc/free", [&]() {
	    for (size_t i = 0; i < ITERATIONS; i++) {
		void* memory = malloc(sizeof(Foo));
		objects[i] = new (memory) Foo(i);
	    }

	    for (size_t i = 0; i < ITERATIONS; i++) {
		if (removalMask[i]) {
		    objects[i]->~Foo();
		    free(objects[i]);
		    objects[i] = nullptr;
		}
	    }

	    for (size_t i = 0; i < REALLOC_COUNT; i++) {
		void* memory = malloc(sizeof(Foo));
		objects[ITERATIONS + i] = new (memory) Foo(i);
	    }

	    for (size_t i = 0; i < ITERATIONS + REALLOC_COUNT; i++) {
		if (objects[i]) {
		    objects[i]->~Foo();
		    free(objects[i]);
		    objects[i] = nullptr;
		}
	    }
	});

	runBenchmark("Carver", [&]() {
	    for (size_t i = 0; i < ITERATIONS; i++) {
		void* memory = carver.allocate();
		objects[i] = new (memory) Foo(i);
	    }

	    for (size_t i = 0; i < ITERATIONS; i++) {
		if (removalMask[i]) {
		    objects[i]->~Foo();
		    carver.release(objects[i]);
		    objects[i] = nullptr;
		}
	    }

	    for (size_t i = 0; i < REALLOC_COUNT; i++) {
		void* memory = carver.allocate();
		objects[ITERATIONS + i] = new (memory) Foo(i);
	    }

	    for (size_t i = 0; i < ITERATIONS + REALLOC_COUNT; i++) {
		if (objects[i]) {
		    objects[i]->~Foo();
		    carver.release(objects[i]);
		    objects[i] = nullptr;
		}
	    }
	});
    delete[] objects;
}
