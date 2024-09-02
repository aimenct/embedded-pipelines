#!/bin/bash

# Aravis 0.8 
function build_aravis8 {
  function is_installed {
       # Detect installation
    if [ "$(ls /usr/local/lib/* | grep libaravis)" != "" ] && \
       [ -d "/usr/local/include/aravis-0.8" ]
    then return $(true)
    else return $(false)
    fi
  }

  if is_installed; then
    echo -e "\n>> Aravis0.8 is already installed."
  else
    echo -e "\n>> Installing Aravis0.8"

    retry sudo apt-get install -y python3 pip
    sudo pip3 install meson
    retry sudo apt-get install -y ninja-build libglib2.0-dev libxml2-dev zlib1g-dev

    cd $SOURCE

    git clone --depth 1 --branch 0.8.26 https://github.com/AravisProject/aravis.git
    
    cd aravis 

    meson build 
    cd build 
    ninja 
    ninja install 
    sudo cp ../src/aravis.rules /etc/udev/rules.d/ &&\
    sudo ldconfig
  fi

  if ! is_installed; then
    ERRORS="$ERRORS\nERROR: Failed to install Aravis0.8"
  fi

  return $(is_installed)
}

# VTK for opencv viz
function build_vtk {
  function is_installed {
    if [ "$(ls /usr/local/lib | grep libvtk)" != "" ] || \
    [ "$(dpkg -l | grep libvtk)" != "" ]
    then return $(true)
    else return $(false)
    fi
  }

  if is_installed; then
    echo -e "\n>> VTK is already installed."
  else
    echo -e "\n>> Building VTK from source."

    retry sudo apt-get install -y git libxt-dev build-essential cmake mesa-common-dev mesa-utils freeglut3-dev
    retry sudo apt-get install -y qt5-default

    cd "$SOURCE"
    git clone -b v9.2.6 --recursive https://gitlab.kitware.com/vtk/vtk.git

    cd vtk
    mkdir -p build
    cd build

    cmakecmd="cmake \
    -D BUILD_SHARED_LIBS=ON \
    -D BUILD_TESTING=ON \
    -D CMAKE_BUILD_TYPE=Release \
    -D CMAKE_INSTALL_PREFIX=/usr/local \
    -D QT_QMAKE_EXECUTABLE=/usr/local/bin/qmake .."
    
    echo \n$cmakecmd
    $cmakecmd

    make -j$(nproc)

    sudo make install
  fi

  if ! is_installed; then
    ERRORS="$ERRORS\nERROR: Failed to install VTK."
  fi

  return $(is_installed)
}

# OpenCV
function install_opencv {
  function is_installed {
    if [ "$(ls /usr/local/lib | grep opencv)" != "" ] || \
    [ "$(dpkg -l | grep libopencv-dev)" != "" ]
    then return $(true)
    else return $(false)
    fi
  }
  if is_installed; then
    echo -e "\n>> OpenCV is already installed."
  else
    echo -e "\n>> Installing OpenCV."
    retry sudo apt-get install -y libopencv-dev
  fi

  if ! is_installed; then
    ERRORS="$ERRORS\nERROR: Failed to install OpenCV."
  fi

  return $(is_installed)
}

# OpenCV (laser-img-processing dependency)
function build_opencv {
  function is_installed {
    if [ "$(ls /usr/local/lib | grep opencv)" != "" ] || \
    [ "$(dpkg -l | grep libopencv-dev)" != "" ]
    then return $(true)
    else return $(false)
    fi
  }

  if is_installed; then
    echo -e "\n>> OpenCV is already installed."
  else
    echo -e "\n>> Building OpenCV from source."

    retry sudo apt install -y build-essential cmake git pkg-config \
      libgtk-3-dev libavcodec-dev libavformat-dev libswscale-dev libv4l-dev \
      libxvidcore-dev libx264-dev libjpeg-dev libpng-dev libtiff-dev \
      gfortran openexr libatlas-base-dev python3-dev python3-numpy \
      libtbb2 libtbb-dev libdc1394-22-dev libopenexr-dev \
      libgstreamer-plugins-base1.0-dev libgstreamer1.0-dev

    cd "$SOURCE"
    mkdir opencv_build 
    cd opencv_build
    git clone -b 4.5.1 https://github.com/opencv/opencv.git
    git clone -b 4.5.1 https://github.com/opencv/opencv_contrib.git

    cd opencv
    mkdir -p build 
    cd build

    cmakecmd="cmake \
    -D CMAKE_BUILD_TYPE=RELEASE \
    -D CMAKE_INSTALL_PREFIX=/usr/local \
    -D INSTALL_C_EXAMPLES=OFF \
    -D INSTALL_PYTHON_EXAMPLES=OFF \
    -D OPENCV_GENERATE_PKGCONFIG=ON \
    -D OPENCV_EXTRA_MODULES_PATH=$HOME/sourcePackages/opencv_build/opencv_contrib/modules \
    -D WITH_VTK=ON \
    -D BUILD_EXAMPLES=OFF .."
    
    echo \n$cmakecmd
    $cmakecmd

    make -j$(nproc)

    sudo make install
  fi

  if ! is_installed; then
    ERRORS="$ERRORS\nERROR: Failed to install OpenCV."
  fi

  return $(is_installed)
}

