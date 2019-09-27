#!/bin/bash

mkdir build/
source /opt/fsl-imx-internal-xwayland/4.19-thud/environment-setup-aarch64-poky-linux
cd build

cmake \
              -DCMAKE_BUILD_TYPE=Release	\
 	      -DENABLE_PYRENDERDOC=OFF		\
              -DENABLE_QRENDERDOC=OFF		\
	      -DENABLE_RENDERDOCCMD=ON		\
              -DENABLE_XLIB=OFF			\
              -DENABLE_XCB=OFF			\
              -DENABLE_GL=OFF			\
	      -DENABLE_GLES=OFF			\
	      -DENABLE_VULKAN=ON		\
	      -DENABLE_WAYLAND=ON		\
	      -DUSE_INTERCEPTOR_LIB=OFF		\
	      -Bbuild -H.			\
              -DCMAKE_TOOLCHAIN_FILE=/opt/fsl-imx-internal-xwayland/4.19-thud/sysroots/x86_64-pokysdk-linux/usr/share/cmake/OEToolchainConfig.cmake \
..

make -j 4
