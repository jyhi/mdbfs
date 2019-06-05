# Mapping DataBases into a File System (MDBFS)

This repository contains code implementing my _COMP4004: Final Year Project I_.

This project implements a file system in userspace (FUSE) that takes a database container or a database management system connector, and mount it as a file system, with the file system hierarchies (i.e. files and folders) be mapped from the database.

## Build

Use [CMake][cmake]:

```shell
mkdir build && cd build
cmake .. # Configure the project using CMake
make     # Build the project, may differ based on the generator you choose
```

You can specify some options in `cmake` to customize your build:

- `-DBUILD_DOCUMENTATION`: Build API documentation using Doxygen in HTML, default `ON`

[cmake]: https://cmake.org

## License

This project is licensed under the MIT license. See [COPYING](COPYING) for details.
