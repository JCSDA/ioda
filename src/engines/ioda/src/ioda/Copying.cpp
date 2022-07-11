/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Copying.cpp
/// \brief Generic copying facility

#include <functional>
#include <numeric>
#include <unordered_set>

#include "eckit/mpi/Comm.h"

#include "ioda/Attributes/Attribute.h"
#include "ioda/Copying.h"
#include "ioda/Exception.h"
#include "ioda/Group.h"
#include "ioda/Misc/DimensionScales.h"
#include "ioda/Misc/IoPool.h"
#include "ioda/Types/Type.h"
#include "ioda/Types/Type_Provider.h"
#include "ioda/Variables/Variable.h"
#include "ioda/Variables/VarUtils.h"

namespace ioda {

constexpr int mpiTagBase = 20000;
constexpr int varNumTagFactor = 100;

// private functions
template <typename AttrType>
void transferAttribute(const std::string & attrName, const Attribute & srcAttr,
                       Has_Attributes & destAttrs) {
    const std::vector<ioda::Dimensions_t> & attrDims = srcAttr.getDimensions().dimsCur;
    Dimensions_t numElements = srcAttr.getDimensions().numElements;

    // Use span to handle scalar case
    std::vector<AttrType> attrData(numElements);
    gsl::span<AttrType> attrSpan(attrData);
    srcAttr.read<AttrType>(attrSpan);
    destAttrs.add<AttrType>(attrName, attrSpan, attrDims);
}

template <typename VarType>
void transferVarData(const IoPool & ioPool, const Variable & srcVar,
                     const std::string & varName, Group & dest) {
    if (ioPool.rank_pool() >= 0) {

        std::vector<VarType> varData;
        srcVar.read<VarType>(varData);
        Variable destVar = dest.vars.open(varName);
        destVar.write<VarType>(varData);
    }
}

void calcVarStartsCounts(const IoPool & ioPool, const Variable & srcVar,
                         std::vector<std::size_t> & varStarts,
                         std::vector<std::size_t> & varCounts,
                         Dimensions_t & dimFactor) {
    varStarts.clear();
    varCounts.clear();
    std::vector<Dimensions_t> srcDims = srcVar.getDimensions().dimsCur;
    // dimFactor holds the number of elements from the product of the second and
    // higher dimensions sizes.
    dimFactor = 1;
    if (srcDims.size() > 1) {
         dimFactor *= std::accumulate(srcDims.begin() + 1, srcDims.end(), 1, 
            std::multiplies<Dimensions_t>());
    }
    std::size_t start;
    if (ioPool.rank_pool() >= 0) {
        // For ranks in the pool, we are placing the data from the non-pool ranks into
        // a slice of the total data vector. The first rank from the assignment will get
        // its data placed at the end of the data from this rank so it's starting value
        // is at this ranks nlocs value.
        start = ioPool.nlocs() * dimFactor;
    } else {
        // For ranks not in the pool, only sending their slice of data, so the initial
        // start value is always zero.
        start = 0;
    }
    // Walk through the rank assignments and issue receive commands.
    for (auto & rankAssignment : ioPool.rank_assignment()) {
        std::size_t count;
        if (ioPool.rank_pool() >= 0) {
            count = rankAssignment.second * dimFactor;
        } else {
            count = ioPool.nlocs() * dimFactor;
        }
        varStarts.push_back(start);
        varCounts.push_back(count);
        start += count;
    }
}

template <typename VarType>
void transferVarDataMPI(const IoPool & ioPool, const Variable & srcVar,
                        const std::string & varName, int varNumber,
                        const std::vector<std::size_t> & varStarts,
                        const std::vector<std::size_t> & varCounts,
                        Dimensions_t dimFactor, Group & dest) {

    std::vector<VarType> varData;
    srcVar.read<VarType>(varData);
    if (ioPool.rank_pool() >= 0) {
        // Resize varData according to total nlocs.
        Dimensions_t numElements = ioPool.total_nlocs() * dimFactor;
        varData.resize(numElements);

        // Walk through the rank assignments and issue receive commands.
        std::vector<eckit::mpi::Request> recvRequests(ioPool.rank_assignment().size());
        for (std::size_t i = 0; i < ioPool.rank_assignment().size(); ++i) {
            int fromRank = ioPool.rank_assignment()[i].first;
            int tag = mpiTagBase + (varNumber * varNumTagFactor) + fromRank;
            recvRequests[i] = ioPool.comm_all().iReceive(
                varData.data() + varStarts[i], varCounts[i], fromRank, tag);
        }
        ioPool.comm_all().waitAll(recvRequests);
        Variable destVar = dest.vars.open(varName);
        destVar.write<VarType>(varData);
    } else {
        // Non io pool ranks. These ranks will always read their data from src, and send it as
        // is to their assigned io pool rank.
        std::vector<eckit::mpi::Request> sendRequests(ioPool.rank_assignment().size());
        for (std::size_t i = 0; i < ioPool.rank_assignment().size(); ++i) {
            int toRank = ioPool.rank_assignment()[i].first;
            int tag = mpiTagBase + (varNumber * varNumTagFactor) + ioPool.rank_all();
            sendRequests[i] = ioPool.comm_all().iSend(
                varData.data() + varStarts[i], varCounts[i], toRank, tag);
        }
        ioPool.comm_all().waitAll(sendRequests);
    }
}

// template specialization for std::string
template <>
void transferVarDataMPI<std::string>(const IoPool & ioPool, const Variable & srcVar,
                        const std::string & varName, const int varNumber,
                        const std::vector<std::size_t> & varStarts,
                        const std::vector<std::size_t> & varCounts,
                        const Dimensions_t dimFactor, Group & dest) {
    constexpr int maxStringLength = 256;

    std::vector<std::string> varData;
    srcVar.read<std::string>(varData);
    if (ioPool.rank_pool() >= 0) {
        // Resize varData according to total nlocs.
        Dimensions_t numElements = ioPool.total_nlocs() * dimFactor;
        varData.resize(numElements);

        // Walk through the rank assignments and issue receive commands.
        for (std::size_t i = 0; i < ioPool.rank_assignment().size(); ++i) {
            int fromRank = ioPool.rank_assignment()[i].first;
            int tag = mpiTagBase + (varNumber * varNumTagFactor) + fromRank;
            std::vector<char> strBuffer(varCounts[i] * maxStringLength, '\0');
            ioPool.comm_all().receive(strBuffer.data(), strBuffer.size(), fromRank, tag);
            for (std::size_t j = 0; j < varCounts[i]; ++j) {
                std::size_t offset = j * maxStringLength;
                auto strEnd = std::find(strBuffer.begin() + offset, strBuffer.end(), '\0');
                if (strEnd == strBuffer.end()) {
                    throw Exception("End of string not found during MPI transfer", ioda_Here());
                }
                std::string str(strBuffer.begin() + offset, strEnd);
                varData[varStarts[i] + j] = str;
            }
        }
        Variable destVar = dest.vars.open(varName);
        destVar.write<std::string>(varData);
    } else {
        // Non io pool ranks. These ranks will always read their data from src, and send it as
        // is to their assigned io pool rank.
        for (std::size_t i = 0; i < ioPool.rank_assignment().size(); ++i) {
            int toRank = ioPool.rank_assignment()[i].first;
            int tag = mpiTagBase + (varNumber * varNumTagFactor) + ioPool.rank_all();
            std::vector<char> strBuffer(varCounts[i] * maxStringLength, '\0');
            for (std::size_t i = 0; i < varData.size(); ++i) {
                for (std::size_t j = 0; j < varData[i].size(); ++j) {
                    std::size_t bufIndx = (i * maxStringLength) + j;
                    strBuffer[(i * maxStringLength) + j] = varData[i][j];
                }
            }
            ioPool.comm_all().send(strBuffer.data(), strBuffer.size(), toRank, tag);
        }
    }
}

template <typename VarType>
void createVariable(const std::string & varName, const Variable & srcVar,
                      const int adjustNlocs, Has_Variables & destVars) {
    VariableCreationParameters params = srcVar.getCreationParameters(false, false);
    Dimensions varDims = srcVar.getDimensions();
    // If adjust Nlocs is >= 0, this means that this is a variable that needs
    // to be created with the total number of locations from the MPI tasks in the pool.
    if (adjustNlocs >= 0) {
        varDims.dimsCur[0] = adjustNlocs;
        if (varDims.dimsMax[0] != ioda::Unlimited) {
            varDims.dimsMax[0] = adjustNlocs;
        }
    }
    Dimensions_t numElements = varDims.numElements;

    Variable destVar = destVars.create<VarType>(varName, varDims, params);
    copyAttributes(srcVar.atts, destVar.atts);
}

void identifyVarsUsingNlocs(const ioda::VarUtils::VarDimMap & varDimMap,
                            std::unordered_set<std::string> & varsUsingNlocs) {
    // start with nlocs itself
    varsUsingNlocs.clear();
    varsUsingNlocs.insert("nlocs");
    for (auto & varPair : varDimMap) {
        if (varPair.second[0].name == "nlocs") {
            varsUsingNlocs.insert(varPair.first.name);
        }
    }
}

void copyVarData(const ioda::IoPool & ioPool, const ioda::Group & src, ioda::Group & dest){
  // All ranks have a src group, but only the io pool ranks have a dest group.
  // Get a sorted list of variables
  const auto allVarList = src.listObjects(ObjectType::Variable, true);

  // Get the variable dimension information from the src group.
  VarUtils::Vec_Named_Variable regularVarList, dimVarList;
  VarUtils::VarDimMap dimsAttachedToVars;
  Dimensions_t maxVarSize0;  // unused in this function
  VarUtils::collectVarDimInfo(src, regularVarList, dimVarList, dimsAttachedToVars, maxVarSize0);

  // Record in an unordered set the names of variables that are associated with
  // the "nlocs" dimension.
  std::unordered_set<std::string> varsUsingNlocs;
  identifyVarsUsingNlocs(dimsAttachedToVars, varsUsingNlocs);

  // For ranks in the io pool, collect the variable data and write out to the file. The
  // ranks not in the io pool will participate only in the MPI send/recv calls.
  int varNumber = 1;
  for (auto & varName : allVarList.at(ObjectType::Variable)) {
    Variable srcVar = src.vars.open(varName);
    bool varTypeSupported = true;
    // Only the variable using the nlocs dimension will need to use MPI send/recv.
    // If the variable is not using nlocs, then simply transfer data from src to dest.
    if(varsUsingNlocs.count(varName) > 0) {
        // Using nlocs -> calculate the starts and counts for each of the ranks
        // in the rank_assignment_ structure.
        std::vector<std::size_t> varStarts;
        std::vector<std::size_t> varCounts;
        Dimensions_t dimFactor;
        calcVarStartsCounts(ioPool, srcVar, varStarts, varCounts, dimFactor);

        VarUtils::forAnySupportedVariableType(
            srcVar,
            [&](auto typeDiscriminator) {
                typedef decltype(typeDiscriminator) T;
                transferVarDataMPI<T>(ioPool, srcVar, varName, varNumber,
                                        varStarts, varCounts, dimFactor, dest);
            },
            VarUtils::ThrowIfVariableIsOfUnsupportedType(varName));

    } else {
        // Var is not using nlocs -> simply transfer data from this process. Ie, the
        // assumption is that all ranks have the same identical copies of this variable
        // so it works to only write the copy on this process.
        VarUtils::forAnySupportedVariableType(
            srcVar,
            [&](auto typeDiscriminator) {
                typedef decltype(typeDiscriminator) T;
                transferVarData<T>(ioPool, srcVar, varName, dest);
            },
            VarUtils::ThrowIfVariableIsOfUnsupportedType(varName));
    }
    varNumber += 1;
  }
}

// public functions

void copyAttributes(const ioda::Has_Attributes& src, ioda::Has_Attributes& dest) {
  using namespace ioda;
  using namespace std;
  vector<pair<string, Attribute>> srcAtts = src.openAll();

  for (const auto &s : srcAtts) {
    // This set contains the names of atttributes that need to be stripped off of
    // variables coming from the input file. The items in the list are related to
    // dimension scales. In general, when copying attributes, the dimension
    // associations in the output file need to be re-created since they are encoded
    // as object references.
    const set<string> ignored_names{
        "CLASS",
        "DIMENSION_LIST",
        "NAME",
        "REFERENCE_LIST",
        "_FillValue",
        "_NCProperties",
        "_Netcdf4Coordinates",
        "_Netcdf4Dimid",
        "_nc3_strict",
        "suggested_chunk_dim"
        };
    if (ignored_names.count(s.first)) continue;
    if (dest.exists(s.first)) continue;

    if (s.second.isA<int>()) {
        transferAttribute<int>(s.first, s.second, dest);
    } else if (s.second.isA<long>()) {                       // NOLINT
        transferAttribute<long>(s.first, s.second, dest);    // NOLINT
    } else if (s.second.isA<float>()) {
        transferAttribute<float>(s.first, s.second, dest);
    } else if (s.second.isA<double>()) {
        transferAttribute<double>(s.first, s.second, dest);
    } else if (s.second.isA<std::string>()) {
        transferAttribute<std::string>(s.first, s.second, dest);
    } else if (s.second.isA<char>()) {
        transferAttribute<char>(s.first, s.second, dest);
    } else {
        std::string ErrorMsg = std::string("Attribute '") + s.first +
                               std::string("' is not of any supported type");
        throw Exception(ErrorMsg.c_str(), ioda_Here());
    }
  }
}

void ioWriteGroup(const ioda::IoPool & ioPool, const ioda::Group& memGroup,
                  ioda::Group& fileGroup) {
  using namespace ioda;
  using namespace std;

  // NOTE: This routine does not respect hard links for groups,
  // types, and variables. Once hard link support is added to IODA,
  // we will need an expanded listObjects function that
  // respects references.

  // For the ranks in the io pool, we need to first create a file (one per rank) containing
  // the groups, attributes and variables. Ie, a complete file except that the variable
  // data has not been collected and written into the file.
  if (ioPool.rank_pool() >= 0) {
    // Get all variable and group names
    const auto objs = memGroup.listObjects(ObjectType::Ignored, true);

    // Make all groups and copy global group attributes.
    copyAttributes(memGroup.atts, fileGroup.atts);
    for (const auto &g_name : objs.at(ObjectType::Group)) {
        Group old_g = memGroup.open(g_name);
        Group new_g = fileGroup.create(g_name);
        copyAttributes(old_g.atts, new_g.atts);
    }

    // Query old data for dimension mappings
    VarUtils::Vec_Named_Variable regularVarList, dimVarList;
    VarUtils::VarDimMap dimsAttachedToVars;
    Dimensions_t maxVarSize0;  // unused in this function
    VarUtils::collectVarDimInfo(memGroup, regularVarList, dimVarList,
                                dimsAttachedToVars, maxVarSize0);

    // Record in an unordered set the names of variables that are associated with
    // the "nlocs" dimension.
    std::unordered_set<std::string> varsUsingNlocs;
    identifyVarsUsingNlocs(dimsAttachedToVars, varsUsingNlocs);

    // Get the total number of locations from the io pool. Use this to adjust
    // the size of the nlocs dimension.
    int totalNlocs = ioPool.total_nlocs();

    // Make all variables and copy data and most attributes.
    // Dimension mappings & scales are handled later.
    for (const auto& var_name : objs.at(ObjectType::Variable)) {
      // See if nlocs needs to be adjusted. If adjustNlocs is set to -1 then that means
      // do not adjust. The adjustment is necessary if we are collecting data from multiple
      // MPI tasks.
      int adjustNlocs = -1;
      if (varsUsingNlocs.count(var_name)) {
          adjustNlocs = totalNlocs;
      }
      const Variable old_var = memGroup.vars.open(var_name);
      VarUtils::forAnySupportedVariableType(
          old_var,
          [&](auto typeDiscriminator) {
              typedef decltype(typeDiscriminator) T;
              createVariable<T>(var_name, old_var, adjustNlocs, fileGroup.vars);
          },
          VarUtils::ThrowIfVariableIsOfUnsupportedType(var_name));
    }

    // TODO(future): Copy named types

    // TODO(future): Copy soft links and external links

    // Make new dimension scales
    for (auto& dim : dimVarList)
        fileGroup.vars[dim.name].setIsDimensionScale(dim.var.getDimensionScaleName());

    // Attach all dimension scales to all variables.
    // We separate this from the variable creation (above)
    // since we use a collective call for performance.
    vector<pair<Variable, vector<Variable>>> dimsAttachedToNewVars;
    for (const auto &old : dimsAttachedToVars) {
      Variable new_var = fileGroup.vars[old.first.name];
      vector<Variable> new_dims;
      for (const auto &old_dim : old.second) {
          new_dims.push_back(fileGroup.vars[old_dim.name]);
      }
      dimsAttachedToNewVars.push_back(make_pair(new_var, std::move(new_dims)));
    }
    fileGroup.vars.attachDimensionScales(dimsAttachedToNewVars);
  }

  // Next for the ranks in the "all" communicator group, we collectively transfer the
  // variable data and write it into the file. 
  copyVarData(ioPool, memGroup, fileGroup); 
}

}  // namespace ioda
