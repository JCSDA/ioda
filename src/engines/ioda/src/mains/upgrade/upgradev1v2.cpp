/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! @file upgrade.cpp
* @brief A program to upgrade ioda files to a newer format.
* 
* Call program as: ioda-upgrade.x YAML_settings_file [input files] ... output_directory
*/

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <numeric>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "ioda/Engines/EngineUtils.h"
#include "ioda/Engines/HH.h"
#include "ioda/Exception.h"
#include "ioda/Group.h"
#include "ioda/ObsGroup.h"
#include "ioda/Misc/DimensionScales.h"
#include "ioda/Misc/StringFuncs.h"
#include "ioda/Variables/VarUtils.h"

// Annoying header junk on Windows. Who in their right mind defines a macro with
// the same name as a standard library function?
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

using namespace ioda::VarUtils;

constexpr ioda::Dimensions_t defaultChunkSize = 100;

/// @brief Check and adjust the chunk sizes used in the VariableCreationParameters struct
/// @details A chunk size of zero is not acceptable, so in the case when a dimension is of
/// zero size, we still want to use a non-zero chunk size spec. This function will simply
/// check the chunk size specs in a VariableCreationParameter spec and change zero sizes
/// to the newChunkSize argument value.
/// @param[out] params The VariabelCreationParameters struct that is being checked/adjusted
/// @param[in] newChunkSize the value to be used for the adjusted chunk size
void checkAdjustChunkSizes(ioda::VariableCreationParameters & params,
                           const ioda::Dimensions_t & newChunkSize) {
  // A chunk size of zero is not acceptable. Also all of the dimensions scales and
  // and variables are set up to use chunking (which is necessary to use unlimited
  // max size) and potentially use the unlimited max size feature. So, if the chunks
  // parameter is set to zero, change it to the newChunkSize parameter.
  for (auto & i : params.chunks) {
    if (i == 0) {
      i = newChunkSize;
    }
  }
}

/// @brief Determine which variables may be grouped.
/// @param[in] inVarList is the list of all variables.
std::size_t getChanSuffixPos(const std::string & name) {
    // Identify the position of the _<number> suffix that represents the channel
    // number (if present in name). Allow for the case where you have multiple
    // _<number> suffixes, and take the last one as the channel number indicator.
    //
    // Go to the last occurrence of an underscore. If no underscores, then return npos.
    // If we have an underscore, check to see if only digits occur after the underscore.
    // If so, then we have a channel number suffix and return pos. If not, then we don't
    // have a channel number suffix and return npos.
    auto pos = name.find_last_of("_");
    if (pos != std::string::npos) {
        // have an underscore, check to see if only digits after the underscore
        if ((pos + 1) < name.length()) {
            std::string testSuffix = name.substr(pos + 1);
            if (testSuffix.find_first_not_of("0123456789") != std::string::npos) {
                // not only digits after the underscore so set pos to npos
                pos = std::string::npos;
            }
        }
    }
    return pos;
}

