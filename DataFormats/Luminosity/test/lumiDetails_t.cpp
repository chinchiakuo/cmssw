#include <cppunit/extensions/HelperMacros.h>
#include "DataFormats/Luminosity/interface/LumiDetails.h"

#include <string>
#include <vector>
#include <iostream>

class TestLumiDetails: public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE(TestLumiDetails);  
  CPPUNIT_TEST(testConstructor);
  CPPUNIT_TEST(testFill);
  CPPUNIT_TEST_SUITE_END();
  
public:
  void setUp() {}
  void tearDown() {}

  void testConstructor();
  void testFill();
};

///registration of the test so that the runner can find it
CPPUNIT_TEST_SUITE_REGISTRATION(TestLumiDetails);

void
TestLumiDetails::testConstructor() {
  std::cout << "\nTesting LumiDetails\n";
  LumiDetails lumiDetails;

  CPPUNIT_ASSERT(lumiDetails.lumiVersion() == std::string("-1"));
  CPPUNIT_ASSERT(!lumiDetails.isValid());
  lumiDetails.setLumiVersion(std::string("v1"));
  CPPUNIT_ASSERT(lumiDetails.isValid());
  CPPUNIT_ASSERT(lumiDetails.lumiVersion() == std::string("v1"));

  LumiDetails lumiDetails1(std::string("v2"));
  CPPUNIT_ASSERT(lumiDetails1.lumiVersion() == std::string("v2"));

  std::vector<std::string> const& v1 = lumiDetails.algoNames();
  std::vector<std::string> const& v2 = lumiDetails.algoNames();
  CPPUNIT_ASSERT(v2[0] == std::string("OCC1"));
  CPPUNIT_ASSERT(v2[1] == std::string("OCC2"));
  CPPUNIT_ASSERT(v2[2] == std::string("ET"));
  CPPUNIT_ASSERT(v2[3] == std::string("Algo3"));
  CPPUNIT_ASSERT(v2[4] == std::string("PLT1"));
  CPPUNIT_ASSERT(v2[5] == std::string("PLT2"));
  CPPUNIT_ASSERT(v1.size() == 6U);
  CPPUNIT_ASSERT(v2.size() == 6U);
}

