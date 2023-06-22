#pragma once
/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <memory>
#include <string>
#include <vector>

#include "eckit/mpi/Comm.h"

#include "oops/util/parameters/OptionalParameter.h"
#include "oops/util/parameters/Parameter.h"
#include "oops/util/parameters/Parameters.h"
#include "oops/util/parameters/RequiredParameter.h"
#include "oops/util/parameters/RequiredPolymorphicParameter.h"
#include "oops/util/Printable.h"

#include "ioda/Engines/EngineUtils.h"
#include "ioda/ObsGroup.h"

/// This file contains the definitions of the base classes for the IODA writer backends.
/// The backends (readers and writers) are the only place where there should exist code
/// that is specific to storage implementations (hdf5 file, odb file, etc.) for ioda engines.
/// Ie, all storage implementation specific code should go into the ioda::Engines namespace.
///
/// There are two base classes, WriterBase and WriterProcBase. The purpose of this is to
/// separate pre- and post-processing steps (that contain storage implementation specific
/// code) from the code that does the writing to to the storage implementation. WriterBase
/// creates a storage backend (eg, hdf5 file) and provides access to the backend.
/// WriteProcBase contains a post function (for now) that is called to do any steps
/// that need to be done after the storage backend is closed (destructed). The potential
/// to add a pre function is there which would run before the storage backend is opened,
/// but a pre-processing step may not ever be necessary for the writers.
///
/// An example post function is the workaround put in place for the hdf5 file that
/// converts fixed length strings to variable length strings. You need the file created
/// by the storage backend to be completely written and closed before running the post step.
///
/// In the case of writing an odb file, you need the files to be completed and closed from
/// all of the pool mpi tasks before you cat them together into a single file in the post
/// function.
///
/// At this point, it will work to use the identical WriterParameters and
/// WriterCreationParameters to create instances of subclasses of both WriterBase and
/// WriteProcBase.

namespace ioda {
namespace Engines {

class Printable;

//----------------------------------------------------------------------------------------
// Writer configuration parameters base class
//----------------------------------------------------------------------------------------

/// \brief Parameters base class for the parameters subclasses associated with the
/// WriterBase subclasses.
class WriterParametersBase : public oops::Parameters {
    OOPS_ABSTRACT_PARAMETERS(WriterParametersBase, Parameters)

 public:
    /// \brief Type of the WriterBase subclass to use.
    oops::RequiredParameter<std::string> type{"type", this};

    /// \brief Path to input file
    oops::RequiredParameter<std::string> fileName{"obsfile", this};

    /// \brief Allow an existing file to be overwritten
    oops::Parameter<bool> allowOverwrite{"allow overwrite", true, this};
};

//----------------------------------------------------------------------------------------
// Writer creation parameters base class
//----------------------------------------------------------------------------------------

class WriterCreationParameters {
  public:
    WriterCreationParameters(const eckit::mpi::Comm & comm, const eckit::mpi::Comm & timeComm,
                             const bool createMultipleFiles, const bool isParallelIo);

    /// \brief io pool communicator group
    const eckit::mpi::Comm & comm;

    /// \brief time communicator group
    const eckit::mpi::Comm & timeComm;

    /// \brief flag indicating how many files to write
    /// \details This flag is being used to handle the case where we have a very large
    /// number of locations to write. This situation could create a file so big that it
    /// becomes unwieldy to handle, so we want a control to be able to choose between
    /// writing one file per MPI task versus writing a single output file (default).
    const bool createMultipleFiles;

    /// \brief flag indicating whether a parallel io backend is to be used
    /// \details This flag is set to true to indicate a parallel io backend (if available)
    /// should be used. In the case of the ODB writer, this flag being true would indicate
    /// that the multiple files created by the io pool should be concatenated together
    /// in the IoPool::finalize() function.
    const bool isParallelIo;
};

//----------------------------------------------------------------------------------------
// Writer base class
//----------------------------------------------------------------------------------------

/// \details The WriterBase class along with its subclasses are responsible for providing
/// a ioda engines Group object that is backed up by a particular ioda engines backend
/// for the purpose of writing out obs data.
///
/// \author Stephen Herbener (JCSDA)

class WriterBase : public util::Printable {
 public:
    WriterBase(const WriterCreationParameters & createParams);
    virtual ~WriterBase() {}

    /// \brief initialize the engine backend after construction
    virtual void initialize() {};

    /// \brief finalize the engine backend before destruction
    virtual void finalize() {};

    /// \brief return the backend that stores the data
    inline ioda::ObsGroup getObsGroup() { return obs_group_; }

    /// \brief return the backend that stores the data
    inline const ioda::ObsGroup getObsGroup() const { return obs_group_; }

 protected:
    //------------------ protected functions ----------------------------------
    /// \brief print() for oops::Printable base class
    /// \param ostream output stream
    virtual void print(std::ostream & os) const = 0;

    //------------------ protected variables ----------------------------------
    /// \brief ObsGroup container associated with the selected backend engine
    ioda::ObsGroup obs_group_;

    /// \brief creation parameters
    WriterCreationParameters createParams_;
};

//----------------------------------------------------------------------------------------
// Writer pre-/post-processor base class
//----------------------------------------------------------------------------------------

/// \details The WriterProcBase class along with its subclasses are responsible for providing
/// pre and post processing function that perform storage implementation specific steps
/// associated with their corresponding WriterBase subclasses.
///
/// \author Stephen Herbener (JCSDA)

class WriterProcBase : public util::Printable {
 public:
    WriterProcBase(const WriterCreationParameters & createParams);
    virtual ~WriterProcBase() {}

    /// \brief Post processor for running after WriterBase subclasses are finsihed
    /// \detail This post processor is placed here in a separate class structure
    /// from the one built on the WriterBase base class. This is done so that a file
    /// getting written can be completely closed (the WriterBase subclass is destructed)
    /// before calling this post processor.
    virtual void post() = 0;

 protected:
    //------------------ protected functions ----------------------------------
    /// \brief print() for oops::Printable base class
    /// \param ostream output stream
    virtual void print(std::ostream & os) const = 0;


    //------------------ protected variables ----------------------------------
    /// \brief creation parameters
    WriterCreationParameters createParams_;
};


}  // namespace Engines
}  // namespace ioda

