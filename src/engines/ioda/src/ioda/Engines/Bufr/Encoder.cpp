/*
* (C) Copyright 2020 NOAA/NWS/NCEP/EMC
*
* This software is licensed under the terms of the Apache Licence Version 2.0
* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
*/

#include "ioda/Engines/Bufr/Encoder.h"

#include <algorithm>
#include <map>
#include <memory>
#include <sstream>
#include <string>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/exception/Exceptions.h"
#include "ioda/Layout.h"
#include "ioda/Misc/DimensionScales.h"
#include "oops/util/Logger.h"

#include "bufr/DataObject.h"


namespace ioda {
namespace Engines {
namespace Bufr {
  template<typename T>
  struct is_vector : public std::false_type {};

  template<typename T, typename A>
  struct is_vector<std::vector<T, A>> : public std::true_type {};

  template <typename T>
  class GlobalWriter : public bufr::encoders::GlobalWriter<T>
  {
  public:
    GlobalWriter() = delete;
    GlobalWriter(ioda::Group& group) : group_(group) {}

    void write(const std::string& name, const T& data) final
    {
      _write(name, data);
    }

  private:
    ioda::Group& group_;

    template<typename U = void>
    void _write(const std::string& name,
                const T& data,
                typename std::enable_if<!is_vector<T>::value, U>::type* = nullptr)
    {
      ioda::Attribute attr = group_.atts.create<T>(name, {1});
      attr.write<T>(data);
    }

    // T is a vector
    template<typename U = void>
    void _write(const std::string& name,
                const T& data,
                typename std::enable_if<is_vector<T>::value, U>::type* = nullptr)
    {
      ioda::Attribute attr = group_.atts.create<typename T::value_type>(name, \
                                               {static_cast<int>(data.size())});
      attr.write<typename T::value_type>(data);
    }
  };

  template <typename T>
  class VarWriter : public bufr::ObjectWriter<T>
  {
  public:
    VarWriter() = delete;
    VarWriter(ioda::ObsGroup& group,
              const std::string& name,
              const std::vector<ioda::Dimensions_t>& chunks,
              int compressionLevel,
              const std::vector<ioda::Variable>& dimensions) :
                 group_(group),
                 name_(name),
                 chunks_(chunks),
                 compressionLevel_(compressionLevel),
                 dimensions_(dimensions)
    {
    }

    void write(const std::vector<T>& data) final
    {
      if (group_.vars.exists(name_))
      {
        group_.vars[name_].write(data);
      }
      else
      {
        auto params = makeCreationParams(chunks_, compressionLevel_);
        auto var = group_.vars.createWithScales<T>(name_, dimensions_, params);
        var.write(data);
      }
    }

  private:
    ioda::ObsGroup& group_;

    std::string name_;
    std::vector<ioda::Dimensions_t> chunks_;
    int compressionLevel_;
    std::vector<ioda::Variable> dimensions_;

    /// \brief Make the variable creation parameters.
    /// \param chunks The chunk sizes
    /// \param compressionLevel The compression level
    /// \return The variable creation patterns.
    ioda::VariableCreationParameters makeCreationParams(
      const std::vector<ioda::Dimensions_t>& chunks,
      int compressionLevel) const
    {
      ioda::VariableCreationParameters params;
      params.chunk = true;
      params.chunks = chunks;
      params.compressWithGZIP(compressionLevel);
      params.setFillValue<T>(bufr::DataObject<T>::missingValue());

      return params;
    }
  };

  template <>
  class VarWriter<std::string> : public bufr::ObjectWriter<std::string>
  {
  public:
    VarWriter() = delete;
    VarWriter(ioda::ObsGroup& group,
              const std::string& name,
              const std::vector<ioda::Dimensions_t>& chunks,
              int compressionLevel,
              const std::vector<ioda::Variable>& dimensions) :
                group_(group),
                name_(name),
                chunks_(chunks),
                compressionLevel_(compressionLevel),
                dimensions_(dimensions)
    {
    }

