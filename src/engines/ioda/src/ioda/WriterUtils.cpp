/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file WriterUtils.cpp
/// \brief Utilities for a ioda io writer backend

#include "ioda/Io/WriterUtils.h"

#include <functional>
#include <numeric>
#include <unordered_set>

#include "eckit/mpi/Comm.h"

#include "ioda/Copying.h"
#include "ioda/Exception.h"
#include "ioda/Group.h"
#include "ioda/Io/IoPool.h"
#include "ioda/Misc/DimensionScales.h"
#include "ioda/Types/Type.h"
#include "ioda/Types/Type_Provider.h"
#include "ioda/Variables/Variable.h"
#include "ioda/Variables/VarUtils.h"

namespace ioda {

constexpr int mpiTagBase = 20000;
constexpr int varNumTagFactor = 100;

// private functions
Selection createBlockSelection(const std::vector<Dimensions_t> & varShape,
                               const Dimensions_t blockStart,
                               const Dimensions_t blockCount,
                               const bool isFile) {
    // We want the selection to go from blockStart and be of size blockCount
    // in the first dimension of the variable.
    std::vector<Dimensions_t> blockCounts = varShape;
    blockCounts[0] = blockCount;

    // Treat the frame size as multi-dimensioned storage. Take the entire range for
    // the second dimension, third dimension, etc.
    std::vector<Dimensions_t> blockStarts(blockCounts.size(), 0);
    blockStarts[0] = blockStart;
    Selection blockSelect;
    std::vector<Dimensions_t> blockExtent;
    if (isFile) {
        blockExtent = varShape;
    } else {
        blockExtent = blockCounts;
    }
    blockSelect.extent(blockExtent)
               .select({ SelectionOperator::SET, blockStarts, blockCounts });
    return blockSelect;
}

template <typename VarType>
void transferVarData(const IoPool & ioPool, const Variable & srcVar,
                     const std::string & varName, Group & dest, const bool isParallelIo) {
    if (ioPool.rank_pool() >= 0) {

        std::vector<VarType> varData;
        srcVar.read<VarType>(varData);
        Variable destVar = dest.vars.open(varName);
        if (isParallelIo) {
            destVar.parallelWrite<VarType>(varData);
        } else {
            destVar.write<VarType>(varData);
        }
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
                        Dimensions_t dimFactor, Group & dest, const bool isParallelIo) {

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
        if (isParallelIo) {
            Selection memSelect = createBlockSelection(destVar.getDimensions().dimsCur,
                                  0, ioPool.total_nlocs(), false);
            Selection fileSelect = createBlockSelection(destVar.getDimensions().dimsCur,
                                   ioPool.nlocs_start(), ioPool.total_nlocs(), true);
            destVar.parallelWrite<VarType>(varData, memSelect, fileSelect);
        } else {
            destVar.write<VarType>(varData);
        }
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
                        const Dimensions_t dimFactor, Group & dest, const bool isParallelIo) {
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
        if (isParallelIo) {
            Selection memSelect = createBlockSelection(destVar.getDimensions().dimsCur,
                                  0, ioPool.total_nlocs(), false);
            Selection fileSelect = createBlockSelection(destVar.getDimensions().dimsCur,
                                   ioPool.nlocs_start(), ioPool.total_nlocs(), true);
            destVar.parallelWrite<std::string>(varData, memSelect, fileSelect);
        } else {
            destVar.write<std::string>(varData);
        }
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
                    const int adjustNlocs, Has_Variables & destVars,
                    const std::size_t strLen) {
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
    Variable destVar = destVars.create<VarType>(varName, varDims, params);
    copyAttributes(srcVar.atts, destVar.atts);
}

// createVariable specialization for string
template <>
void createVariable<std::string>(const std::string & varName, const Variable & srcVar,
                                 const int adjustNlocs, Has_Variables & destVars,
                                 const std::size_t strLen) {
    // Since the fill value is coming from a variable length string, and we are
    // writing out a fixed length string, the fill value might be a longer length
    // than the string length. For now, record the fill value in an attribute
    // named "_orig_fill_value" and unset the fill value for the fixed length string.
    // The origFillValue attribute can be used by the "convert back to variable length
    // string" application to restore the fill value.
    VariableCreationParameters params = srcVar.getCreationParameters(false, false);
    auto fv = srcVar.getFillValue();
    std::string origFillValue = detail::getFillValue<std::string>(fv);
    params.unsetFillValue();
    Dimensions varDims = srcVar.getDimensions();
    // If adjust Nlocs is >= 0, this means that this is a variable that needs
    // to be created with the total number of locations from the MPI tasks in the pool.
    if (adjustNlocs >= 0) {
        varDims.dimsCur[0] = adjustNlocs;
        if (varDims.dimsMax[0] != ioda::Unlimited) {
            varDims.dimsMax[0] = adjustNlocs;
        }
    }
    // Set the string length in a specialized type.
    Type fixedStrType =
        destVars.getTypeProvider()->makeStringType(typeid(std::string), strLen);

    Variable destVar = destVars.create(varName, fixedStrType, varDims, params);
    copyAttributes(srcVar.atts, destVar.atts);
    destVar.atts.add<std::string>("_orig_fill_value", origFillValue);
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

void copyVarData(const ioda::IoPool & ioPool, const ioda::Group & src, ioda::Group & dest,
                 const VarUtils::Vec_Named_Variable & srcNamedVars,
                 const std::unordered_set<std::string> & varsUsingNlocs,
                 const bool isParallelIo){
  // For ranks in the io pool, collect the variable data and write out to the file. The
  // ranks not in the io pool will participate only in the MPI send/recv calls.
  int varNumber = 1;
  for (auto & srcNamedVar : srcNamedVars) {
    std::string varName = srcNamedVar.name;
    Variable srcVar = srcNamedVar.var;
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
                                      varStarts, varCounts, dimFactor, dest, isParallelIo);
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
                transferVarData<T>(ioPool, srcVar, varName, dest, isParallelIo);
            },
            VarUtils::ThrowIfVariableIsOfUnsupportedType(varName));
    }
    varNumber += 1;
  }
}