# GStreamer
function install_gstreamer {
  function is_installed {
    if [ "$(ls /usr/local/lib | grep gstreamer)" != "" ] || \
    [ "$(ls /usr/local/lib/$(uname -i)-linux-gnu | grep gstreamer)" != "" ] || \
    [ "$(dpkg -l | grep libopencv-dev)" != "" ]; then
      return $(true)
    else 
      return $(false)
    fi
  }

  

  PACKAGES="libgstreamer1.0-0 libgstrtspserver-1.0-dev"

  if is_installed; then
    echo -e "\n>> GStreamer is already installed."
  else
    echo -e "\n>> Installing GSTreamer packages."
    retry sudo apt-get install -q -y $PACKAGES  
  fi


  if [ "$is_installed" = false ]; then
    ERRORS="$ERRORS\nERROR: Failed to install GStreamer."
  fi

  return $(is_installed)
}

# YAML-cpp
function build_yamlcpp {
  function is_installed {
    if [ "$(ls /usr/local/lib | grep yaml-cpp)" != "" ] || \
    [ "$(dpkg -l | grep yaml-cpp)" != "" ]
    then return $(true)
    else return $(false)
    fi
  }

  if is_installed; then
    echo -e "\n>> yaml-cpp is already installed."
  else
    echo -e "\n>> Installing yaml-cpp."
    cd "$SOURCE"
    git clone -b yaml-cpp-0.7.0 https://github.com/jbeder/yaml-cpp.git
    cd yaml-cpp
    mkdir build
    cd build

    cmakecmd="cmake \
    -D YAML_CPP_BUILD_TESTS=OFF \
    -D YAML_BUILD_SHARED_LIBS=ON .."

    echo \n$cmakecmd
    $cmakecmd

    sudo make install
  fi

  if ! is_installed; then
    ERRORS="$ERRORS\nERROR: Failed to install yaml-cpp."
  fi

  return $(is_installed)
}


# YAML-cpp
function install_yamlcpp {
  function is_installed {
    if [ "$(ls /usr/local/lib | grep yaml-cpp)" != "" ] || \
    [ "$(dpkg -l | grep yaml-cpp)" != "" ]
    then return $(true)
    else return $(false)
    fi
  }

  if is_installed; then
    echo -e "\n>> yaml-cpp is already installed."
  else
    echo -e "\n>> Installing yaml-cpp."
    sudo apt install -y libyaml-cpp-dev
  fi

  if ! is_installed; then
    ERRORS="$ERRORS\nERROR: Failed to install yaml-cpp."
  fi

  return $(is_installed)
}

# Libmodbus
function build_libmodbus {
  function is_installed {
    if [ "$(ls /usr/local/lib | grep libmodbus)" != "" ] || \
    [ "$(dpkg -l | grep libmodbus)" != "" ]
    then return $(true)
    else return $(false)
    fi
  }

  if is_installed; then
    echo -e "\n>> libmodbus is already installed."
  else
    echo -e "\n>> Installing libmodbus."
    cd "$SOURCE"
    retry sudo apt-get install -y automake autoconf libtool
    git clone -b v3.1.10 https://github.com/stephane/libmodbus.git
    cd libmodbus
    ./autogen.sh
    ./configure
    sudo make install
  fi

  if ! is_installed; then
    ERRORS="$ERRORS\nERROR: Failed to install libmodbus."
  fi

  return $(is_installed)
}

