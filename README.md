# urtwm
FreeBSD driver for RTL8812AU/RTL8821AU

### **How-to-build:**

1) Clone / download this repository.

2) Apply 'patch-usbdevs.diff' to your source tree checkout; e.g.

  *cd /usr/src/ && svn patch <patch-usbdevs.diff location>*

3) Build and install firmware:

   *cd <repository location>/sys/modules/urtwmfw && make && make install*

4) Build and install driver:

   *cd ../urtwm && make && make install*
   
   
   
### **How-to-test:**

1) Load the driver:

   *kldload if_urtwm*
   
In case if device was recognized successfully driver will report about that:

> urtwm0: <802.11n WLAN Adapter> on usbus4

> urtwm0: MAC/BB RTL8821AU, RF 6052 1T1R

or

> urtwm0: MAC/BB RTL8812AU, RF 6052 2T2R

2) Load necessary modules (if not already loaded):

   *kldload wlan_amrr wlan_ccmp wlan_tkip wlan_wep*
   
3) Create wlan(4) interface:

   *ifconfig wlan1 create wlandev urtwm0*
   
3) Start wpa_supplicant(8):

   wpa_supplicant -i wlan1 -c /etc/wpa_supplicant.conf
   
4) Start dhclient(8) after association / 4-Way handshake:

   *dhclient wlan1*
