--Masterbus protocol captured with a raspberry pi canhat and the command `tshark -i can0 -w canhat-capture.pcap`
local f_can_id = Field.new("can.id")
local f_can_len = Field.new("can.len")


local MESSAGE_ID_TO_FRAMENUM = {
}

local DEVICE_ID = {
    [0x31297]="DC Shunt",
    [0x2f412]="Masscombi",
    [0x0840a]="Masterview",
}

-- Masterview areas:
--1: DCSHUNT_BATTERY_PERCENTAGE
--2: DCSHUNT_BATTERY_AMPS_CONSUMED
--3: DCSHUNT_BATTERY_AMPS
--4: DCSHUNT_BATTERY_VOLTS
--5: INVERTER_MODE
--6: INVERTER_AC_AMPS_OUT

local MASSCOMBI_ITEMS={
    [0x06]="INVERTER_DC_VOLTAGE_IN",
    [0x07]="INVERTER_DC_AMPS_IN",
    [0x08]="CHARGER_AC_VOLTAGE_IN", --Maybe also the power assit function
    [0x09]="CHARGER_AC_AMPS_IN", --Maybe also the power assit function
    [0x0A]="INVERTER_AC_VOLTS_OUT",
    [0x0B]="INVERTER_AC_AMPS_OUT",
    [0x0E]="INVERTER_INPUT_GENSET",
    [0x10]="INVERTER_STATE", --"Inverting", "Standby" This is NOT the state switch. 
    [0x11]="INVERTER_LOAD_PERCENT",
    [0x12]="State of charger", -- "No shore"
    [0x13]="INVERTER_SHORE_FUSE", --Can configure the value here (like a switch). 14.0=>18 Amps
    [0x14]="INVERTER_MODE", --On or standby
    [0x38]="INVERTER_POWER_STATE",
    [0x3A]="CHARGER_SWITCH_STATE", --This is the state switch
    [0x3C]="CHARGER_MODE", --Auto, ...
}

local DCSHUNT_ITEMS ={
    [0x00]="DCSHUNT_BATTERY_PERCENTAGE",
    [0x01]="DCSHUNT_BATTERY_VOLTS",
    [0x02]="DCSHUNT_BATTERY_AMPS",
    [0x03]="DCSHUNT_BATTERY_AMPS_CONSUMED",
    [0x04]="0x04 Unknown ", --Values seen: 579120 567600
    [0x05]="DCSHUNT_BATTERY_TEMPERATURE", --Celcius
    [0x09]="DCSHUNT_TIME", --The float represents the number of seconds elapsed since 00:00:00
    [0x0A]="DCSHUNT_DATE", --The number of days since day 0
}

local masterbus_proto = Proto("masterbus","Mastervolt masterbus protocol, based on canbus")

masterbus_proto.fields.unhandled=ProtoField.bytes("masterbus.unhandled", "Unhandeled message type", base.NONE)

masterbus_proto.fields.canid=ProtoField.uint32("masterbus.canid", "Masterbus Id", base.HEX, nil)
masterbus_proto.fields.messagekind=ProtoField.uint32("masterbus.messagekind", "Masterbus 10-bit message kind", base.HEX, nil, 0x1FFC0000)
masterbus_proto.fields.deviceid=ProtoField.uint32("masterbus.deviceid", "Masterbus 18-bit unique Id", base.HEX, DEVICE_ID, 0x0003FFFF)

masterbus_proto.fields.floatValue=ProtoField.float("masterbus.floatvalue", "float value", base.DEC)

masterbus_proto.fields.stringlabelkey=ProtoField.uint16("masterbus.stringlabelkey", "String label key", base.HEX, nil)
masterbus_proto.fields.requestedstringlabeloffset=ProtoField.uint16("masterbus.requestedstringlabeloffset", "Requested String label offset", base.DEC, nil)
masterbus_proto.fields.stringValueOffset=ProtoField.uint16("masterbus.stringValueoffset", "Label Offset", base.DEC, nil)
masterbus_proto.fields.stringValue=ProtoField.string("masterbus.stringvalue", "String value")
masterbus_proto.fields.intValue=ProtoField.uint32("masterbus.intValue", "int value", base.DEC, nil)

