#!/bin/bash

echo "Making all pistereocam."

make -B -C libs/print_OMX
make -B -C libs/socket_helper
make -B -C libs/rcam

make -B -C rcam_remote_slave

make -B -C stereo_cam

make -B -C camera

