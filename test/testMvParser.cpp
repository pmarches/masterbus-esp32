#include <unity.h>
#include <stdio.h>

#include <mvParser.hpp>
#include <string>

TEST_CASE("Sanity test", "[mvparser]") {
  MvParser mvParser;
  MastervoltMessageFloat* percentMsg = (MastervoltMessageFloat*) mvParser.parse(0x21b, 0x31297, std::string("\x00\x00\x50\xff\xc7\x42"));
  printf("percentMsg->floatValue=%f\n", percentMsg->floatValue);
  assert(percentMsg->floatValue*10000==999987);
  delete percentMsg;

}