masterbus_proto.fields.unknown=ProtoField.bytes("masterbus.unknown", "Unknown bytes", base.NONE)
masterbus_proto.fields.unknownint=ProtoField.uint32("masterbus.unknownint", "Unknown int", base.DEC, nil)
masterbus_proto.fields.fullyparsed=ProtoField.bool("masterbus.fullyparsed", "These message types are fully parsed", base.NONE)

local query_proto = Proto("masterbus.query","Query")
masterbus_proto.fields.querydevice=ProtoField.uint16("masterbus.querydevice", "Requested Device unique Id", base.HEX, nil, nil)
masterbus_proto.fields.queryitem=ProtoField.uint16("masterbus.queryitem", "Requested item", base.HEX, nil, nil)

local response_proto = Proto("masterbus.response","Response")
response_proto.fields.responseitem=ProtoField.uint16("masterbus.response.item", "Item", base.HEX, nil, nil)
response_proto.fields.responseitemindex=ProtoField.uint16("masterbus.response.itemindex", "Item index", base.HEX, nil, nil)
response_proto.fields.responseintvalue=ProtoField.uint16("masterbus.response.intvalue", "Int value", base.DEC)

response_proto.fields.responsefloatvalue=ProtoField.float("masterbus.response.floatvalue", "Float value", base.DEC)

response_proto.fields.responsemasscombiitem=ProtoField.uint16("masterbus.response.masscombi.item", "Masscombi Item", base.HEX, MASSCOMBI_ITEMS, nil)
masterbus_proto.fields.responsemasscombifloatValue=ProtoField.float("masterbus.response.masscombi.floatvalue", "Masscombi float value", base.DEC)

response_proto.fields.responseddcshuntitem=ProtoField.uint16("masterbus.response.dcshunt.item", "DCShunt Item", base.HEX, DCSHUNT_ITEMS, nil)
masterbus_proto.fields.responsedcshuntfloatValue=ProtoField.float("masterbus.response.dcshunt.floatvalue", "DCShunt float value", base.DEC)

masterbus_proto.fields.request=ProtoField.new("MasterbusRequest", "MasterbusRequest",  ftypes.FRAMENUM, frametype.REQUEST)
masterbus_proto.fields.response=ProtoField.new("MasterbusResponse", "MasterbusResponse",  ftypes.FRAMENUM, frametype.RESPONSE)

local CONFIGURATION_PARAMETER_TYPES = {
    [0x06]="Minimum value",
    [0x07]="Maximum value",
    [0x08]="0x08 Linked to MinMax?", --Same format as Minimum and Maximum
    [0x0b]="0x0b",
    [0x0c]="0x0c",
    [0x28]="0x28",
}
local CONFIGURATION_PARAMETER_KEYS = {
}
response_proto.fields.configparametertype=ProtoField.uint8("masterbus.response.configparametertype", "Configuration parameter type", base.HEX, CONFIGURATION_PARAMETER_TYPES, nil)
response_proto.fields.configparameterkey=ProtoField.uint8("masterbus.response.configparameterkey", "Configuration parameter key", base.HEX, CONFIGURATION_PARAMETER_KEYS, nil)

masterbus_proto.fields.canbusref=ProtoField.uint32("masterbus.canbusref", "Reference another canbus", base.HEX)

local unhandledPacket=ProtoExpert.new("masterbus.unhandledPacket", "Unhandled masterbus packet", expert.group.UNDECODED, expert.severity.WARN)
local truncatedPacket=ProtoExpert.new("masterbus.truncatedPacket", "Packet is too short. Likely truncated", expert.group.UNDECODED, expert.severity.WARN)
masterbus_proto.experts = {
    unhandledPacket,
    truncatedPacket,
}

function parseRequestStringLabelMessage(buffer,pinfo,tree)
    pinfo.cols.info:set("parseRequestStringLabelMessage")
    tree:add_le(masterbus_proto.fields.stringlabelkey, buffer(0,2))
    if(buffer:len()==4) then
        tree:add(masterbus_proto.fields.requestedstringlabeloffset, buffer(2,2)) --For messages 0x1db I need to parse this as big endian????
    end
end

