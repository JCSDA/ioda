/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/ObsSpace.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "eckit/config/Configuration.h"
#include "oops/parallel/mpi/mpi.h"
#include "oops/util/abor1_cpp.h"
#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"

#include "distribution/DistributionFactory.h"
#include "fileio/IodaIO.h"
#include "fileio/IodaIOfactory.h"

namespace ioda {

// -----------------------------------------------------------------------------

ObsSpace::ObsSpace(const eckit::Configuration & config,
                   const util::DateTime & bgn, const util::DateTime & end)
  : oops::ObsSpaceBase(config, bgn, end),
    winbgn_(bgn), winend_(end), commMPI_(oops::mpi::comm()),
    database_()
{
  oops::Log::trace() << "ioda::ObsSpace config  = " << config << std::endl;

  obsname_ = config.getString("ObsType");

  // Open the file and read in variables from the file into the database.
  filein_ = config.getString("ObsData.ObsDataIn.obsfile");
  oops::Log::trace() << obsname_ << " file in = " << filein_ << std::endl;

  InitFromFile(filein_, "r", windowStart(), windowEnd());

  // Check to see if an output file has been requested.
  if (config.has("ObsData.ObsDataOut.obsfile")) {
    std::string filename = config.getString("ObsData.ObsDataOut.obsfile");

    // Find the left-most dot in the file name, and use that to pick off the file name
    // and file extension.
    std::size_t found = filename.find_last_of(".");
    if (found == std::string::npos)
      found = filename.length();

    // Get the process rank number and format it
    std::ostringstream ss;
    ss << "_" << std::setw(4) << std::setfill('0') << comm().rank();

    // Construct the output file name
    fileout_ = filename.insert(found, ss.str());

    // Check to see if user is trying to overwrite an existing file. For now always allow
    // the overwrite, but issue a warning if we are about to clobber an existing file.
    std::ifstream infile(fileout_);
    if (infile.good())
      oops::Log::warning() << "ioda::ObsSpace WARNING: Overwriting output file "
                           << fileout_ << std::endl;
  } else {
    oops::Log::debug() << "ioda::ObsSpace output file is not required " << std::endl;
  }

  oops::Log::trace() << "ioda::ObsSpace contructed name = " << obsname() << std::endl;
}

// -----------------------------------------------------------------------------

ObsSpace::~ObsSpace() {
  oops::Log::trace() << "ioda::ObsSpace destructor begin" << std::endl;
  if (fileout_.size() != 0) {
    oops::Log::info() << obsname() << ": save database to " << fileout_ << std::endl;
    SaveToFile(fileout_);
  } else {
    oops::Log::info() << obsname() << " :  no output" << std::endl;
  }
  oops::Log::trace() << "ioda::ObsSpace destructor end" << std::endl;
}

// -----------------------------------------------------------------------------

void ObsSpace::get_db(const std::string & group, const std::string & name,
                      const size_t & vsize, int vdata[]) const {
  get_db_helper<int>(group, name, vsize, vdata);
}

void ObsSpace::get_db(const std::string & group, const std::string & name,
                      const size_t & vsize, float vdata[]) const {
  get_db_helper<float>(group, name, vsize, vdata);
}

void ObsSpace::get_db(const std::string & group, const std::string & name,
                      const size_t & vsize, double vdata[]) const {
  // load the float values from the database and convert to double
  std::unique_ptr<float> FloatData(new float[vsize]);
  get_db_helper<float>(group, name, vsize, FloatData.get());
  ConvertVarType<float, double>(FloatData.get(), &vdata[0], vsize);
}

void ObsSpace::get_db(const std::string & group, const std::string & name,
                      const size_t & vsize, util::DateTime vdata[]) const {
  get_db_helper<util::DateTime>(group, name, vsize, vdata);
}

template <typename DATATYPE>
void ObsSpace::get_db_helper(const std::string & group, const std::string & name,
                             const size_t & vsize, DATATYPE vdata[]) const {
  std::string gname = (group.size() <= 0)? "GroupUndefined" : group;
  std::vector<std::size_t> vshape(1, vsize);
  database_.LoadFromDb(gname, name, vshape, &vdata[0]);
}

// -----------------------------------------------------------------------------

void ObsSpace::put_db(const std::string & group, const std::string & name,
                      const size_t & vsize, const int vdata[]) {
  put_db_helper<int>(group, name, vsize, vdata);
}

void ObsSpace::put_db(const std::string & group, const std::string & name,
                      const size_t & vsize, const float vdata[]) {
  put_db_helper<float>(group, name, vsize, vdata);
}

void ObsSpace::put_db(const std::string & group, const std::string & name,
                      const size_t & vsize, const double vdata[]) {
  // convert to float, then load into the database
  std::unique_ptr<float> FloatData(new float[vsize]);
  ConvertVarType<double, float>(&vdata[0], FloatData.get(), vsize);
  put_db_helper<float>(group, name, vsize, FloatData.get());
}

template <typename DATATYPE>
void ObsSpace::put_db_helper(const std::string & group, const std::string & name,
                             const std::size_t & vsize, const DATATYPE vdata[]) {
  std::string gname = (group.size() <= 0)? "GroupUndefined" : group;
  std::vector<std::size_t> vshape(1, vsize);
  database_.StoreToDb(gname, name, vshape, &vdata[0]);
}

// -----------------------------------------------------------------------------

bool ObsSpace::has(const std::string & group, const std::string & name) const {
  return database_.has(group, name);
}
// -----------------------------------------------------------------------------

std::size_t ObsSpace::nlocs() const {
  return nlocs_;
}

// -----------------------------------------------------------------------------

std::size_t ObsSpace::nrecs() const {
  return nrecs_;
}

// -----------------------------------------------------------------------------

std::size_t ObsSpace::nvars() const {
  return nvars_;
}

// -----------------------------------------------------------------------------

void ObsSpace::generateDistribution(const eckit::Configuration & conf) {
  int fvlen  = conf.getInt("nobs");
  float lat  = conf.getFloat("lat");
  float lon1 = conf.getFloat("lon1");
  float lon2 = conf.getFloat("lon2");

  // Apply the round-robin distribution, which yields the size and indices that
  // are to be selected by this process element out of the file.
  DistributionFactory * distFactory;
  Distribution * dist{distFactory->createDistribution("roundrobin")};
  dist->distribution(comm(), fvlen);
  int nlocs = dist->size();

  // For now, set nvars to one.
  int nvars = 1;

  // Record obs type
  std::string MyObsType = conf.getString("ObsType");
  oops::Log::info() << obsname() << " : " << MyObsType << std::endl;

  // Create variables and generate the values specified by the arguments.
  std::unique_ptr<double[]> latitude {new double[nlocs]};
  for (std::size_t ii = 0; ii < nlocs; ++ii) {
    latitude.get()[ii] = static_cast<double>(lat);
  }
  put_db("", "latitude", nlocs, latitude.get());

  std::unique_ptr<double[]> longitude {new double[nlocs]};
  for (std::size_t ii = 0; ii < nlocs; ++ii) {
    longitude.get()[ii] = static_cast<double>(lon1 + (ii-1)*(lon2-lon1)/(nlocs-1));
  }
  put_db("", "longitude", nlocs, longitude.get());
}

// -----------------------------------------------------------------------------

void ObsSpace::print(std::ostream & os) const {
  os << "ObsSpace::print not implemented";
}

// -----------------------------------------------------------------------------