    void write(const std::vector<std::string>& data) final
    {
      auto params = makeCreationParams(chunks_, compressionLevel_);
      auto var = group_.vars.createWithScales<std::string>(name_, dimensions_, params);
      var.write(data);
    }

  private:
    ioda::ObsGroup& group_;

    std::string name_;
    std::vector<ioda::Dimensions_t> chunks_;
    int compressionLevel_;
    std::vector<ioda::Variable> dimensions_;

    /// \param chunks The chunk sizes
    /// \param compressionLevel The compression level
    /// \return The variable creation patterns.
    ioda::VariableCreationParameters makeCreationParams(
      const std::vector<ioda::Dimensions_t>& chunks,
      int compressionLevel) const
    {
      ioda::VariableCreationParameters params;
      params.chunk = true;
      params.chunks = chunks;
      params.compressWithGZIP(compressionLevel);
      params.setFillValue<std::string>(bufr::DataObject<std::string>::missingValue());

      return params;
    }
  };

  static const char* LocationName = "Location";
  static const char* DefualtDimName = "dim";

  Encoder::Encoder(const std::string& yamlPath) :
    description_(bufr::encoders::Description(yamlPath))
  {
  }

  Encoder::Encoder(const bufr::encoders::Description& description) :
    description_(description)
  {
  }

  Encoder::Encoder(const eckit::Configuration& conf) :
    description_(bufr::encoders::Description(conf))
  {
  }

