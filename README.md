# pistereocam

This is an attempt to create a working (and usable) stereocam from 2 raspberry pi's.
Currently the it is displaying a preview from both cameras.

******** SETUP
The 2 cameras are connected via a ethernet cable.
Both the IPs on the eth0 interface are set to static currently the static are hard coded in the camera software.
I will need to implenet a way of telling the program if another IP is wanted/needed.

I have also decided to use Wifi which has complicated things a bit



******* NETWORK SETUP

Firstly I have changed the hostnames of each machine

in the raspberry pi configeration change the hostname of the main pi (with the Wifi and screen)
to StereoCameraServer or ...

*** in /etc/hostname (Just the one line)

StereoCameraServer

*** in /etc/hosts

(keep the rest the same and edit this line)
127.0.1.1       StereoCameraServer


Next I had to set a static IP, this has changed a lot thanks to DHCPCD
I decided to embrace change and try and use the new method (which is good but badly documented).

*** in /etc/dhcpcd.conf

(add these lines to the bottom of the file keeping all the other options the same)
# Custom static IP for eth0 crossover cable, metrix is set higher to priorotize wan0
interface eth0
metric 2000
static ip_address=192.168.0.21/24
nogateway



*** next create a file /lib/dhcpcd/dhcpcd-hooks/40-myroutes (with just the 1 line)
route add 192.168.0.22



*** on the second pi just add this to the bottom of /etc/dhcpcd.conf
#THIS IS CURRENTLY NOT WORKING AND I DON'T KNOW WHY :S

interface eth0
static ip_address=192.168.0.22/24
nogateway




******** MAKEFILES 

The Makefiles are very raw at the moment 
currently you need to build each "lib" seperatly (and the ilclient lib in the /opt/vc/src/hello_pi/libs/ilclient)
and then build remote_cam and stereo_cam seperatly.