  void ObsSpace::InitFromFile(const std::string & filename, const std::string & mode,
                              const util::DateTime &, const util::DateTime &) {
    oops::Log::trace() << "ioda::ObsSpace opening file: " << filename << std::endl;

    // Open the file for reading and record nlocs and nvars from the file.
    std::unique_ptr<IodaIO> fileio {ioda::IodaIOfactory::Create(filename, mode)};
    file_nlocs_ = fileio->nlocs();
    nvars_ = fileio->nvars();
    nrecs_ = fileio->nrecs();

    // Create the MPI distribution
    DistributionFactory * DistFactory;
    dist_.reset(DistFactory->createDistribution("roundrobin"));
    dist_->distribution(commMPI_, file_nlocs_);

    // Read in the datetime values and filter out any variables outside the
    // timing window.
    std::unique_ptr<char[]> dt_char_array_(new char[file_nlocs_ * 20]);
    std::vector<std::size_t> dt_shape_{ file_nlocs_, 20 };
    // Look for datetime@MetaData first, then datetime@GroupUndefined
    std::string DtGroupName = "MetaData";
    std::string DtVarName = "datetime";
    if (!fileio->grp_var_exist(DtGroupName, DtVarName)) {
      DtGroupName = "GroupUndefined";
      if (!fileio->grp_var_exist(DtGroupName, DtVarName)) {
        std::string ErrorMsg = "ObsSpace::InitFromFile: datetime information is not available";
        ABORT(ErrorMsg);
      }
    }
    fileio->ReadVar(DtGroupName, DtVarName, dt_shape_, dt_char_array_.get());
    std::vector<std::string> dt_strings_ =
             CharArrayToStringVector(dt_char_array_.get(), dt_shape_);

    std::vector<std::size_t> to_be_removed;
    std::size_t index;
    for (std::size_t ii = 0; ii < dist_->size(); ++ii) {
      index = dist_->index()[ii];
      util::DateTime test_dt_(dt_strings_[index]);
      if ((test_dt_ <= winbgn_) || (test_dt_ > winend_)) {
        // Outside of the DA time window
        to_be_removed.push_back(index);
      }
    }

    for (std::size_t ii = 0; ii < to_be_removed.size(); ++ii) {
      dist_->erase(to_be_removed[ii]);
    }

    nlocs_ = dist_->size();

    // Read in all variables from the file and store them into the database.
    for (IodaIO::GroupIter igrp = fileio->group_begin();
                           igrp != fileio->group_end(); ++igrp) {
      for (IodaIO::VarIter ivar = fileio->var_begin(igrp);
                           ivar != fileio->var_end(igrp); ++ivar) {
        std::string GroupName = fileio->group_name(igrp);
        std::string VarName = fileio->var_name(ivar);
        std::string FileVarType = fileio->var_dtype(ivar);

        // VarShape, VarSize hold dimension sizes from file.
        // AdjVarShape, AdjVarSize hold dimension sizes needed when the
        // fisrt dimension is nlocs in size. Ie, all variables with nlocs
        // as the first dimension need to be distributed across that dimension.
        std::vector<std::size_t> VarShape = fileio->var_shape(ivar);
        std::size_t VarSize = 1;
        for (std::size_t i = 0; i < VarShape.size(); i++) {
          VarSize *= VarShape[i];
        }

        // Get the desired data type for the database.
        std::string DbVarType = DesiredVarType(GroupName, FileVarType);

        // Read the variable from the file and transfer it to the database.
        if (FileVarType.compare("int") == 0) {
          std::unique_ptr<int> FileData(new int[VarSize]);
          fileio->ReadVar(GroupName, VarName, VarShape, FileData.get());

          std::unique_ptr<int> IndexedData;
          std::vector<std::size_t> IndexedShape;
          std::size_t IndexedSize;
          ApplyDistIndex<int>(FileData, VarShape, IndexedData, IndexedShape, IndexedSize);
          database_.StoreToDb(GroupName, VarName, IndexedShape, IndexedData.get());
        } else if (FileVarType.compare("float") == 0) {
          std::unique_ptr<float> FileData(new float[VarSize]);
          fileio->ReadVar(GroupName, VarName, VarShape, FileData.get());

          std::unique_ptr<float> IndexedData;
          std::vector<std::size_t> IndexedShape;
          std::size_t IndexedSize;
          ApplyDistIndex<float>(FileData, VarShape, IndexedData, IndexedShape, IndexedSize);

          if (DbVarType.compare("int") == 0) {
            ConvertStoreFileVar<float, int>(GroupName, VarName, IndexedShape,
                                       IndexedSize, IndexedData.get());
          } else {
            database_.StoreToDb(GroupName, VarName, IndexedShape, IndexedData.get());
          }
        } else if (FileVarType.compare("double") == 0) {
          // Convert double to float before storing into the database.
          std::unique_ptr<double> FileData(new double[VarSize]);
          fileio->ReadVar(GroupName, VarName, VarShape, FileData.get());

          std::unique_ptr<double> IndexedData;
          std::vector<std::size_t> IndexedShape;
          std::size_t IndexedSize;
          ApplyDistIndex<double>(FileData, VarShape, IndexedData, IndexedShape, IndexedSize);

          ConvertStoreFileVar<double, float>(GroupName, VarName, IndexedShape,
                                       IndexedSize, IndexedData.get());
        } else if (FileVarType.compare("char") == 0) {
          // Convert the char array to a vector of strings. If we are working
          // on the variable "datetime", then convert the strings to DateTime
          // objects.
          std::unique_ptr<char> FileData(new char[VarSize]);
          fileio->ReadVar(GroupName, VarName, VarShape, FileData.get());

          std::unique_ptr<char> IndexedData;
          std::vector<std::size_t> IndexedShape;
          std::size_t IndexedSize;
          ApplyDistIndex<char>(FileData, VarShape, IndexedData, IndexedShape, IndexedSize);

          std::vector<std::string> StringData =
                 CharArrayToStringVector(IndexedData.get(), IndexedShape);
          std::vector<std::size_t> AdjVarShape;
          std::size_t AdjVarSize = 1;
          for (std::size_t j = 0; j < (IndexedShape.size()-1); j++) {
            AdjVarShape.push_back(IndexedShape[j]);
            AdjVarSize *= IndexedShape[j];
          }

          if (VarName.compare("datetime") == 0) {
            std::vector<util::DateTime> DtData(AdjVarSize);
            for (std::size_t j = 0; j < AdjVarSize; j++) {
              util::DateTime TempDt(StringData[j]);
              DtData[j] = TempDt;
            }
            database_.StoreToDb(GroupName, VarName, AdjVarShape, DtData.data());
          } else {
            database_.StoreToDb(GroupName, VarName, AdjVarShape, StringData.data());
          }
        } else {
          oops::Log::warning() << "ioda::IodaIO::InitFromFile: Unrecognized file data type: "
                               << FileVarType << std::endl;
          oops::Log::warning() << "  File IO Currently supports data types int, float and char."
                               << std::endl;
          oops::Log::warning() << "  Skipping read of " << VarName << " @ " << GroupName
                               << " from the input file." << std::endl;
        }
      }
    }
    oops::Log::trace() << "ioda::ObsSpaceContainer opening file ends " << std::endl;
  }

// -----------------------------------------------------------------------------