  std::map<bufr::SubCategory, ioda::ObsGroup>
  Encoder::encode(const std::shared_ptr<bufr::DataContainer>& dataContainer, bool append)
  {
    auto backendParams = ioda::Engines::BackendCreationParameters();
    std::map<bufr::SubCategory, ioda::ObsGroup> obsGroups;

    // Get the named dimensions
    NamedPathDims namedLocDims;
    NamedPathDims namedExtraDims;

    // Get a list of all the named dimensions
    {
      std::set<std::string> dimNames;
      std::set<bufr::Query> dimPaths;
      for (const auto& dim : description_.getDims())
      {
        if (dimNames.find(dim.name) != dimNames.end())
        {
          throw eckit::UserError("ioda::dimensions: Duplicate dimension name: " + dim.name);
        }

        dimNames.insert(dim.name);

        // Validate the dimension paths so that we don't have duplicates and they all start
        // with a *.
        for (auto path : dim.paths)
        {
          if (dimPaths.find(path) != dimPaths.end())
          {
            throw eckit::BadParameter("ioda::dimensions: Declared duplicate dim. path: "
                                      + path.str());
          }

          if (path.str().substr(0, 1) != "*")
          {
            std::ostringstream errStr;
            errStr << "ioda::dimensions: ";
            errStr << "Path " << path.str() << " must start with *. ";
            errStr << "Subset specific named dimensions are not supported.";

            throw eckit::BadParameter(errStr.str());
          }

          dimPaths.insert(path);
        }

        namedExtraDims.insert({dim.paths, dim});
      }
    }

    // Got through each unique category
    for (const auto& categories : dataContainer->allSubCategories())
    {
      // Create the dimensions variables
      std::map<std::string, std::shared_ptr<bufr::DimensionDataBase>> dimMap;

      auto dataObjectGroupBy
        = dataContainer->getGroupByObject(description_.getVariables()[0].source, categories);

      // When we find that the primary index is zero we need to skip this category
      if (dataObjectGroupBy->getDims()[0] == 0)
      {
        oops::Log::warning() << "  Category (";
        for (auto category : categories)
        {
          oops::Log::warning() << category;

          if (category != categories.back())
          {
            oops::Log::warning() << ", ";
          }
        }

        oops::Log::warning() << ") was not found in file." << std::endl;
      }

      // Create the root Location dimension for this category
      auto rootDim
        = std::make_shared<bufr::DimensionData<int>>(LocationName, dataObjectGroupBy->getDims()[0]);
      dimMap[LocationName] = rootDim;

      // Add the root Location dimension as a named dimension
      auto rootLocation = bufr::encoders::DimensionDescription();
      rootLocation.name = LocationName;
      rootLocation.source = "";
      namedLocDims[{dataObjectGroupBy->getDimPaths()[0]}] = rootLocation;

      // Create the dimension data for dimensions which include source data
      for (const auto& dimDesc : description_.getDims())
      {
        if (!dimDesc.source.empty())
        {
          auto dataObject = dataContainer->get(dimDesc.source, categories);

          // Validate the path for the source field makes sense for the dimension
          if (std::find(dimDesc.paths.begin(), dimDesc.paths.end(),
                        dataObject->getDimPaths().back())
              == dimDesc.paths.end())
          {
            std::stringstream errStr;
            errStr << "ioda::dimensions: Source field " << dimDesc.source << " in ";
            errStr << dimDesc.name << " is not in the correct path.";
            throw eckit::BadParameter(errStr.str());
          }

          // Create the dimension data
          dimMap[dimDesc.name] = dataObject->createDimensionFromData(
            dimDesc.name, dataObject->getDimPaths().size() - 1);
        }
      }

      // Discover and create the dimension data for dimensions with no source field. If
      // dim is un-named (not listed) then call it dim_<number>
      int autoGenDimNumber = 2;
      for (const auto& varDesc : description_.getVariables())
      {
        auto dataObject = dataContainer->get(varDesc.source, categories);

        for (std::size_t dimIdx = 1; dimIdx < dataObject->getDimPaths().size(); dimIdx++)
        {
          auto dimPath = dataObject->getDimPaths()[dimIdx];
          std::string dimName = "";

          if (existsInNamedPath(dimPath, namedExtraDims))
          {
            dimName = dimForDimPath(dimPath, namedExtraDims).name;
          }
          else
          {
            auto newDimStr = std::ostringstream();
            newDimStr << DefualtDimName << "_" << autoGenDimNumber;

            dimName = newDimStr.str();

            auto dimDesc   = bufr::encoders::DimensionDescription();
            dimDesc.name   = dimName;
            dimDesc.source = "";

            namedExtraDims[{dimPath}] = dimDesc;
            autoGenDimNumber++;
          }

          if (dimMap.find(dimName) == dimMap.end())
          {
            dimMap[dimName] = dataObject->createEmptyDimension(dimName, dimIdx);
          }
        }
      }

      // Make the categories
      size_t catIdx = 0;
      std::map<std::string, std::string> substitutions;
      for (const auto& catPair : dataContainer->getCategoryMap())
      {
        substitutions.insert({catPair.first, categories.at(catIdx)});
        catIdx++;
      }

      // Make the obsstore parameters
      backendParams.openMode   = ioda::Engines::BackendOpenModes::Read_Write;
      backendParams.createMode = ioda::Engines::BackendCreateModes::Truncate_If_Exists;
      backendParams.action     = append ? ioda::Engines::BackendFileActions::Open
                                        : ioda::Engines::BackendFileActions::Create;
      backendParams.flush      = true;
      //     backendParams.allocBytes = dataContainer->size();

      auto rootGroup
        = ioda::Engines::constructBackend(ioda::Engines::BackendNames::ObsStore, backendParams);

      ioda::NewDimensionScales_t allDims;
      for (auto dimPair : dimMap)
      {
        auto dimScale = ioda::NewDimensionScale<int>(dimPair.first, dimPair.second->size());
        allDims.push_back(dimScale);
      }

      auto policy = ioda::detail::DataLayoutPolicy::Policies::ObsGroup;
      auto layoutPolicy = ioda::detail::DataLayoutPolicy::generate(policy);
      auto obsGroup = ioda::ObsGroup::generate(rootGroup, allDims, layoutPolicy);

      // Create Globals
      for (auto& global : description_.getGlobals())
      {
        std::shared_ptr<bufr::encoders::GlobalWriterBase> writer = nullptr;
        if (auto intGlobal
            = std::dynamic_pointer_cast<bufr::encoders::GlobalDescription<int>>(global))
        {
          writer = std::make_shared<GlobalWriter<int>>(rootGroup);
          intGlobal->writeTo(writer);
        }
        else if (auto intGlobal
            = std::dynamic_pointer_cast<bufr::encoders::GlobalDescription<std::vector<int>>>(
              global))
        {
          writer = std::make_shared<GlobalWriter<std::vector<int>>>(rootGroup);
          intGlobal->writeTo(writer);
        }
        else if (auto floatGlobal
                   = std::dynamic_pointer_cast<bufr::encoders::GlobalDescription<float>>(global))
        {
          writer = std::make_shared<GlobalWriter<float>>(rootGroup);
          floatGlobal->writeTo(writer);
        }
        else if (auto floatGlobal = std::dynamic_pointer_cast<
                     bufr::encoders::GlobalDescription<std::vector<float>>>(global))
        {
          writer = std::make_shared<GlobalWriter<std::vector<float>>>(rootGroup);
          floatGlobal->writeTo(writer);
        }
        else if (auto doubleGlobal
                   = std::dynamic_pointer_cast<bufr::encoders::GlobalDescription<std::string>>(
                     global))
        {
          writer = std::make_shared<GlobalWriter<std::string>>(rootGroup);
          doubleGlobal->writeTo(writer);
        }
        else
        {
          throw eckit::BadParameter("Unsupported global type encountered.");
        }
      }

      // Write the Dimension Variables
      for (const auto& dimDesc : description_.getDims())
      {
        if (!dimDesc.source.empty())
        {
          auto dataObject = dataContainer->get(dimDesc.source, categories);
          for (size_t dimIdx = 0; dimIdx < dataObject->getDims().size(); dimIdx++)
          {
            auto dimPath = dataObject->getDimPaths()[dimIdx];

            NamedPathDims namedPathDims;
            if (dimIdx == 0)
            {
              namedPathDims = namedLocDims;
            }
            else
            {
              namedPathDims = namedExtraDims;
            }

            auto dimName    = dimForDimPath(dimPath, namedPathDims).name;
            auto dimensions = std::vector<ioda::Variable>();
            auto chunks     = std::vector<ioda::Dimensions_t>();
            dimMap[dimName]->write(
              std::make_shared<VarWriter<int>>(obsGroup, dimName, chunks, 0, dimensions));
          }
        }
      }

      // Write all the other Variables
      for (const auto& varDesc : description_.getVariables())
      {
        std::vector<ioda::Dimensions_t> chunks;
        auto dimensions = std::vector<ioda::Variable>();
        auto dataObject = dataContainer->get(varDesc.source, categories);
        for (size_t dimIdx = 0; dimIdx < dataObject->getDims().size(); dimIdx++)
        {
          auto dimPath = dataObject->getDimPaths()[dimIdx];

          NamedPathDims namedPathDims;
          if (dimIdx == 0)
          {
            namedPathDims = namedLocDims;
          }
          else
          {
            namedPathDims = namedExtraDims;
          }

          auto dimVar = obsGroup.vars[dimForDimPath(dimPath, namedPathDims).name];
          dimensions.push_back(dimVar);

          if (dimIdx < varDesc.chunks.size())
          {
            chunks.push_back(std::min(dimVar.getChunkSizes()[0],
                                      static_cast<ioda::Dimensions_t>(varDesc.chunks[dimIdx])));
          }
          else
          {
            chunks.push_back(dimVar.getChunkSizes()[0]);
          }
        }

        // Check that dateTime variable has the right dimensions
        if (varDesc.name == "MetaData/dateTime" || varDesc.name == "MetaData/datetime")
        {
          if (dimensions.size() != 1) {
            throw eckit::BadParameter("IODA requires Datetime variable to be one dimensional.");
          }
        }

        if (auto intObj = std::dynamic_pointer_cast<bufr::DataObject<int>>(dataObject))
        {
          auto objectWriter = std::make_shared<VarWriter<int>>(
            obsGroup, varDesc.name, chunks, varDesc.compressionLevel, dimensions);
          intObj->write(objectWriter);
        }
        else if (auto uintObj = std::dynamic_pointer_cast<bufr::DataObject<uint>>(dataObject))
        {
          auto objectWriter = std::make_shared<VarWriter<uint>>(
            obsGroup, varDesc.name, chunks, varDesc.compressionLevel, dimensions);
          uintObj->write(objectWriter);
        }
        else if (auto int64Obj = std::dynamic_pointer_cast<bufr::DataObject<int64_t>>(dataObject))
        {
          auto objectWriter = std::make_shared<VarWriter<int64_t>>(
            obsGroup, varDesc.name, chunks, varDesc.compressionLevel, dimensions);
          int64Obj->write(objectWriter);
        }
        else if (auto uint64Obj = std::dynamic_pointer_cast<bufr::DataObject<uint64_t>>(dataObject))
        {
          auto objectWriter = std::make_shared<VarWriter<uint64_t>>(
            obsGroup, varDesc.name, chunks, varDesc.compressionLevel, dimensions);
          uint64Obj->write(objectWriter);
        }
        else if (auto floatObj = std::dynamic_pointer_cast<bufr::DataObject<float>>(dataObject))
        {
          auto objectWriter = std::make_shared<VarWriter<float>>(
            obsGroup, varDesc.name, chunks, varDesc.compressionLevel, dimensions);
          floatObj->write(objectWriter);
        }
        else if (auto dblObj = std::dynamic_pointer_cast<bufr::DataObject<double>>(dataObject))
        {
          auto objectWriter = std::make_shared<VarWriter<double>>(
            obsGroup, varDesc.name, chunks, varDesc.compressionLevel, dimensions);
          dblObj->write(objectWriter);
        }
        else if (auto strObj = std::dynamic_pointer_cast<bufr::DataObject<std::string>>(dataObject))
        {
          auto objectWriter = std::make_shared<VarWriter<std::string>>(
            obsGroup, varDesc.name, chunks, varDesc.compressionLevel, dimensions);
          strObj->write(objectWriter);
        }
        else
        {
          throw eckit::BadParameter("Unsupported data type encountered.");
        }

        auto var = obsGroup.vars[varDesc.name];
        var.atts.add<std::string>("long_name", {varDesc.longName}, {1});

        if (!varDesc.units.empty())
        {
          var.atts.add<std::string>("units", {varDesc.units}, {1});
        }

        if (varDesc.coordinates)
        {
          var.atts.add<std::string>("coordinates", {*varDesc.coordinates}, {1});
        }

        if (varDesc.range)
        {
          var.atts.add<float>("valid_range", {varDesc.range->start, varDesc.range->end}, {2});
        }
      }

      obsGroups.insert({categories, obsGroup});
    }

   return obsGroups;
  }

  bool Encoder::existsInNamedPath(const bufr::Query& path, const NamedPathDims& pathMap) const
  {
   for (auto& paths : pathMap)
   {
     if (std::find(paths.first.begin(), paths.first.end(), path) != paths.first.end())
     {
       return true;
     }
   }

   return false;
  }

  bufr::encoders::DimensionDescription Encoder::dimForDimPath(const bufr::Query& path,
                                                 const NamedPathDims& pathMap) const
  {
   bufr::encoders::DimensionDescription dimDesc;

   for (auto paths : pathMap)
   {
     if (std::find(paths.first.begin(), paths.first.end(), path) != paths.first.end())
     {
       dimDesc = paths.second;
       break;
     }
   }

   return dimDesc;
  }
}  // namespace Bufr
}  // namespace Engines
}  // namespace ioda
