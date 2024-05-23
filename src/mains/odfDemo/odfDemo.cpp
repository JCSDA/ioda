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
#include "ioda/containers/ObsDataFrameRows.h"

#include "ioda/containers/Datum.h"

std::int32_t main() {
  ObsDataFrameRows dfRow;

  // dfRow.addColumnMetadata({{"lat", consts::eDouble},
  //                          {"lon", consts::eDouble},
  //                          {"StatId", consts::eString},
  //                          {"channel", consts::eInt32},
  //                          {"temp", consts::eDouble},
  //                          {"time", consts::eInt32}});

  // dfRow.addColumnMetadata({ColumnMetadatum("lat", consts::eDouble),
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
  // dfRow.configColumns(cols);

  dfRow.appendNewRow(-73., 128., "00000", 11, -25.6568, 1710460200);  // Fail, without
                                                                      // existing columns

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

  dfRow.appendNewColumn("lat", lats);               // Pass
  dfRow.appendNewColumn("lon", lons);               // Pass
  dfRow.appendNewColumn("StatId", statIds);         // Pass
  dfRow.appendNewColumn("channel", channels);       // Pass
  dfRow.appendNewColumn("temp", temps);             // Pass
  dfRow.appendNewColumn("time", times);             // Pass

  // Fail - too few columns
  dfRow.appendNewRow("00010", 11, -25.6568, 1710460270);
  // Fail - too many columns
  dfRow.appendNewRow(-73, 128, -73, 128, "00010", 11, -25.6568, 1710460280);
  // Fail - wrong order
  dfRow.appendNewRow("00010", -73, 128, 11, -25.6568, 1710460290);
  // Pass - with read-write columns
  dfRow.appendNewRow(-73., 128., "00010", 14, -25.6568, 1710460300);

  std::cout << std::endl << "getColumn" << std::endl;
  std::vector<std::string> vec;
  dfRow.getColumn("StatId", vec);              // Pass - with correct name/case and data type
  dfRow.print();

  std::cout << std::endl << "setColumn" << std::endl;
  std::vector<std::string> vec2{"3", "3"};
  dfRow.setColumn("StatId", vec2);
  dfRow.print();

  std::cout << std::endl << "removeColumn" << std::endl;
  dfRow.removeColumn("StatId");
  dfRow.print();

  std::cout  << std::endl << "removeRow" << std::endl;
  dfRow.removeRow(0);
  dfRow.print();

  std::cout  << std::endl << "sort 1" << std::endl;
  dfRow.sort("channel", consts::eDescending);
  dfRow.print();

  std::cout  << std::endl << "sort 2" << std::endl;
  std::int32_t columnIndex = 2;
  dfRow.sort([&](DataRow& dataRowA, DataRow& dataRowB){
    std::shared_ptr<DatumBase> datumA = dataRowA.getColumn(columnIndex);
    std::shared_ptr<DatumBase> datumB = dataRowB.getColumn(columnIndex);

    std::shared_ptr<Datum<std::int32_t>> datumAInt32 =
            std::static_pointer_cast<Datum<std::int32_t>>(datumA);
    std::shared_ptr<Datum<std::int32_t>> datumBInt32 =
            std::static_pointer_cast<Datum<std::int32_t>>(datumB);
    return datumAInt32->getDatum() < datumBInt32->getDatum();
  });
  dfRow.print();

  std::cout  << std::endl << "test slice 1" << std::endl;
  std::shared_ptr<ObsDataFrame> test1 = dfRow.slice("lat", consts::eLessThan, -70.);
  std::shared_ptr<ObsDataFrameRows> testRows1 = std::static_pointer_cast<ObsDataFrameRows>(test1);
  test1->print();

  std::cout  << std::endl << "test slice 2" << std::endl;
  std::shared_ptr<ObsDataFrame> test2 = dfRow.slice([&](DataRow& dataRow) {
    std::shared_ptr<DatumBase> datum = dataRow.getColumn(0);
    std::shared_ptr<Datum<double>> datumDouble = std::static_pointer_cast<Datum<double>>(datum);
    double datumValue = datumDouble->getDatum();
    return datumValue < -70.;
  });
  test2->print();

  return 0;
}
