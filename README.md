# One Billion Row Challenge in C++

Solution to [1BRC](https://github.com/gunnarmorling/1brc) using C++20.

## Implementation

This solution tries to use as much standard C++ features as possible.
Only the following is done,
without the extra tricks implemented by the fastest solutions
(SIMD, ILP, custom hashtables, etc):

- Use memory mapping
- Use multithreading
- Use custom hash function for strings (slightly faster)
- Parse values as integers

A simple baseline solution is also provided for comparison,
using standard file I/O and single-threaded

## How to build

Use CMake:

``` console
$ cmake -B build -S .
$ cmake --build build
```

## How to run

To run the _main_ memory-mapped, multithreaded solution:

``` console
$ time ./build/main [ <input_file> ]
```

This uses all cores in the machine.
Set the environment variable `NUM_THREADS` to configure the number of runtime threads.

To run a baseline single-thread solution:

``` console
$ time ./build/baseline [ <input_file> ]
```

By default the `measurements.txt` file is expected to be in the working directory.

## Other solutions

C implementation:

- [austindonisan](https://github.com/gunnarmorling/1brc/discussions/710) (fastest, uses AVX2 and other tricks)
- [dannyvankooten](https://github.com/gunnarmorling/1brc/discussions/46) (faster, uses custom hashtable)

C++ implementation:

- [lehuyduc](https://github.com/gunnarmorling/1brc/discussions/138) (fastest, uses SIMD and other tricks)
- [charlielye](https://github.com/gunnarmorling/1brc/discussions/495) (fastest, uses SIMD and other tricks)
- [kajott](https://github.com/gunnarmorling/1brc/discussions/135) (faster, uses custom hashmap and allocation)
- [csantv](https://github.com/gunnarmorling/1brc/discussions/43)