function parseResponseStringLabelMessage(buffer,pinfo,tree)
    pinfo.cols.info:set("parseResponseStringLabelMessage")
--     if(MESSAGE_ID_TO_FRAMENUM[0x19b+0x40] ~= nil) then
--         tree:add(masterbus_proto.fields.request, MESSAGE_ID_TO_FRAMENUM[0x19b+0x40])
--     end

    tree:add_le(masterbus_proto.fields.stringlabelkey, buffer(0,2)) --Keys for strings always start with 0x30. I have seen 0x09 but they do not have a payload other than the string index offset.
    tree:add(masterbus_proto.fields.stringValueOffset, buffer(2,2)) --For messages 0x19b I need to parse this field as a big endian?
    
        local labelBuf=buffer(4)
        tree:add(masterbus_proto.fields.stringValue, labelBuf) --An empty string indicates the end of the label

end

function parseConfigurationMessage(buffer,pinfo,tree)
    pinfo.cols.info:set("parseConfigurationMessage");
    local configparametertypeBuf=buffer(0,1)
    tree:add(response_proto.fields.configparametertype, configparametertypeBuf)
    if(configparametertypeBuf:uint()>=0x06 and configparametertypeBuf:uint()<=0x08) then
        tree:add(response_proto.fields.configparameterkey, buffer(1,1))
        tree:add_le(masterbus_proto.fields.floatValue, buffer(buffer:len()-4,4))
    else
        tree:add_le(masterbus_proto.fields.unknown, buffer(1,buffer:len()-1))
    end
end

function parseQueryItem(buffer,pinfo,tree)
    pinfo.cols.info:set("parseQueryItem");
--     if(MESSAGE_ID_TO_FRAMENUM[messageKind+0x40] ~= nil) then
--         tree:add(masterbus_proto.fields.request, MESSAGE_ID_TO_FRAMENUM[messageKind+0x40])
--     end

    tree:add_packet_field(masterbus_proto.fields.queryitem, buffer(0,2), ENC_LITTLE_ENDIAN)
    if(buffer:len()==3) then
        tree:add_packet_field(masterbus_proto.fields.queryitem, buffer(2,1), ENC_LITTLE_ENDIAN)
    elseif(buffer:len()==4) then
        tree:add_packet_field(masterbus_proto.fields.queryitem, buffer(2,2), ENC_LITTLE_ENDIAN)
    else
        --Nothing
    end
end

function parseQueryItemAndIndex(buffer,pinfo,tree)
    pinfo.cols.info:set("parseQueryItemAndIndex");
    tree:add_packet_field(masterbus_proto.fields.queryitem, buffer(0,2), ENC_LITTLE_ENDIAN)
    tree:add_packet_field(masterbus_proto.fields.queryitem, buffer(2,1), ENC_LITTLE_ENDIAN)
end

function parseResponseItemAndFloat(buffer,pinfo,tree)
    pinfo.cols.info:set("parseResponseItemAndFloat");
    local attbuf=buffer(0,2)
    tree:add_packet_field(response_proto.fields.responseitem, attbuf, ENC_LITTLE_ENDIAN)
    local itemId=attbuf:le_uint()
    --TODO Here, we need to distinguish between the messages sent by the masscombi, and those sent by the DC Shunt. The message format is the same, but the items and decoding of the floats is different.
    tree:add_le(response_proto.fields.responsefloatvalue, buffer(2,4))
--     pinfo.cols.info:set(string.format("itemId=%s", attbuf:le_uint()));
end

