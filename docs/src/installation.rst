Installation
#############

Pre-requisites
--------------

The library has been developed and tested on Linux Ubuntu 20.04 and 22.04.
Main pre-requisites are:

- **Compiler**: C++17 or later.
- **CMake**: Version 3.13.0 or later.
- **POSIX Pthreads**: For threading support.
- **FreeGlut**: For image diplay.
- **Yaml-cpp**: Yaml parser and emitter.

You can install the necessary packages on these systems using: 

.. code-block:: bash

   sudo apt install libpthread-stubs0-dev libyaml-cpp-dev freeglut3-dev cmake

Apart from the core, *epp* also provides various modules that enhance the functionality of the library. Each module adds additional dependencies to provide its functionalities.

.. list-table:: Module Overview
   :header-rows: 1

   * - MODULE
     - DESCRIPTION
     - Dependencies
   * - CORE
     - Core EPP Library
     - yaml-cpp, pthreads, freeglut3
   * - CAMERA
     - Enabling connection to GeniCam compliant cameras
     - `aravis 0.8 <https://aravisproject.github.io/aravis/aravis-stable/building.html>`_
   * - OPCUA
     - Enabling client/server OPC-UA communication
     - `open62541 1.4 <https://www.open62541.org/doc/v1.4.1/building.html>`_
   * - RTSP
     - Enabling RTSP video streaming
     - `gstreamer 1.X <https://gstreamer.freedesktop.org/documentation/installing/on-linux.html?gi-language=c>`_
   * - SOCKETS
     - Enabling communication through raw sockets
     - 
   * - MODBUS
     - Enabling MODBUS communication
     - libmodbus
   * - DOCS
     - Documentation
     - Doxygen, Sphinx, Breathe
       



Installing Dependencies
^^^^^^^^^^^^^^^^^^^^^^^
Below are the instructions to manually install the dependencies for each module on Ubuntu.

**CAMERA Module**

 To install Aravis and its necessary dependencies, follow these steps:
 
 1.  First, update `pip` to the latest version and then install `meson` and `ninja` using `pip`:
 
    .. code-block:: bash
    
       pip install -U pip
       pip install meson ninja
      
 2. Install aravis dependencies
 
    .. code-block:: bash

       sudo apt-get update
       sudo apt-get install -y --no-install-recommends \
          zlib1g-dev \
          libcanberra-gtk-module \
          libcanberra-gtk3-module \
          gstreamer1.0-gtk3 \
          libgtk-3-dev \
          usbutils \
          ninja-build \
          udev \
          gettext
          
 3. Install `aravis 0.8`

  .. code-block:: bash

     sudo apt install meson ninja-build
     git clone --depth 1 --branch 0.8.26 https://github.com/AravisProject/aravis.git
     cd aravis
     meson build
     cd build
     ninja
     sudo ninja install
     cp ../src/aravis.rules /etc/udev/rules.d/
     ldconfig
     
**OPCUA Module**

 1. Install open62541 dependencies
 
  .. code-block:: bash
     
     sudo apt-get install git build-essential gcc pkg-config cmake python3
     
 2. Install `open62541 1.4.1`:
 
  .. code-block:: bash

     git clone --branch v1.4.1 https://github.com/open62541/open62541.git
     cd open62541
     git submodule update --init --recursive
     mkdir build && cd build
     cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo -DUA_NAMESPACE_ZERO=FULL \
     -DUA_ENABLE_METHODCALLS=ON -DUA_ENABLE_NODEMANAGEMENT=ON -DUA_ENABLE_NODESETLOADER=ON ..
     make
     sudo make install
     
**RTSP Module**

  To install `gstreamer 1.X`:

  .. code-block:: bash

     sudo apt install libgstreamer1.0-0 libgstrtspserver-1.0-dev
     
**MODBUS Module**

  To install `libmodbus`:

  .. code-block:: bash

     sudo apt install libmodbus-dev
     
**DOCS Module**

  To install `Doxygen`, `Sphinx` and `Breathe`:
  
  .. code-block:: bash
     
     sudo apt install doxygen
     sudo pip install sphinx breathe
 
 

Dockerfile
^^^^^^^^^^

Alternatively, to avoid cluttering your system with dependencies, the library offers a ``Dockerfile`` which defines a Docker image containing the entire environment of the library (core and its modules) ready for development and deployment. Instructions on how to install Docker can be found `here <https://docs.docker.com/engine/install/>`_ and recommended post-installation steps `here <https://docs.docker.com/engine/install/linux-postinstall/>`_. To build the image and run the container:

.. code-block:: bash

   sh build-docker.sh
   

Build and Install
-----------------

Once the dependencies are installed (or the Docker container is deployed), you can compile the library as a shared library using CMake:

.. code-block:: bash

   cd embedded-pipelines
   mkdir build
   cd build  
   cmake ..
   make

The file *libembedded-pipelines.so* will be created in the ``build/src`` directory.

By default, the build process only compiles the core libary. To compile additional modules, enable them using CMake options. To build these modules run the following commands from the build folder:

.. code-block:: bash

   cmake -DENABLE_{MODULE}=ON .. 
   make

Replace {MODULE} with the name of the module you wish to enable (e.g., CAMERA, OPC-UA). Explore the available options with:

.. code-block:: bash

   cmake -L ..

To install the library to the system directories or a custom directory use `make install`. You can specify the installation path by setting the `CMAKE_INSTALL_PREFIX` variable. You'll need to update your library path environment variable (`LD_LIBRARY_PATH` on Linux) to include this directory if you do not install in the default location.

.. code-block:: bash
		
   cmake -DCMAKE_INSTALL_PREFIX=/path/to/install .. # optional for specific installation path
   make install

This command will copy the header files of the selected options and the dynamic library file libepp.so to the system folders. In Ubuntu the header files will go to */usr/local/include/epp* and the library file to */usr/local/lib*.

Tests are excluded from the plain make execution. To compile the tests available for the modules activated just run from the build folder: 

.. code-block:: bash

   make tests

The library provides for some example pipelines (in the *examples* folder) to demonstrate the capabilities of the library. To compile them:

.. code-block:: bash

   make examples

Usage
-----

Building with CMake
^^^^^^^^^^^^^^^^^^^

For projects using CMake, here's an example `CMakeLists.txt` file that sets up the EPPF library and `yaml-cpp`. Update the paths and filenames as necessary for your project:

To use the library in other libraries or executables just include the headers of the library with the library name in front:

.. code-block:: cpp

   #include <embedded-pipelines/core.h>


And add the following lines to your CMake file:

.. code-block:: cmake
		
   find_package(epp 0.2.0 REQUIRED)

   target_link_libraries(
   ${PROJECT_NAME}
   PUBLIC
   embedded-pipelines::embedded-pipelines
   )

