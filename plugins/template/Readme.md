# Plugin Template

This is a plugin template. It shows how to set up the basic structure of a plugin with initialization, event-handling
and unit tests.

## How to Build

- Install Build Requirements
    - g++ or clang
    - cmake
    - vcpkg
- Clone this repository
- Inside the source directory, run:

```console
[user@localhost source_dir]$ cmake --preset <gcc/clang>-debug
[user@localhost source_dir]$ cmake --build --preset <gcc/clang>-build-debug
```
