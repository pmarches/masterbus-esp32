#define UNITY_SUPPORT_TEST_CASES
#include <unity.h>
#include <stdio.h>

#include <mvParser.hpp>
#include <string>

#define MASSCOMBI_ID 0x0002f412
#define DCSHUNT_ID 0x31297

TEST_CASE("Test batteryPercentage", "[mvparser]") {
  MvParser mvParser;
  MastervoltMessageFloat* msg = (MastervoltMessageFloat*) mvParser.parseStdCanbusId(0x21b, DCSHUNT_ID, std::string("\x00\x00\x50\xff\xc7\x42", 6));
  TEST_ASSERT_EQUAL(0x00, msg->attributeId);
  TEST_ASSERT_EQUAL_FLOAT(msg->floatValue, 99.998657);
  delete msg;
}


TEST_CASE("Test inverterOutputAmps", "[mvparser]") {
  MvParser mvParser;
  MastervoltMessageFloat* msg = (MastervoltMessageFloat*) mvParser.parseStdCanbusId(0x20e, MASSCOMBI_ID, std::string("\x0b\x00\xcd\xcc\x4c\x3f", 6));
  TEST_ASSERT_EQUAL(0x0b, msg->attributeId);
  TEST_ASSERT_EQUAL_FLOAT(msg->floatValue, 0.8);
  delete msg;
}

TEST_CASE("Test dcShuntVoltage", "[mvparser]") {
  MvParser mvParser;
  MastervoltMessageFloat* msg = (MastervoltMessageFloat*) mvParser.parseStdCanbusId(0x21b, DCSHUNT_ID, std::string("\x01\x00\xad\x1c\x4a\x41", 6));
  TEST_ASSERT_EQUAL(0x01, msg->attributeId);
  TEST_ASSERT_EQUAL_FLOAT(msg->floatValue, 12.632);
  delete msg;
}


TEST_CASE("Test dcShuntAmpsFlow", "[mvparser]") {
  MvParser mvParser;
  MastervoltMessageFloat* msg = (MastervoltMessageFloat*) mvParser.parseStdCanbusId(0x21b, DCSHUNT_ID, std::string("\x02\x00\x5a\x02\x2c\xc1", 6));
  TEST_ASSERT_EQUAL(0x02, msg->attributeId);
  TEST_ASSERT_EQUAL_FLOAT(msg->floatValue, -10.7506);
  delete msg;
}

TEST_CASE("Test dcShuntAmpsConsumed", "[mvparser]") {
  MvParser mvParser;
  MastervoltMessageFloat* msg = (MastervoltMessageFloat*) mvParser.parseStdCanbusId(0x21b, DCSHUNT_ID, std::string("\x03\x00\x00\x80\x5c\x3d", 6));
  TEST_ASSERT_EQUAL(0x03, msg->attributeId);
  TEST_ASSERT_EQUAL_FLOAT(msg->floatValue, 0.053833);
  delete msg;
}

TEST_CASE("Test dcShuntTimeOfDay", "[mvparser]") {
  MvParser mvParser;
  MastervoltMessageTime* msg = (MastervoltMessageTime*) mvParser.parseStdCanbusId(0x21b, DCSHUNT_ID, std::string("\x09\x00\x80\x3f\x83\x47", 6));
  TEST_ASSERT_EQUAL(0x09, msg->attributeId);
  TEST_ASSERT_EQUAL(18, msg->hour);
  TEST_ASSERT_EQUAL(39, msg->minute);
  TEST_ASSERT_EQUAL(59, msg->second);
  delete msg;
}


TEST_CASE("Test dcShuntDateOfDay", "[mvparser]") {
  MvParser mvParser;
  MastervoltMessageDate* msg = (MastervoltMessageDate*) mvParser.parseStdCanbusId(0x21b, DCSHUNT_ID, std::string("\x0a\x00\x20\x5f\x4d\x49", 6));
  TEST_ASSERT_EQUAL(0x0a, msg->attributeId);
  TEST_ASSERT_EQUAL(18, msg->day);
  TEST_ASSERT_EQUAL(2022, msg->year);
  TEST_ASSERT_EQUAL(1, msg->month);
  delete msg;
}

void assertConvertFloatToDate(uint8_t expectedDay, uint8_t expectedMonth, uint16_t expectedYear, float floatDate){
  MastervoltMessageDate* dateMsg = new MastervoltMessageDate(0x0, 0x0, 0x0);
  MvParser::convertMastervoltFloatToDate(floatDate, dateMsg);
  TEST_ASSERT_EQUAL(expectedDay, dateMsg->day);
  TEST_ASSERT_EQUAL(expectedMonth, dateMsg->month);
  TEST_ASSERT_EQUAL(expectedYear, dateMsg->year);
  delete dateMsg;
}

TEST_CASE("Test convertMastervoltFloatToDate", "[mvparser]") {
  assertConvertFloatToDate(29, 5, 2020, 840509.00);
  assertConvertFloatToDate(16, 6, 2020, 840528.00);
  assertConvertFloatToDate(16, 6, 2021, 840944.00);
  assertConvertFloatToDate(15, 6, 2021, 840943.00);
  assertConvertFloatToDate(15, 6, 2020, 840527);
  assertConvertFloatToDate(15, 7, 2020, 840559);

  assertConvertFloatToDate(15, 6, 2000, 832207); //The masterview interface does not allow for less than year 2000
  assertConvertFloatToDate(02, 1, 2000, 832034); //The interface fails to show the date if we set the date to 01/01/2000. This is probably the default
}