function install_libmodbus {
  function is_installed {
    if [ "$(ls /usr/local/lib | grep libmodbus)" != "" ] || \
    [ "$(dpkg -l | grep libmodbus)" != "" ]
    then return $(true)
    else return $(false)
    fi
  }

  if is_installed; then
    echo -e "\n>> libmodbus is already installed."
  else
    echo -e "\n>> Installing libmodbus."

    sudo apt install -y libmodbus-dev

  fi

  if ! is_installed; then
    ERRORS="$ERRORS\nERROR: Failed to install libmodbus."
  fi

  return $(is_installed)
}


function build_cmake {
  function is_installed {
    if [ "$(ls /usr/bin/ | grep cmake)" != "" ] || \
    [ "$(ls /usr/local/bin/ | grep cmake)" != "" ] || \
    [ "$(dpkg -l | grep cmake)" != "" ]
    then return $(true)
    else return $(false)
    fi
  }

  if is_installed; then
    echo -e "\n>> CMake is already installed."
  else
    echo -e "\n>> Installing CMake."
    cd "$SOURCE"
    
    retry sudo apt-get install -y automake autoconf libtool

    CMAKE_LINK="https://github.com/Kitware/CMake/releases/download/v3.26.4/cmake-3.26.4-linux"
    VERSION=""

    if [ "$(uname -i)" == "arm*" ]; then
      VERSION="-aarch64.sh"
    else
      VERSION="-x86_64.sh"
    fi
    wget "$CMAKE_LINK$VERSION" && \
    chmod +x cmake-3.26.4-linux-aarch64.sh && \
    sudo ./cmake-3.26.4-linux-aarch64.sh --prefix=/usr/local --exclude-subdir --skip-license
  fi

  if ! is_installed; then
    ERRORS="$ERRORS\nERROR: Failed to install CMake."
  fi

  return $(is_installed)
}

function install_cmake {
  function is_installed {
    if [ "$(ls /usr/bin/ | grep cmake)" != "" ] || \
    [ "$(ls /usr/local/bin/ | grep cmake)" != "" ] || \
    [ "$(dpkg -l | grep cmake)" != "" ]
    then return $(true)
    else return $(false)
    fi
  }

  if is_installed; then
    echo -e "\n>> CMake is already installed."
  else
    retry sudo apt-get update
    retry sudo apt-get install -y cmake
  fi

  if ! is_installed; then
    ERRORS="$ERRORS\nERROR: Failed to install CMake."
  fi

  return $(is_installed)
}

# Open62541 OPCUA library
function install_open62541 {
  function is_installed {
    if [ "$(ls /usr/local/lib | grep open62541)" != "" ] || \
    [ "$(dpkg -l | grep open62541)" != "" ]
    then return $(true)
    else return $(false)
    fi
  }

  if is_installed; then
    echo -e "\n>> open62541 is already installed."
  else
    echo -e "\n>> Installing open62541."
    cd "$SOURCE"
    retry sudo add-apt-repository -y ppa:open62541-team/ppa
    retry sudo apt-get update
    retry sudo apt-get install -y libopen62541-1-dev
  fi

  if ! is_installed; then
    ERRORS="$ERRORS\nERROR: Failed to install open62541."
  fi

  return $(is_installed)
}

# Open62541 OPCUA library
function build_open62541 {
  function is_installed {
    if [ "$(ls /usr/local/lib | grep open62541)" != "" ] || \
    [ "$(dpkg -l | grep open62541)" != "" ]
    then return $(true)
    else return $(false)
    fi
  }

  if is_installed; then
    echo -e "\n>> open62541 is already installed."
  else
    echo -e "\n>> Installing open62541 from source."

    cd "$SOURCE"

    git clone -b v1.3.6 https://github.com/open62541/open62541.git
    
    cd open62541
    git submodule update --init --recursive

    mkdir build
    cd build

    cmakecmd="cmake \
    -D BUILD_SHARED_LIBS=ON \
    -D CMAKE_BUILD_TYPE=RelWithDebInfo \
    -D UA_NAMESPACE_ZERO=FULL .."
    # -D UA_ENABLE_PUBSUB=ON \
    # -D BUILD_EXAMPLES=OFF \

    echo \n$cmakecmd
    $cmakecmd

    make -j$(nproc)
    sudo make install
  fi

  if ! is_installed; then
    ERRORS="$ERRORS\nERROR: Failed to install open62541."
  fi

  return $(is_installed)
}

