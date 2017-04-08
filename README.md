Repro case for spurious(?) GL debug messages
====

To build and run

```
git submodule update --init --recursive
mkdir build
cd build
cmake ..
make
./querystall
```

Observe the following performance debug messages:

```
GL_DEBUG_SOURCE_API GL_DEBUG_TYPE_PERFORMANCE GL_DEBUG_SEVERITY_MEDIUM  id: 0xb
CPU mapping a busy query object BO stalled and took 1.875 ms.
```

