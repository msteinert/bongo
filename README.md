# Bongo

![Baby Bongo](share/images/baby-bongo.jpg)

---

Bongo is a port of [Go][] APIs to C++. This code is heavily based on the [Go
source][] and is therefore covered by the same copyright and 3-clause BSD
[license][].

[Go]: https://golang.org
[Go source]: https://github.com/golang/go
[license]: LICENSE

## Contents

- [API](#api)
- [Development](#development)
- [Versioning](#versioning)

## API

The author found himself missing some of the nice features provided by Go
when developing C++ applications. This is the result of an effort to fill in
those gaps. It also turns out to be an interesting way to understand how the
Go standard library is built. Take it as you will.

Sample code demonstrating some of the APIs can be found in the
[examples](examples/) directory.

## Development

CMake and a C++20 compiler are required to build the project. The test suite
requires [Catch2][]. To build with gcc-10 and run the test suite:

```
$ mkdir build && cd build
$ CXX=gcc-10 cmake ..
$ ninja
$ ./bongo/bongo-test
```

To run the benchmarks:

```
./bongo/bongo-test "~[benchmark]"
```

[Catch2]: https://github.com/catchorg/Catch2
