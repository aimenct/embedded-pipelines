# Embedded Pipelines Framework - EPF

This repository contains a C++ generic library to build concurrent data processing pipelines for embedded applications in industrial environments.

Data Processing Pipelines are constructed by combining modular components called Filters. Each Filter performs a specific task on the data, they connect between each other to create a sequence of processing steps. Filters communicate through thread-safe Queues, ensuring efficient and synchronized data sharing without unnecessary data copying.

The library includes essential classes for building and managing pipelines (Queue, Filter, Pipeline), and a common information model facilitating structured data and configuration management. It comes with a set of pre-implemented filters such as camera, display, OPC-UA client and server, and with documentation and instructions for creating custom filters. 

The library is designed to be flexible and extensible. You can easily integrate it into your embedded pipelines applications to handle near real-time data processing with minimal overhead.

## How to build

### Compiling core

The library has been developed and tested in Ubuntu 20.04 and 22.04. To build the library's core you will need to install the following packages:

```bash
sudo apt install libpthread-stubs0-dev libyaml-cpp-dev freeglut3-dev build-essential cmake
```

Alternatively, to avoid cluttering your system with dependencies, the library provides a development image defined by `docker/Dockerfile.dev`. It contains the complete environment for developing the core library and its modules. See the [Docker installation instructions](https://docs.docker.com/engine/install/) and the recommended [Linux post-installation steps](https://docs.docker.com/engine/install/linux-postinstall/). The Compose configuration builds this development image automatically. To build it and start the container:

```bash
docker compose up -d --build
```

Once the dependencies are installed (or the docker container deployed), you can compile the library as a shared library using CMake:

```bash
cd epf
mkdir build
cd build  
cmake ..
make
```

The file *libepf.so* is created in build/src. By default the library only compiles the core. You can set different options (e.g., to activate additional modules) for the compilation of the library using CMake `-D` flags. Explore all the available options with:

```bash
cmake -L ..
```

### Framework modules

Apart from the core, EPF also provides different modules that enhance the functionality of the library. Every module adds additional dependencies that provide their functionalities.

| MODULE  | DESCRIPTION   | Dependencies   |
|---|---| ---|
| CAMERA  | Enabling to connect to GeniCam compliant cameras | [aravis 0.8](https://aravisproject.github.io/aravis/aravis-stable/building.html)|
| OPCUA  | Enabling client/server OPC-UA communication | [open62541 1.4](https://www.open62541.org/doc/v1.4.12/building.html) (recommended flags -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo -DUA_NAMESPACE_ZERO=REDUCED  -DUA_ENABLE_METHODCALLS=ON )|
| DOCS  | Documentation | Doxygen, Sphinx, Breathe |

To build these modules run from the build folder:

```bash
cmake -DENABLE_{MODULE}=ON ..
make
```

Options are saved in CMake cache, so they are saved between compilations and when changing other options. To return to initial state remove build folder and start again.

#### Using the library as git submodule

This option is more suitable when you are using a feature of EPF that is actively being developed, since you can easily pull new changes as soon as they are published. To follow this approach you just need to add the library as a git submodule of your project:

```bash
git submodule add <url>
```

Then you just need to add the submodule folder from the CMakeLists.txt main file:

```cmake
add_subdirectory(your_submodules_dir/epf)
```

From this point you can include the library in your sources:

```cpp
#include "core.h"
```

And link the library to your targets:

```cmake
target_link_libraries(
  target
  PUBLIC 
    epf
)
```

If your target uses an optional EPF module, also link the corresponding module target:

```cmake
target_link_libraries(
  target
  PUBLIC
    epf
    epf_camera
    epf_opcua
)
```

#### Installing the library

To install the library to the system directories or a custom directory use `make install`. You can specify the installation path by setting the `CMAKE_INSTALL_PREFIX` variable. You'll need to update your library path environment variable (`LD_LIBRARY_PATH` on Linux) to include this directory if you do not install in the default location.

```bash
cmake -DCMAKE_INSTALL_PREFIX=/path/to/install .. # optional for specific installation path
make install
```

This command will copy the header files of the selected options and the dynamic library file libepf.so to the system folders. In Ubuntu the header files will go to */usr/local/include/epf* and the library file to */usr/local/lib*.

#### Using the installed library

To use the installed library in other libraries or executables, include the headers of the library with the library name in front:

```cpp
#include <epf/core.h>
```

And add the following lines to your CMake file:

```cmake
find_package(epf 0.1.0 REQUIRED)

target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC
    epf::epf
)
```

If your project uses an optional EPF module, request it explicitly as a package component:

```cmake
find_package(epf 0.1.0 REQUIRED COMPONENTS camera opcua)

target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC
    epf::epf
    epf::epf_camera
    epf::epf_opcua
)
```

### Tests Compilation

Tests are excluded from the plain make execution. To compile the tests available for the modules activated just run from the build folder: 

```bash
make tests
```

### Examples Compilation

The library provides for some example pipelines (in the *examples* folder) to demonstrate the capabilities of the library. To compile them:

```bash
make examples
```

## License

This project is licensed under the terms of the Mozilla Public License v2.0. See the [LICENSE](./LICENSE) file for details.

This project includes code from the signals library by Fei Teng (https://github.com/TheWisp/signals/), which is licensed under the MIT License. 

