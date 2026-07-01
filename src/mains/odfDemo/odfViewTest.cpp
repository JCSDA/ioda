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

#include "ioda/containers/Datum.h"
#include "ioda/containers/FrameCols.h"
#include "ioda/containers/FrameRows.h"
#include "ioda/containers/IFrame.h"
#include "ioda/containers/IView.h"
#include "ioda/containers/ViewCols.h"
#include "ioda/containers/ViewRows.h"

void makeViewCols(osdf::FrameCols& frameCols) {
  osdf::ViewCols viewCols = frameCols.makeView();
}

void makeViewRows(osdf::FrameRows& frameRows) {
  osdf::ViewRows viewRows = frameRows.makeView();
}

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

int main() {
  // Create data vectors
  std::vector<float> lats = {-65.0, -66.6, -67.2, -68.6, -69.1,
                             -70.9, -71.132, -72.56, -73.0, -73.1};
  std::vector<float> lons = {120.0, 121.1, 122.2, 123.3, 124.4,
                             125.5, 126.6, 127.7, 128.8, 128.9};
  std::vector<std::string> statIds = {"00001", "00001", "00002", "00001", "00004",
                                      "00002", "00005", "00005", "00009", "00009"};
  std::vector<int> channels = {10, 10, 11, 11, 12, 12, 11, 15, 11, 13};
  std::vector<float> temps = {-10.231, -15.68, -15.54, -14.98, -16.123,
                              -19.11, -22.3324, -22.667, -25.6568, -25.63211};
  std::vector<std::int64_t> times = {1710460225, 1710460225, 1710460225, 1710460225, 1710460226,
                                     1710460226, 1710460226, 1710460226, 1710460226, 1710460227};
  // Create data containers
  osdf::FrameCols frameCols;
  osdf::FrameRows frameRows;

  // Fill data containers
  frameCols.appendNewColumn("lat", lats);
  frameCols.appendNewColumn("lon", lons);
  frameCols.appendNewColumn("StatId", statIds);
  frameCols.appendNewColumn("channel", channels);
  frameCols.appendNewColumn("temp", temps);
  frameCols.appendNewColumn("time", times);

  frameRows.appendNewColumn("lat", lats);
  frameRows.appendNewColumn("lon", lons);
  frameRows.appendNewColumn("StatId", statIds);
  frameRows.appendNewColumn("channel", channels);
  frameRows.appendNewColumn("temp", temps);
  frameRows.appendNewColumn("time", times);

  ////////////////////////////////////////////////// 1. Data population
  osdf::ViewRows viewRows1 = frameRows.makeView();
  osdf::ViewCols viewCols1 = frameCols.makeView();

  const std::string textFrameRows1 = getFramePrintText(&frameRows);
  const std::string textFrameCols1 = getFramePrintText(&frameCols);
  const std::string textViewRows1 = getViewPrintText(&viewRows1);
  const std::string textViewCols1 = getViewPrintText(&viewCols1);

  std::cout << "1. Data population - ";
  assert(textFrameRows1 == textFrameCols1);
  assert(textFrameRows1 == textViewRows1);
  assert(textFrameRows1 == textViewCols1);
  assert(textFrameCols1 == textViewRows1);
  assert(textFrameCols1 == textViewCols1);
  assert(textViewRows1 == textViewCols1);
  std::cout << "PASS" << std::endl;

  ////////////////////////////////////////////////// 2. Slice of ViewRows and ViewCols
  osdf::ViewRows viewRows2 = viewRows1.sliceRows("lat", osdf::consts::eLessThan, -70.0f);
  osdf::ViewCols viewCols2 = viewCols1.sliceRows("lat", osdf::consts::eLessThan, -70.0f);

  const std::string textFrameRows2 = getFramePrintText(&frameRows);
  const std::string textFrameCols2 = getFramePrintText(&frameCols);
  const std::string textViewRows2 = getViewPrintText(&viewRows2);
  const std::string textViewCols2 = getViewPrintText(&viewCols2);

  std::cout << "2. Slice of ViewRows and ViewCols - ";
  assert(textFrameRows2 == textFrameCols2);
  assert(textFrameRows2 != textViewRows2);
  assert(textFrameRows2 != textViewCols2);
  assert(textFrameCols2 != textViewRows2);
  assert(textFrameCols2 != textViewCols2);
  assert(textViewRows2 == textViewCols2);
  std::cout << "PASS" << std::endl;

  ////////////////////////////////////////////////// 3. Sort of ViewRows
  viewRows1.sortRows("channel", osdf::consts::eAscending);
  const std::string textFrameRows3 = getFramePrintText(&frameRows);
  const std::string textFrameCols3 = getFramePrintText(&frameCols);
  const std::string textViewRows3a = getViewPrintText(&viewRows1);
  const std::string textViewCols3 = getViewPrintText(&viewCols1);

  viewRows1.sortRows("channel", osdf::consts::eDescending);
  const std::string textViewRows3b = getViewPrintText(&viewRows1);

  viewRows1.sortRows("channel", [&](std::shared_ptr<osdf::DatumBase> datumA,
                                    std::shared_ptr<osdf::DatumBase> datumB) {
    std::shared_ptr<osdf::Datum<int>> datumAType =
                                std::static_pointer_cast<osdf::Datum<int>>(datumA);
    std::shared_ptr<osdf::Datum<int>> datumBType =
                                std::static_pointer_cast<osdf::Datum<int>>(datumB);
    return datumAType->getValue() < datumBType->getValue();
  });
  const std::string textViewRows3c = getViewPrintText(&viewRows1);

  std::cout << "3. Sort of ViewRows - ";
  assert(textFrameRows3 == textFrameCols3);
  assert(textFrameRows3 != textViewRows3a);
  assert(textFrameRows3 == textViewCols3);
  assert(textFrameCols3 != textViewRows3a);
  assert(textFrameCols3 == textViewCols3);
  assert(textViewRows3a != textViewCols3);
  assert(textViewRows3a != textViewRows3b);
  assert(textViewRows3a == textViewRows3c);
  assert(textViewRows3b != textViewRows3c);

  std::cout << "PASS" << std::endl;

  ////////////////////////////////////////////////// 4. Data modification
  osdf::ViewCols viewCols3 = frameCols.makeView();
  osdf::ViewRows viewRows3 = frameRows.makeView();

  osdf::ViewCols viewCols4 = viewCols3.sliceRows("lat", osdf::consts::eLessThan, -70.0f);
  osdf::ViewRows viewRows4 = viewRows3.sliceRows("lat", osdf::consts::eLessThan, -70.0f);

  frameCols.appendNewRow(-73.0f, 128.0f, "00010", 66, -25.6568f, static_cast<int64_t>(1710460300L));
  frameRows.appendNewRow(-73.0f, 128.0f, "00010", 66, -25.6568f, static_cast<int64_t>(1710460300L));

  std::vector<std::string> vec = {"3", "3", "3", "3", "3", "3", "3", "3", "3", "3", "3"};
  frameCols.setColumn("StatId", vec);
  frameRows.setColumn("StatId", vec);

  const std::string textFrameRows4 = getFramePrintText(&frameRows);
  const std::string textFrameCols4 = getFramePrintText(&frameCols);
  const std::string textViewRows4 = getViewPrintText(&viewRows3);
  const std::string textViewCols4 = getViewPrintText(&viewCols3);
  const std::string textViewRows4a = getViewPrintText(&viewRows4);
  const std::string textViewCols4a = getViewPrintText(&viewCols4);

  std::cout << "4. Data modification - ";
  assert(textFrameRows4 == textFrameCols4);
  assert(textFrameRows4 == textViewRows4);
  assert(textFrameRows4 == textViewCols4);
  assert(textFrameCols4 == textViewRows4);
  assert(textFrameCols4 == textViewCols4);
  assert(textViewRows4 == textViewCols4);
  assert(textViewRows4 == textViewCols4a);
  assert(textViewRows4a == textViewCols4);
  assert(textViewRows4a == textViewCols4a);
  std::cout << "PASS" << std::endl;

  ////////////////////////////////////////////////// 5. Data clearance
  frameCols.clear();
  frameRows.clear();

  const std::string textFrameRows5 = getFramePrintText(&frameRows);
  const std::string textFrameCols5 = getFramePrintText(&frameCols);
  const std::string textViewRows5 = getViewPrintText(&viewRows1);
  const std::string textViewCols5 = getViewPrintText(&viewCols1);
  const std::string textViewRows5a = getViewPrintText(&viewRows3);
  const std::string textViewCols5a = getViewPrintText(&viewCols3);
  const std::string textViewRows5b = getViewPrintText(&viewRows4);
  const std::string textViewCols5b = getViewPrintText(&viewCols4);

  std::cout << "5. Data clearance - ";
  assert(textFrameRows5 == textFrameCols5);
  assert(textFrameRows5 == textViewRows5);
  assert(textFrameRows5 == textViewCols5);
  assert(textFrameCols5 == textViewRows5);
  assert(textFrameCols5 == textViewCols5);
  assert(textViewRows5 == textViewCols5);
  assert(textViewRows5a == textViewCols5);
  assert(textViewRows5 == textViewCols5a);
  assert(textViewRows5a == textViewCols5a);
  assert(textViewRows5a == textViewCols5b);
  assert(textViewRows5b == textViewCols5a);
  assert(textViewRows5b == textViewCols5b);
  std::cout << "PASS" << std::endl;

  ////////////////////////////////////////////////// 6. View out-of-scope

  makeViewCols(frameCols);
  makeViewRows(frameRows);

  frameCols.appendNewColumn("lat", lats);
  frameCols.appendNewColumn("lon", lons);
  frameCols.appendNewColumn("StatId", statIds);
  frameCols.appendNewColumn("channel", channels);
  frameCols.appendNewColumn("temp", temps);
  frameCols.appendNewColumn("time", times);

  frameRows.appendNewColumn("lat", lats);
  frameRows.appendNewColumn("lon", lons);
  frameRows.appendNewColumn("StatId", statIds);
  frameRows.appendNewColumn("channel", channels);
  frameRows.appendNewColumn("temp", temps);
  frameRows.appendNewColumn("time", times);

  const std::string textFrameRows6 = getFramePrintText(&frameRows);
  const std::string textFrameCols6 = getFramePrintText(&frameCols);

  std::cout << "6. View out-of-scope - ";
  assert(textFrameRows1 == textFrameCols1);
  assert(textFrameRows1 == textFrameCols6);
  assert(textFrameRows6 == textFrameCols1);
  assert(textFrameRows6 == textFrameCols6);
  std::cout << "PASS" << std::endl;

  return 0;
}
