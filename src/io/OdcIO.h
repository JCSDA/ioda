/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IO_ODCIO_H_
#define IO_ODCIO_H_

#include <map>
#include <string>
#include <vector>

#include "odc/api/odc.h"

#include "oops/util/ObjectCounter.h"

#include "ioda/io/IodaIO.h"

////////////////////////////////////////////////////////////////////////
// Implementation of IodaIO for ODB.
////////////////////////////////////////////////////////////////////////

// Forward declarations
namespace eckit {
  class Configuration;
}

namespace ioda {

/*! \brief Implementation of IodaIO for ODC.
 *
 * \details The OdcIO class defines the constructor and methods for ODB2
 *          file access using the ODC API. These fill in the abstract base
 *          class IodaIO methods.
 *
 * \author Stephen Herbener (JCSDA)
 */
class OdcIO : public IodaIO, private util::ObjectCounter<OdcIO> {
 public:
  /*!
   * \brief classname method for object counter
   *
   * \details This method is supplied for the ObjectCounter base class.
   *          It defines a name to identify an object of this class
   *          for reporting by OOPS.
   */
  static const std::string classname() {return "ioda::OdcIO";}

  OdcIO(const std::string & FileName, const std::string & FileMode,
        const std::size_t MaxFrameSize);
  ~OdcIO();

 private:
  // typedefs
  typedef std::map<std::string, std::size_t> VarIdMap;
  typedef typename VarIdMap::const_iterator VarIdIter;

  // For the oops::Printable base class
  void print(std::ostream & os) const;

  static std::string OdcTypeName(int OdcDataType);

  void CheckOdcCall(int RetCode, std::string & ErrorMsg);

  void DimInsert(const std::string & Name, const std::size_t Size);

  void InitializeFrame();
  void FinalizeFrame();

  void OdcReadVar(const std::size_t VarId, std::vector<int> & VarData);
  void OdcReadVar(const std::size_t VarId, std::vector<float> & VarData);
  void OdcReadVar(const std::size_t VarId, std::vector<std::string> & VarData, bool IsDateTime);

  void OdcCopyVar(int varID, std::vector<int> frameData, const int ncols,
                  double *arrayInOut);
  void OdcCopyVar(int varID, std::vector<float> frameData, const int ncols,
                  double *arrayInOut);
  void OdcCopyVar(int varID, std::vector<std::string> frameData, const int ncols,
                  double *arrayInOut);

  void ReadConvertDateTime(std::vector<std::string> & DtStrings);

  void WriteConvertDateTime(std::vector<std::string> & DtStrings,
                            std::vector<int>  & intDates, std::vector<int> & intTimes);

  void ReadFrame(IodaIO::FrameIter & iframe);
  void WriteFrame(IodaIO::FrameIter & iframe);

  void GrpVarInsert(const std::string & groupName, const std::string & varName,
                    const std::string & varType, const std::vector<std::size_t> & varShape,
                    const std::string & fileVarName, const std::string & fileType,
                    const std::size_t maxStringSize);

  std::size_t var_id_get(const std::string & GrpVarName);

  // Data members

  /*! \brief ODC reader */
  odc_reader_t* odc_reader_;

  /*! \brief ODC frame */
  odc_frame_t* odc_frame_;

  /*! \brief ODC decoder */
  odc_decoder_t* odc_decoder_;

  /*! \brief ODC encoder */
  odc_encoder_t* odc_encoder_;

  /*! \brief ODC frame data */
  double* odc_frame_data_;

  /*! \brief output file descriptor */
  int out_fd_;

  /*! \brief for generating dimension id numbers */
  std::size_t next_dim_id_;

  /*! \brief number of columns in first frame */
  int num_odc_cols_ = 0;

  /*! \brief variable ids (column number from file) */
  VarIdMap var_ids_;

  /*! \brief ODC columnMajor value for writing */
  bool columnMajorWrite_;

  /*! \brief file descriptor for file being written */
  int file_descriptor_;

  /*! \brief Indicator whether ODC has been initialized */
  static bool odc_initialized_;
};

}  // namespace ioda

#endif  // IO_ODCIO_H_