// public functions

void calcMaxStringLengths(const ioda::IoPool & ioPool,
                          const VarUtils::Vec_Named_Variable & allVarsList,
                          std::map<std::string, std::size_t> & maxStringLengths) {
    // Only want the members of the io pool participating in this function.
    //
    // Walk through all variables and figure out the max string length which must
    // be done over the entire io pool.
    if (ioPool.comm_pool() != nullptr) {
        maxStringLengths.clear();
        for (auto & namedVar : allVarsList) {
            std::string varName = namedVar.name;
            Variable var = namedVar.var;
            bool isString = var.isA<std::string>();

            if (isString) {
                // Variable is a string type. Read in the values and find the maximum
                // string length. Then do an allReduce to send the maximum of all ranks
                // to every rank.
                std::vector<std::string> varData;
                var.read(varData);
                std::size_t maxStringLen = 0;
                for (std::size_t i = 0; i < varData.size(); ++i) {
                    if (varData[i].size() > maxStringLen) {
                        maxStringLen = varData[i].size();
                    }
                }
                std::size_t globalMaxStringLen;
                ioPool.comm_pool()->
                    allReduce(maxStringLen, globalMaxStringLen, eckit::mpi::max());
                maxStringLengths
                    .insert(std::pair<std::string, std::size_t>(varName, globalMaxStringLen));
            }
        }
    }
}