/// @brief Determine which variables may be grouped.
/// @param[in] inVarList is the list of all variables.
/// @param[out] similarVariables is the collection of similar variables, grouped by similarity and sorted numerically.
/// @param[out] dissimilarVariables are all variables that are not "similar".
void identifySimilarVariables(const Vec_Named_Variable& inVarList, VarDimMap& similarVariables,
  Vec_Named_Variable &dissimilarVariables) {
  using namespace ioda;
  using namespace std;
  dissimilarVariables.reserve(inVarList.size());

  // Transform names to new format so that all groups come first. Then, sort so that similar
  // variables are lexically related.
  Vec_Named_Variable sortedNames = inVarList;
  for (auto& v : sortedNames) v.name = convertV1PathToV2Path(v.name);
  sort(sortedNames.begin(), sortedNames.end());

  // Iterate through the sorted list. Find ranges of *similar* variable names where the
  // variables end in '_' + a number.
  auto varsAreSimilar = [](const std::string& lhs, const std::string& rhs) -> bool {
    // Don't allow variables in the meta data groups to be associated. These variables
    // should always be vectors dimensioned by the axis they describe.
    if ((lhs.find("MetaData/") != string::npos) || (rhs.find("MetaData/") != string::npos)) {
      return false;
    }
    // Split if statements to avoid an out-of-bounds bug that could otherwise occur with
    // substr(0, found_range+1).
    if (lhs.find_first_of("0123456789") == string::npos
        && rhs.find_first_of("0123456789") == string::npos)
      return lhs == rhs;
    string id_lhs = lhs.substr(0, getChanSuffixPos(lhs));
    string id_rhs = rhs.substr(0, getChanSuffixPos(rhs));
    return id_lhs == id_rhs;
  };

  // Collect up similar named variables and place them under their "base" name. If the
  // variable name is unique and doesn't have a channel suffix, then place it in the
  // dissimilarVariables list. Otherwise record the name variants under the base name
  // in the similarVariables list.
  auto collect = [&dissimilarVariables, &similarVariables](Vec_Named_Variable::const_iterator start, Vec_Named_Variable::const_iterator end) {
    // End of a range. If range has only one variable, check if it has channel suffix
    if (start == end) {
      if (start->name.find("MetaData/") != string::npos ||
          getChanSuffixPos(start->name) == std::string::npos) {
        // Metadata variable, or no channel suffix: save as unique variable
        cout << " Unique variable: " << start->name << ".\n";
        dissimilarVariables.push_back(*start);
      } else {
        // Not Metadata varaible and channel suffix: figure out the new name.
        string rangeName
          = start->name.substr(0, getChanSuffixPos(start->name));
        cout << " Grouping 1 variable into: " << rangeName << ".\n";
        similarVariables[Named_Variable{rangeName, Variable()}] = {*start};
      }
    }
    // If a range has multiple variables, sort and group.
    else {
      // A range has been found. Pack into similarVariables.
      Vec_Named_Variable range(start, end+1);
      // Sort this range based on a true numeric sort. The usual lexical sort is problematic
      // because variable suffixes have different lengths.
      sort(range.begin(), range.end(),
           [](const Named_Variable& lhs, const Named_Variable& rhs) -> bool {
             string sidnum_lhs = lhs.name.substr(getChanSuffixPos(lhs.name) + 1);
             string sidnum_rhs = rhs.name.substr(getChanSuffixPos(rhs.name) + 1);
             int idnum_lhs     = std::atoi(sidnum_lhs.c_str());
             int idnum_rhs     = std::atoi(sidnum_rhs.c_str());
             return idnum_lhs < idnum_rhs;
           });

      // Figure out the new name.
      string rangeName
        = start->name.substr(0, getChanSuffixPos(start->name));
      cout << " Grouping " << range.size() << " variables into: " << rangeName << ".\n";
      similarVariables[Named_Variable{rangeName, Variable()}] = std::move(range);
    }
  };

  auto rangeStart = sortedNames.cbegin();
  auto rangeEnd   = rangeStart;
  for (auto it = sortedNames.cbegin() + 1; it != sortedNames.cend(); ++it) {
    if (varsAreSimilar(rangeStart->name, it->name)) {
      rangeEnd = it;
    } else {
      collect(rangeStart, rangeEnd);
      rangeStart = it;
      rangeEnd   = it;
    }

    // Special case terminating the variable sequence.
    if ((it + 1) == sortedNames.cend()) {
        collect(rangeStart, rangeEnd);
    }
  }
}




/*
/// @brief Swap out new dimension scale names
/// @param oldDimsAttachedToVars 
/// @param newDimList 
/// @return A variable -> dimension map that references the new dimension scales. Old variable mappings are used.
VarDimMap translateToNewDims(
  const Vec_Named_Variable& newDimList,
  const VarDimMap& oldDimsAttachedToVars) {
  std::map<std::string, ioda::Named_Variable> newDims;
  for (const auto& d : newDimList) newDims[d.name] = d;

  VarDimMap res;

  for (const auto& oldVar : oldDimsAttachedToVars) {
    const auto& oldVec = oldVar.second;
    Vec_Named_Variable newVec(oldVec);
    for (auto& s : newVec) s.var = newDims.at(s.name).var;
    res[ioda::Named_Variable(oldVar.first.name, ioda::Variable())] = newVec;
  }

  return res;
}
*/

