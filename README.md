# pistereocam

This is an attempt to create a working (and usable) stereocam from 2 raspberry pi's.
Currently the it is only displaying a preview from both cameras.
I am currently working on a rcam api which will uses posix threads so I can control the remote camera.

The IP's are hard coded atm and the remote cam needs to be pointed to the stereocams IP address.
This will be changed.

Note I have had to change the settings of dhcpcd so the wifi is a higher priority to be able to use the internet.
And then using the dhcpcd handles I created a routing for 19.168.0.22 (the ip of the client camera) so it will select eth0.
this was put in the handle file 90
(will add better tutorial for this when camera is more operational)


The Makefiles are very raw at the moment 
currently you need to build each "lib" seperatly (and the ilclient lib in the /opt/vc/src/hello_pi/libs/ilclient)
and then build remote_cam and stereo_cam seperatly.
