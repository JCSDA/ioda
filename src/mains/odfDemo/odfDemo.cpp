/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <cstdint>
#include <string>
#include <vector>

#include "ioda/containers/ColumnMetadatum.h"
#include "ioda/containers/Constants.h"
#include "ioda/containers/ObsDataFrameCols.h"
#include "ioda/containers/ObsDataFrameRows.h"

#include "ioda/containers/Datum.h"

#include "oops/util/Logger.h"

std::int32_t main() {
  /// Row Priority /////////////////////////////////////////////////////////////////////////////////
  oops::Log::info()<< std::endl << "### ObsDataFrameRows ###############"
                                   "####################################" << std::endl;
  osdf::ObsDataFrameRows dfRows;

  // dfRows.addColumnMetadata({{"lat", consts::eDouble},
  //                          {"lon", consts::eDouble},
  //                          {"StatId", consts::eString},
  //                          {"channel", consts::eInt32},
  //                          {"temp", consts::eDouble},
  //                          {"time", consts::eInt32}});

  // dfRows.addColumnMetadata({ColumnMetadatum("lat", consts::eDouble),
  //                          ColumnMetadatum("lon", 1),
  //                          ColumnMetadatum("StatId", 1),
  //                          ColumnMetadatum("channel", 1),
  //                          ColumnMetadatum("temp", 1)});

  // std::vector<ColumnMetadatum> cols;
  // cols.push_back(ColumnMetadatum("lat", consts::eDouble));
  // cols.push_back(ColumnMetadatum("lon", consts::eDouble));
  // cols.push_back(ColumnMetadatum("StatId", consts::eString));
  // cols.push_back(ColumnMetadatum("channel", consts::eInt32));
  // cols.push_back(ColumnMetadatum("temp", consts::eDouble));
  // cols.push_back(ColumnMetadatum("time", consts::eInt32));
  // dfRows.configColumns(cols);

  // Fail, without existing columns
  dfRows.appendNewRow(-73., 128., "00000", 11, -25.6568, 1710460200);

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

  dfRows.appendNewColumn("lat", lats);                                         // Pass
  dfRows.appendNewColumn("lon", lons);                                         // Pass
  dfRows.appendNewColumn("StatId", statIds);                                   // Pass
  dfRows.appendNewColumn("channel", channels);                                 // Pass
  dfRows.appendNewColumn("temp", temps);                                       // Pass
  dfRows.appendNewColumn("time", times);                                       // Pass
  dfRows.print();

  // Fail - Too fiew columns
  dfRows.appendNewRow("00010", 11, -25.6568, 1710460270);
  // Fail - too many columns
  dfRows.appendNewRow(-73, 128, -73, 128, "00010", 11, -25.6568, 1710460280);
  // Fail - wrong order
  dfRows.appendNewRow("00010", -73, 128, 11, -25.6568, 1710460290);
  // Pass - with read-write columns
  dfRows.appendNewRow(-73., 128., "00010", 14, -25.6568, 1710460300);
  dfRows.print();

  oops::Log::info()<< std::endl << "getColumn" << std::endl;
  std::vector<std::string> vec;
  dfRows.getColumn("StatId", vec);  // Pass - with correct name/case and data type
  dfRows.print();

  oops::Log::info()<< std::endl << "setColumn" << std::endl;
  std::vector<std::string> vec2{"3", "3"};
  dfRows.setColumn("StatId", vec2);

  std::vector<std::string> vec3{"3", "3", "3", "3", "3", "3", "3", "3", "3", "3", "3"};
  dfRows.setColumn("StatId", vec3);
  dfRows.print();

  oops::Log::info()<< std::endl << "removeColumn" << std::endl;
  dfRows.removeColumn("StatId");
  dfRows.print();

  oops::Log::info() << std::endl << "removeRow" << std::endl;
  dfRows.removeRow(0);
  dfRows.appendNewRow(-73., 128., 14, -25.6568, 1710460301);
  dfRows.print();
  oops::Log::info() << std::endl << "removeRow2" << std::endl;
  dfRows.removeRow(9);
  dfRows.appendNewRow(-74., 129., 15, -25.6567, 1710460302);
  dfRows.print();

  oops::Log::info() << std::endl << "sort 1" << std::endl;
  dfRows.sort("channel", osdf::consts::eDescending);
  dfRows.print();

  oops::Log::info() << std::endl << "sort 2" << std::endl;
  dfRows.sort("channel", [&](std::shared_ptr<osdf::DatumBase> datumA,
              std::shared_ptr<osdf::DatumBase> datumB){
    std::shared_ptr<osdf::Datum<std::int32_t>> datumAInt32 =
        std::static_pointer_cast<osdf::Datum<std::int32_t>>(datumA);
    std::shared_ptr<osdf::Datum<std::int32_t>> datumBInt32 =
        std::static_pointer_cast<osdf::Datum<std::int32_t>>(datumB);
    return datumAInt32->getDatum() < datumBInt32->getDatum();
  });
  dfRows.print();

  oops::Log::info() << std::endl << "test slice 1" << std::endl;
  std::shared_ptr<osdf::ObsDataFrame> test1 = dfRows.slice("lat", osdf::consts::eLessThan, -70.);
  test1->print();

  oops::Log::info() << std::endl << "test slice 2" << std::endl;
  std::shared_ptr<osdf::ObsDataFrame> test2 = dfRows.slice([&](const osdf::DataRow& dataRow) {
    std::shared_ptr<osdf::DatumBase> datum = dataRow.getColumn(0);
    std::shared_ptr<osdf::Datum<double>> datumDouble =
      std::static_pointer_cast<osdf::Datum<double>>(datum);
    double datumValue = datumDouble->getDatum();
    return datumValue < -70.;
  });
  test2->print();

  /// Column Priority //////////////////////////////////////////////////////////////////////////////
  oops::Log::info() << std::endl << "### ObsDataFrameCols ###############"
                                    "####################################" << std::endl;

  osdf::ObsDataFrameCols dfCols;

  dfCols.appendNewColumn("lat", lats);                                         // Pass
  dfCols.appendNewColumn("lon", lons);                                         // Pass
  dfCols.appendNewColumn("StatId", statIds);                                   // Pass
  dfCols.appendNewColumn("channel", channels);                                 // Pass
  dfCols.appendNewColumn("temp", temps);                                       // Pass
  dfCols.appendNewColumn("time", times);                                       // Pass
  dfCols.print();

  // Fail - Too fiew columns
  dfCols.appendNewRow("00010", 11, -25.6568, 1710460270);
  // Fail - too many columns
  dfCols.appendNewRow(-73, 128, -73, 128, "00010", 11, -25.6568, 1710460280);
  // Fail - wrong order
  dfCols.appendNewRow("00010", -73, 128, 11, -25.6568, 1710460290);
  // Pass - with read-write columns
  dfCols.appendNewRow(-73., 128., "00010", 14, -25.6568, 1710460300);
  dfCols.print();

  oops::Log::info()<< std::endl << "getColumn" << std::endl;
  std::vector<std::string> vec4;
  dfCols.getColumn("StatId", vec4);  // Pass - with correct name/case and data type
  dfCols.print();

  oops::Log::info()<< std::endl << "setColumn" << std::endl;
  std::vector<std::string> vec5{"3", "3"};
  dfCols.setColumn("StatId", vec5);

  std::vector<std::string> vec6{"3", "3", "3", "3", "3", "3", "3", "3", "3", "3", "3"};
  dfCols.setColumn("StatId", vec6);
  dfCols.print();

  oops::Log::info()<< std::endl << "removeColumn" << std::endl;
  dfCols.removeColumn("StatId");
  dfCols.print();

  oops::Log::info()<< std::endl << "removeRow" << std::endl;
  dfCols.removeRow(0);
  dfCols.appendNewRow(-73., 128., 14, -25.6568, 1710460301);
  dfCols.print();
  oops::Log::info() << std::endl << "removeRow2" << std::endl;
  dfCols.removeRow(9);
  dfCols.appendNewRow(-74., 129., 15, -25.6567, 1710460302);
  dfCols.print();

  oops::Log::info() << std::endl << "sort 1" << std::endl;
  dfCols.sort("channel", osdf::consts::eDescending);
  dfCols.print();

  oops::Log::info() << std::endl << "test slice 1" << std::endl;
  std::shared_ptr<osdf::ObsDataFrame> test3 = dfCols.slice("lat", osdf::consts::eLessThan, -70.);
  test3->print();

  return 0;
}