# HDF5
function install_hdf5 {
  function is_installed {
    if [ "$(ls /usr/local/lib | grep hdf5)" != "" ] || \
    [ "$(dpkg -l | grep libhdf5-dev)" != "" ]
    then return $(true)
    else return $(false)
    fi
  }
  if is_installed; then
    echo -e "\n>> HDF5 is already installed."
  else
    echo -e "\n>> Installing HDF5."
    retry sudo apt-get install -y libhdf5-dev
  fi

  if ! is_installed; then
    ERRORS="$ERRORS\nERROR: Failed to install HDF5."
  fi

  return $(is_installed)
}
### Utility functions #########################################################

function retry {
  # Keep trying a command until it succeeds
  # Useful for locking when another instance of apt is running
  counter=1
  while [ $counter -le 10 ]
  do
    $@
    if [ $? -ne 0 ]; then
      sleep 1
    else
      return
    fi
    ((counter++))
  done
}

function startsudo {
  # Keep validating sudo credentials to extend the default timeout
  sudo -v
  ( while true; do sudo -v; sleep 50; done; ) &
  SUDO_PID="$!"
  trap stopsudo SIGINT SIGTERM
}
function stopsudo {
  kill "$SUDO_PID" 2>/dev/null
  trap - SIGINT SIGTERM
}

### Main script ###############################################################

# Change working directory to this file
EMBEDDED_PIPELINES_DIR=$(cd "$(dirname "$0")" >/dev/null 2>&1 && pwd)

cd $HOME
mkdir dependencies-sources
SOURCE="$HOME/dependencies-sources"

unset ERRORS
unset WARNINGS

echo -e "\n>> Getting sudo privileges."
startsudo

# Check Ubuntu version
if [ "$(lsb_release -a 2>&1 | grep 22.)" != "" ] || \
   [ "$(lsb_release -a 2>&1 | grep 20.)" != "" ] || \
   [ "$(lsb_release -a 2>&1 | grep 18.)" != "" ] || \
   [ "$(echo $@ | grep forceinstall)" != "" ]; then
  echo -e "\n>> Installing general dependencies."
  sudo rm -rf /var/lib/apt/lists/*
  sudo apt-get clean
  sudo apt-get update
  # sudo apt-get update -o Acquire::http::No-Cache=true -o Acquire::http::Pipeline-Depth=0

  retry sudo apt-get install -y git build-essential software-properties-common \
  libboost-filesystem-dev libpthread-stubs0-dev libusb-1.0-0-dev librtlsdr-dev curl \
  python3-dev udev wget
  

  

  if [ "$(lsb_release -a 2>&1 | grep 18.)" != "" ] || \
     [ "$(echo $@ | grep build-cmake)" != "" ]; then
    build_cmake
  else
    install_cmake
  fi

  if [ "$(echo $@ | grep build-opencv)" != "" ]; then
    if [ "$(echo $@ | grep withopencvviz)" != "" ]; then
      build_vtk
    fi
    build_opencv
  else
    install_opencv
  fi

  if [ "$(echo $@ | grep build-yamlcpp)" != "" ]; then
    build_yamlcpp
  else
    install_yamlcpp
  fi

  if [ "$(echo $@ | grep build-libmodbus)" != "" ]; then
    build_libmodbus
  else
    install_libmodbus
  fi

  
  if [ "$(echo $@ | grep build-open62541)" != "" ]; then
    build_open62541
  else
    if [ "$(lsb_release -a 2>&1 | grep 22.)" != "" ]; then
      build_open62541
    else
      install_open62541
    fi    
  fi

  install_gstreamer
  install_hdf5
  build_aravis8



else
  ERRORS="$ERRORS\nERROR: Automatic install not supported for this Ubuntu version."
fi

cd $EMBEDDED_PIPELINES_DIR

echo -ne "\n>> Installation script completed."
if [ ! -z "$WARNINGS" ]; then
  echo -e "$WARNINGS"
fi
if [ -z "$ERRORS" ]; then
  echo -e "\n>> Dependencies installed succesfully."
  EXIT_CODE=0
else
  echo -e "$ERRORS"
  EXIT_CODE=1
fi

stopsudo
exit $EXIT_CODE
