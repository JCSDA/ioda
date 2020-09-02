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
#include "ioda/Variables/Variable.h"

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
        /// \brief return number of variables from an ObsIo source
        std::size_t numVars();

        /// \brief insert a record into the variable info data structure
        /// \param varName name of variable
        /// \param varSize0 size of first dimension of variable
        /// \param varDtype data type of variable
        /// \param varIsDist true if this variable distributable across MPI tasks
        void insertVarInfo(const std::string & varName, const Dimensions_t varSize0,
                           const std::type_index & varDtype, const bool varIsDist); 

        /// \brief insert a record into the dimension variable info data structure
        /// \param varName name of variable
        /// \param varSize0 size of first dimension of variable
        /// \param varDtype data type of variable
        /// \param varIsDist true if this variable distributable across MPI tasks
        void insertDimVarInfo(const std::string & varName, const Dimensions_t varSize0,
                              const std::type_index & varDtype, const bool varIsDist); 


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

        /// \brief set up frontend and backend selection objects for the given variable
        /// \param varName Name of variable associated with the selection objects
        /// \param feSelect Front end selection object
        /// \param beSelect Back end selection object
        void createFrameSelection(const std::string & varName, Selection & feSelect,
                                  Selection & beSelect);

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
        Dimensions_t varSize0Max();

        /// \brief print() for oops::Printable base class
        /// \param ostream output stream
        virtual void print(std::ostream & os) const = 0;
};

}  // namespace ioda

#endif  // IO_OBSIO_H_
