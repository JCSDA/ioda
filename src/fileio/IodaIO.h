/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef FILEIO_IODAIO_H_
#define FILEIO_IODAIO_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "eckit/mpi/Comm.h"

#include "distribution/Distribution.h"
#include "oops/util/Printable.h"

// Forward declarations
namespace eckit {
  class Configuration;
}

namespace ioda {

//---------------------------------------------------------------------------------------
/*! \brief frame data map
 *
 * \details This class contains the current frame data
 *
 */
template<typename FrameType>
class FrameDataMap {
  private:
    typedef std::map<std::string, std::vector<FrameType>> FrameStore;

  public:
    typedef typename FrameStore::iterator FrameStoreIter;
    FrameStoreIter begin() { return frame_container_.begin(); }
    FrameStoreIter end() { return frame_container_.end(); }

    std::string get_name(FrameStoreIter & Iframe) { return Iframe->first; }
    std::vector<FrameType> get_data(FrameStoreIter & Iframe) { return Iframe->second; }

    void put_data(const std::string & GroupName, const std::string & VarName,
                  const std::vector<FrameType> & VarData) {
      std::string VarGrpName = VarName + "@" + GroupName;
      frame_container_[VarGrpName] = VarData;
    }
  private:
    FrameStore frame_container_;
};

//---------------------------------------------------------------------------------------
/*!
 * \brief File access class for IODA
 *
 * \details The IodaIO class provides the interface for file access. Note that IodaIO is an
 *          abstract base class.
 *
 * Eventually, we want to get to the same file format for every obs type.
 * Currently we are defining this as follows. A file can contain any
 * number of variables. Each variable is a 1D vector that is nlocs long.
 * Variables can contain missing values.
 *
 * There are two dimensions defined in the file:
 *
 *   nlocs: number of locations
 *   nvars: number of variables
 *
 * A record is an atomic unit that is to stay intact when distributing
 * observations across multiple processes.
 *
 * The constructor that you fill in with a subclass is responsible for:
 *    1. Open the file
 *         The file name and mode (read, write) is passed in to the subclass
 *         constructor via a call to the factory method Create in the class IodaIOfactory.
 *    2. The following data members are set according to the file mode
 *         * nlocs_
 *         * nvars_
 *         * grp_var_info_
 *
 *       If in read mode, metadata from the input file are used to set the data members
 *       If in write mode, the data members are set from the constructor arguments 
 *       (grp_var_info_ is not used in the write mode case).
 *
 * \author Stephen Herbener (JCSDA)
 */

class IodaIO : public util::Printable {
 protected:
    // typedefs - these need to defined here before the public typedefs that use them

    // Container for information about a variable. Use a nested map structure with the group
    // name for the key in the outer nest, and the variable name for the key in the inner
    // nest. var_id relates to the variable's id in the file. file_shape relates to the
    // variable's shape in the file, whereas shape relates to the variable's shape internally.
    //
    // The place where file_shape and shape are different, for example, are strings in netcdf
    // files. In the file, a vector of strings is stored as a character array (2D) whereas
    // internally, a vector of strings is stored in the std::vector<std::string> container
    // which is 1D.
    struct VarInfoRec {
      std::string dtype;
      std::size_t var_id;
      std::vector<std::size_t> file_shape;
      std::vector<std::size_t> shape;
      std::vector<std::string> dim_names;
    };

    /*!
     * \brief variable information map
     *
     * \details This typedef is part of the group-variable map which is a nested map
     *          containing information about the variables in the input file (see
     *          GroupVarInfoMap typedef for details).
     */
    typedef std::map<std::string, VarInfoRec> VarInfoMap;

    /*!
     * \brief group-variable information map
     *
     * \details This typedef is defining the group-variable map which is a nested map
     *          containing information about the variables in the input file.
     *          This map is keyed first by group name, then by variable name and is
     *          used to pass information to the caller so that the caller can iterate
     *          through the contents of the input file.
     */
    typedef std::map<std::string, VarInfoMap> GroupVarInfoMap;

