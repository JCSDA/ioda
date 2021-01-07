/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include <cmath>
#include <iostream>
#include <vector>

#include "ioda/Engines/Factory.h"
#include "ioda/Group.h"

void test_group_backend_engine(ioda::Group g) {
  // Want to test create, open, [] operator (open) and exists functions
  // that use hierarchical paths in their name arguments.

  // Groups
  std::string amsuaGroup("AMSU-A/ObsValue");
  g.create(amsuaGroup);
  if (!g.exists(amsuaGroup)) {
    throw;  // jedi_throw.add("Reason", "Group exists check failed");
  }

  auto g1 = g.open(amsuaGroup);
  g1.create("Child1");
  auto childList = g1.list();
  if (childList.size() != 1) {
    throw;  // jedi_throw.add("Reason", "Group list children size check 1 failed");
  }
  if (childList[0] != "Child1") {
    throw;  // jedi_throw.add("Reason", "Group list children names check 1 failed");
  }

  // Try create where part of the hierarchy already exists
  std::string amsuaGroupChild2 = amsuaGroup + "/Child2";
  g.create(amsuaGroupChild2);
  childList = g1.list();
  if (childList.size() != 2) {
    throw;  // jedi_throw.add("Reason", "Group list children size check 2 failed");
  }
  if ((childList[0] != "Child1") || (childList[1] != "Child2")) {
    throw;  // jedi_throw.add("Reason", "Group list children names check 2 failed");
  }

  // Variables
  std::string sondeTopGroup = "Sonde";
  std::string sondeMidGroup = "ObsValue";
  std::string sondeVar = "air_temperature";
  std::string sondeGroupVar = sondeTopGroup + "/" + sondeMidGroup + "/" + sondeVar;
  ioda::VariableCreationParameters params;
  params.chunk = true;
  params.compressWithGZIP();
  params.setFillValue<float>(-999);
  g.vars.create<float>(sondeGroupVar, {4}, {4}, params);

  auto v1 = g.vars.open(sondeGroupVar);
  std::vector<float> v1_data({1.5, 2.5, 3.5, 4.5});
  v1.write(v1_data);

  std::vector<float> v1_check;
  auto v2 = g.open(sondeTopGroup).open(sondeMidGroup).vars[sondeVar];
  v2.read<float>(v1_check);

  if (v1_check.size() != 4) {
    throw;  // jedi_throw.add("Reason", "Var size check failed");
  }
  for (std::size_t i = 0; i < v1_check.size(); ++i) {
    float checkVal = fabs((v1_check[i] / v1_data[i]) - 1.0f);
    if (checkVal > 1.e-3) {
      throw; /* jedi_throw
              .add("Reason", "Var contents check failed")
              .add("Index", i)
              .add("Result value", v1_check[i])
              .add("Expected value", v1_data[i]); */
    }
  }
}

int main(int argc, char** argv) {
  using namespace ioda;
  using namespace std;
  try {
    auto f = Engines::constructFromCmdLine(argc, argv, "test-hier_paths.hdf5");

    test_group_backend_engine(f);

  } catch (const std::exception& e) {
    cerr << e.what() << endl;
    return 1;
  }
  return 0;
}