/// @brief Copy data from oldvar into newvar. Offsets are supported for variable combination. 
/// @param oldvar is the old variable.
/// @param newvar is the new variable.
/// @param base is the ObsGroup root object. Used in detecting ioda file versions.
/// @todo Add offset, oldvar_dims, newvar_dims.
void copyData(const Vec_Named_Variable& old, ioda::Variable& newvar, const ioda::ObsGroup& base,
              const std::string & newVarName, const std::map<int, int> & chanNumToIndex) {
  using namespace ioda;
  using namespace std;

  // DEBUG(ryan): Add the old variable names as a string attribute to ensure proper alignment.
  //vector<string> var_alignment;
  //for (const auto& v : old) var_alignment.push_back(v.name);
  //newvar.atts.add<string>("check_copy_vars", var_alignment);

  // Loop over each variable in old and apply to the appropriate place in newvar.
  for (size_t i = 0; i < old.size(); ++i) {
    const Variable oldvar   = old[i].var;

    Dimensions oldvar_dims  = oldvar.getDimensions();
    Dimensions newvar_dims  = newvar.getDimensions();
    size_t sz_type_in_bytes = oldvar.getType().getSize();
    if (oldvar.isA<std::string>()) {
      // Some old ioda files have really odd string formats. We detect these here and
      // repack the strings appropriately.
      vector<string> buf_in;
      oldvar.read<string>(buf_in);
      if (oldvar_dims.numElements == newvar_dims.numElements) {
        newvar.write<string>(buf_in);
      } else {
        if (oldvar_dims.dimensionality > 0) {
          vector<string> buf_out;
          buf_out.reserve(gsl::narrow<size_t>(newvar_dims.numElements));
          // Look at the last dimension in oldvar_dims. Group according to this size.
          size_t group_sz
            = gsl::narrow<size_t>(oldvar_dims.dimsCur[oldvar_dims.dimensionality - 1]);
          vector<char> new_str(group_sz + 1, '\0');
          for (size_t i = 0; i < buf_in.size(); ++i) {
            size_t idx   = i % group_sz;
            new_str[idx] = buf_in[i][0];

            string str(new_str.data());
            // In-place right trim
            str.erase(std::find_if(str.rbegin(), str.rend(),
                      [](unsigned char ch) { return !std::isspace(ch); }).base(), str.end());

            if (idx + 1 == group_sz) buf_out.push_back(str);
          }
          newvar.write<string>(buf_out);
        }
      }
    } else {
      vector<char> buf(oldvar_dims.numElements * sz_type_in_bytes);
      oldvar.read(gsl::make_span<char>(buf.data(), buf.size()), oldvar.getType());
      if (old.size() == 1) {
        // We are writing out the entire variable.
        newvar.write(gsl::make_span<char>(buf.data(), buf.size()), newvar.getType());
      } else {
        // If the chanNumToIndex is not empty, extract the channel number from the
        // var name suffix and use the corresponding index for writing the variable.
        int chanIndex = -1;
        if (!chanNumToIndex.empty()) {
          string oldVarName = old[i].name;
          if (oldVarName.find(newVarName) == 0) {
            // have a name with a channel suffix
            int pos = newVarName.length() + 1;
            int chanNum = stoi(oldVarName.substr(pos));
            chanIndex = chanNumToIndex.at(chanNum);
          }
        }

        // We are writing a selection. Needs start, count, stride, block.
        Selection::VecDimensions_t extent_ioda = newvar_dims.dimsCur;
        Selection::VecDimensions_t extent_mem  = newvar_dims.dimsCur;
        *extent_mem.rbegin()                   = 1;

        Selection::VecDimensions_t start_mem(newvar_dims.dimensionality);
        Selection::VecDimensions_t start_ioda(newvar_dims.dimensionality);
        if (chanIndex >= 0) {
          *start_ioda.rbegin()             = chanIndex;
        } else {
          *start_ioda.rbegin()             = i;
        }
        Selection::VecDimensions_t count = newvar_dims.dimsCur;
        *count.rbegin()                  = 1;
        Selection::VecDimensions_t stride(newvar_dims.dimensionality, 1);
        Selection::VecDimensions_t block(newvar_dims.dimensionality, 1);

        Selection::SingleSelection sel_mem(SelectionOperator::SET, start_mem, count, stride, block);
        Selection mem_selection(extent_mem);
        mem_selection.select(sel_mem);

        Selection::SingleSelection sel_ioda(SelectionOperator::SET, start_ioda, count, stride, block);
        Selection ioda_selection(extent_ioda);
        ioda_selection.select(sel_ioda);

        newvar.write(gsl::make_span<char>(buf.data(), buf.size()), newvar.getType(), mem_selection,
                     ioda_selection);
      }
    }
  }
}