function parseDCShuntResponseItemAndFloat(buffer,pinfo,tree)
    pinfo.cols.info:set("parseDCShuntResponseItemAndFloat");
    local attbuf=buffer(0,2)
    tree:add_packet_field(response_proto.fields.responseddcshuntitem, attbuf, ENC_LITTLE_ENDIAN)
    local itemId=attbuf:le_uint()
    --TODO Here, we need to distinguish between the messages sent by the masscombi, and those sent by the DC Shunt. The message format is the same, but the items and decoding of the floats is different.
    if(itemId==0x09) then --Time of the DC Shunt
        local timeAsFloatItem=tree:add_le(masterbus_proto.fields.responsedcshuntfloatValue, buffer(2,4))
        local secondElapsedSinceMidnight=buffer(2,4):le_float()
        local hour=math.floor(secondElapsedSinceMidnight/60/60);
		secondElapsedSinceMidnight=secondElapsedSinceMidnight-(hour*60*60);
		local minute=math.floor(secondElapsedSinceMidnight/60);
		secondElapsedSinceMidnight=secondElapsedSinceMidnight-(minute*60);
		local second=secondElapsedSinceMidnight;
        pinfo.cols.info:set(string.format("%s=>%d:%d:%d", timeAsFloatItem.text, hour, minute, second))
    elseif(itemId==0x0a) then --Date of the DC Shunt
        local floatItem=tree:add_le(masterbus_proto.fields.responsedcshuntfloatValue, buffer(2,4))
        local floatDate=buffer(2,4):le_float()
        local day=(floatDate%32)+1
        floatDate=floatDate-day;
        local yearAndFraction=floatDate/416.0; --32 days *13 months=416
        local year=math.floor(yearAndFraction)
        yearAndFraction=yearAndFraction-year
        local month=yearAndFraction*13
        pinfo.cols.info:set(string.format("%s=>Day/Month/Year=%d/%d/%d", floatItem.text, day, month, year))
    else
        tree:add_le(masterbus_proto.fields.responsedcshuntfloatValue, buffer(2,4))
    end
--     tree:add_le(masterbus_proto.fields.fullyparsed, buffer())
--     pinfo.cols.info:set(string.format("itemId=%s", attbuf:le_uint()));
end

function parseMasscombiResponseItemAndFloat(buffer,pinfo,tree)
    pinfo.cols.info:set("parseMasscombiResponseItemAndFloat");
    local attbuf=buffer(0,2)
    tree:add_packet_field(response_proto.fields.responsemasscombiitem, attbuf, ENC_LITTLE_ENDIAN)
    local itemId=attbuf:le_uint()
    tree:add_le(masterbus_proto.fields.responsemasscombifloatValue, buffer(2,4))
--     pinfo.cols.info:set(string.format("itemId=%s", attbuf:le_uint()));
end

function parsePeriodicMessage(buffer,pinfo,tree)
    pinfo.cols.info:set("parsePeriodicMessage");
    local unknownBuf=buffer(0,2)
    if(unknownBuf:uint()==0x083f) then
        pinfo.cols.info:set("Maybe some sort of refresh response?");
    end
end

function parseShortItemResponseMessage(buffer,pinfo,tree)
    pinfo.cols.info:set("parseShortItemResponseMessage");
    tree:add_le(response_proto.fields.rritemkey, buffer(0,2))
    tree:add_le(response_proto.fields.rritemindex, buffer(2,1))
end

function parseEmptyMessage(buffer,pinfo,tree)
    pinfo.cols.info:set("parseEmptyMessage");
end

function parseByteRefBytesMessage(buffer,pinfo,tree)
    pinfo.cols.info:set("parseByteRefBytesMessage");
    tree:add_le(masterbus_proto.fields.unknown, buffer(0,1))
    tree:add_le(masterbus_proto.fields.canbusref, buffer(1,3))
    tree:add_le(masterbus_proto.fields.unknownint, buffer(4,4))
end

function parseShortMessage(buffer,pinfo,tree)
    pinfo.cols.info:set("parseShortMessage");
    tree:add_le(masterbus_proto.fields.unknown, buffer(0,2))
end

function parseShortShortMessage(buffer,pinfo,tree)
    pinfo.cols.info:set("parseShortShortMessage");
    tree:add_le(masterbus_proto.fields.unknown, buffer(0,2))
    tree:add_le(masterbus_proto.fields.unknown, buffer(2,2))
end

function parseItemIndexShort(buffer,pinfo,tree)
    pinfo.cols.info:set("parseItemIndexShort");
    tree:add_le(response_proto.fields.responseitem, buffer(0,2))
    tree:add_le(response_proto.fields.responseitemindex, buffer(2,2))
    tree:add_le(response_proto.fields.responseintvalue, buffer(4,1))
end