  void ObsSpace::SaveToFile(const std::string & file_name) {
    // Open the file for output
    std::unique_ptr<IodaIO> fileio
      {ioda::IodaIOfactory::Create(file_name, "W", nlocs_, nrecs_, nvars_)};

    // List all records and write out the every record
    for (ObsSpaceContainer::VarIter ivar = database_.var_iter_begin();
      ivar != database_.var_iter_end(); ++ivar) {
      std::string GroupName = database_.var_iter_gname(ivar);
      std::string VarName = database_.var_iter_vname(ivar);
      const std::type_info & VarType = database_.var_iter_type(ivar);
      std::vector<std::size_t> VarShape = database_.var_iter_shape(ivar);
      std::size_t VarSize = database_.var_iter_size(ivar);

      if (VarType == typeid(int)) {
        std::unique_ptr<int> VarData(new int[VarSize]);
        database_.LoadFromDb(GroupName, VarName, VarShape, VarData.get());
        fileio->WriteVar(GroupName, VarName, VarShape, VarData.get());
      } else if (VarType == typeid(float)) {
        std::unique_ptr<float> VarData(new float[VarSize]);
        database_.LoadFromDb(GroupName, VarName, VarShape, VarData.get());
        fileio->WriteVar(GroupName, VarName, VarShape, VarData.get());
      } else if (VarType == typeid(std::string)) {
        std::vector<std::string> VarData(VarSize, "");
        database_.LoadFromDb(GroupName, VarName, VarShape, VarData.data());

        // Get the shape needed for the character array, which will be a 2D array.
        // The total number of char elelments will be CharShape[0] * CharShape[1].
        std::vector<std::size_t> CharShape = CharShapeFromStringVector(VarData);
        std::unique_ptr<char> CharData(new char[CharShape[0] * CharShape[1]]);
        StringVectorToCharArray(VarData, CharShape, CharData.get());
        fileio->WriteVar(GroupName, VarName, CharShape, CharData.get());
      } else if (VarType == typeid(util::DateTime)) {
        util::DateTime TempDt("0000-01-01T00:00:00Z");
        std::vector<util::DateTime> VarData(VarSize, TempDt);
        database_.LoadFromDb(GroupName, VarName, VarShape, VarData.data());

        // Convert the DateTime vector to a string vector, then save into the file.
        std::vector<std::string> StringVector(VarSize, "");
        for (std::size_t i = 0; i < VarSize; i++) {
          StringVector[i] = VarData[i].toString();
        }
        std::vector<std::size_t> CharShape = CharShapeFromStringVector(StringVector);
        std::unique_ptr<char> CharData(new char[CharShape[0] * CharShape[1]]);
        StringVectorToCharArray(StringVector, CharShape, CharData.get());
        fileio->WriteVar(GroupName, VarName, CharShape, CharData.get());
      } else {
        oops::Log::warning() << "ioda::IodaIO::SaveToFile: Unrecognized data type: "
                             << VarType.name() << std::endl;
        oops::Log::warning() << "  ObsSpaceContainer currently supports data types "
                             << "int, float and char." << std::endl;
        oops::Log::warning() << "  Skipping save of " << VarName << " @ " << GroupName
                             << " from the input file." << std::endl;
      }
    }
  }

// -----------------------------------------------------------------------------

std::vector<std::string> ObsSpace::CharArrayToStringVector(const char * CharData,
                                            const std::vector<std::size_t> & CharShape) {
  // CharShape[0] is the number of strings
  // CharShape[1] is the length of each string
  std::size_t Nstrings = CharShape[0];
  std::size_t StrLength = CharShape[1];

  std::vector<std::string> StringVector(Nstrings, "");
  for (std::size_t i = 0; i < Nstrings; i++) {
    // Copy characters for i-th string into a char vector
    std::vector<char> CharVector(StrLength, ' ');
    for (std::size_t j = 0; j < StrLength; j++) {
      CharVector[j] = CharData[(i*StrLength) + j];
    }

    // Convert the char vector to a single string. Any trailing white space will be
    // included in the string, so strip off the trailing white space.
    std::string String(CharVector.begin(), CharVector.end());
    String.erase(String.find_last_not_of(" \t\n\r\f\v") + 1);
    StringVector[i] = String;
  }

  return StringVector;
}

// -----------------------------------------------------------------------------

std::vector<std::size_t> ObsSpace::CharShapeFromStringVector(
                                  const std::vector<std::string> & StringVector) {
  std::size_t MaxStrLen = 0;
  for (std::size_t i = 0; i < StringVector.size(); i++) {
    std::size_t StrSize = StringVector[i].size();
    if (StrSize > MaxStrLen) {
      MaxStrLen = StrSize;
    }
  }

  std::vector<std::size_t> Shape{ StringVector.size(), MaxStrLen };
  return Shape;
}

// -----------------------------------------------------------------------------

void ObsSpace::StringVectorToCharArray(const std::vector<std::string> & StringVector,
                             const std::vector<std::size_t> & CharShape, char * CharData) {
  // CharShape[0] is the number of strings, and CharShape[1] is the maximum
  // string lenghth. Walk through the string vector, copy the string and fill
  // with white space at the ends of strings if necessary.
  for (std::size_t i = 0; i < CharShape[0]; i++) {
    for (std::size_t j = 0; j < CharShape[1]; j++) {
      std::size_t ichar = (i * CharShape[1]) + j;
      if (j < StringVector[i].size()) {
        CharData[ichar] = StringVector[i].data()[j];
      } else {
        CharData[ichar] = ' ';
      }
    }
  }
}

// -----------------------------------------------------------------------------

template<typename FromType, typename ToType>
void ObsSpace::ConvertVarType(const FromType * FromVar, ToType * ToVar,
                              const std::size_t & VarSize) const {
  std::string FromTypeName = TypeIdName(typeid(FromType));
  std::string ToTypeName = TypeIdName(typeid(ToType));
  // It is assumed that the caller has allocated memory for both input and output
  // variables.
  //
  // Allow the following type changes:
  //      float to double
  //      double to float
  //
  // In any type change, the missing values need to be switched.
  if ((typeid(FromType) == typeid(float)) && (typeid(ToType) == typeid(double))) {
    // float to double
    const float fmiss = util::missingValue(fmiss);
    const double dmiss = util::missingValue(dmiss);
    for (std::size_t i = 0; i < VarSize; i++) {
      if (FromVar[i] == fmiss) {
        ToVar[i] = dmiss;
      } else {
        ToVar[i] = static_cast<double>(FromVar[i]);
      }
    }
  } else if ((typeid(FromType) == typeid(double)) && (typeid(ToType) == typeid(float))) {
    // double to float
    const float fmiss = util::missingValue(fmiss);
    const double dmiss = util::missingValue(dmiss);
    for (std::size_t i = 0; i < VarSize; i++) {
      if (FromVar[i] == dmiss) {
        ToVar[i] = fmiss;
      } else {
        ToVar[i] = static_cast<float>(FromVar[i]);
      }
    }
  } else if ((typeid(FromType) == typeid(float)) && (typeid(ToType) == typeid(int))) {
    // float to int
    const float fmiss = util::missingValue(fmiss);
    const int imiss = util::missingValue(imiss);
    for (std::size_t i = 0; i < VarSize; i++) {
      if (FromVar[i] == fmiss) {
        ToVar[i] = imiss;
      } else {
        ToVar[i] = static_cast<int>(FromVar[i]);
      }
    }
  } else if ((typeid(FromType) == typeid(int)) && (typeid(ToType) == typeid(float))) {
    // float to int
    const float fmiss = util::missingValue(fmiss);
    const int imiss = util::missingValue(imiss);
    for (std::size_t i = 0; i < VarSize; i++) {
      if (FromVar[i] == imiss) {
        ToVar[i] = fmiss;
      } else {
        ToVar[i] = static_cast<float>(FromVar[i]);
      }
    }
  } else if ((typeid(FromType) == typeid(double)) && (typeid(ToType) == typeid(int))) {
    // float to int
    const double dmiss = util::missingValue(dmiss);
    const int imiss = util::missingValue(imiss);
    for (std::size_t i = 0; i < VarSize; i++) {
      if (FromVar[i] == dmiss) {
        ToVar[i] = imiss;
      } else {
        ToVar[i] = static_cast<int>(FromVar[i]);
      }
    }
  } else if ((typeid(FromType) == typeid(int)) && (typeid(ToType) == typeid(double))) {
    // float to int
    const double dmiss = util::missingValue(dmiss);
    const int imiss = util::missingValue(imiss);
    for (std::size_t i = 0; i < VarSize; i++) {
      if (FromVar[i] == imiss) {
        ToVar[i] = dmiss;
      } else {
        ToVar[i] = static_cast<double>(FromVar[i]);
      }
    }
  } else {
    std::string ErrorMsg = "Unsupported variable data type conversion: " +
       FromTypeName + " to " + ToTypeName;
    ABORT(ErrorMsg);
  }
}

// -----------------------------------------------------------------------------

template<typename FileType, typename DbType>
void ObsSpace::ConvertStoreFileVar(const std::string & GroupName, const std::string & VarName,
                   const std::vector<std::size_t> & VarShape, const std::size_t & VarSize,
                   const FileType * VarData) {
  // Print a warning so we know to fix this, and convert the data.
  std::string FileTypeName = TypeIdName(typeid(FileType));
  std::string DbTypeName = TypeIdName(typeid(DbType));
  oops::Log::warning() << "ObsSpace::StoreFileVar: WARNING: input file contains "
                       << "unexpected data type." << std::endl
                       << "  Input file: " << filein_ << std::endl
                       << "  Variable: " << VarName << " @ " << GroupName << std::endl
                       << "  Type in file: " << FileTypeName << std::endl
                       << "  Expected type: " << DbTypeName << std::endl
                       << "Converting data, including missing marks, from "
                       << FileTypeName << " to " << DbTypeName << std::endl << std::endl;

  std::unique_ptr<DbType> DbData(new DbType[VarSize]);
  ConvertVarType<FileType, DbType>(VarData, DbData.get(), VarSize);
  database_.StoreToDb(GroupName, VarName, VarShape, DbData.get());
}

// -----------------------------------------------------------------------------

template<typename VarType>
void ObsSpace::ApplyDistIndex(std::unique_ptr<VarType> & FullData,
                              const std::vector<std::size_t> & FullShape,
                              std::unique_ptr<VarType> & IndexedData,
                              std::vector<std::size_t> & IndexedShape,
                              std::size_t & IndexedSize) {
  IndexedShape = FullShape;
  IndexedSize = 1;
  // Apply the distribution index only when the first dimension size (FullShape[0])
  // is equal to file_nlocs_. Ie, only apply the index to obs data (values, errors
  // and QC marks) and metadata related to the obs data.
  if (FullShape[0] == file_nlocs_) {
    // Need to compute the new Shape and size. Keep track of the number of items
    // between each element of the first dimension (IndexIncrement). IndexIncrement
    // will define the space between each element of the first dimension, which will
    // be general no matter how many dimensions in the variable.
    IndexedShape[0] = nlocs_;
    std::size_t IndexIncrement = 1;
    for (std::size_t i = 0; i < IndexedShape.size(); i++) {
      IndexedSize *= IndexedShape[i];
      if (i > 0) {
        IndexIncrement *= IndexedShape[i];
      }
    }

    IndexedData.reset(new VarType[IndexedSize]);
    for (std::size_t i = 0; i < IndexedShape[0]; i++) {
      for (std::size_t j = 0; j < IndexIncrement; j++) {
        std::size_t isrc = (dist_->index()[i] * IndexIncrement) + j;
        std::size_t idest = (i * IndexIncrement) + j;
        IndexedData.get()[idest] = FullData.get()[isrc];
      }
    }
  } else {
    // Transfer the full data pointer to the indexed data pointer
    for (std::size_t i = 0; i < IndexedShape.size(); i++) {
      IndexedSize *= IndexedShape[i];
    }
    IndexedData.reset(FullData.release());
  }
}

// -----------------------------------------------------------------------------

std::string ObsSpace::DesiredVarType(std::string & GroupName, std::string & FileVarType) {
  // By default, make the DbVarType equal to the FileVarType
  // Exceptions are:
  //   Force the group "PreQC" to an integer type.
  //   Force double to float.
  std::string DbVarType = FileVarType;

  if (GroupName.compare("PreQC") == 0) {
    DbVarType = "int";
  } else if (FileVarType.compare("double") == 0) {
    DbVarType = "float";
  }

  return DbVarType;
}

// -----------------------------------------------------------------------------

std::string ObsSpace::TypeIdName(const std::type_info & TypeId) const {
  std::string TypeName;
  if (TypeId == typeid(int)) {
    TypeName = "integer";
  } else if (TypeId == typeid(float)) {
    TypeName = "float";
  } else if (TypeId == typeid(double)) {
    TypeName = "double";
  } else if (TypeId == typeid(std::string)) {
    TypeName = "string";
  } else if (TypeId == typeid(util::DateTime)) {
    TypeName = "DateTime";
  } else {
    TypeName = TypeId.name();
  }

  return TypeName;
}

// -----------------------------------------------------------------------------

void ObsSpace::printJo(const ObsVector & dy, const ObsVector & grad) {
  oops::Log::info() << "ObsSpace::printJo not implemented" << std::endl;
}

// -----------------------------------------------------------------------------

}  // namespace ioda
