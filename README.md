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

 