/// @brief Copy attributes from src to dest. Ignore duplicates and dimension scales.
/// @param src is the source.
/// @param dest is the destination.
void copyAttributes(const ioda::Has_Attributes& src, ioda::Has_Attributes& dest) {
  using namespace ioda;
  using namespace std;
  vector<pair<string, Attribute>> srcAtts = src.openAll();

  for (const auto &s : srcAtts) {
    // This set contains the names of atttributes that need to be stripped off of
    // variables coming from the input file. The items in the list are related to
    // dimension scales and will confuse the netcdf API and tools if allowed to be
    // copied to the output file variables.
    //
    // In other words, these attributes assist the netcdf API in navigating the
    // association of variables with dimension scales and have meaning to the netcdf API. 
    // These represent the associations in the input file and need to be stripped off
    // since the associations in the output file will be re-created (and will not
    // necessarily match the associations in the input file).
    const set<string> ignored_names{
        "CLASS",
        "DIMENSION_LIST",
        "NAME",
        "REFERENCE_LIST",
        "_Netcdf4Coordinates",
        "_Netcdf4Dimid",
        "_nc3_strict"
        };
    if (ignored_names.count(s.first)) continue;
    if (dest.exists(s.first)) continue;

    Dimensions dims  = s.second.getDimensions();
    Type typ                = s.second.getType();
    size_t sz_type_in_bytes = typ.getSize();

    // Some variable attributes consist of an empty string in which case
    // numElements is zero. In this is the case, create an empty string in the
    // destination output, but make it consist of the null byte.
    if (dims.numElements == 0) {
      vector<char> buf(1, '\0');
      Attribute newatt = dest.create(s.first, typ, { 1 });
      newatt.write(gsl::make_span<char>(buf.data(), buf.size()), typ);
    } else {
      // copy from src attribute to dest attribute
      vector<char> buf(dims.numElements * sz_type_in_bytes);
      s.second.read(gsl::make_span<char>(buf.data(), buf.size()), typ);

      Attribute newatt = dest.create(s.first, typ, dims.dimsCur);
      newatt.write(gsl::make_span<char>(buf.data(), buf.size()), typ);
    }
  }
}

struct UpgradeParameters {
  bool groupSimilarVariables = true;
};

