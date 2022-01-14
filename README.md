# `dualis`

`dualis` is a C++20 header-only library for working with binary data, that is, on sequences of bytes.
Some highlights:

- `std::byte` is consequently used to represent a byte to improve type safety and thus improve the correctness of implementations.
- `dualis` offers functions for packing and unpacking of typed data, such as integers, into and from `std::byte` ranges while taking word order and other architecture idiosyncrasies into account.
- `dualis` offers containers that satisfy the concept `std::contiguous_range` and improve the ease of use considering they contain binary data.

The following example displays the resolution of a `.bmp` file and illustrates some of the above features:
```cpp
#include <dualis.h>
// ...
using namespace dualis;
// Load the contents from test.bmp into a byte_vector.
auto const binary = load_bytes<byte_vector>("test.bmp");
// Read two signed 32-bit integers from offset 18 using little-endian word order
auto const [width, height] = unpack_tuple<int32_le, int32_le>(binary, 18);
// Display width x height (negative height signifies bottom-top storage).
std::cout << width << "x" << std::abs(height) << "\n";
```

## Table of contents

1. Getting started
2. Packing and unpacking of bytes
    1. `byte_packing`s
    2. Unpacking (convert bytes to typed data)
        1. `unpack()`
        2. `unpack_tuple()`
        3. `unpack_range()`
    3. Packing (convert typed data to bytes)
        1. `pack()`
        2. `pack_tuple()`
        3. `pack_range()`
    4. Predefined `byte_packing`s
        1. `little_endian<T>` and `big_endian<T>`
        2. `raw<T>`
        3. `tuple_packing<Packings...>`
3. Byte containers and spans
    1. `byte_vector` (data is is always dynamically allocated)
    2. `byte_string` (no dynamic allocation for small data)
    3. `byte_container<Allocator, SmallSize>`
    4. `byte_span` and `writable_byte_span`
4. Utilities
    1. `byte_swap<T>()`
    2. `copy_bytes()`, `move_bytes()`, `set_bytes()`, `compare_bytes()`
    3. `to_hex_string()`: displaying bytes
    4. `unsigned_int<Width>` and `signed_int<Width>`: sized integers

## Getting started

### Requirements

`dualis` depends on C++20 features, in particular concepts and ranges, and, consequently, requires a compiler and standard library that support these features.
For reference, `dualis` has been successfully compiled with the following compilers and standard libraries:

| Compiler | tested version |
|----------|-----------------|
| Visual C++ | 19.30.30706 (Visual Studio 17.0.2) |
| clang | 11.1.0 (with stdlibc++ of gcc 10) |
| gcc | 10.2.1 |

When using the CMake project, `dualis` also requires CMake >= 3.15.
Finally, to build the tests, you need Catch2 v3.

### Installation

`dualis` is a header-only library.

This gives you multiple options of including `dualis` into one of your projects.
The following work without CMake:
- Copy the generated single-header version under Releases into your project and include `dualis.h`.
- Copy the `src` directory of this repository into your project and include `dualis/dualis.h`.

If your project is already using CMake you can also:

- Copy or clone this repository (possibly as a submodule) into your project and include it via `add_subdirectory`.
    This makes it easy to use a specific version of `dualis` and/or upgrade to new versions.
- Clone this repository somewhere, install it, and include it into your project using `find_package`.

Using CMake, you can then "link" `dualis` to your library or executable using the following:

```CMake
target_link_libraries(<name> PRIVATE/PUBLIC/INTERFACE Dualis::Library)
```
This automatically sets any required flags (e.g. C++20) and include directories up for you.
Note that `dualis` is implemented as an *interface library* because it is header-only.

## Packing and unpacking of bytes

