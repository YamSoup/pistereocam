#!/bin/bash

echo "Making all pistereocam."

echo "*** print_OMX ***"
make -B -C libs/print_OMX
echo "*** socket_helper ***"
make -B -C libs/socket_helper
echo "*** rcam ***"
make -B -C libs/rcam

echo "***** rcam_remote_slave *****"
make -B -C rcam_remote_slave

echo "***** stereo_cam *****"
make -B -C stereo_cam

echo "***** camera *****"
make -B -C camera

