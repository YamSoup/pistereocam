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

*** in /etc/hostname (Just the one line)

StereoCameraServer

*** in /etc/hosts

127.0.0.1       localhost
::1             localhost ip6-localhost ip6-loopback
ff02::1         ip6-allnodes
ff02::2         ip6-allrouters

127.0.1.1       StereoCameraServer


Next I had to set a static IP, this has changed a lot thanks to DHCPCD
I decided to embrace change and try and use the new method (which is good but badly doicumented).

*** in /etc/dhcpcd.conf

# A sample configuration for dhcpcd.
# See dhcpcd.conf(5) for details.

# Allow users of this group to interact with dhcpcd via the control socket.
#controlgroup wheel

# Inform the DHCP server of our hostname for DDNS.
hostname

# Use the hardware address of the interface for the Client ID.
clientid
# or
# Use the same DUID + IAID as set in DHCPv6 for DHCPv4 ClientID as per RFC4361.
#duid

# Persist interface configuration when dhcpcd exits.
persistent

# Rapid commit support.
# Safe to enable by default because it requires the equivalent option set
# on the server to actually work.
option rapid_commit

# A list of options to request from the DHCP server.
option domain_name_servers, domain_name, domain_search, host_name
option classless_static_routes
# Most distributions have NTP support.
option ntp_servers
# Respect the network MTU.
# Some interface drivers reset when changing the MTU so disabled by default.
#option interface_mtu

# A ServerID is required by RFC2131.
require dhcp_server_identifier

# Generate Stable Private IPv6 Addresses instead of hardware based ones
slaac private

# A hook script is provided to lookup the hostname if not set by the DHCP
# server, but it should not be run by default.
nohook lookup-hostname

# Custom static IP for eth0 metrix is set higher to priorotize wan0
interface eth0
metric 2000
static ip_address=192.168.0.21/24
nogateway

*** next create a file /lib/dhcpcd/dhcpcd-hooks/40-myroutes (with just the 1 line)
route add 192.168.0.22



******** MAKEFILES 

The Makefiles are very raw at the moment 
currently you need to build each "lib" seperatly (and the ilclient lib in the /opt/vc/src/hello_pi/libs/ilclient)
and then build remote_cam and stereo_cam seperatly.