Binary data is stored as a sequence of bytes, and it is just that&mdash;it has no inherent meaning.
Only by imposing a meaning onto groups of bytes is it that the magic of structured data happens.
For example, consider the following bytes, displayed in hexadecimal (let's assume a byte has 8 bits, as is the case with virtually all architectures nowadays):
```
11 12 13 ff
```
This sequence of four bytes could, for instance, represent a 32-bit unsigned integer.
But even this is not yet an unambiguous interpretation:
There are two types of so-called word order: little endian (the most significant byte comes last) and big endian (the most significant byte comes first).
In C++, we could assign these interpretations as follows:
```cxx
uint32_t little_endian = 0xff131211;
uint32_t big_endian = 0x111213ff;
```
See how the value of the integer completely changes based on the interpretation.

Alternatively, the bytes could also represent two 16-bit signed integers.
Let's consider little endian:
```cxx
int16_t first = 0x1211;
int16_t second = static_cast<int16_t>(0xff13); // actually -237
```

While choosing what a sequence of bytes represents is completely up to you, `dualis` provides functions to *pack* and *unpack* typed data according to your chosen interpretation.

All these functions interpret a given range of `std::byte` using a `byte_packing`, which is a concept that describes classes that implement the actual unpacking and packing operations.
The predefined `byte_packing`s and how to implement custom ones is explained further below.
For now, let us consider the `byte_packing` called `little_endian<T>`, which packs or unpacks an integer of type `T` into or from bytes using the little endian word order.

The examples below use the following alias provided by `dualis`:

```cxx
using uint16_le = dualis::little_endian<uint16_t>;
```

### Unpacking from bytes

The unpacking functions also make use of the concept `byte_range` to describe the argument that represents the sequence of bytes to unpack from:

```cxx
template <class R>
concept byte_range = std::ranges::contiguous_range<R> &&
                     std::same_as<std::remove_cv_t<std::ranges::range_value_t<R>>,
                                  std::byte>;
```

In other words, a `byte_range` is an `std::contiguous_range` with `std::byte` as its value type.
This includes not only simple arrays (`const std::byte[]`), but also many standard library containers, such as `std::vector<std::byte>` or `std::span<const std::byte>`, as well as the containers provided by `dualis`.

#### `unpack`

The main work horse is the template function
```cxx
template <byte_packing Packing, byte_range Bytes>
[[nodiscard]] auto unpack(const Bytes& bytes, std::size_t offset)
    -> typename Packing::value_type;
```

The first argument, `bytes`, represents the `byte_range` to unpack from, while `offset` specifies an offset to start unpacking from.
Consider the example from above:
```cxx
// _b is a literal operator to convert integral literals into std::byte
const std::byte bytes[] = {0x11_b, 0x12_b, 0x13_b, 0xff_b};
auto const first = unpack<int16_le>(bytes, 0);
auto const second = unpack<int16_le>(bytes, 2);
// first == 0x1211 (4625)
// second == 0xff13 (-237)
```

#### `unpack_tuple`

Often, data values are structured (for example records/structs), so it makes sense to unpack them together.
This functionality is provided by this template function:

```cxx
template <byte_packing... Packings, byte_range Bytes>
[[nodiscard]] auto unpack_record(const Bytes& bytes, std::size_t offset);
```

It takes a sequence of `byte_packing` template arguments and unpacks each in the given order and advancing within the given `byte_range` accordingly.
The return type is an `std::tuple`.
Again, consider the above example:

```cxx
const std::byte bytes[] = {0x11_b, 0x12_b, 0x13_b, 0xff_b};
auto const [first, second] = unpack_tuple<int16_le, int16_le>(bytes, 0);
// first == 0x1211 (4625)
// second == 0xff13 (-237)
```

#### `unpack_range`

Finally, binary data often contains typed data values packed as arrays.
The following template function unpacks a sequence of typed data values using the same `byte_packing`:

```cxx
template <byte_packing Packing, byte_range Bytes, class Iterator>
requires std::output_iterator<Iterator, typename Packing::value_type>
auto unpack_range(const Bytes& bytes, std::size_t offset, Iterator first, std::size_t count)
    -> Iterator;
```

This function unpacks `n` consecutive values from the `byte_range` `bytes` using the same given `byte_packing` `Packing` into the given iterator `first`, advancing it after each unpacked value.
It returns the iterator pointing to the element after the last unpacked value.

Consider the above example one more time:
```cxx
const std::byte bytes[] = {0x11_b, 0x12_b, 0x13_b, 0xff_b};
std::vector<int16_t> values(2);
const auto iter = unpack_range<int16_le>(bytes, 0, values.begin(), 2);
// values == {4625, -237}
// iter == values.begin() + 2 == values.end()
```

### Packing into bytes

Packing is the reverse operation of unpacking, so the functions mirror those of unpacking.
They work on `writable_byte_range`s:

```cxx
template <class R>
concept writable_byte_range = byte_range<R> && requires(R& r)
{
    { std::ranges::data(r) } -> std::same_as<std::byte*>;
};
```

In other words, a `writable_byte_range` is a `byte_range` whose data is mutable (note, however, that this does not imply that the range is resizable).
Again, this includes static arrays (`std::byte[]`), many standard library containers with `std::byte` as value type, and the containers offered by `dualis`.

#### `pack`

This function is the inverse of `unpack`:

```cxx
template <byte_packing Packing, class U, writable_byte_range Bytes>
void pack(Bytes& bytes, std::size_t offset, const U& value);
```

It packs the given `value` into `bytes` at `offest` using the given `Packing`.
For example:

```cxx
pack<uint16_lt>(bytes, 2, 0x1234);
// bytes[2] == 0x34_b && bytes[3] == 0x12_b
```

#### `pack_tuple`

This function provides convenient packing of multiple values using different packings:

```cxx
template <byte_packing... Packings, writable_byte_range Bytes>
void pack_tuple(Bytes& bytes, std::size_t offset,
                const typename Packings::value_type&... values);
```

It packs the given `values` into `bytes` at `offset` using the specified `Packings`.
Note that the `values` are passed directly as arguments without the need for a tuple.
For example:

```cxx
pack_tuple<uint16_le, uint16_le>(bytes, 0, /* values: */ 0x1234, 0x5678);
// bytes == {0x34_b, 0x12_b, 0x78_b, 0x56_b, ...}
```

#### `pack_range`

This function provides packing of a range of values with the same packing.
There are two overloads:

```cxx
template <byte_packing Packing, writable_byte_range Bytes, std::input_iterator Iterator>
void pack_range(Bytes& bytes, std::size_t offset, Iterator first, Iterator last);

template <byte_packing Packing, writable_byte_range Bytes, std::ranges::range Range>
void pack_range(Bytes& bytes, std::size_t offset, const Range& range)
```

Both pack the given range of values [`first`, `last`) respectively `range` into `bytes` at `offset` using the specified `Packing` for each value in the range.
For example:

```cxx
std::vector<std::byte> bytes(8);
const uint16_le values[] = {0xdead, 0xbeef};
pack_range<uint16_le>(bytes, 0, values_1, values_1 + 4);
const std::vector<uint16_t> values_2{0x1234, 0x5678};
pack_range<uint16_le>(bytes, 4, values_2);
// bytes == {0xad_b, 0xde_b, 0xef_b, 0xbe_b, 0x34_b, 0x12_b, 0x78_b, 0x56_b}
```

### Predefined and custom `byte_packing`s

Each packing and unpacking function is passed one or multiple `byte_packing`s to specify how the given typed data is encoded into a sequence of bytes or vice versa.
A `byte_packing` is characterized by the following concept:

```cxx
template<class T>
concept byte_packing = requires(const std::byte* bytes,
                                std::byte* writable_bytes,
                                const typename T::value_type& value)
{
    typename T::value_type;
    { T::size() } -> std::same_as<std::size_t>;
    { T::unpack(bytes) } -> std::same_as<typename T::value_type>;
    T::pack(writable_bytes, value);
};
```

Put simply, a class adhering to `byte_packing` requires:
- A nested type named `value_type` that specifies the type to be packed or unpacked.
- A static (most likely `constexpr`) method `size()` that returns the size of a packed value.
- A static method `unpack(const std::byte*)` that unpacks the value from the given byte array, which is at least `size()` bytes long.
- A static method `pack(std::byte*, const value_type&)` that packs the given value into the given byte array, which is at least `size()` bytes long.

Note that these requirements imply that `byte_packing` only describes packings with static size.
Furthermore, `unpack` and `pack` do not need to do bound checking;
instead, bound checking is expected to be done by the functions using a `byte_packing` and its `size()` function.

#### `little_endian` and `big_endian`

As explained above, the individual bytes of an integral value can be stored either with little-endian or big-endian byte order.
Since one goal of `dualis` is to be portable and performant, the corresponding `byte_packing`s `little_endian<T>` and `big_endian<T>` have different implementations on different architectures.

For example, x86/64 processors support unaligned memory access and are little endian, so the fastest way to unpack a little-endian integer is to cast the given byte array to an array of the integers and let the processor do the rearranging.
Reading a big-endian integer, however, requires swapping the bytes.

The fallback solution is to read each byte individually.
Of course, this is also the slowest solution (other than when the compiler is clever enough to group them into a single read or write).

#### `raw<T>`

Sometimes, you want to read a value with the same layout the compiler generates, such as a `struct`.
In this case, use `raw`:

```cxx
struct a_struct { /* ... */ };
// assume bytes is a byte_range
const auto a_struct_value = dualis::unpack<raw<a_struct>>(bytes, 0);
```

Since this is very much architecture, compiler-, and even version- and compilation-flag-dependent, use this at your own risk.
If you use it, make sure to include fail-safes (such as a `static_assert` that ensures the struct has the correct size).

```cxx
struct a_struct { /* ... */ };
static_assert(sizeof(a_struct) == X); // makes it most likely safe
```

#### `tuple_packing<Packings...>`

Combine multiple packings into a single packing that unpacks the values sequentially.This is used internally by `unpack_tuple`.
You can use this directly, for example, to get the size of a tuple packing:
```cxx
constexpr auto size = tuple_packing<uint32_t, uint64_t>::size();
// size == 12
```
Or, as another use case, for unpacking arrays of records:
```cxx
std::vector<std::tuple<uint16_t, uint16_t>> values(n);
dualis::unpack_range<tuple_packing<uint16_t, uint16_t>>(bytes, 0, values.begin(), n);
```

Note that `value_type` of `tuple_packing<Packings...>` is `std::tuple<Packings::value_type...>`.

## Byte containers and spans

`dualis` provides a the template class `byte_container<Allocator, SmallSize>`, which manages an array of `std::byte`.
It owns the associated memory.
`dualis` also provides two instantiations of this template class:

```cxx
/// Enforce dynamic allocation for its owned memory.
using byte_vector = byte_container<std::allocator<std::byte>, 0>;
/// Enable small array optimization.
using byte_string = byte_container<std::allocator<std::byte>,
                                   sizeof(typename std::allocator<std::byte>::size_type) * 2>;                                   
```

Usually, you should use one of these two instantiations, which is why we will cover there use cases first.

### `byte_vector`

The container `byte_vector` is the equivalent to `std::vector<std::byte>` and designed for the same use cases.
It always dynamically allocates its memory if its size is larger than 0, which it then owns and deallocates upon destruction.

### `byte_string`

By contrast, `byte_string` uses the small array optimization:
for small data it stores the data within itself and only dynamically allocates memory when the size exceeds the given threshold (that is, `SmallSize`).
Because dynamic allocation is usually a very expensive operation, this may improve many performance metrics such as cache efficiency and execution time.

`byte_string` is also useful when returning small byte sequences from functions.
Consider the following example, where machine instructions are assembled:

```cxx
using namespace dualis;
using namespace dualis::literals;

auto x64_make_multi_byte_nop(std::size_t size) -> byte_string
{
    if (size == 4)
    {
        return byte_string{0x0f_b, 0x1f_b, 0x40_b, 0x00_b}; // no heap allocation
    }
    else if (...)
    {
        // ...
    }
    // ...
}

byte_vector machine_code;
// ...
machine_code += x64_make_multi_byte_nop(4);
```

### `byte_container<Allocator, SmallSize>`

The template class `byte_container` represents an array of `std::byte` stored in contiguous memory, where the array is stored:

- within the `byte_container` instance as long as its `size()` is less or equal to `SmallSize`.

    (Note that in contrast to `std::string`, which also often uses the small-string optimization, `byte_container` does not store a terminating zero-byte.)

- in memory dynamically allocated using `Allocator` when `size()` is larger than `SmallSize`.
    
    (`Allocator` is a class that adheres to the [*Allocator* named requirement](https://en.cppreference.com/w/cpp/named_req/Allocator))

By design, `byte_container` is very similar to existing standard library containers such as `std::vector` and `std::string`, but additionally offers an interface specifically tuned to working with binary data.

#### Offset-based operations

Most standard library containers work solely on iterators.
However, because with binary data, offsets often offer a more intuitive way of indexing the data, `byte_container` additionally provides overloads that take offsets instead of iterators.
For example:

```cxx
using namespace dualis;
using namespace dualis::literals;

byte_vector bytes{0xde_b, 0xef_b};

// equivalent to bytes.insert(bytes.begin() + 1, {0xad_b, 0xbe_b});
bytes.insert(1, {0xad_b, 0xbe_b});
// bytes == {0xde_b, 0xad_b, 0xbe_b, 0xef_b}

// equivalent to bytes.erase(bytes.begin(), bytes.begin() + 2);
bytes.erase(0, 2);
// bytes == {0xbe_b, 0xef_b}
```

#### Slicing

Binary data is often structured, making it necessary to work on sub-parts of it.
`byte_container` therefore provides multiple ways of accessing parts of its data:

```cxx
using namespace dualis;
using namespace dualis::literals;

byte_container<> bytes{0xde_b, 0xad_b, 0xbe_b, 0xef_b};

// slice is of type byte_container<> and owns its memory
auto const slice = bytes.extract(2, byte_vector::npos); // from 2 to end {0xbe_b, 0xef_b}
// view is of type byte_span, does not own its memory, and is immutable
auto const view = bytes.subspan(2, byte_vector::npos);
// span is of type writable_byte_span, does not own its memory, and is mutable
auto const span = bytes.writable_subspan(2, byte_vector::npos);
```

#### Inserting and appending packed data

It is often useful to directly insert or append packed typed data:

```cxx
byte_vector bytes{0xde_b, 0xef_b};
bytes.insert_packed<uint16_le>(1, 0xbead);
// bytes == {0xde_b, 0xad_b, 0xbe_b, 0xef_b}
```

### `byte_span` and `writable_byte_span`

These classes provide views into a given byte array without owning the associated memory.
This makes them amenable to the creation of sub-views, since it then is a constant-time operation.
Whereas the data of `byte_span` is immutable, the data of `writable_byte_span` is mutable.
Note that `writable_byte_span` can be implicitly cast to `byte_span`.

```cxx
std::byte some_data[] = {0xde_b, 0xad_b, 0xbe_b, 0xef_b};
auto const view = byte_span{some_data, 4};
auto const sub_view = view.subspan(2, byte_span::npos);
// sub_view == {0xbe_b, 0xef_b}

auto writable_view = writable_byte_span{some_data, 4};
writeable_view[0] = 0xbe_b;
writeable_view[1] = 0xef_b;
// some_data == {0xbe_b, 0xef_b, 0xbe_b, 0xef_b}
```

The most common use case for views is to be passed as arguments into functions;
this is because they encapsulate both the size of the data, as well as that the memory is not owned.
In this, they are more versatile than directly passing, for example, a `const byte_vector&` argument because a `byte_span` can be constructed from many different sources, whereas `byte_vector` always refers to its own memory.

Note that at the moment, both `byte_span` and `writable_byte_span` are aliases for `std::span<std::byte>` and `std::span<const std::byte>`, respectively.
However, this might change in the future to provide more convenience methods.

## Byte streams

A common use case is to parse binary data like a stream, that is, not using random access, but rather from front to back in a linear fashion.
It is then rather inconvenient to have to manage the current reading offset (`unpack` does not return the next offset along with the unpacked value because, quite frankly, this would make the interface very ugly&mdash;nobody wants to `std::tie` everything).
To counter this, `dualis` offers `byte_stream`, which is constructed using a `byte_span` and acts as a view with a current offset that is automatically updated according to a given sequence of `unpack` calls.

For example:

```cxx
using namespace dualis;

// let bytes be a byte_span
byte_stream stream{bytes};
// stream.tellg() == 0

const auto value_1 = stream.unpack<uint32_le>();
// stream.tellg() == 4
const auto [value_2, value_3] = stream.unpack_tuple<uint16_le, uint16_le>();
// stream.tellg() == 8

std::vector<int64_t> values(4);
stream.unpack_range<int64_le>(values.begin(), 4);
// stream.tellg() == 40 (8 + int64_le::size() * 4)
```

Of course, it is also possible to change the current offset using `seekg`, just like the streams in the standard library.
Note, however, that `byte_stream` is not in fact derived from `std::istream`.

`byte_stream` also supports the streaming operator:

```cxx
uint16_t first, second;
stream >> unpack<uint16_le>(first)   // same as: first = stream.unpack<uint16_le>();
       >> unpack<uint16_le>(second); //          second = stream.unpack<uint16_le>();
```

Wrapping the variable in `unpack` is necessary because the type of the variable is not enough to discern the desired packing.

## Utilities

`dualis` also offers many utilities that help operating on binary data in general.

### Byte literals

Since `std::byte` is a non-arithmetic type, integer literals must be explicitly cast to it:
```cxx
std::byte a_byte{0xff};
a_byte = 0xef; // error
```
This also makes it annoying to use in initializer lists and other places:
```cxx
auto const list = {std::byte{0xff}, std::byte{0xff}};
```
Therefore, `dualis` provides a convenient literal operator `_b`:
```cxx
using namespace dualis::literals;
auto const list = {0xff_b, 0xff_b};
```

### Byte swapping

Often, you need to swap the word order of an integer, for example when reading big-endian data on a little-endian machine, or when transferring via a network (network order is big endian).
Most processors provide fast machine instructions for this that are exposed by most compilers using intrinsics.
However, since these intrinsics vary from compiler to compiler, and because they are not type-safe (or rather are divided into multiple intrinsics depending on the argument size), `dualis` provides a convenience function:
```cxx
template <std::unsigned_integral T>
[[nodiscard]] auto byte_swap(T value) -> T;
```
For example:
```cxx
const uint32 value = 0x12345678;
const auto swapped = dualis::byte_swap(value);
// swapped == 0x78563412
```
If the compiler does not offer intrinsics, a general (albeit most likely slower) implementation is used instead.

Note that `byte_swap` operates on unsigned integers;
for signed integers, first `static_cast` them to their unsigned equivalent.

### Byte data copying, setting, and comparing

Similar to byte swapping, some compilers offer intrinsics for fast copying, setting, and comparing of memory regions.
To use these in the case of `std::byte` arrays, `dualis` provides the following convenience functions:

```cxx
void copy_bytes(std::byte* dest, const std::byte* src, std::size_t size);
void move_bytes(std::byte* dest, const std::byte* src, std::size_t size);
void set_bytes(std::byte* dest, std::byte value, std::size_t size);
int compare_bytes(const std::byte* a, const std::byte* b, std::size_t size);
```

If the compiler does not offer intrinsics, `std::memcpy`, `std::memset`, and `std::memcmp` are used as default.

### Displaying bytes

Often, binary memory content needs to be displayed to the user.
To facilitate this, `dualis` provides the following convenience function:

```cxx
auto to_hex_string(const std::byte value) -> std::string;
```

This function converts the given byte value into a hexadecimal string with a length of two (it is always zero-padded).

```cxx
auto const as_hex = to_hex_string(0xef_b);
// as_hex == "ef"
```

### Integers of specific byte width

While C++ offers integers of a given width (such as `uint32_t`) in `<cstdint>`, sometimes it is convenient to have the width as a template argument.
For this, `dualis` offers the following two templates:

```cxx
template <std::size_t Width> class unsigned_int;
template <std::size_t Width> class signed_int;
```

For example, `unsigned_int<4>` resolves to `uint32_t`.
For now, it is an error if no built-in type of the given width exists (such as `unsigned_int<7>`).

## Design philosophy

I designed most parts of `dualis` based on the standard library.
The library is therefore mostly separated into containers (for example `byte_vector`, `byte_span`) and "algorithms" (`pack` and `unpack`).
However, `dualis` deviates in places I deem ...
For example, while `dualis` uses iterators where it makes sense, it also provides methods and functions that deal with offsets instead (which often is the most intuitive with binary data).
Also, I think it makes more sense to have immutability ("constness") as the default, which is why mutable constructs have the odd-one-out naming (for example `writable_subspan`) where possible, instead of the immutable ones (for example `cbegin` from the standard library).