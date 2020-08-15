Azure Sphere C++ wrapper
========================

Overview
--------

This C++ wrapper for the Azure Sphere API can be easily imported into your Azure
Sphere project. The wrapper abstracts the following functionality (so far):

* Event loop;
* Updates notifications;
* Power management;
* Application watchdog;
* GPIOs;
* Timers;

Setting up a project
--------------------

1) Make `sphereplusplus` a Git submodule in your project's source. If not using
   Git, copy `sphereplusplus` inside your source tree.

2) Add `CXX` to the `project()` directive inside the `CMakeLists.txt`.

3) Add the following to the `CMakeLists.txt`:

```
# Setup use of C++.
add_compile_options(-std=c++14 -fno-exceptions -fno-non-call-exceptions -fno-rtti)

# Setup the Sphere++ library.
set(SPHERE_PLUS_PLUS_SOURCE
    sphereplusplus/abort.hh
    sphereplusplus/application.hh
    sphereplusplus/delegate.hh
    sphereplusplus/enums.hh
    sphereplusplus/gpio.hh
    sphereplusplus/sphereplusplus.cc
    sphereplusplus/std.hh
    sphereplusplus/timer.hh)
```

4) Add the `${SPHERE_PLUS_PLUS_SOURCE}` variable to the list of source files to
   build (in the `add_executable()` directive). Make sure that header files are
   reachable:

```
target_include_directories (${PROJECT_NAME} PRIVATE .)
```

5) Modify the application manifest to enable application capabilities for the
   features used by the application.