bool upgradeFile(const std::string& inputName, const std::string& outputName, const UpgradeParameters &params) {
  // Open file, determine dimension scales and variables.
  using namespace ioda;
  using namespace std;
  const Group in = Engines::HH::openMemoryFile(inputName);

  Vec_Named_Variable varList, dimVarList;
  VarDimMap dimsAttachedToVars;
  Dimensions_t maxVarSize0;

  collectVarDimInfo(in, varList, dimVarList, dimsAttachedToVars, maxVarSize0);

  // Figure out which variables can be combined
  Vec_Named_Variable ungrouped_varList;
  VarDimMap old_grouped_vars;
  if (params.groupSimilarVariables)
    identifySimilarVariables(varList, old_grouped_vars, ungrouped_varList);
  else
    ungrouped_varList = varList;

  // Create the output file

  // TODO(ryan): Fix this odd workaround where the map searches fail oddly.
  map<string, Vec_Named_Variable> dimsAttachedToVars_bystring;
  for (const auto& val : dimsAttachedToVars)
    dimsAttachedToVars_bystring[convertV1PathToV2Path(val.first.name)] = val.second;

  // Construct the ObsGroup with the same scales as the input file.
  //
  // There are some cases where extraneous dimensions get included. An extraneous
  // dimension is one that is not attached to any variable in the file. Exclude defining
  // extraneous dimensions in the output file. To help with this, create a set
  // of dim names and use this to mark which dimensions are being used.
  set<string> attachedDims;
  for (const auto & ivar : dimsAttachedToVars_bystring) {
      for (const auto & idim : dimsAttachedToVars_bystring.at(ivar.first)) {
          attachedDims.insert(idim.name);
      }
  }

  NewDimensionScales_t newdims;
  for (const auto& dim : dimVarList) {
    // GMI data bug: nchans already exists. Suppress creation of this scale if
    // we are grouping new data to nchans (below).
    // Also suppress creation of any scales not being used in the input file.
    if (!(dim.name == "nchans" && old_grouped_vars.size()) &&
         (attachedDims.find(dim.name) != attachedDims.end()))
      newdims.push_back(
        NewDimensionScale(dim.name, dim.var, ScaleSizes{Unspecified, Unspecified, defaultChunkSize}));
  }
  if (old_grouped_vars.size()) {
    cout << " Creating nchans variable.\n";
    // Extract the channel numbers
    //
    // First, find the variable with the maximum number of channels and use that
    // as a template for the others. This covers cases where some of the channel variables
    // are missing in some groups. These variables will end up with missing data for the
    // channels they don't have.
    VarDimMap::iterator chanTemplate;
    size_t maxChanSize = 0;
    for (VarDimMap::iterator ivar = old_grouped_vars.begin();
                             ivar != old_grouped_vars.end(); ++ivar) {
      if (ivar->second.size() > maxChanSize) {
          chanTemplate = ivar;
          maxChanSize = ivar->second.size();
      }
    }

    vector<int32_t> channels(chanTemplate->second.size());
    for (size_t i = 0; i < chanTemplate->second.size(); ++i) {
      string schan = chanTemplate->second[i].name.substr(
        chanTemplate->second[i].name.find_last_not_of("_0123456789") + 2);
      channels[i]  = std::atoi(schan.c_str());
    }

    // Limited dimension. Channels are chunked together.
    auto nds = NewDimensionScale<int32_t>("nchans", gsl::narrow<Dimensions_t>(channels.size()),
                                          gsl::narrow<Dimensions_t>(channels.size()),
                                          gsl::narrow<Dimensions_t>(channels.size()));
    nds->initdata_ = channels; // Pass initial channel data.
    newdims.push_back(nds);
  }

  Group g_out = Engines::HH::createFile(outputName,
      Engines::BackendCreateModes::Truncate_If_Exists,
      Engines::HH::HDF5_Version_Range{Engines::HH::HDF5_Version::V18,
      Engines::HH::HDF5_Version::V18});
  ObsGroup out = ObsGroup::generate(g_out, newdims);

  // Copy attributes from the root group
  copyAttributes(in.atts, out.atts);

  // Open all new scales
  map<string, Variable> newscales, newvars;
  for (const auto& dim : newdims) newscales[dim->name_] = out.vars[dim->name_];
  // Copy missing attributes from old scales.
  for (const Named_Variable& d : dimVarList) {
    if (attachedDims.find(d.name) != attachedDims.end()) {
        copyAttributes(d.var.atts, newscales.at(d.name).atts);
    }
  }
  
  
  // Make all variables and store handles. Do not attach dimension scales yet.
  // Loop is split for ungrouped vs grouped vars.
  auto makeNewVar = [&newvars,&out](const Named_Variable &oldVar, const Dimensions &dims, const VariableCreationParameters &params) {
    // Check if we are creating a string variable. If so, determine if we are upgrading
    // the string format. This is also relevant for the copyData function, which checks
    // the re-mapping of dimensions to see if a string repack is needed.
    if (oldVar.var.isA<string>()) {
      // In the really old format, fixed-length strings each have a size of one byte.
      // We use this as the discriminator to signify that these strings need conversion.
      size_t sz_bytes     = oldVar.var.getType().getSize();
      Dimensions mod_dims = dims;
      VariableCreationParameters adjustedParams = params;
      if (sz_bytes == 1 && mod_dims.dimensionality > 1) {
        mod_dims.dimensionality -= 1;
        mod_dims.dimsCur.resize(mod_dims.dimsCur.size() - 1);
        mod_dims.dimsMax.resize(mod_dims.dimsMax.size() - 1);
        mod_dims.numElements = (mod_dims.dimensionality == 0) ? 0 : 1;
        for (const auto& d : mod_dims.dimsCur) mod_dims.numElements *= d;

        adjustedParams.chunks = mod_dims.dimsCur;  // A suggestion.
      }

      // make sure we are not specifying zero chunk sizes
      checkAdjustChunkSizes(adjustedParams, defaultChunkSize);

      // Set the fill value to an empty string. The calls to getCreationParameters()
      // on the ioda v1 variables that preceed the call to this function set the fill
      // value to a null character (\0) since the ioda v1 format for strings is a
      // character array style. We are going to convert that character array to a vector
      // of strings and the fill value needs to use the special string container instead
      // of the union (which the character uses).
      adjustedParams.setFillValue<string>("");

      cout << " Converting old-format string variable: " << oldVar.name << "\n";

      newvars[oldVar.name] = out.vars.create<string>(oldVar.name, mod_dims, adjustedParams);
      return newvars[oldVar.name];
    } else {
      // TODO(ryan): turn on chunking and compression everywhere relevant.
      VariableCreationParameters adjustedParams = params;
      adjustedParams.chunk                      = true;
      {
        // Ideal chunking is a bit complicated.
        // Start with using all dimensions. If this is greater than 6400,
        // reduce the rightmost dimension. If rightmost dimension equals 1,
        // then target the second-to-last dimension, and so on.
        adjustedParams.chunks = dims.dimsCur;  // Initial suggestion.
        auto& c               = adjustedParams.chunks;
        const Dimensions_t max_chunk_size = 6400;
        while (accumulate(c.begin(), c.end(),
          static_cast<Dimensions_t>(1), multiplies<Dimensions_t>())> max_chunk_size)
        {
          auto dim = c.rbegin();
          while (*dim == 1) dim++;
          *dim /= 2;
        }

      }

      // make sure we are not specifying zero chunk sizes
      checkAdjustChunkSizes(adjustedParams, defaultChunkSize);

      adjustedParams.compressWithGZIP();

      newvars[oldVar.name]
        = out.vars.create(oldVar.name, oldVar.var.getType(), dims, adjustedParams);
      return newvars[oldVar.name];
    }
  };
  VarDimMap dimsForNewVars;


  // create vars in the ungrouped list, including copy of their attributes
  for (const auto& oldVar : ungrouped_varList) {
    Dimensions dims = oldVar.var.getDimensions();
    // TODO(ryan): copy over other attributes?
    VariableCreationParameters params          = oldVar.var.getCreationParameters(false, false);
    auto newvar                                = makeNewVar(oldVar, dims, params);
    copyAttributes(oldVar.var.atts, newvar.atts);
    const Vec_Named_Variable old_attached_dims
      = dimsAttachedToVars_bystring.at(convertV1PathToV2Path(oldVar.name));
    dimsForNewVars[Named_Variable{oldVar.name, newvar}] = old_attached_dims;
  }

  const Dimensions_t suggested_chan_chunking
    = (newscales.count("nchans")) ? newscales["nchans"].atts["suggested_chunk_dim"].read<Dimensions_t>() : defaultChunkSize;
  map<string, Named_Variable> new_grouped_vars;
  Dimensions_t numChans;
  if (old_grouped_vars.size() > 0) {
    numChans = out.vars.open("nchans").getDimensions().dimsCur[0];
  }
  for (const auto& oldGroup : old_grouped_vars) {
    Dimensions dims = oldGroup.second.begin()->var.getDimensions();
    Dimensions_t n  = gsl::narrow<Dimensions_t>(oldGroup.second.size());
    if (n > 1) {
      n = numChans;
    }
    dims.dimensionality++;
    dims.dimsCur.push_back(n);
    dims.dimsMax.push_back(n);
    dims.numElements *= n;

    VariableCreationParameters params
      = oldGroup.second.begin()->var.getCreationParameters(false, false);
    params.chunks.push_back(suggested_chan_chunking);

    Named_Variable proto_var{oldGroup.first.name, oldGroup.second.begin()->var};
    auto createdVar = makeNewVar(proto_var, dims, params);
    // Copy attributes from all old variables.
    for (const auto& src : oldGroup.second) copyAttributes(src.var.atts, createdVar.atts);

    // Also add in a new entry in dimsAttachedToVars for this variable grouping.
    Named_Variable created{proto_var.name, createdVar};
    Vec_Named_Variable ungrouped_scales
      = dimsAttachedToVars_bystring.at(convertV1PathToV2Path(oldGroup.second.begin()->name));
    Vec_Named_Variable grouped_scales = ungrouped_scales;
    grouped_scales.push_back(Named_Variable{"nchans", newscales["nchans"]});
    dimsForNewVars[created]               = grouped_scales;
    new_grouped_vars[oldGroup.first.name] = created;
  }



  // Attach all dimension scales to all variables.
  // We separate this from the variable creation (above) since we might want to implement a
  // collective call.
  {
    vector<pair<Variable, vector<Variable>>> out_dimsAttachedToVars;
    auto make_out_dimsAttachedToVars
      = [&newvars, &newscales, &out_dimsAttachedToVars](const Vec_Named_Variable& olddims,
                                                        const Named_Variable& m) {
          Variable newvar{newvars[m.name]};
          vector<Variable> newdims;
          for (const auto& d : olddims)
            newdims.emplace_back(newscales[d.name]);
          // Check for an old-format string. If found, drop the last dimension.
          if (m.var.isA<string>()) {
            if (m.var.getType().getSize() == 1) {
              newdims.pop_back();
            }
          }
          out_dimsAttachedToVars.emplace_back(make_pair(newvar, newdims));
        };
    for (const auto& m : ungrouped_varList) {
      make_out_dimsAttachedToVars(dimsForNewVars.at(m), m);
    }
    for (const auto& m : new_grouped_vars) {
      make_out_dimsAttachedToVars(dimsForNewVars.at(m.second), m.second);
    }
    out.vars.attachDimensionScales(out_dimsAttachedToVars);
  }
  
  cout << "\n Copying data:\n";

  // Copy over all data.
  // Do this for both variables and scales!
  for (const auto& oldvar : ungrouped_varList) {
    cout << "  " << oldvar.name << "\n";
    copyData(Vec_Named_Variable{oldvar}, newvars[oldvar.name], out, string(""), { });
  }
  // If we have grouped variables, create a map going from channel number to channel index
  if (old_grouped_vars.size() > 0) {
    std::map<int, int> chanNumToIndex;
    std::vector<int> chanNums;
    out.vars.open("nchans").read<int>(chanNums);
    for (size_t i = 0; i < chanNums.size(); ++i) {
      chanNumToIndex[chanNums[i]] = i;
    }

    for (const auto& v : old_grouped_vars) {
      cout << "  " << v.first.name << "\n";
      copyData(v.second, newvars[v.first.name], out, v.first.name, chanNumToIndex);
    }
  }


  return true;
}

