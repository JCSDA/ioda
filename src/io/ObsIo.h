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

#include "ioda/core/IodaUtils.h"
#include "ioda/distribution/Distribution.h"
#include "ioda/Misc/Dimensions.h"
#include "ioda/Variables/Variable.h"

#include "oops/util/Logger.h"
#include "oops/util/Printable.h"

namespace ioda {

/// \details The ObsIo class along with its subclasses are responsible for providing an obs
/// data source (for the ObsSpace constructor) and an obs data destination (for the
/// ObsSpace destructor). An obs data source can either be a file (obsdatain.obsfile YAML
/// specification) or a "generator" (obsdatain.generate YAML specification). The generator
/// provides a means for creating obs data through YAML specification, which is useful for
/// testing purposes, thus bypassing the need for a file.
///
/// \author Stephen Herbener (JCSDA)

class ObsIo : public util::Printable {
 public:
    ObsIo();
    virtual ~ObsIo() {}

    /// \brief return number of maximum variable size (along first dimension)
    Dimensions_t maxVarSize() const {return max_var_size_;}

    /// \brief return number of locations from the source
    Dimensions_t numLocs() const {return nlocs_;}

    /// \brief return number of regular variables from the source
    Dimensions_t numVars() const {return var_list_.size();}

    /// \brief return number of dimension scale variables from the source
    Dimensions_t numDimVars() const {return dim_var_list_.size();}

    /// \brief return list of regular variable names
    /// \details This routine is only guarenteed to return correct results if
    ///          updateVarDimInfo has been called and the variables and dimensions
    ///          haven't been modified since.
    const VarNameObjectList & varList() const {return var_list_;}

    /// \brief return list of dimension scale variable names
    /// \details This routine is only guarenteed to return correct results if
    ///          updateVarDimInfo has been called and the variables and dimensions
    ///          haven't been modified since.
    const VarNameObjectList & dimVarList() const {return dim_var_list_;}

    /// \brief return map of variables to attached dimension scales
    VarDimMap varDimMap() const {return dims_attached_to_vars_;}

    /// \brief return true if variable's first dimension is nlocs
    /// \param varName variable name to check
    bool isVarDimByNlocs(const std::string & varName) const;

    /// \brief access to the variables container in the associated ObsGroup
    Has_Variables & vars() {return obs_group_.vars;}

    /// \brief access to the attributes container in the associated ObsGroup
    Has_Attributes & atts() {return obs_group_.atts;}

    /// \brief update the variable and dimension information
    void updateVarDimInfo();

    /// \brief return the names of variables to be used to group observations into records
    const std::vector<std::string> &obsGroupingVars() const { return obs_grouping_vars_; }

    /// \brief return true if the locations data (lat, lon, datetime) need to be checked
    virtual bool applyLocationsCheck() const { return true; }

    /// \brief return true if each process generates a separate series of observations
    /// (e.g. read from different files).
    virtual bool eachProcessGeneratesSeparateObs() const { return false; }

 protected:
    //------------------ protected data members ------------------------------
    /// \brief ObsGroup object representing io source/destination
    ObsGroup obs_group_;

    /// \brief maximum variable size (ie, first dimension size)
    Dimensions_t max_var_size_;

    /// \brief number of locations from source (file or generator)
    Dimensions_t nlocs_;

    /// \brief list of regular variables from source (file or generator)
    VarNameObjectList var_list_;

    /// \brief list of dimension scale variables from source (file or generator)
    VarNameObjectList dim_var_list_;

    /// \brief map containing variables with their attached dimension scales
    VarDimMap dims_attached_to_vars_;

    /// \brief names of variables to be used to group observations into records
    std::vector<std::string> obs_grouping_vars_;

    //------------------ protected functions ----------------------------------
    /// \brief print() for oops::Printable base class
    /// \param ostream output stream
    virtual void print(std::ostream & os) const = 0;
};

}  // namespace ioda

#endif  // IO_OBSIO_H_