    // Container for information about a dimension. Holds dimension id and size.
    struct DimInfoRec {
      std::size_t size;
      int id;
    };

    /*!
     * \brief dimension information map
     *
     * \details This typedef is dimension information map which containes
     *          information about the dimensions of the variables.
     */
    typedef std::map<std::string, DimInfoRec> DimInfoMap;

    // Container for information about frames.
    struct FrameInfoRec {
      std::size_t start;
      std::size_t size;

      // Constructor
      FrameInfoRec(const std::size_t Start, const std::size_t Size) :
          start(Start), size(Size) {}
    };

    /*!
     * \brief frame information map
     *
     * \details This typedef contains information about the frames in the file
     */
    typedef std::vector<FrameInfoRec> FrameInfo;

 public:
    IodaIO(const std::string & FileName, const std::string & FileMode,
           const std::size_t MaxFrameSize);

    virtual ~IodaIO() = 0;

    // Methods provided by subclasses

    // Methods defined in base class
    std::string fname() const;
    std::string fmode() const;

    std::size_t nlocs() const;
    std::size_t nvars() const;

    bool missing_group_names() const;

    // Group level iterator
    /*!
     * \brief group-variable map, group iterator
     *
     * \details This typedef is defining the iterator for the group key in the
     *          group-variable map. See the GrpVarInfoMap typedef for details.
     */
    typedef GroupVarInfoMap::const_iterator GroupIter;
    GroupIter group_begin();
    GroupIter group_end();
    std::string group_name(GroupIter);

    // Variable level iterator
    /*!
     * \brief group-variable map, variable iterator
     *
     * \details This typedef is defining the iterator for the variable key in the
     *          group-variable map. See the GrpVarInfoMap typedef for details.
     */
    typedef VarInfoMap::const_iterator VarIter;
    VarIter var_begin(GroupIter);
    VarIter var_end(GroupIter);
    std::string var_name(VarIter);

    // Access to variable information
    bool grp_var_exists(const std::string &, const std::string &);
    std::string var_dtype(VarIter);
    std::string var_dtype(const std::string &, const std::string &);
    std::vector<std::size_t> var_shape(VarIter);
    std::vector<std::size_t> var_shape(const std::string &, const std::string &);
    std::vector<std::size_t> file_shape(VarIter);
    std::vector<std::size_t> file_shape(const std::string &, const std::string &);
    std::size_t var_id(VarIter);
    std::size_t var_id(const std::string &, const std::string &);

    // Access to dimension information
    typedef DimInfoMap::const_iterator DimIter;
    DimIter dim_begin();
    DimIter dim_end();

    bool dim_exists(const std::string &);

    std::string dim_name(DimIter);
    int         dim_id(DimIter);
    std::size_t dim_size(DimIter);

    std::size_t dim_id_size(const int &);
    std::string dim_id_name(const int &);

    std::size_t dim_name_size(const std::string &);
    int         dim_name_id(const std::string &);

    // Access to data frames
    typedef FrameInfo::const_iterator FrameIter;
    FrameIter frame_begin();
    void frame_next(FrameIter &);
    FrameIter frame_end();

    std::size_t frame_start(FrameIter);
    std::size_t frame_size(FrameIter);

    typedef FrameDataMap<int>::FrameStoreIter FrameIntIter;
    FrameIntIter frame_int_begin() { return int_frame_data_->begin(); }
    FrameIntIter frame_int_end() { return int_frame_data_->end(); }
    std::vector<int> frame_int_get_data(FrameIntIter & iframe) {
      return int_frame_data_->get_data(iframe);
    }
    std::string frame_int_get_name(FrameIntIter & iframe) {
      return int_frame_data_->get_name(iframe);
    }
    void frame_int_put_data(std::string & GroupName, std::string& VarName,
                            std::vector<int> & VarData) {
      int_frame_data_->put_data(GroupName, VarName, VarData);
    }

