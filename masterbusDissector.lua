--Masterbus protocol captured with a raspberry pi canhat and the command `tshark -i can0 -w canhat-capture.pcap`
local f_can_id = Field.new("can.id")
local f_can_len = Field.new("can.len")


local DEVICE_ID = {
    [0x31297]="DC Shunt",
    [0x2f412]="Masscombi",
    [0x0840a]="Masterview",
}

local MASSCOMBI_ATTRIBUTES={
    [0x07]="INVERTER_DC_AMPS_IN",
    [0x06]="INVERTER_DC_VOLTAGE_IN",
    [0x0A]="INVERTER_AC_VOLTS_OUT",
    [0x0B]="INVERTER_AC_AMPS_OUT",
    [0x0E]="INVERTER_INPUT_GENSET",
    [0x10]="Mystery mode",
    [0x11]="INVERTER_LOAD_PERCENT",
    [0x13]="INVERTER_SHORE_FUSE",
    [0x14]="INVERTER_MODE",
    [0x38]="INVERTER_POWER_STATE",
    [0x3A]="CHARGER_STATE",
}

local DCSHUNT_ATTRIBUTES ={
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
masterbus_proto.fields.queryattribute=ProtoField.uint16("masterbus.queryattribute", "Requested attribute", base.HEX, nil, nil)

local response_proto = Proto("masterbus.response","Response")
response_proto.fields.responseattribute=ProtoField.uint16("masterbus.response.attribute", "Attribute", base.HEX, nil, nil)
response_proto.fields.responseattributeindex=ProtoField.uint16("masterbus.response.attributeindex", "Attribute index", base.HEX, nil, nil)
response_proto.fields.responseintvalue=ProtoField.uint16("masterbus.response.intvalue", "Int value", base.DEC)

response_proto.fields.responsefloatvalue=ProtoField.float("masterbus.response.floatvalue", "Float value", base.DEC)

response_proto.fields.responsemasscombiattribute=ProtoField.uint16("masterbus.response.masscombi.attribute", "Masscombi Attribute", base.HEX, MASSCOMBI_ATTRIBUTES, nil)
masterbus_proto.fields.responsemasscombifloatValue=ProtoField.float("masterbus.response.masscombi.floatvalue", "Masscombi float value", base.DEC)

response_proto.fields.responseddcshuntattribute=ProtoField.uint16("masterbus.response.dcshunt.attribute", "DCShunt Attribute", base.HEX, DCSHUNT_ATTRIBUTES, nil)
masterbus_proto.fields.responsedcshuntfloatValue=ProtoField.float("masterbus.response.dcshunt.floatvalue", "DCShunt float value", base.DEC)


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
        tree:add_le(masterbus_proto.fields.requestedstringlabeloffset, buffer(2,2))
    end
end

function parseStringLabelMessage(buffer,pinfo,tree)
    pinfo.cols.info:set("parseStringLabelMessage")
    
    tree:add_le(masterbus_proto.fields.stringlabelkey, buffer(0,2))
    tree:add_le(masterbus_proto.fields.stringValueOffset, buffer(2,2))
    
    if(buffer:len() > 4) then
        local labelBuf=buffer(4)
        tree:add(masterbus_proto.fields.stringValue, labelBuf)
    end

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

function parseQueryAttribute(buffer,pinfo,tree)
    pinfo.cols.info:set("parseQueryAttribute");
    tree:add_packet_field(masterbus_proto.fields.queryattribute, buffer(0,2), ENC_LITTLE_ENDIAN)
    if(buffer:len()==3) then
        tree:add_packet_field(masterbus_proto.fields.queryattribute, buffer(2,1), ENC_LITTLE_ENDIAN)
    elseif(buffer:len()==4) then
        tree:add_packet_field(masterbus_proto.fields.queryattribute, buffer(2,2), ENC_LITTLE_ENDIAN)
    else
        --Nothing
    end
end

function parseQueryAttributeAndIndex(buffer,pinfo,tree)
    pinfo.cols.info:set("parseQueryAttributeAndIndex");
    tree:add_packet_field(masterbus_proto.fields.queryattribute, buffer(0,2), ENC_LITTLE_ENDIAN)
    tree:add_packet_field(masterbus_proto.fields.queryattribute, buffer(2,1), ENC_LITTLE_ENDIAN)
end

function parseResponseAttributeAndFloat(buffer,pinfo,tree)
    pinfo.cols.info:set("parseResponseAttributeAndFloat");
    local attbuf=buffer(0,2)
    tree:add_packet_field(response_proto.fields.responseattribute, attbuf, ENC_LITTLE_ENDIAN)
    local attributeId=attbuf:le_uint()
    --TODO Here, we need to distinguish between the messages sent by the masscombi, and those sent by the DC Shunt. The message format is the same, but the attributes and decoding of the floats is different.
    tree:add_le(response_proto.fields.responsefloatvalue, buffer(2,4))
--     pinfo.cols.info:set(string.format("attributeId=%s", attbuf:le_uint()));
end

function parseDCShuntResponseAttributeAndFloat(buffer,pinfo,tree)
    pinfo.cols.info:set("parseDCShuntResponseAttributeAndFloat");
    local attbuf=buffer(0,2)
    tree:add_packet_field(response_proto.fields.responseddcshuntattribute, attbuf, ENC_LITTLE_ENDIAN)
    local attributeId=attbuf:le_uint()
    --TODO Here, we need to distinguish between the messages sent by the masscombi, and those sent by the DC Shunt. The message format is the same, but the attributes and decoding of the floats is different.
    if(attributeId==0x09) then --Time of the DC Shunt
        local timeAsFloatItem=tree:add_le(masterbus_proto.fields.responsedcshuntfloatValue, buffer(2,4))
        local secondElapsedSinceMidnight=buffer(2,4):le_float()
        local hour=math.floor(secondElapsedSinceMidnight/60/60);
		secondElapsedSinceMidnight=secondElapsedSinceMidnight-(hour*60*60);
		local minute=math.floor(secondElapsedSinceMidnight/60);
		secondElapsedSinceMidnight=secondElapsedSinceMidnight-(minute*60);
		local second=secondElapsedSinceMidnight;
        pinfo.cols.info:set(string.format("%s=>%d:%d:%d", timeAsFloatItem.text, hour, minute, second))
    elseif(attributeId==0x0a) then --Date of the DC Shunt
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
--     pinfo.cols.info:set(string.format("attributeId=%s", attbuf:le_uint()));
end

function parseMasscombiResponseAttributeAndFloat(buffer,pinfo,tree)
    pinfo.cols.info:set("parseMasscombiResponseAttributeAndFloat");
    local attbuf=buffer(0,2)
    tree:add_packet_field(response_proto.fields.responsemasscombiattribute, attbuf, ENC_LITTLE_ENDIAN)
    local attributeId=attbuf:le_uint()
    tree:add_le(masterbus_proto.fields.responsemasscombifloatValue, buffer(2,4))
--     pinfo.cols.info:set(string.format("attributeId=%s", attbuf:le_uint()));
end

function parsePeriodicMessage(buffer,pinfo,tree)
    pinfo.cols.info:set("parsePeriodicMessage");
    local unknownBuf=buffer(0,2)
    if(unknownBuf:uint()==0x083f) then
        pinfo.cols.info:set("Maybe some sort of refresh response?");
    end
end

function parseShortAttributeResponseMessage(buffer,pinfo,tree)
    pinfo.cols.info:set("parseShortAttributeResponseMessage");
    tree:add_le(response_proto.fields.rrattributekey, buffer(0,2))
    tree:add_le(response_proto.fields.rrattributeindex, buffer(2,1))
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

function parseShortShortMessage(buffer,pinfo,tree)
    pinfo.cols.info:set("parseShortShortMessage");
    tree:add_le(masterbus_proto.fields.unknown, buffer(0,2))
    tree:add_le(masterbus_proto.fields.unknown, buffer(2,2))
end

function parseAttributeIndexShort(buffer,pinfo,tree)
    pinfo.cols.info:set("parseAttributeIndexShort");
    tree:add_le(response_proto.fields.responseattribute, buffer(0,2))
    tree:add_le(response_proto.fields.responseattributeindex, buffer(2,2))
    tree:add_le(response_proto.fields.responseintvalue, buffer(4,1))
end

function parseAttributeIndexUnknown(buffer,pinfo,tree)
    pinfo.cols.info:set("parseAttributeIndexUnknown");
    tree:add_le(response_proto.fields.responseattribute, buffer(0,2))
    tree:add_le(response_proto.fields.responseattributeindex, buffer(2,2))
    if(buffer:len()==8) then
        tree:add_le(masterbus_proto.fields.responsemasscombifloatValue, buffer(4,4))
    elseif(buffer:len()==6) then
        tree:add_le(masterbus_proto.fields.unknownint, buffer(4,2))
    else
        tree:add_le(masterbus_proto.fields.unknown, buffer(4,buffer:len()-4))
    end
end


local MESSAGE_ID_TO_PARSER = {
    [0x10e]=parseByteRefBytesMessage,
    [0x11b]=parseByteRefBytesMessage,
    [0x114]=parseByteRefBytesMessage,
    [0x1d4]=parseQueryAttribute,
    [0x14e]=parseEmptyMessage,
    [0x154]=parseEmptyMessage,
    [0x15b]=parseEmptyMessage,
    [0x18e]=parseStringLabelMessage,
    [0x194]=parseShortShortMessage,
    [0x19b]=parseStringLabelMessage,
    [0x1ae]=parseRequestStringLabelMessage,
    [0x1db]=parseRequestStringLabelMessage,
    [0x1bb]=parseRequestStringLabelMessage,
    [0x1ce]=parseRequestStringLabelMessage,

    [0x20e]=parseMasscombiResponseAttributeAndFloat, --Response to 0x60e
    [0x21b]=parseDCShuntResponseAttributeAndFloat, --Response to 0x61b
    [0x22e]=parseAttributeIndexUnknown, --Response to 0x62e
    [0x23b]=parseConfigurationMessage, --Response to 0x63b
    [0x24e]=parseAttributeIndexShort, --Response to 0x64e
    [0x25b]=parseAttributeIndexShort, --Response to 0x65b

    [0x60e]=parseQueryAttribute, --Response is 0x20e
    [0x61b]=parseQueryAttribute, --Response is 0x21b
    [0x62e]=parseQueryAttributeAndIndex, --This is a request, the response is 0x22e
    [0x63b]=parseQueryAttribute, --Response is 0x23b
    [0x64e]=parseQueryAttributeAndIndex, --Response is 0x24e
    [0x65b]=parseQueryAttribute, --Response 0x25b
}

function lookupDeviceAndAttributeInfo(deviceKind, attributeId, pinfo)
    if(DEVICEKIND_ATTR[deviceKind] ~= nil) then
        pinfo.cols.info:set(string.format("attributeId=%x", attributeId));
        local attrDesc=DEVICEKIND_ATTR[deviceKind][attributeId]
        if(attrDesc ~= nil) then
            pinfo.cols.info:set(attrDesc);
        end
    end
end

function masterbus_proto.dissector(buffer,pinfo,tree)
    local canIdBuf=f_can_id().tvb
    
    pinfo.cols.info:set("")
    pinfo.cols.protocol = masterbus_proto.name
    local toptree = tree:add(masterbus_proto, buffer(),"Masterbus message")

    local messageKind=bit.rshift(bit.band(canIdBuf:le_uint(), 0x1FFC0000), 18)
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
