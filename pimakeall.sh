#!/bin/bash

echo "Making all pistereocam."

echo "*** print_OMX ***"
make -B -C libs/print_OMX
echo "*** socket_helper ***"
make -B -C libs/socket_helper
echo "*** rcam ***"
make -B -C libs/rcam

echo "\n***** rcam_remote_slave *****"
make -B -C rcam_remote_slave

echo "\n***** stereo_cam *****"
make -B -C stereo_cam

echo "\n***** camera *****"
make -B -C camera

echo "\n***** remote_cam *****"
make -B -C remote_cam
