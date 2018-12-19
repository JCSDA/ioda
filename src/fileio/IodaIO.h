/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef FILEIO_IODAIO_H_
#define FILEIO_IODAIO_H_

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include <boost/any.hpp>

#include "eckit/mpi/Comm.h"

#include "distribution/Distribution.h"
#include "oops/util/Printable.h"

// Forward declarations
namespace eckit {
  class Configuration;
}

namespace ioda {

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
 * There are four dimensions defined in the file:
 *
 *   nlocs: number of locations (length of each variable)
 *   nvars: number of variables
 *   nobs:  number of observations (equal to nlocs * nvars)
 *   nrecs: number of records
 *
 * A record is an atomic unit that is to stay intact when distributing
 * observations across multiple processes.
 *
 * Older netcdf files have either a single variable, or have multiple variables
 * (satellite channels, eg) flattened out into a single variable. These
 * vectors are nobs long. Locations will be repeated in the case of multiple
 * variables so the ObsSpace constructor (client of this class) needs to
 * reshape these vectors into a set of variables that correspond to the new
 * file format above.
 *
 * For now, limit the write interface to writing 1D vectors that are nlocs
 * in length. This may be too restrictive, so we should revisit this in the future.
 *
 * The constructor that you fill in a subclass is responsible for:
 *    1. Open the file
 *         The file name and mode (read, write) is passed in to the subclass
 *         constructor via a call to the factory method Create in the class IodaIOfactory.
 *    2. The following data members are set according to the file mode
 *         * nlocs_
 *         * nobs_
 *         * nrecs_
 *         * nvars_
 *         * vname_group_type_
 *
 *       If in read mode, metadata from the input file are used to set the data members
 *       If in write mode, the data members are set from the constructor arguments 
 *
 * \author Stephen Herbener (JCSDA)
 */

class IodaIO : public util::Printable {
 public:
    explicit IodaIO(const eckit::mpi::Comm &);
    virtual ~IodaIO() = 0;

    // Methods provided by subclasses
    virtual void ReadVar_any(const std::string & VarName, boost::any * VarData) = 0;

    virtual void ReadVar(const std::string & VarName, int* VarData) = 0;
    virtual void ReadVar(const std::string & VarName, float* VarData) = 0;
    virtual void ReadVar(const std::string & VarName, double* VarData) = 0;

    virtual void WriteVar_any(const std::string & VarName, boost::any * VarData) = 0;
    virtual void WriteVar(const std::string & VarName, int* VarData) = 0;
    virtual void WriteVar(const std::string & VarName, float* VarData) = 0;
    virtual void WriteVar(const std::string & VarName, double* VarData) = 0;

    virtual void ReadDateTime(uint64_t* VarDate, int* VarTime)= 0;

    // Methods inherited from base class
    std::string fname() const;
    std::string fmode() const;

    std::size_t nlocs() const;
    std::size_t nobs() const;
    std::size_t nrecs() const;
    std::size_t nvars() const;
    const eckit::mpi::Comm & comm() const {return commMPI_;}
    std::vector<std::tuple<std::string, std::string>> * const varlist();

 protected:
    // Methods provided by subclasses

    // Methods inherited from base class

    // Data members

    /*! \brief file name */
    std::string fname_;

    /*! \brief file mode ("r" -> read, "w" -> overwrite, "W" -> create and write) */
    std::string fmode_;

    /*! \brief number of unique locations in domain*/
    std::size_t nlocs_;

    /*! \brief number of unique locations in file*/
    std::size_t nfvlen_;

    /*! \brief number of unique observations */
    std::size_t nobs_;

    /*! \brief number of unique records */
    std::size_t nrecs_;

    /*! \brief number of unique variables */
    std::size_t nvars_;

    /*! \brief MPI communicator */
    const eckit::mpi::Comm & commMPI_;

    /*! \brief This missing value will be used to fill the missing data slots. */
    double missingvalue_;

    /*! \brief Distribution among processors */
    std::unique_ptr<Distribution> dist_;

    /*! \brief Variable Name : Group Name */
    std::vector<std::tuple<std::string, std::string>> vname_group_;
};

}  // namespace ioda

#endif  // FILEIO_IODAIO_H_
