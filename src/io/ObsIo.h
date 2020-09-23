/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IO_OBSIO_H_
#define IO_OBSIO_H_

#include <iostream>
#include <typeindex>
#include <typeinfo>

#include "eckit/config/LocalConfiguration.h"

#include "ioda/distribution/Distribution.h"
#include "ioda/io/ObsFrame.h"
#include "ioda/Misc/Dimensions.h"
#include "ioda/ObsSpaceParameters.h"
#include "ioda/Variables/Variable.h"

#include "oops/util/Logger.h"
#include "oops/util/Printable.h"

////////////////////////////////////////////////////////////////////////
// Base class for observation data IO
////////////////////////////////////////////////////////////////////////

namespace ioda {

/// \brief Implementation of ObsIo base class
/// \author Stephen Herbener (JCSDA)

class ObsIo : public util::Printable {
 public:
    ObsIo(const ObsIoActions action, const ObsIoModes mode, const ObsSpaceParameters & params);
    virtual ~ObsIo() = 0;

    /// \brief return number of maximum variable size (along first dimension)
    Dimensions_t maxVarSize() const {return max_var_size_;}

    /// \brief return number of locations from the source
    Dimensions_t numLocs() const {return nlocs_;}

    /// \brief return number of regular variables from the source
    Dimensions_t numVars() const {return var_list_.size();}

    /// \brief return number of dimension scale variables from the source
    Dimensions_t numDimVars() const {return dim_var_list_.size();}

    /// \brief return list of regular variable names
    std::vector<std::string> varList() const {return var_list_;}

    /// \brief return list of dimension scale variable names
    std::vector<std::string> dimVarList() const {return dim_var_list_;}

    /// \brief open a variable
    Variable openVar(const std::string & varName) const;

    /// \brief access to the vars container in the associated ObsGroup
    Has_Variables & vars() {return obs_group_.vars;}

    /// \brief reset the regular variable list
    void resetVarList();

    /// \brief reset the dimension scale variable list
    void resetDimVarList();

 protected:
    //------------------ protected data members ------------------------------
    /// \brief ObsGroup object representing io source/destination
    ObsGroup obs_group_;

    /// \brief ObsIo action
    ObsIoActions action_;

    /// \brief ObsIo mode
    ObsIoModes mode_;

    /// \brief ObsIo parameter specs
    ObsSpaceParameters params_;

    /// \brief maximum variable size (ie, first dimension size)
    Dimensions_t max_var_size_;

    /// \brief number of locations from source (file or generator)
    Dimensions_t nlocs_;

    /// \brief list of regular variables from source (file or generator)
    std::vector<std::string> var_list_;

    /// \brief list of dimension scale variables from source (file or generator)
    std::vector<std::string> dim_var_list_;

    //------------------ protected functions ----------------------------------
    /// \brief print() for oops::Printable base class
    /// \param ostream output stream
    virtual void print(std::ostream & os) const = 0;

};

}  // namespace ioda

#endif  // IO_OBSIO_H_
