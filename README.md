# `dualis`

`dualis` is a C++20 header-only library that provides convenience classes and functions to operator on binary data (that is, a sequence of bytes).

## Features

`dualis` offers features regarding the manipulation and investigation of binary data that I encountered most often in my own projects. These include:

- Both owning and non-owning convenience classes for binary data (`byte_vector` and `byte_span`)
- Convenient loading and saving of binary files (`byte_vector::from_file()` and `save_bytes()`)
- Reading and writing of integers, both little and big endian (`read_integer<T, WordOrder>()` and `write_integer<T, WordOrder>()`)
- A helper class that tracks the offset to support linear data access (`byte_reader`)

All of these are used in the Windows bitmap file reader `examples/bmp`, so have a look there if you're interested.

## Example usage

A simple example that displays the resolution of a `.bmp` file:
```cpp
#include <dualis.h>
using namespace dualis;
auto const binary = byte_vector::from_file("test.bmp");
auto const width = read_integer<int32_t, little_endian>(binary, 18);
auto const height = read_integer<int32_t, little_endian>(binary, 22);
std::cout << std::abs(width) << "x" << std::abs(height) << "\n";
```

For more comprehensive examples, refer to the `examples` directory.

## Design

I designed most parts of `dualis` based on how the standard library operates. The library is therefore mostly separated into containers (for example `byte_vector`, `byte_span`) and "algorithms" (for example `read_integer`, `write_integer`). It uses iterators where it makes sense, but usually also provides methods and functions that deal with offsets instead (which is the most common case with binary data).

It is also a fact that code is read many more times than it is written, so while some constructs in `dualis` might seem verbose (such as `read_integer<uint32_t, little_endian>(...)`), they serve to make the code much more readable.

## Requirements

`dualis` uses C++20 (in particular concepts and `std::span`), and as such, requires a compiler and standard library that support C++20:

| Compiler | tested version |
|----------|-----------------|
| Visual C++ | 17.0.2 (VS2022) |
| clang | 11 |
| gcc | 10 |

`dualis` also requires CMake >= 3.15. To build the tests, you need Catch2 v3.

## Installation

Simply copy `dualis.h` into your project and `#include` it.
