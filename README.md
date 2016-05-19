# pistereocam

This is an attempt to create a working (and usable) stereocam from 2 raspberry pi's.
Currently the remote cam is comunicating with the stereocam but the stereocam does not display its own preview (next step).

The IP's are hard coded atm and the remote cam needs to be pointed to the stereocams IP address.
This will be changed.

The Makefiles are very raw at the moment 
currently you need to build each "lib" seperatly (and the ilclient lib in the /opt/vc/src/hello_pi/libs/ilclient)
and then build remote_cam and stereo_cam seperatly.
