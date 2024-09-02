# Embedded Pipelines Framework - EPF

This repository contains a C++ generic library to build concurrent data processing pipelines for embedded pipelines applications in industrial environments.

Data Processing Pipelines are constructed by combining modular components called Filters. Each Filter performs a specific task on the data, and filters can be connected to create a sequence of processing steps. Filters communicate through thread-safe Queues, ensuring efficient and synchronized data sharing without unnecessary data copying.

The library includes essential classes for building and managing pipelines (Queue, Filter, Pipeline), and a common information model facilitating structured data and configuration management. It comes with a set of pre-implemented filters such as camera, display, OPC-UA client and server, and with documentation and instructions for creating custom filters. 

The library is designed to be flexible and extensible. You can easily integrate it into your embedded pipelines applications to handle near real-time data processing with minimal overhead.

#### More information

More information on the project´s [wiki](http://gitlab.aimen.local/embedded-pipelines/embedded-pipelines/-/wikis/home).

## How to build

### Compiling core

The library has been developed and tested in Ubuntu 20.04 and 22.04. To build the library's core you will need to install the following packages:

```bash
sudo apt install libpthread-stubs0-dev libyaml-cpp-dev freeglut3-dev cmake
```

Alternatively, to avoid cluttering your system with dependencies, the library offers a `Dockerfile` which defines a docker image containing the whole environment of the library (core and its modules) ready for development/deployment. Instructions of how to install Docker [here](https://docs.docker.com/engine/install/) and recomended post-installation steps [here](https://docs.docker.com/engine/install/linux-postinstall/). To build the image and run the container:

```bash
sh build-docker.sh
```

Once the dependencies are installed (or the docker container deployed), you can compile the library as a shared library using CMake:

```bash
cd embedded-pipelines
mkdir build
cd build  
cmake ..
make
```

The file *libepf.so* is created in build/src. By default the library only compiles the core. You can set different options (e.g. to activate additional modules) for the compilation of the library using cmake `-D` flag. Explore all the available options with:

```bash
cmake -L ..
```

### Framework modules

Apart from the core, *epp* also provides for different modules that enhance the functionality of the library. Every module add additional dependencies that provide the functionalities.

| MODULE  | DESCRIPTION   | Dependencies   |
|---|---| ---|
| CAMERA  | Enabling to connect to GeniCam compliant cameras | [aravis 0.8](https://aravisproject.github.io/aravis/aravis-stable/building.html)|
| OPCUA  | Enabling client/server OPC-UA communication | [open62541 1.4](https://www.open62541.org/doc/v1.4.1/building.html) |
| RTSP  | Enabling the RTSP video streaming   | [gstreamer 1.X](https://gstreamer.freedesktop.org/documentation/installing/on-linux.html?gi-language=c) |
| SOCKETS  | Enabling communication through raw sockets  | |
| MODBUS  |  Enabling MODBUS communication | libmodbus |
| DOCS  | Documentation | Doxygen, Sphinx, Breathe |

To build this modules run from the build folder:

```bash
cmake -DENABLE_{MODULE}=ON ..
make
```

Options are saved in CMake cache, so they are saved between compilations and when changing other options. To return to initial state remove build folder and start again.

#### System install

To install the library execute the following command:

```bash
sudo make install
```

This command will copy the header files of the selected options and the dynamic library file libepp.so to the system folders. In Ubuntu the header files will go to */usr/local/include/epf* and the library file to */usr/local/lib*.

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


# Using the installed library

To use the library in other libraries or executables just include the headers of the library with the library name in front:

```cpp
#include <ep/core.h>
```

And add the following lines to your CMake file:

```cmake
find_package(ep 0.2.0 REQUIRED)

target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC
    ep::ep
)
```


## License

This project is licensed under the terms of the Mozilla Public License v2.0. See the [LICENSE](./LICENSE) file for details.

This project includes code from the signals library by Fei Teng (https://github.com/TheWisp/signals/), which is licensed under the MIT License. 


