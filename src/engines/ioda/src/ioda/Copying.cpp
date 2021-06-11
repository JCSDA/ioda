/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Copying.cpp
/// \brief Generic copying facility

#include "ioda/Copying.h"

namespace ioda {
ObjectSelection::~ObjectSelection() {}
ObjectSelection::ObjectSelection() {}
ObjectSelection::ObjectSelection(const Group& g, bool recurse) : g_(g), recurse_(recurse) {}

/*
void ObjectSelection::insert(const ObjectSelection&);
void ObjectSelection::insert(const Variable&);
void ObjectSelection::insert(const std::vector<Variable>&);
void ObjectSelection::insert(const Has_Variables&);
void ObjectSelection::insert(const Group&, bool recurse);
void ObjectSelection::insert(const Group&, const std::vector<std::string>&);
*/
/*
template <class T> ObjectSelection add(
        const ObjectSelection &src, const T& obj) {
        ObjectSelection res = src;
        res.insert(obj);
        return res;
}
*/
/*
ObjectSelection ObjectSelection::operator+(const ObjectSelection& obj) const { return add(*this,
obj); } ObjectSelection ObjectSelection::operator+(const Variable& obj) const { return add(*this,
obj); } ObjectSelection ObjectSelection::operator+(const std::vector<Variable>& obj) const { return
add(*this, obj); } ObjectSelection ObjectSelection::operator+(const Has_Variables& obj) const {
return add(*this, obj); }
*/

/*
template <class T> ObjectSelection& emplace(ObjectSelection& src, const T& obj) {
        src.insert(obj);
        return src;
}
*/

/*
template<> ObjectSelection& emplace<ObjectSelection>(ObjectSelection& src, const T& obj) {
        if (&src == &obj) return src;
        src.insert(obj);
        return src;
}
ObjectSelection& ObjectSelection::operator+=(const ObjectSelection& obj) { return emplace(*this,
obj); } ObjectSelection& ObjectSelection::operator+=(const Variable& obj) { return emplace(*this,
obj); } ObjectSelection& ObjectSelection::operator+=(const std::vector<Variable>& obj) { return
emplace(*this, obj); } ObjectSelection& ObjectSelection::operator+=(const Has_Variables& obj) {
return emplace(*this, obj); }
*/

ScaleMapping::~ScaleMapping() {}

void copy(const ObjectSelection& from, ObjectSelection& to, const ScaleMapping&) {
  using namespace std;

  // We really want to maximize performance here and avoid excessive variable
  // re-opens and closures that would kill the HDF5 backend.
  // We want to:
  // 1) separate the dimension scales from the regular variables.
  // 2) determine the maximum size along the 0-th dimension.
  // 3) determine which dimensions are attached to which variable axes.

  // Convenience lambda to hint if a variable is a scale.
  auto isPossiblyScale = [](const std::string& name) -> bool {
    return (std::string::npos == name.find('@')) && (std::string::npos == name.find('/')) ? true
                                                                                          : false;
  };

  // We start with the names of all of the variables.
  const std::vector<std::string> allVars = from.g_.listObjects<ObjectType::Variable>(true);

  size_t max_var_size_ = 0;
  std::vector<std::string> var_list_, dim_var_list_;
  var_list_.reserve(allVars.size());
  dim_var_list_.reserve(allVars.size());
  std::map<std::string, std::vector<std::string>> dims_attached_to_vars_;

  // In our processing loop, we want to ideally open each variable only once.
  // But, the variables and dimension scales are mixed together. To give a "hint"
  // for where to start, we can partially sort allVars. Our dimension scales all
  // lack '@' and '/' in their path listings. "nlocs" should always come first,
  // since this is the most frequently-occurring dimension.
  std::list<std::string> sortedAllVars;
  for (const auto& name : allVars) {
    if (sortedAllVars.empty())
      sortedAllVars.push_back(name);
    else {
      if (isPossiblyScale(name)) {
        auto second = sortedAllVars.begin();
        second++;
        if (sortedAllVars.front() == "nlocs")
          sortedAllVars.insert(second, name);
        else
          sortedAllVars.push_front(name);
      } else
        sortedAllVars.push_back(name);
    }
  }

  // Now for the main processing loop.
  // We separate dimension scales from non-dimension scale variables.
  // We record the maximum sizes of variables.
  // We construct the in-memory mapping of dimension scales and variable axes.
  // Keep track of these to avoid re-opening the scales repeatedly.
  std::list<std::pair<std::string, Variable>> dimension_scales;

  auto group = from.g_;

  for (const auto& vname : sortedAllVars) {
    Variable v      = group.vars.open(vname);
    const auto dims = v.getDimensions();
    if (dims.dimensionality >= 1) {
#ifdef max
#  undef max
#endif
      max_var_size_ = std::max(max_var_size_, (size_t)dims.dimsCur[0]);
    }
    // Expensive function call.
    // Only 1-D variables can be scales. Also pre-filter based on name.
    if (dims.dimensionality == 1 && isPossiblyScale(vname)) {
      if (v.isDimensionScale()) {
        (vname == "nlocs")  // true / false ternary
          ? dimension_scales.push_front(std::make_pair(vname, v))
          : dimension_scales.push_back(std::make_pair(vname, v));
        dim_var_list_.push_back(vname);
        continue;  // Move on to next variable in the for loop.
      }
    }
    // See above block. By this point in execution, we know that this variable
    // is not a dimension scale.
    var_list_.push_back(vname);
    // Let's figure out which scales are attached to which dimensions.
    auto attached_dimensions = v.getDimensionScaleMappings(dimension_scales);
    std::vector<std::string> dimVarNames;
    dimVarNames.reserve(dims.dimensionality);
    for (const auto& dim_scales_along_axis : attached_dimensions) {
      if (dim_scales_along_axis.empty())
        throw jedi_throw.add("Reason",
                             "Bad dimension mapping. Not all dimension scales are known.");
      dimVarNames.push_back(dim_scales_along_axis[0].first);
    }
    dims_attached_to_vars_.emplace(vname, dimVarNames);
  }
  // record lists of regular variables and dimension scale variables
  // this->resetVarLists();
  // this->resetVarDimMap();

  /*
  // Find the dimension scales
  const map<string, Variable> scale_id_var_from
          = [](const Group& baseGrp)
  {
          map<string, Variable> res;
          auto basevars = baseGrp.vars.list();
          //std::cerr << "Finding dim scales. There are " << basevars.size() << " candidates." <<
  std::endl; for (const auto& name : basevars) { auto v = baseGrp.vars.open(name); if
  (v.isDimensionScale()) { string id = name;
                          //string id_scale = v.getDimensionScaleName();
                          //if (id_scale.size()) id = id_scale;
                          res[id] = v;
                          //std::cerr << "\tAdded scale var " << id << std::endl;
                  }
          }
          return res;
  }(from.g_);

  //std::cerr << "Creating scales in destination." << std::endl;

  const map<string, Variable> scale_id_var_to
          = [](Group& destGrp, const map<string, Variable>& srcScales)
  {
          map<string, Variable> res;
          for (const auto& src : srcScales) {
                  //std::cerr << "\tCreating scale " << src.first << std::endl;
                  VariableCreationParameters params;
                  params.chunk = true;
                  params.chunks = src.second.getChunkSizes();
                  params.compressWithGZIP();
                  params.setIsDimensionScale(src.second.getDimensionScaleName());
                  // TODO: Propagate attributes
                  // TODO: Propagate fill value
                  BasicTypes typ = src.second.getBasicType();
                  if (typ == BasicTypes::undefined_)
                          throw jedi_throw.add("Reason", "Unrecognized basic type");
                  const auto dims = src.second.getDimensions();

                  auto newscale = destGrp.vars._create_py(src.first, typ, dims.dimsCur,
  dims.dimsMax, {}, params); res[src.first] = newscale;
          }
          return res;
  }(to.g_, scale_id_var_from);

  // Iterate over all of the variables.
  // Select those that are not dimension scales, and get a few properties.
  //std::cerr << "Iterating over variables to copy." << std::endl;
  auto basevars = from.g_.vars.list(); //std::vector<std::string>{ "datetime@MetaData"}; //
  from.g_.vars.list(); for (const auto& name : basevars) { Variable v = from.g_.vars.open(name); if
  (!v.isDimensionScale()) {
                  //std::cerr << "\t" << name << "\n";  //std::endl;
                  for (size_t i = 0; i < (size_t)v.getDimensions().dimensionality; ++i) {
                          for (const auto& scales_from : scale_id_var_from) {
                                  if (v.isDimensionScaleAttached((unsigned)i, scales_from.second)) {
                                          //std::cerr << "\t\t" << i << "\t" << scales_from.first <<
  "\n"; // std::endl;
                                  }
                          }

                  }
          }
  }
  */
}
}  // namespace ioda
