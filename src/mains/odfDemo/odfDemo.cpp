/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <cstdint>
#include <string>
#include <vector>

#include "oops/util/Logger.h"

#include "ioda/containers/ColumnMetadatum.h"
#include "ioda/containers/Constants.h"
#include "ioda/containers/FrameCols.h"
#include "ioda/containers/FrameRows.h"
#include "ioda/containers/ViewRows.h"

#include "ioda/containers/Datum.h"

std::int32_t main() {
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

  oops::Log::info() << std::endl << "### FrameRows ################################################"
                    << std::endl;
  osdf::FrameRows frameRows;

  // frameRows.configColumns({{"lat", osdf::consts::eDouble},
  //                          {"lon", osdf::consts::eDouble},
  //                          {"StatId", osdf::consts::eString},
  //                          {"channel", osdf::consts::eInt32},
  //                          {"temp", osdf::consts::eDouble},
  //                          {"time", osdf::consts::eInt32}});

  // frameRows.configColumns({osdf::ColumnMetadatum("lat", osdf::consts::eDouble),
  //                          osdf::ColumnMetadatum("lon", osdf::consts::eDouble),
  //                          osdf::ColumnMetadatum("StatId", osdf::consts::eString),
  //                          osdf::ColumnMetadatum("channel", osdf::consts::eInt32),
  //                          osdf::ColumnMetadatum("temp", osdf::consts::eDouble),
  //                          osdf::ColumnMetadatum("time", osdf::consts::eInt32)});

  frameRows.appendNewRow(-73., 128., "00000", 11, -25.6568, 1710460200);
  frameRows.print();

  frameRows.appendNewColumn("lat", lats);
  frameRows.appendNewColumn("lon", lons);
  frameRows.appendNewColumn("StatId", statIds);
  frameRows.appendNewColumn("channel", channels);
  frameRows.appendNewColumn("temp", temps);
  frameRows.appendNewColumn("time", times);
  frameRows.print();

  frameRows.appendNewRow("00010", 11, -25.6568, 1710460270);
  frameRows.appendNewRow(-73, 128, -73, 128, "00010", 11, -25.6568, 1710460280);
  frameRows.appendNewRow("00010", -73, 128, 11, -25.6568, 1710460290);
  frameRows.appendNewRow(-73., 128., "00010", 14, -25.6568, 1710460300);
  frameRows.print();

  oops::Log::info() << std::endl << "getColumn" << std::endl;
  std::vector<std::int32_t> vec;
  frameRows.getColumn("time", vec);

  oops::Log::info() << std::endl << "setColumn" << std::endl;
  std::vector<std::string> wec{"3", "3"};
  frameRows.setColumn("StatId", wec);

  wec = {"3", "3", "3", "3", "3", "3", "3", "3", "3", "3", "3"};
  frameRows.setColumn("StatId", wec);
  frameRows.print();

  oops::Log::info() << std::endl << "removeColumn" << std::endl;
  frameRows.removeColumn("StatId");
  frameRows.print();

  oops::Log::info() << std::endl << "removeRow" << std::endl;
  frameRows.removeRow(0);
  frameRows.appendNewRow(-73., 128., 14, -25.6568, 1710460301);
  frameRows.print();
  oops::Log::info() << std::endl << "removeRow2" << std::endl;
  frameRows.removeRow(9);
  frameRows.appendNewRow(-74., 129., 15, -25.6567, 1710460302);
  frameRows.print();

  oops::Log::info() << std::endl << "sort 1" << std::endl;
  frameRows.sortRows("channel", osdf::consts::eDescending);
  frameRows.print();

  oops::Log::info() << std::endl << "sort 2" << std::endl;
  frameRows.sortRows("channel", [&](std::shared_ptr<osdf::DatumBase> datumA,
                                    std::shared_ptr<osdf::DatumBase> datumB){
    std::shared_ptr<osdf::Datum<std::int32_t>> datumAType =
                                std::static_pointer_cast<osdf::Datum<std::int32_t>>(datumA);
    std::shared_ptr<osdf::Datum<std::int32_t>> datumBType =
                                std::static_pointer_cast<osdf::Datum<std::int32_t>>(datumB);
    return datumAType->getValue() < datumBType->getValue();
  });
  frameRows.print();

  oops::Log::info() << std::endl << "test slice 1" << std::endl;
  frameRows.sliceRows("lat", osdf::consts::eLessThan, -70.).print();

  oops::Log::info() << std::endl << "test slice 2" << std::endl;
  frameRows.sliceRows([&](const osdf::DataRow& dataRow) {
    std::shared_ptr<osdf::DatumBase> datum = dataRow.getColumn(0);
    std::shared_ptr<osdf::Datum<double>> datumType =
                                         std::static_pointer_cast<osdf::Datum<double>>(datum);
    return datumType->getValue() < -70.;
  }).print();

  // oops::Log::info() << std::endl << "clear" << std::endl;
  // frameRows.clear();
  // frameRows.print();

  oops::Log::info() << std::endl << "### FrameCols ################################################"
                     << std::endl;
  osdf::FrameCols frameCols;

  // frameCols.configColumns({{"lat", osdf::consts::eDouble},
  //                          {"lon", osdf::consts::eDouble},
  //                          {"StatId", osdf::consts::eString},
  //                          {"channel", osdf::consts::eInt32},
  //                          {"temp", osdf::consts::eDouble},
  //                          {"time", osdf::consts::eInt32}});

  // frameCols.configColumns({osdf::ColumnMetadatum("lat", osdf::consts::eDouble),
  //                          osdf::ColumnMetadatum("lon", osdf::consts::eDouble),
  //                          osdf::ColumnMetadatum("StatId", osdf::consts::eString),
  //                          osdf::ColumnMetadatum("channel", osdf::consts::eInt32),
  //                          osdf::ColumnMetadatum("temp", osdf::consts::eDouble),
  //                          osdf::ColumnMetadatum("time", osdf::consts::eInt32)});

  frameCols.appendNewRow(-73., 128., "00000", 11, -25.6568, 1710460200);
  frameCols.print();

  frameCols.appendNewColumn("lat", lats);
  frameCols.appendNewColumn("lon", lons);
  frameCols.appendNewColumn("StatId", statIds);
  frameCols.appendNewColumn("channel", channels);
  frameCols.appendNewColumn("temp", temps);
  frameCols.appendNewColumn("time", times);
  frameCols.print();

  frameCols.appendNewRow("00010", 11, -25.6568, 1710460270);
  frameCols.appendNewRow(-73, 128, -73, 128, "00010", 11, -25.6568, 1710460280);
  frameCols.appendNewRow("00010", -73, 128, 11, -25.6568, 1710460290);
  frameCols.appendNewRow(-73., 128., "00010", 14, -25.6568, 1710460300);
  frameCols.print();

  oops::Log::info() << std::endl << "getColumn" << std::endl;
  std::vector<std::int32_t> vec2;
  frameCols.getColumn("time", vec2);

  oops::Log::info() << std::endl << "setColumn" << std::endl;
  std::vector<std::string> wec2{"3", "3"};
  frameCols.setColumn("StatId", wec2);

  wec2 = {"3", "3", "3", "3", "3", "3", "3", "3", "3", "3", "3"};
  frameCols.setColumn("StatId", wec2);
  frameCols.print();

  oops::Log::info() << std::endl << "removeColumn" << std::endl;
  frameCols.removeColumn("StatId");
  frameCols.print();

  oops::Log::info() << std::endl << "removeRow" << std::endl;
  frameCols.removeRow(0);
  frameCols.appendNewRow(-73., 128., 14, -25.6568, 1710460301);
  frameCols.print();
  oops::Log::info() << std::endl << "removeRow2" << std::endl;
  frameCols.removeRow(9);
  frameCols.appendNewRow(-74., 129., 15, -25.6567, 1710460302);
  frameCols.print();

  oops::Log::info() << std::endl << "sort 1" << std::endl;
  frameCols.sortRows("channel", osdf::consts::eAscending);
  frameCols.print();

  oops::Log::info() << std::endl << "test slice 1" << std::endl;
  frameCols.sliceRows("lat", osdf::consts::eLessThan, -70.).print();

  // oops::Log::info() << std::endl << "clear" << std::endl;
  // frameCols.clear();
  // frameCols.print();

  oops::Log::info() << std::endl << "### ViewRows #################################################"
             << std::endl;
  osdf::ViewRows viewRows = frameRows.makeView();
  viewRows.print();

  oops::Log::info() << std::endl << "getColumn" << std::endl;
  std::vector<std::int32_t> vec3;
  viewRows.getColumn("time", vec3);

  oops::Log::info() << std::endl << "sort 1" << std::endl;
  viewRows.sortRows("channel", osdf::consts::eDescending);
  viewRows.print();

  oops::Log::info() << std::endl << "sort 2" << std::endl;
  viewRows.sortRows("channel", [&](std::shared_ptr<osdf::DatumBase> datumA,
                               std::shared_ptr<osdf::DatumBase> datumB){
    std::shared_ptr<osdf::Datum<std::int32_t>> datumAType =
                                std::static_pointer_cast<osdf::Datum<std::int32_t>>(datumA);
    std::shared_ptr<osdf::Datum<std::int32_t>> datumBType =
                                std::static_pointer_cast<osdf::Datum<std::int32_t>>(datumB);
    return datumAType->getValue() < datumBType->getValue();
  });
  viewRows.print();

  oops::Log::info() << std::endl << "test slice 1" << std::endl;
  viewRows.sliceRows("lat", osdf::consts::eLessThan, -70.).print();

  oops::Log::info() << std::endl << "test slice 2" << std::endl;
  frameRows.sliceRows([&](const osdf::DataRow& dataRow) {
    std::shared_ptr<osdf::DatumBase> datum = dataRow.getColumn(0);
    std::shared_ptr<osdf::Datum<double>> datumType =
                                         std::static_pointer_cast<osdf::Datum<double>>(datum);
    return datumType->getValue() < -70.;
  }).print();

  oops::Log::info() << std::endl << "### ViewCols #################################################"
             << std::endl;
  osdf::ViewCols viewCols = frameCols.makeView();
  viewCols.print();

  oops::Log::info() << std::endl << "getColumn" << std::endl;
  std::vector<std::int32_t> vec4;
  viewCols.getColumn("time", vec4);

  // Sort on ViewCols modified original container and so has been removed
  // oops::Log::info() << std::endl << "sort 1" << std::endl;
  // viewCols.sortRows("channel", osdf::consts::eAscending);
  // viewCols.print();

  oops::Log::info() << std::endl << "test slice 1" << std::endl;
  viewCols.sliceRows("lat", osdf::consts::eLessThan, -70.).print();

  oops::Log::info() << std::endl << "### FrameRows(FrameCols) #####################################"
             << std::endl;
  osdf::FrameRows frameRows3(frameCols);
  frameRows3.print();

  oops::Log::info() << std::endl << "### FrameCols(FrameRows) #####################################"
                    << std::endl;
  osdf::FrameCols frameCols3(frameRows);
  frameCols3.print();

  return 0;
}