int main(int argc, char** argv) {
  using namespace std;
  try {
    // Program options
    auto doHelp = []() {
      cerr << "Usage: ioda-upgrade-v1-to-v2.x [-n] input_file output_file\n"
           << "       -n: do not group similar variables into one 2D varible\n";
      exit(1);
    };
    // quick and dirty argument parsing meant to hold us over until the YAML
    // configuration is implemented
    string sInputFile;
    string sOutputFile;
    bool groupVariables = true;
    if (argc == 3) {
      sInputFile = argv[1];
      sOutputFile = argv[2];
    } else if ((argc == 4) && (strcmp(argv[1],"-n") == 0)) {
      sInputFile = argv[2];
      sOutputFile = argv[3];
      groupVariables = false;
    } else {
      doHelp();
    }

    // Parse YAML file here
    // Unimplemented

    cout << "Input: " << sInputFile << "\nOutput: " << sOutputFile << endl;
    UpgradeParameters params;
    params.groupSimilarVariables = groupVariables;
    upgradeFile(sInputFile, sOutputFile, params);
    cout << " Success!\n";

  } catch (const std::exception& e) {
    cerr << "Exception: " << e.what() << endl << endl;
    return 1;
  } catch (...) {
    cerr << "An uncaught exception occurred." << endl << endl;
    return 1;
  }
  return 0;
}