    typedef FrameDataMap<float>::FrameStoreIter FrameFloatIter;
    FrameFloatIter frame_float_begin() { return float_frame_data_->begin(); }
    FrameFloatIter frame_float_end() { return float_frame_data_->end(); }
    std::vector<float> frame_float_get_data(FrameFloatIter & iframe) {
      return float_frame_data_->get_data(iframe);
    }
    std::string frame_float_get_name(FrameFloatIter & iframe) {
      return float_frame_data_->get_name(iframe);
    }
    void frame_float_put_data(std::string & GroupName, std::string& VarName,
                            std::vector<float> & VarData) {
      float_frame_data_->put_data(GroupName, VarName, VarData);
    }

    typedef FrameDataMap<double>::FrameStoreIter FrameDoubleIter;
    FrameDoubleIter frame_double_begin() { return double_frame_data_->begin(); }
    FrameDoubleIter frame_double_end() { return double_frame_data_->end(); }
    std::vector<double> frame_double_get_data(FrameDoubleIter & iframe) {
      return double_frame_data_->get_data(iframe);
    }
    std::string frame_double_get_name(FrameDoubleIter & iframe) {
      return double_frame_data_->get_name(iframe);
    }
    void frame_double_put_data(std::string & GroupName, std::string& VarName,
                            std::vector<double> & VarData) {
      double_frame_data_->put_data(GroupName, VarName, VarData);
    }

    typedef FrameDataMap<std::string>::FrameStoreIter FrameStringIter;
    FrameStringIter frame_string_begin() { return string_frame_data_->begin(); }
    FrameStringIter frame_string_end() { return string_frame_data_->end(); }
    std::vector<std::string> frame_string_get_data(FrameStringIter & iframe) {
      return string_frame_data_->get_data(iframe);
    }
    std::string frame_string_get_name(FrameStringIter & iframe) {
      return string_frame_data_->get_name(iframe);
    }
    void frame_string_put_data(std::string & GroupName, std::string& VarName,
                            std::vector<std::string> & VarData) {
      string_frame_data_->put_data(GroupName, VarName, VarData);
    }


 protected:
    // Methods provided by subclasses
    virtual void ReadFrame(IodaIO::FrameIter & iframe) = 0;

    // Methods inherited from base class

    void ExtractGrpVarName(const std::string & Name, std::string & GroupName,
                           std::string & VarName);

    // Data members

    /*!
     * \brief file name
     */
    std::string fname_;

    /*!
     * \brief file mode
     *
     * \details File modes that are accepted are: "r" -> read, "w" -> overwrite,
                and "W" -> create and write.
     */
    std::string fmode_;

    /*!
     * \brief number of unique locations
     */
    std::size_t nlocs_;

    /*!
     * \brief number of unique variables
     */
    std::size_t nvars_;

    /*!
     * \brief count of undefined group names
     *
     * \details This data member holds a count of the number of variables in the
     *          netcdf file that are missing a group name (@GroupName suffix).
     */
    std::size_t num_missing_gnames_;

    /*!
     * \brief group-variable information map
     *
     * \details See the GrpVarInfoMap typedef for details.
     */
    GroupVarInfoMap grp_var_info_;

    /*!
     * \brief dimension information map
     */
    DimInfoMap dim_info_;

    /*!
     * \brief frame information vector
     */
    FrameInfo frame_info_;

    /*!
     * \brief maximum frame size
     */
    std::size_t max_frame_size_;

    /*! \brief Containers for file frame */
    std::unique_ptr<FrameDataMap<int>> int_frame_data_;
    std::unique_ptr<FrameDataMap<float>> float_frame_data_;
    std::unique_ptr<FrameDataMap<double>> double_frame_data_;
    std::unique_ptr<FrameDataMap<std::string>> string_frame_data_;
};

}  // namespace ioda

#endif  // FILEIO_IODAIO_H_
