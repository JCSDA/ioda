/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <cassert>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <vector>

#include "ioda/containers/FrameCols.h"
#include "ioda/containers/FrameRows.h"
#include "ioda/containers/IFrame.h"
#include "ioda/containers/IView.h"

std::string getFramePrintText(osdf::IFrame* frame) {
  std::stringstream buffer;
  std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
  frame->print();
  std::string text = buffer.str();
  std::cout.rdbuf(old);
  return text;
}

std::string getViewPrintText(osdf::IView* view) {
  std::stringstream buffer;
  std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
  view->print();
  std::string text = buffer.str();
  std::cout.rdbuf(old);
  return text;
}

std::int32_t main() {
  // Create data vectors
  std::vector<double> lats = {-65.0, -66.6, -67.2, -68.6, -69.1,
                              -70.9, -71.132, -72.56, -73.0, -73.1};
  std::vector<double> lons = {120.0, 121.1, 122.2, 123.3, 124.4,
                              125.5, 126.6, 127.7, 128.8, 128.9};
  std::vector<std::string> statIds = {"00001", "00001", "00002", "00001", "00004",
                                      "00002", "00005", "00005", "00009", "00009"};
  std::vector<std::int32_t> channels = {10, 10, 11, 11, 12, 12, 11, 15, 11, 13};
  std::vector<double> temps = {-10.231, -15.68, -15.54, -14.98, -16.123,
                               -19.11, -22.3324, -22.667, -25.6568, -25.63211};
  std::vector<std::int32_t> times = {1710460225, 1710460225, 1710460225, 1710460225, 1710460226,
                                     1710460226, 1710460226, 1710460226, 1710460226, 1710460227};
  // Create data containers
  osdf::FrameRows frameRows1;
  osdf::FrameCols frameCols1;

  // Fill data containers
  frameRows1.appendNewColumn("lat", lats);
  frameRows1.appendNewColumn("lon", lons);
  frameRows1.appendNewColumn("StatId", statIds);
  frameRows1.appendNewColumn("channel", channels);
  frameRows1.appendNewColumn("temp", temps);
  frameRows1.appendNewColumn("time", times);

  frameCols1.appendNewColumn("lat", lats);
  frameCols1.appendNewColumn("lon", lons);
  frameCols1.appendNewColumn("StatId", statIds);
  frameCols1.appendNewColumn("channel", channels);
  frameCols1.appendNewColumn("temp", temps);
  frameCols1.appendNewColumn("time", times);

  ////////////////////////////////////////////////// Test 1: Comparison of initialisation
  std::cout << "Test 1: Comparison of initialisation - ";
  std::string textRows1 = getFramePrintText(&frameRows1);
  std::string textCols1 = getFramePrintText(&frameCols1);
  assert(textRows1 == textCols1);
  std::cout << "PASS" << std::endl;

  ////////////////////////////////////////////////// Test 2: Comparison of Frame::getColumn()
  std::cout << "Test 2: Comparison of Frame::getColumn() - ";
  std::vector<std::int32_t> vecRows1;
  std::vector<std::int32_t> vecCols1;
  frameRows1.getColumn("time", vecRows1);
  frameCols1.getColumn("time", vecCols1);
  assert(vecRows1 == vecCols1);
  std::cout << "PASS" << std::endl;

  ////////////////////////////////////////////////// Test 3: Comparison of Frame::setColumn()
  std::cout << "Test 3: Comparison of Frame::setColumn() - ";
  std::vector<std::string> vec3{"3", "3", "3", "3", "3", "3", "3", "3", "3", "3"};
  frameRows1.setColumn("StatId", vec3);
  frameCols1.setColumn("StatId", vec3);
  std::string textRows3 = getFramePrintText(&frameRows1);
  std::string textCols3 = getFramePrintText(&frameCols1);
  assert(textRows3 == textCols3);
  std::cout << "PASS" << std::endl;

  ////////////////////////////////////////////////// Test 4: Comparison of Frame::removeColumn()
  std::cout << "Test 4: Comparison of Frame::removeColumn() - ";
  frameRows1.removeColumn("StatId");
  frameCols1.removeColumn("StatId");
  std::string textRows4 = getFramePrintText(&frameRows1);
  std::string textCols4 = getFramePrintText(&frameCols1);
  assert(textRows4 == textCols4);
  std::cout << "PASS" << std::endl;

  ////////////////////////////////////////////////// Test 5: Comparison of Frame::removeRow()
  std::cout << "Test 5: Comparison of Frame::removeRow() - ";
  std::int64_t rowIndex = 5;
  frameRows1.removeRow(rowIndex);
  frameCols1.removeRow(rowIndex);
  std::string textRows5 = getFramePrintText(&frameRows1);
  std::string textCols5 = getFramePrintText(&frameCols1);
  assert(textRows5 == textCols5);
  std::cout << "PASS" << std::endl;

  ////////////////////////////////////////////////// Test 6: Comparison of Frame::appendRow()
  std::cout << "Test 6: Comparison of Frame::appendRow() - ";
  frameRows1.appendNewRow(-74., 129., 15, -25.6567, 1710460300);
  frameCols1.appendNewRow(-74., 129., 15, -25.6567, 1710460300);
  std::string textRows6 = getFramePrintText(&frameRows1);
  std::string textCols6 = getFramePrintText(&frameCols1);
  assert(textRows6 == textCols6);
  std::cout << "PASS" << std::endl;

  ////////////////////////////////////////////////// Test 7: Comparison of Frame::sortRows()
  std::cout << "Test 7: Comparison of Frame::sortRows() - ";
  frameRows1.sortRows("channel", osdf::consts::eDescending);
  frameCols1.sortRows("channel", osdf::consts::eDescending);
  std::string textRows7 = getFramePrintText(&frameRows1);
  std::string textCols7 = getFramePrintText(&frameCols1);
  assert(textRows7 == textCols7);
  std::cout << "PASS" << std::endl;

  ////////////////////////////////////////////////// Test 8: Comparison of Frame::sliceRows()
  std::cout << "Test 8: Comparison of Frame::sliceRows() - ";
  osdf::FrameRows frameRows2 = frameRows1.sliceRows("lat", osdf::consts::eLessThan, -70.);
  osdf::FrameCols frameCols2 = frameCols1.sliceRows("lat", osdf::consts::eLessThan, -70.);
  std::string textRows8 = getFramePrintText(&frameRows2);
  std::string textCols8 = getFramePrintText(&frameCols2);
  assert(textRows8 == textCols8);
  std::cout << "PASS" << std::endl;

  ////////////////////////////////////////////////// Test 9: Comparison of Frame::makeView()
  std::cout << "Test 9: Comparison of Frame::makeView() - ";
  osdf::ViewRows viewRows1 = frameRows1.makeView();
  osdf::ViewCols viewCols1 = frameCols1.makeView();
  std::string textFrameRows9 = getFramePrintText(&frameRows1);
  std::string textFrameCols9 = getFramePrintText(&frameCols1);
  std::string textViewRows9 = getViewPrintText(&viewRows1);
  std::string textViewCols9 = getViewPrintText(&viewCols1);
  assert(textFrameRows9 == textFrameCols9);
  assert(textViewRows9 == textViewCols9);
  std::cout << "PASS" << std::endl;

  ////////////////////////////////////////////////// Test 10: Comparison of View::getColumn()
  std::cout << "Test 10: Comparison of View::getColumn() - ";
  std::vector<std::int32_t> vecRows2;
  std::vector<std::int32_t> vecCols2;
  viewRows1.getColumn("time", vecRows2);
  viewCols1.getColumn("time", vecCols2);
  std::string textFrameRows10 = getFramePrintText(&frameRows1);
  std::string textFrameCols10 = getFramePrintText(&frameCols1);
  assert(vecRows2 == vecCols2);
  assert(textFrameRows10 == textFrameCols10);
  std::cout << "PASS" << std::endl;

  ////////////////////////////////////////////////// Test 11: Comparison of FrameRows <-> FrameCols
  std::cout << "Test 11: Comparison of FrameRows <-> FrameCols - ";
  osdf::FrameRows frameRows3(frameCols1);
  osdf::FrameCols frameCols3(frameRows1);
  std::string textRows11 = getFramePrintText(&frameRows3);
  std::string textCols11 = getFramePrintText(&frameCols3);
  assert(textRows11 == textCols11);
  std::cout << "PASS" << std::endl;

  return 0;
}
