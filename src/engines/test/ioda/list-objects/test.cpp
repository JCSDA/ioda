/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include <Eigen/Dense>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "ioda/Engines/Factory.h"
#include "ioda/Exception.h"
#include "ioda/Group.h"

// These tests really need a better check system.
// Boost unit tests would have been excellent here.

/// Run a series of tests on the input group.
void test_group_backend_engine(ioda::Group g) {
  // Create a group structure with some hierarchy
  auto gObsValue = g.create("ObsValue");
  auto gObsError = g.create("ObsError");
  auto gMetaData = g.create("MetaData");

  auto gMdChild = gMetaData.create("Child 1");

  // Create some variables
  ioda::VariableCreationParameters params;
  params.setFillValue<double>(-999);
  params.chunk = true;
  params.compressWithGZIP();

  auto obsVar = gObsValue.vars.create<double>("myobs", { 2, 2 }, { 2, 2 }, params)
                         .write<double>({ 1.0, 2.0, 3.0, 4.0 });
  auto errVar = gObsError.vars.create<double>("myobs", { 2, 2 }, { 2, 2 }, params)
                         .write<double>({ 0.5, 0.1, 0.05, 0.01 });
  auto latVar = gMetaData.vars.create<double>("latitude", { 2, 2 }, { 2, 2 }, params)
                         .write<double>({ 1.5, 2.5, 3.5, 4.5 });

  // Can we list groups?
  auto g_list = g.list();
  if (g_list.size() != 3) throw ioda::Exception(ioda_Here());
  // Can we list another way?
  auto g_list2 = g.listObjects();
  if (g_list2.empty()) throw ioda::Exception(ioda_Here());

  // Can we list groups and variables recursively?
  auto g_list3 = g.listObjects(ioda::ObjectType::Ignored, true);
  if (g_list3.empty()) throw ioda::Exception(ioda_Here());

  // Can we list variables recursively (templated form)?
  auto g_list4 = g.listObjects<ioda::ObjectType::Variable>(true);
  if (g_list4.empty()) throw ioda::Exception(ioda_Here());
}

int main(int argc, char** argv) {
  using namespace ioda;
  using namespace std;
  try {
    auto f = Engines::constructFromCmdLine(argc, argv, "test-list_objects.hdf5");

    test_group_backend_engine(f);

  } catch (const std::exception& e) {
    ioda::unwind_exception_stack(e);
    return 1;
  }
  return 0;
}
