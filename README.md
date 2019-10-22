# Mapping DataBases into a File System (MDBFS)

This repository contains code implementing my _COMP4004: Final Year Project I_.

This project implements a file system in userspace (FUSE) that takes a database container or a database management system connector, and mount it as a file system, with the file system hierarchies (i.e. files and folders) be mapped from the database.

## Dependencies

### Required Build Dependencies

- **LibFUSE** as facilities to file system in userspace implementation
- A C compiler that supports **C99**
- A C++ compiler that supports **C++17**, particularly **std::filesystem**

### Required Runtime Dependencies

- **LibFUSE** for FUSE to work
- **libgcc** and **libstdc++(.6)** that supports **C++17 std::filesystem**

The above dependencies can be statically compiled into the binary, so that the binary can be easily distributed. See [Build](#Build).

### Optional Dependencies

- **SQLite3** for SQLite (version 3) database backend support
- **Berkeley DB 18.1.32** for Berkeley DB database backend support
- **Doxygen** for API documentation generation

For selecting optional dependencies in the build system, see [Build](#Build).

## Build

Use [CMake][cmake]:

```shell
mkdir build && cd build
cmake .. # Configure the project using CMake
make     # Build the project, may differ based on the generator you choose
```

You can specify some options in `cmake` to customize your build:

- `-DBUILD_DOCUMENTATION`: Enable build target for API documentation using Doxygen in HTML, default to `OFF`
  - The documentation is not built automatically (not in `ALL` target). To build the API documentation, use `make docs` (or equivalences in other build systems).
- `-DBUILD_SQLITE3`: Enable the SQLite3 database backend, default to `ON`
- `-DBUILD_BERKELEY_DB`: Enable the Berkeley DB ("DB") database backend, default to `ON`

The following options make parts of code be statically compiled into the binary:

- `-DSTATIC_SQLITE3`: Statically compile the SQLite3 library into the backend, default to `OFF`
- `-DSTATIC_BERKELEY_DB`: Statically compile the Berkeley DB library into the backend, default to `OFF`
- `-DSTATIC_FUSE`: Statically compile the FUSE code into the binary, default to `OFF`
- `-DSTATIC_LIBGCC`: Statically compile `libgcc` into the binary, default to `OFF`
- `-DSTATIC_LIBSTDCXX`: Statically compile `libstdc++` into the binary, default to `OFF`

[cmake]: https://cmake.org

## License

This project is licensed under the MIT license. See [COPYING](COPYING) for details.
