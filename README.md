################################################################################
#Copyright (C) 2023  Clounix, Inc.
#This program is free software; you can redistribute it and/or
#modify it under the terms of version 2 of the GNU General Public
#License as published by the Free Software Foundation.
#This program is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.
#You should have received a copy of the GNU General Public License
#version 2 along with this program.
################################################################################

#clounix-modules
Device drivers for support of Clounix platform

Step 1~4 show how to build and execute CLX Kernel modules.

1. Modify modules/Makefile to specify the output directory to BUILD_OUTPUT_DIR.
   The default output path is modules/build.

2. Compile:
   cd modules/ && make

3. The output kernel modules will be found in $(BUILD_OUTPUT_DIR)/module/
   - clx_dev.ko
   - clx_netif.ko

4. Load modules:
   (1) insmod clx_dev.ko
   (2) insmod clx_netif.ko

   Note that the module inserting sequence cannot be changed.
