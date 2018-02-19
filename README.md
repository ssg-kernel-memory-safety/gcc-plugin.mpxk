# gcc-plugin.mpxk

Exploratory MPX gcc-plugin for the Linux kernel. At present this plugin relies on specific compile
flags used by the Linux kernel build system, and as such cannot be used in any general purpose kind
of way.

The plugin is currently tested only with GCC v.6.3. On Ubuntu 17.04 the `gcc-6-plugin-dev` package
is needed to compile, but you're mileage may vary.

Provided all prerequisites are available the plugin can be compiled by executing `make`.

## Stand-alone tests

The included test functionality allows running some test outside the Linux kernel build system. This
is achieved by using mockups for needed kernel functions, including the MPXK wrappers and loaders.

To compile and run MPXK tests, run:

```
make test_mpxk
```

To compile and run test with MPX, bit without MPXK, run:

```
make test_mpx
```

To run all tests, along with a minimal `objdump | grep ...` check for some unexpected instructions,
run:

```
make test
```

## Limitations

Function argument bounds are currently managed correctly only for void pointers!

## External code

The `gcc*` and `Makefile` files are based on Linux source-code, with originals found under `scripts/gcc-plugins/`.