function parseItemIndexUnknown(buffer,pinfo,tree)
    pinfo.cols.info:set("parseItemIndexUnknown");
    tree:add_le(response_proto.fields.responseitem, buffer(0,2))
    tree:add_le(response_proto.fields.responseitemindex, buffer(2,2))
    if(buffer:len()==8) then
        tree:add_le(masterbus_proto.fields.responsemasscombifloatValue, buffer(4,4))
    elseif(buffer:len()==6) then
        tree:add_le(masterbus_proto.fields.unknownint, buffer(4,2))
    else
        tree:add_le(masterbus_proto.fields.unknown, buffer(4,buffer:len()-4))
    end
end

--Message ids seem to be divided into requests, and responses. The response ids correspond to the RequestId plus 0x40.

local MESSAGE_ID_TO_PARSER = {
    [0x10e]=parseByteRefBytesMessage,
    [0x11b]=parseByteRefBytesMessage, --Looks like an annoucement from DCShunt, 2x every 30 seconds. No response
    [0x114]=parseByteRefBytesMessage, --Looks like an annoucement from Masterview, 1x every 30 seconds. No response
    [0x1d4]=parseQueryItem,
    [0x14e]=parseEmptyMessage, --First message from the masscombi I see on the bus when booting up. Looks like an annoucement from Masscombi, 1x every 30 seconds when network is quiet. Response to 0x10e
    [0x154]=parseEmptyMessage, --Looks like an annoucement from Masterview, 1x every 30 seconds when network is quiet. 
    [0x15b]=parseEmptyMessage, --First message from the dcshunt I see on the bus when booting up. Never seen this message again
    [0x18e]=parseResponseStringLabelMessage, --Found some strings in here!!!
    [0x194]=parseShortShortMessage,
    [0x19b]=parseResponseStringLabelMessage, --Response to 0x1db
    [0x1ae]=parseShortMessage, --Response, request is 0x1ce
    [0x1db]=parseRequestStringLabelMessage, --Request, response is 0x19b (if the item is a string) or 0x1bb (if the item is a number)
    [0x1bb]=parseRequestStringLabelMessage, --Response to 0x1db when the item 
    [0x1ce]=parseQueryItem, --Request, response is 0x18e . Response is also 0x1ae

    [0x20e]=parseMasscombiResponseItemAndFloat, --Response to 0x60e
    [0x21b]=parseDCShuntResponseItemAndFloat, --Response to 0x61b
    [0x22e]=parseItemIndexUnknown, --Response to 0x62e
    [0x23b]=parseConfigurationMessage, --Response to 0x63b
    [0x24e]=parseItemIndexShort, --Response to 0x64e
    [0x25b]=parseItemIndexShort, --Response to 0x65b

    [0x60e]=parseQueryItem, --Response is 0x20e
    [0x61b]=parseQueryItem, --Response is 0x21b
    [0x62e]=parseQueryItemAndIndex, --This is a request, the response is 0x22e
    [0x63b]=parseQueryItem, --Response is 0x23b
    [0x64e]=parseQueryItemAndIndex, --Response is 0x24e
    [0x65b]=parseQueryItem, --Response 0x25b
}

function masterbus_proto.dissector(buffer,pinfo,tree)
    local canIdBuf=f_can_id().tvb
    
    pinfo.cols.info:set("")
    pinfo.cols.protocol = masterbus_proto.name
    local toptree = tree:add(masterbus_proto, buffer(0),"Masterbus message")

    local messageKind=bit.rshift(bit.band(canIdBuf:le_uint(), 0x1FFC0000), 18)
--     MESSAGE_ID_TO_FRAMENUM[messageKind]=pinfo.number
    local canIdTree=toptree:add_le(masterbus_proto.fields.canid, canIdBuf)
    canIdTree:add_le(masterbus_proto.fields.messagekind, canIdBuf);
    canIdTree:add_le(masterbus_proto.fields.deviceid, canIdBuf)

    local parserFunction=MESSAGE_ID_TO_PARSER[messageKind]
    if(parserFunction~=nil) then
        parserFunction(buffer,pinfo,toptree)
    else
        toptree:add(masterbus_proto.fields.unhandled)
    end
end --function

--DissectorTable.get("can.subdissector"):add(masterbus_proto, 0)
DissectorTable.get("can.subdissector"):add_for_decode_as(masterbus_proto)