void
TestLumiDetails::testFill() {
  LumiDetails lumiDetails;
  std::vector<float> val;
  val.push_back(1.0);
  val.push_back(2.0);
  val.push_back(3.0);

  std::vector<float> err;
  err.push_back(4.0);
  err.push_back(5.0);
  err.push_back(6.0);
 
  std::vector<short> qual;
  qual.push_back(7);
  qual.push_back(8);
  qual.push_back(9);

  std::vector<short> beam1;
  beam1.push_back(10);
  beam1.push_back(11);
  beam1.push_back(12);

  std::vector<short> beam2;
  beam2.push_back(13);
  beam2.push_back(14);
  beam2.push_back(15);

  lumiDetails.fill(2, val, err, qual, beam1, beam2);

  std::vector<float> val0;
  val0.push_back(1.0);

  std::vector<float> err0;
  err0.push_back(4.0);
 
  std::vector<short> qual0;
  qual0.push_back(7);

  std::vector<short> beam1_0;
  beam1_0.push_back(10);

  std::vector<short> beam2_0;
  beam2_0.push_back(113);

  lumiDetails.fill(0, val0, err0, qual0, beam1_0, beam2_0);

  std::vector<float> val1;
  std::vector<float> err1; 
  std::vector<short> qual1;
  std::vector<short> beam1_1;
  std::vector<short> beam2_1;
  lumiDetails.fill(1, val1, err1, qual1, beam1_1, beam2_1);

  std::vector<float> val3;
  val3.push_back(11.0);
  val3.push_back(11.0);

  std::vector<float> err3;
  err3.push_back(21.0);
  err3.push_back(21.0);
 
  std::vector<short> qual3;
  qual3.push_back(31);
  qual3.push_back(31);

  std::vector<short> beam1_3;
  beam1_3.push_back(31);
  beam1_3.push_back(31);

  std::vector<short> beam2_3;
  beam2_3.push_back(31);
  beam2_3.push_back(31);

  lumiDetails.fill(3, val3, err3, qual3, beam1_3, beam2_3);
  qual3[0] = 32;
  qual3[1] = 33;
  lumiDetails.fill(5, val3, err3, qual3, beam1_3, beam2_3);
  beam1_3[1] = 100;
  beam2_3[1] = 100;
  lumiDetails.fill(4, val3, err3, qual3, beam1_3, beam2_3);

  LumiDetails::ValueRange rangeVal = lumiDetails.lumiValuesForAlgo(2);
  std::cout << "values\n";
  int i = 1;
  for (std::vector<float>::const_iterator val = rangeVal.first;
       val != rangeVal.second; ++val, ++i) {
    std::cout << *val << " ";
    CPPUNIT_ASSERT(*val == i);
  }
  std::cout << "\n";
  CPPUNIT_ASSERT(lumiDetails.lumiValue(2,0) == 1);
  CPPUNIT_ASSERT(lumiDetails.lumiValue(2,1) == 2);
  CPPUNIT_ASSERT(lumiDetails.lumiValue(2,2) == 3);

  LumiDetails::ErrorRange rangeErr = lumiDetails.lumiErrorsForAlgo(2);
  std::cout << "errors\n";
  i = 4;
  for (std::vector<float>::const_iterator err = rangeErr.first;
       err != rangeErr.second; ++err, ++i) {
    std::cout << *err << " ";
    CPPUNIT_ASSERT(*err == i);
  }
  std::cout << "\n";
  CPPUNIT_ASSERT(lumiDetails.lumiError(2,0) == 4);
  CPPUNIT_ASSERT(lumiDetails.lumiError(2,1) == 5);
  CPPUNIT_ASSERT(lumiDetails.lumiError(2,2) == 6);

  LumiDetails::QualityRange rangeQual = lumiDetails.lumiQualitiesForAlgo(2);
  std::cout << "qualities\n";
  i = 7;
  for (std::vector<short>::const_iterator qual = rangeQual.first;
       qual != rangeQual.second; ++qual, ++i) {
    std::cout << *qual << " ";
    CPPUNIT_ASSERT(*qual == i);
  }
  std::cout << "\n";
  CPPUNIT_ASSERT(lumiDetails.lumiQuality(2,0) == 7);
  CPPUNIT_ASSERT(lumiDetails.lumiQuality(2,1) == 8);
  CPPUNIT_ASSERT(lumiDetails.lumiQuality(2,2) == 9);

  LumiDetails::Beam1IntensityRange rangeBeam1 = lumiDetails.lumiBeam1IntensitiesForAlgo(2);
  std::cout << "beam1Intensities\n";
  i = 10;
  for (std::vector<short>::const_iterator beam1 = rangeBeam1.first;
       beam1 != rangeBeam1.second; ++beam1, ++i) {
    std::cout << *beam1 << " ";
    CPPUNIT_ASSERT(*beam1 == i);
  }
  std::cout << "\n";
  CPPUNIT_ASSERT(lumiDetails.lumiBeam1Intensity(2,0) == 10);
  CPPUNIT_ASSERT(lumiDetails.lumiBeam1Intensity(2,1) == 11);
  CPPUNIT_ASSERT(lumiDetails.lumiBeam1Intensity(2,2) == 12);

  LumiDetails::Beam2IntensityRange rangeBeam2 = lumiDetails.lumiBeam2IntensitiesForAlgo(2);
  std::cout << "beam2Intensities\n";
  i = 13;
  for (std::vector<short>::const_iterator beam2 = rangeBeam2.first;
       beam2 != rangeBeam2.second; ++beam2, ++i) {
    std::cout << *beam2 << " ";
    CPPUNIT_ASSERT(*beam2 == i);
  }
  std::cout << "\n";
  CPPUNIT_ASSERT(lumiDetails.lumiBeam2Intensity(2,0) == 13);
  CPPUNIT_ASSERT(lumiDetails.lumiBeam2Intensity(2,1) == 14);
  CPPUNIT_ASSERT(lumiDetails.lumiBeam2Intensity(2,2) == 15);

  CPPUNIT_ASSERT(lumiDetails.isProductEqual(lumiDetails));

  LumiDetails lumiDetails2;
  CPPUNIT_ASSERT(!lumiDetails.isProductEqual(lumiDetails2));

  std::cout << lumiDetails;
}
