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

#include "ioda/io/ObsIoParameters.h"
#include "ioda/Misc/Dimensions.h"

#include "oops/util/Logger.h"
#include "oops/util/Printable.h"

////////////////////////////////////////////////////////////////////////
// Base class for observation data IO
////////////////////////////////////////////////////////////////////////

namespace ioda {

/*! \brief Implementation of ObsIo base class
 *
 * \author Stephen Herbener (JCSDA)
 */
class ObsIo : public util::Printable {
    public:
        /// \brief ObsGroup object representing io source/destination
        ObsGroup obs_group_;

        ObsIo(const ObsIoActions action, const ObsIoModes mode, const ObsIoParameters & params);
	virtual ~ObsIo() = 0;

        //--------- Access to variable info data structure -------------
        /// \brief return number of variables from and ObsIo source
        std::size_t numVars();

        /// \brief return size of variable's first dimension (dimension number 0)
        /// \param varName name of variable
        Dimensions_t varSize0(const std::string & varName);

        /// \brief return data type of variable
        /// \param varName name of variable
        std::type_index varDtype(const std::string & varName);

        /// \brief return true if variable is to be MPI distributed
        /// \param varName name of variable
        bool varIsDist(const std::string & varName);

        /// \brief return a list of all the dimension scale variables
        std::vector<std::string> listDimVars();

        /// \brief return a list of all the regular variables
        std::vector<std::string> listVars();

        //----------------- Access to frame selection -------------------
        /// \brief initialize for walking through the frames
        void frameInit();

        /// \brief move to the next frame
        void frameNext();

        /// \brief true if a frame is available (not past end of frames)
        bool frameAvailable();

        /// \brief return current frame starting index
        /// \param varName name of variable
        int frameStart();

        /// \brief return current frame count for variable
        /// \details Variables can be of different sizes so it's possible that the
        /// frame has moved past the end of some variables but not so for other
        /// variables. When the frame is past the end of the given variable, this
        /// routine returns a zero to indicate that we're done with this variable.
        /// \param varName name of variable
        int frameCount(const std::string & varName);

    protected:
        //------------------ typedefs ----------------------------------
        /// \brief information about variables in the io object
        /// \details in this context, size0 refers to the size of the first
        /// dimension of the variable. This is important for doing the frame
        /// by frame transfer and for doing the MPI distribution.
        struct VarInfoRec {
            Dimensions_t size0_;
            std::type_index dtype_;
            bool is_dist_;

            VarInfoRec(const Dimensions_t size0, const std::type_index dtype,
                       const bool isDist) : size0_(size0), dtype_(dtype), is_dist_(isDist) {}
        };

        /// \brief variable information map
        typedef std::map<std::string, VarInfoRec> VarInfoMap;

        //------------------ data members ----------------------------------
        /// \brief ObsIo action
        ObsIoActions action_;

        /// \brief ObsIo mode
        ObsIoModes mode_;

        /// \brief ObsIo parameter specs
        ObsIoParameters params_;

        /// \brief maximum frame size
        int max_frame_size_;

        /// \brief maximum variable size
        int max_var_size_;

        /// \brief current frame starting index
        int frame_start_;

        /// \brief ObsGroup dimension scale variable information
        VarInfoMap dim_var_info_;

        /// \brief ObsGroup variable information
        VarInfoMap var_info_;

        //------------------ functions ----------------------------------
        /// \brief get variable size along the first dimension
        Dimensions_t getVarSize0(const std::string & varName);

        /// \brief get variable data type along the first dimension
        std::type_index getVarDtype(const std::string & varName);

        /// \brief true if first dimension is nlocs dimension
        bool getVarIsDist(const std::string & varName);

        /// \brief true if variable is a dimension scale
        bool getVarIsDimScale(const std::string & varName);

        /// \brief get variable size along the first dimension
        Dimensions_t getVarSizeMax();

        /// \brief print() for oops::Printable base class
        /// \param ostream output stream
        virtual void print(std::ostream & os) const = 0;
};

}  // namespace ioda

#endif  // IO_OBSIO_H_
