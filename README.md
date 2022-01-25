# masterbus-esp32
Reverse engineering of the masterbus protocol used by mastervolt products. 

# Green Mastervolt cable pinout

1-Can High
2-Can Low
3-GND
4-+12V
5-+12V
6-GND
7-NC
8-NC

Green: Ground

Green/White: Ground

Blue	: +12 volts

Blue/White : +12 Volts

2- Red	: CAN L

1- Red/White: CAN H

Brown: Not connected

Brown/White: Not connected


# CANBus configuration

Masterbus is based on standard canbus. The bus speed is 250000. 

`sudo ip link set down can0`

`sudo ip link set can0 type can bitrate 250000 restart-ms 100`

`sudo ip link set up can0`

`sudo ip -details -statistics link show can0`



# Protocol overview 

The protocol is inspired by a request/response scheme. When a masterview device starts up, it broadcasts a message that requires connected devices to tell the masterview what attributes they offer. Then, the masterview will send a request to each networked device to request the value of a particular attribute. The value is transmitted in clear, as a IEEE734 float. 


#CAN Packet format

Values are sent over the wire encoded as floats. Labels for units such as "A" for Amps, "V" for volts, etc.. are also sent over the wire. Packet length is determined by the canId and attribute type. 

#Devices

The masscombi and DCShunt are the device supplying power power to the bus. The masterview starts talking on the bus later because it needs time to boot up.

#Groups & Items
Each device has a number of "items" that can be monitored. For example the DCShunt has a voltage item. On the masterview, when you configure the favorites page, you enter the item number you want to see in each area. Item numbers are device dependent, and are reused for different purpose across different devices.
There is probably some sort of lookup done to resolve the group&Item to a single number.

      For the masscombi:
      Group1 + Item1 = 16 = 0x10 --> State Inverting
      Group1 + Item2 = 60 = 0x3C
      Group1 + Item3 = 56 = 0x38
      Group1 + Item4 = 20 = 0x14
      Group1 + Item5 = 58 = 0x3A
      Group1 + Item6 = 19 = 0x13
      Group1 + Item7 = 14 = 0x0E
      Group1 + Item8 = 17 = 0x11
      Group1 + Item9 = -- = --
      
      Group2 + Item1 = 18 = 0x12
      Group2 + Item2 =  6 = 0x06
      Group2 + Item3 =  7 = 0x07
      Group2 + Item4 =  -- = --
      
      Group3 + Item1 =  8 = 0x08
      Group3 + Item2 =  9 = 0x09
      Group3 + Item3 = -- = --
      
      Group4 + Item1 = 10 = 0x0A --> AC Output 120V (0x0A)
      Group4 + Item2 = 11 = 0x0B
      Group4 + Item3 = -- = --
      
      Group5 + Item1 = -- = --
      
      For the DC Shunt:
      Group1 + Item1 = 0 = 0x00
      Group1 + Item2 = 4 = 0x04
      Group1 + Item3 = 3 = 0x03
      Group1 + Item4 = 1 = 0x01


## Masterview display device
Device Id is 0x0194 or 0x840a

This device will query for other devices to announce themselves upon startup. It will then start querying regularly for certain field values from selected devices. The fields selected reflect what is required to be displayed on the screen.

How can I use the wireshark dissector?
----
You can start wireshark with the following flags to enable the masterbus dissector. The sample capture files will work with this dissector. However, the packet format is not of the canbus format. In canbus, nothing is byte aligned and I do not want to spend the work doing it.

`wireshark -Xlua_script:masterbusDissector.lua`

How can I capture my own wireshark capture file?
---
`tshark -i can0 -w ~/my_capture_file.pcap`

How can I decode more messages?
---
Please modify the wireshark dissector first. It is better to add information in the dissector, you can iterate faster. Also, other projects might find it a usefull starting point.

TODO
---

* Add a filter in the MCP2515 to filter out other messages than the ones I am interrested in. Will reduce processing time, power, dropped messages.
* Add functions to craft a value request packet. Emulate the masterview.
* Move the GPIO configuration to sdkconfig file
* When we get disconnected from the Wifi, the BLE monitoring does not restart automatically. 
* Possible memory leak

Issue
---

