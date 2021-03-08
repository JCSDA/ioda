/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_internals_engines_obsstore
 *
 * @{
 * \file ObsStore-groups.h
 * \brief Functions for ioda::Group backed by ObsStore
 */
#pragma once

#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "./Attributes.hpp"
#include "./Group.hpp"
#include "./ObsStore-attributes.h"
#include "./ObsStore-variables.h"
#include "ioda/Engines/ObsStore.h"
#include "ioda/Group.h"
#include "ioda/defs.h"

namespace ioda {
namespace Engines {
namespace ObsStore {
/// \brief This is the implementation of Groups using ObsStore.
/// \ingroup ioda_internals_engines_obsstore
class IODA_HIDDEN ObsStore_Group_Backend : public ioda::detail::Group_Backend {
private:
  /// \brief ObsStore Group
  std::shared_ptr<ioda::ObsStore::Group> backend_;

public:
  ObsStore_Group_Backend(std::shared_ptr<ioda::ObsStore::Group> grp)
      : ioda::detail::Group_Backend(), backend_(grp) {
    std::shared_ptr<ioda::ObsStore::Has_Attributes> b_atts(backend_->atts);
    atts = Has_Attributes(std::make_shared<ObsStore_HasAttributes_Backend>(b_atts));

    std::shared_ptr<ioda::ObsStore::Has_Variables> b_vars(backend_->vars);
    vars = Has_Variables(std::make_shared<ObsStore_HasVariables_Backend>(b_vars));
  }
  virtual ~ObsStore_Group_Backend() {}

  /// \brief returns list of child groups and variables
  std::map<ObjectType, std::vector<std::string>> listObjects(ObjectType filter,
                                                             bool recurse) const final {
    std::map<ObjectType, std::list<std::string>> data;
    backend_->listObjects(filter, recurse, data);

    std::map<ObjectType, std::vector<std::string>> res;
    for (auto& cls : data) {
      if (filter == ObjectType::Ignored || filter == cls.first)
        res[cls.first] = std::vector<std::string>(std::make_move_iterator(cls.second.begin()),
                                                  std::make_move_iterator(cls.second.end()));
    }
    return res;
  }

  /// \brief returns the capabilities of the ObsStore backend
  ::ioda::Engines::Capabilities getCapabilities() const final {
    return ::ioda::Engines::ObsStore::getCapabilities();
  };

  /// \brief returns true if child group exists
  /// \param name name of child group
  bool exists(const std::string& name) const override { return backend_->exists(name); }

  /// \brief create a new child group
  /// \param name name of child group
  Group create(const std::string& name) override {
    auto backend = std::make_shared<ObsStore_Group_Backend>(backend_->create(name));
    return ::ioda::Group{backend};
  }

  /// \brief open an existing child group (throws an exception if not found)
  /// \param name name of child group
  Group open(const std::string& name) const override {
    auto backend = std::make_shared<ObsStore_Group_Backend>(backend_->open(name));
    return ::ioda::Group{backend};
  }
};

}  // namespace ObsStore
}  // namespace Engines
}  // namespace ioda

/// @}