void ioWriteGroup(const ioda::IoPool & ioPool, const ioda::Group& memGroup,
                  ioda::Group& fileGroup, const bool isParallelIo) {
  using namespace ioda;
  using namespace std;

  // NOTE: This routine does not respect hard links for groups,
  // types, and variables. Once hard link support is added to IODA,
  // we will need an expanded listObjects function that
  // respects references.

  // For the ranks in the io pool, we need to first create a file (one per rank) containing
  // the groups, attributes and variables. Ie, a complete file except that the variable
  // data has not been collected and written into the file.
  std::unordered_set<std::string> varsUsingNlocs;
  VarUtils::Vec_Named_Variable allVarsList;
  std::map<std::string, std::size_t> maxStringLengths;
  if (ioPool.rank_pool() >= 0) {
    // Get all variable and group names
    const auto memObjects = memGroup.listObjects(ObjectType::Ignored, true);

    // Make all groups and copy global group attributes.
    copyAttributes(memGroup.atts, fileGroup.atts);
    for (const auto &g_name : memObjects.at(ObjectType::Group)) {
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

    allVarsList = regularVarList;
    allVarsList.insert(allVarsList.end(), dimVarList.begin(), dimVarList.end());

    // We need to adjust any string variables to be output as char arrays which
    // entails adding an extra dimension for the maximum string length.
    calcMaxStringLengths(ioPool, allVarsList, maxStringLengths);

    // Record in an unordered set the names of variables that are associated with
    // the "nlocs" dimension.
    identifyVarsUsingNlocs(dimsAttachedToVars, varsUsingNlocs);

    // Get the total number of locations from the io pool. Use this to adjust
    // the size of the nlocs dimension. If we are writing one file, use the global nlocs
    // value from the pool. If we are writing multiple files, use the total nlocs value
    // from the pool.
    int poolNlocs;
    if (isParallelIo) {
        poolNlocs = ioPool.global_nlocs();
    } else {
        poolNlocs = ioPool.total_nlocs();
    }

    // Make all variables and copy data and most attributes.
    // Dimension mappings & scales are handled later.
    for (const auto& namedVar : allVarsList) {
      // See if nlocs needs to be adjusted. If adjustNlocs is set to -1 then that means
      // do not adjust. The adjustment is necessary if we are collecting data from multiple
      // MPI tasks.
      std::string var_name = namedVar.name;
      int adjustNlocs = -1;
      if (varsUsingNlocs.count(var_name)) {
          adjustNlocs = poolNlocs;
      }
      const Variable old_var = namedVar.var;
      std::size_t strLen = 0;
      if (maxStringLengths.find(var_name) != maxStringLengths.end()) {
          strLen = maxStringLengths.at(var_name);
      }
      VarUtils::forAnySupportedVariableType(
          old_var,
          [&](auto typeDiscriminator) {
              typedef decltype(typeDiscriminator) T;
              createVariable<T>(var_name, old_var, adjustNlocs, fileGroup.vars, strLen);
          },
          VarUtils::ThrowIfVariableIsOfUnsupportedType(var_name));
    }

    // TODO(future): Copy named types

    // TODO(future): Copy soft links and external links

    // Make new dimension scales
    for (auto& dim : dimVarList) {
        fileGroup.vars[dim.name].setIsDimensionScale(dim.var.getDimensionScaleName());
    }

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
  } else {
    // Not a member of the io pool. Need to set the allVarsList and varsUsingNlocs
    // Call collectVarDimInfo to get the regular and dimension variables lists which
    // can be concatenated to form the proper allVarsList value.
    // Then call identifyVarsUsingNlocs to set varsUsingNlocs.
    VarUtils::Vec_Named_Variable regularVarList, dimVarList;
    VarUtils::VarDimMap dimsAttachedToVars;
    Dimensions_t maxVarSize0;  // unused in this function
    VarUtils::collectVarDimInfo(memGroup, regularVarList, dimVarList,
                                dimsAttachedToVars, maxVarSize0);

    allVarsList = regularVarList;
    allVarsList.insert(allVarsList.end(), dimVarList.begin(), dimVarList.end());

    identifyVarsUsingNlocs(dimsAttachedToVars, varsUsingNlocs);
  }

  // Next for the ranks in the "all" communicator group, we collectively transfer the
  // variable data and write it into the file. 
  copyVarData(ioPool, memGroup, fileGroup, allVarsList, varsUsingNlocs, isParallelIo);
}

}  // namespace ioda
