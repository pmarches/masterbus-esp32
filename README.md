# masterbus-esp32
Reverse engineering of the masterbus protocol used by mastervolt products. 

# Green Mastervolt cable pinout


Green: Ground

Green/White: Ground

Blue	: +12 volts

Blue/White : +12 Volts

Red	: CAN L

Red/White: CAN H

Brown: Not connected

Brown/White: Not connected


# Hardware interface

Masterbus is based on standard canbus. 

# Protocol overview 

The protocol is inspired by a request/response scheme. When a masterview device starts up, it broadcasts a packet that requires connected devices to tell the masterview what attributes they offer. Then, the masterview will send a request to each networked device to request the value of a particular attribute. The value is transmitted in clear, as a IEEE734 float. 

# CANId

Each device on the bus had a different canbus Id. Devices of the same type have different canids. When the first bit of the can id is set, this indicates a request for an attribute's value. The CanIds of identical devies share some bits, but are unique. The numbers do not relate to the serial numbers printed on the devices. 

#CAN Packet format

Values are sent over the wire encoded as floats. Labels for units such as "A" for Amps, "V" for volts, etc.. are also sent over the wire. Packet length is determined by the canId and attribute type. 

#Devices

## Mastervolt DC Shunt labels
CANId is 0x019b

## Mastervolt DC Shunt field values
CANId is 0x021b

|000 | Percentage full|
|001 | Volts |
|002 | Battery Amps flow|
|003 | Battery Amps consumed|
|004 | ??????? |
|005 | Battery temperature in celcius|
|009 | Current Time (On 24/08/2021, at 04:50:00 local time, I saw the value 17400.0)|
|00A | Maybe the date -OR- seomthing to do with configuration |

## Labels of the masscombi
CanId 0x022E
I think these field ids represents various labels to b displayed on the masterview.

## Field values of the masscombi
CANId is 0x020E

|0000| AC Output Amps |
|0006| Inverter DC input voltage |
|0007| Inverter DC amp flow |
|0008| AC input voltage (For DC Charger and/or maybe the power assist function?) |
|0009| AC input amps (For DC Charger and/or maybe the power assist function?) |
|000A| AC output voltage |
|000B| AC output amps |
|0010| Invverter state? I saw the value 3.0==When inverting only, charger is off |
|0011| Load in percent|
|0012| State of charger 5.0=No shore power| 
|0013| Shore power shore fuse: 14.0=18 Amps| 
|0014| Inverter switch state. 1.0=On|
|000E| Input genset 0.0=off |
|0038| Masscombi power state. 1.0=On |
|003A| Battery charger switch 0.0=Off |
|003C| Mode |

## Battery charger of the masscombi
CANId is 0x0667

## Masterview display device
CANId is 0x0194

This device will query for other devices to announce themselves upon startup. It will then start querying regularly for certain field values from selected devices. The fields selected reflect what is required to be displayed on the screen.

## Mystery devices
These are canIds I do not know the purpose.
0x0209
