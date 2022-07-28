import datetime as dt
import numpy as np
import os
import ioda

class ObsSpace:

    def __repr__(self):
        return f"ObsSpace({self.name},{self.iodafile})"

    def __str__(self):
        return f"IODA ObsSpace Object - {self.name}"

    def __init__(self, path, dim_dict=None, mode='r', name="NoName", iodalayout=0):
        self.name = name
        if mode not in ['r', 'w' ,'rw']:
            raise TypeError(f"{mode} not one of 'r','w','rw'")
        self.write = True if 'w' in mode else False
        self.read = True if 'r' in mode else False
        # It's possible that path actually is just a file name (without a leading path)
        # when writing to the current working directory. We want to check in the write case
        # that the directory where we want the output to be created exists. If there is no
        # leading path then we are okay since we are in that directory running this script.
        if  os.path.isfile(path) and self.read:
            self.iodafile = path
        elif (os.path.basename(path) == path or os.path.exists(os.path.dirname(path))) and self.write:
            self.iodafile = path
        else:
            raise OSError(f"{path} does not specify a valid path to a file")
        self.iodalayout = iodalayout
        # read obsgroup from IODA
        if self.read:
            self.file, self.obsgroup = self._read_obsgroup(self.iodafile)
            self._def_classvars()
        else:
            if dim_dict is not None:
                self.file, self.obsgroup = self._create_obsgroup(self.iodafile, dim_dict)
                # insert dimension values if necessary
                for key, value in dim_dict.items():
                    if isinstance(value, (list, np.ndarray)):
                        _tmpvar = self.obsgroup.vars.open(key)
                        _tmpvar.writeNPArray.int32(value)
            else:
                raise TypeError("dim_dict is not defined")

    def _def_classvars(self):
        # define some variables
        self.dimensions = self.obsgroup.vars.list()
        self.groups = self.file.listGroups(recurse=True)
        self.attrs = self.obsgroup.atts.list()
        self.nlocs = len(self.obsgroup.vars.open('nlocs').readVector.int())
        self.variables = self.file.listVars(recurse=True)
        self.nvars = len(self.variables)

    def read_attr(self, attrName):
        """
          returns single value or array of values of requested attribute
        """
        _attr = self.obsgroup.atts.open(attrName)
        # figure out what datatype the attribute is
        attrType = _attr.getType()
        attrTypeSize = attrType.getSize()
        if (attrType.getClass() == ioda.TypeClass.Integer):
            if (attrTypeSize == 4):
                # int
                attrval = _attr.readVector.int()
            elif (attrTypeSize == 8):
                # long
                attrval = _attr.readVector.int64()
        elif (attrType.getClass() == ioda.TypeClass.Float):
            if (attrTypeSize == 4):
                # float
                attrval = _attr.readVector.float()
            elif (attrTypeSize == 8):
                # float
                attrval = _attr.readVector.double()
        elif (attrType.getClass() == ioda.TypeClass.String):
            # string
            attrval = _attr.readVector.str()
        else:
            raise TypeError(f"Attribute {attrName} type not supported")

        if len(attrval) == 1:
            attrval = attrval[0]
        return attrval

    def write_attr(self, attrName, attrVal):
        # get type of variable
        try:
            attrType = self.NumpyToIodaDtype(attrVal)
            if attrType == ioda.Types.float:
                if np.array(attrVal).size == 1:
                    self.obsgroup.atts.create(attrName, attrType,
                                     [1]).writeDatum.float(attrVal)
                else:
                    self.obsgroup.atts.create(attrName, attrType,
                                     len(attrVal)).writeVector.float(attrVal)
            elif attrType == ioda.Types.double:
                if np.array(attrVal).size == 1:
                    self.obsgroup.atts.create(attrName, attrType,
                                     [1]).writeDatum.double(attrVal)
                else:
                    self.obsgroup.atts.create(attrName, attrType,
                                     len(attrVal)).writeVector.double(attrVal)
            elif attrType == ioda.Types.int64:
                if np.array(attrVal).size == 1:
                    self.obsgroup.atts.create(attrName, attrType,
                                     [1]).writeDatum.int64(attrVal)
                else:
                    self.obsgroup.atts.create(attrName, attrType,
                                     len(attrVal)).writeVector.int64(attrVal)
            elif attrType == ioda.Types.int32:
                if np.array(attrVal).size == 1:
                    self.obsgroup.atts.create(attrName, attrType,
                                     [1]).writeDatum.int32(attrVal)
                else:
                    self.obsgroup.atts.create(attrName, attrType,
                                     len(attrVal)).writeVector.int32(attrVal)
            # add other elif here TODO
        except AttributeError:  # if string
            # TODO(CoryMartin-NOAA): Sprint PR: see chrono.py for how to write fixed-length string attributes.
            if (type(attrVal) == str):
                attrType = ioda.Types.str
                self.obsgroup.atts.create(
                    attrName, attrType, [1]).writeDatum.str(attrVal)

    def _read_obsgroup(self, path):
        # open the obs group for a specified file and return file and obsgroup objects
        if self.write:
            iodamode = ioda.Engines.BackendOpenModes.Read_Write
        else:
            iodamode = ioda.Engines.BackendOpenModes.Read_Only
        g = ioda.Engines.HH.openFile(
                        name=path,
                        mode=iodamode,
                        )
        dlp = ioda.DLP.DataLayoutPolicy.generate(
                  ioda.DLP.DataLayoutPolicy.Policies(self.iodalayout))
        og = ioda.ObsGroup(g, dlp)
        return g, og

    def _create_obsgroup(self, path, dim_dict):
        # open a new file for writing and return file and obsgroup objects
        g = ioda.Engines.HH.createFile(
                        name=path,
                        mode=ioda.Engines.BackendCreateModes.Truncate_If_Exists,
                        )
        # create list of dims in obs group
        _dim_list = []
        for key, value in dim_dict.items():
            # figure out size of dimension
            if isinstance(value, (list, np.ndarray)):
                dimlen = len(value)
            else:
                dimlen = int(value)
            if key == 'nlocs':
                _nlocs_var = ioda.NewDimensionScale.int32(
                    'nlocs', dimlen, ioda.Unlimited, min(dimlen, 10000))
                _dim_list.append(_nlocs_var)
            else:
                _dim_list.append(ioda.NewDimensionScale.int32(
                key, dimlen, dimlen, dimlen))
        # create obs group
        og = ioda.ObsGroup.generate(g, _dim_list)
        # default compression option
        self._p1 = ioda.VariableCreationParameters()
        self._p1.compressWithGZIP()
        return g, og

    def create_var(self, varname, groupname=None, dtype=np.dtype('float32'),
                   dim_list=['nlocs'], fillval=None):
        # If the variable is dateTime and it is not already in the native int64
        # form, then change the dtype to 'M' which is the numpy datetime object type.
        if (varname == 'MetaData/dateTime') and (dtype != np.dtype('int64')):
            dtype = np.dtype('M')
        dtype_tmp = np.array([],dtype=dtype)
        typeVar = self.NumpyToIodaDtype(dtype_tmp)
        _varstr = varname
        if groupname is not None:
            _varstr = f"{groupname}/{varname}"
        # get list of dimension variables
        dims = [self.obsgroup.vars.open(dim) for dim in dim_list]
        fparams = self._p1 # default values
        # replace default fill value
        if fillval is not None:
            _p1 = ioda.VariableCreationParameters()
            _p1.compressWithGZIP()
            _p1 = self.setFillValue(fparams, typeVar, fillval)
            fparams = _p1
        newVar = self.file.vars.create(_varstr, typeVar,
                                       scales=dims, params=fparams)

    def setFillValue(self, params, datatype, value):
        # set fill value for input VariableCreationParameters,
        # datatype, and value and return a new VariableCreationParameters
        # object with the new fill value
        if datatype == ioda.Types.float:
            params.setFillValue.float(value)
        elif datatype == ioda.Types.double:
            params.setFillValue.double(value)
        elif datatype == ioda.Types.int64:
            params.setFillValue.int64(value)
        elif datatype == ioda.Types.int32:
            params.setFillValue.int32(value)
        elif datatype == ioda.Types.str:
            params.setFillValue.str(value)
        elif datatype == ioda.Types.datetime:
            params.setFillValue.datetime(dt.datetime(2200, 1, 1, tzinfo=dt.timezone.utc))
        # add other elif here TODO
        return params

    def Variable(self, varname, groupname=None):
        return self._Variable(self, varname, groupname)

    def NumpyToIodaDtype(self, NumpyArr):
        ############################################################
        # This method converts the numpy data type to the
        # corresponding ioda datatype

        NumpyDtype = NumpyArr.dtype

        if (NumpyDtype == np.dtype('float64')):
            IodaDtype = ioda.Types.double    # convert double to float
        elif (NumpyDtype == np.dtype('float32')):
            IodaDtype = ioda.Types.float
        elif (NumpyDtype == np.dtype('int64')):
            IodaDtype = ioda.Types.int64    # convert long to int
        elif (NumpyDtype == np.dtype('int32')):
            IodaDtype = ioda.Types.int32
        elif (NumpyDtype == np.dtype('int16')):
            IodaDtype = ioda.Types.int16
        elif (NumpyDtype == np.dtype('int8')):
            IodaDtype = ioda.Types.int16
        elif (NumpyDtype == np.dtype('S1')):
            IodaDtype = ioda.Types.str
        elif (NumpyDtype == np.dtype('M')):
            IodaDtype = ioda.Types.datetime
        elif (NumpyDtype == np.dtype('object')):
            try:
                if (isinstance(NumpyArr[0], dt.datetime)):
                    IodaDtype = ioda.Types.datetime
                else:
                    IodaDtype = ioda.Types.str
            except IndexError:
                IodaDtype = ioda.Types.str
        else:
            try:
                a = str(NumpyArr[0])
                IodaDtype = ioda.Types.str
            except TypeError:
                print("ERROR: Unrecognized numpy data type: ", NumpyDtype)
                exit(-2)
        return IodaDtype

    class _Variable:
        def __repr__(self):
            return f"IODA variable ({self._varstr})"

        def __str__(self):
            return f"IODA variable object - {self._varstr}"

        def __init__(self, obsspace, varname, groupname=None):
            self.obsspace = obsspace
            self._varstr = varname
            if groupname is not None:
                self._varstr = f"{groupname}/{varname}"
            self._iodavar = self.obsspace.obsgroup.vars.open(self._varstr)
            self.attrs = self._iodavar.atts.list()
            self.dims = self._iodavar.dims
            self.dimsizes = self._iodavar.dims.dimsCur

        def numpy_dtype(self):
            if self._iodavar.isA2(ioda.Types.float):
                dtype = np.dtype('float32')
            elif self._iodavar.isA2(ioda.Types.double):
                dtype = np.dtype('float64')
            elif self._iodavar.isA2(ioda.Types.int64):
                dtype = np.dtype('int64')
            elif self._iodavar.isA2(ioda.Types.int32):
                dtype = np.dtype('int32')
            elif self._iodavar.isA2(ioda.Types.int16):
                dtype = np.dtype('int16')
            elif self._iodavar.isA2(ioda.Types.str):
                dtype = np.dtype('object')
            else:
                raise TypeError("Unrecognized IODA type")
            return dtype

        def write_data(self, npArray):
            datatype = self.obsspace.NumpyToIodaDtype(npArray)
            if datatype == ioda.Types.float:
                self._iodavar.writeNPArray.float(npArray)
            elif datatype == ioda.Types.double:
                self._iodavar.writeNPArray.double(npArray)
            elif datatype == ioda.Types.int64:
                self._iodavar.writeNPArray.int64(npArray)
            elif datatype == ioda.Types.int32:
                self._iodavar.writeNPArray.int32(npArray)
            elif datatype == ioda.Types.str:
                self._iodavar.writeVector.str(npArray)
            elif datatype == ioda.Types.datetime:
                if npArray[0].tzinfo is None:
                    for i in range(len(npArray)):
                        npArray[i] = npArray[i].replace(tzinfo=dt.timezone.utc)
                self._iodavar.writeVector.datetime(npArray)
                epochstr = 'seconds since 1970-01-01T00:00:00Z'
                self._iodavar.atts.create(
                    'units', ioda.Types.str, [1]).writeDatum.str(epochstr)
                # update the attributes data member since we have added this attribute
                # after we created the variable
                self.attrs = self._iodavar.atts.list()
            # add other elif here TODO

        def read_attr(self, attrName):
            """
              returns single value or array of values of requested attribute
            """
            _attr = self._iodavar.atts.open(attrName)
            # figure out what datatype the attribute is
            attrType = _attr.getType()
            attrTypeSize = attrType.getSize()
            if (attrType.getClass() == ioda.TypeClass.Integer):
                if (attrTypeSize == 4):
                    # int
                    attrval = _attr.readVector.int()
                elif (attrTypeSize == 8):
                    # long
                    attrval = _attr.readVector.int64()
            elif (attrType.getClass() == ioda.TypeClass.Float):
                if (attrTypeSize == 4):
                    # float
                    attrval = _attr.readVector.float()
                elif (attrTypeSize == 8):
                    # float
                    attrval = _attr.readVector.double()
            elif (attrType.getClass() == ioda.TypeClass.String):
                # string
                attrval = _attr.readVector.str()
            else:
                raise TypeError(f"Attribute {attrName} type not supported")

            if len(attrval) == 1:
                attrval = attrval[0]
            return attrval

        def write_attr(self, attrName, attrVal):
            # get type of variable
            try:
                attrType = self.obsspace.NumpyToIodaDtype(attrVal)
                if attrType == ioda.Types.float:
                    if np.array(attrVal).size == 1:
                        self._iodavar.atts.create(attrName, attrType,
                                         [1]).writeDatum.float(attrVal)
                    else:
                        self._iodavar.atts.create(attrName, attrType,
                                         len(attrVal)).writeVector.float(attrVal)
                elif attrType == ioda.Types.double:
                    if np.array(attrVal).size == 1:
                        self._iodavar.atts.create(attrName, attrType,
                                         [1]).writeDatum.double(attrVal)
                    else:
                        self._iodavar.atts.create(attrName, attrType,
                                         len(attrVal)).writeVector.double(attrVal)
                elif attrType == ioda.Types.int64:
                    if np.array(attrVal).size == 1:
                        self._iodavar.atts.create(attrName, attrType,
                                         [1]).writeDatum.int64(attrVal)
                    else:
                        self._iodavar.atts.create(attrName, attrType,
                                         len(attrVal)).writeVector.int64(attrVal)
                elif attrType == ioda.Types.int32:
                    if np.array(attrVal).size == 1:
                        self._iodavar.atts.create(attrName, attrType,
                                         [1]).writeDatum.int32(attrVal)
                    else:
                        self._iodavar.atts.create(attrName, attrType,
                                         len(attrVal)).writeVector.int32(attrVal)
                # add other elif here TODO
            except AttributeError:  # if string
                if (type(attrVal) == str):
                    attrType = ioda.Types.str
                    self._iodavar.atts.create(
                        attrName, attrType, [1]).writeDatum.str(attrVal)

        def read_data(self):
            """
              returns numpy array/python list of variable data
            """
            # figure out what datatype the variable is
            varType = self._iodavar.getType()
            varTypeSize = varType.getSize()
            if (varType.getClass() == ioda.TypeClass.Integer):
                if (varTypeSize == 4):
                    # int
                    data = self._iodavar.readNPArray.int32()
                elif (varTypeSize == 8):
                    # long
                    if 'units' in self.attrs:
                        varunits = self.read_attr('units')
                        if 'since' in varunits:
                            data = self._iodavar.readVector.datetime()
                            # Ioda stores datetimes as int64 with the assumption that
                            # they are UTC times. If we get a naive object (no time zone
                            # specified, tzinfo == None), change it to UTC.
                            if data[0].tzinfo is None:
                                for i in range(len(data)):
                                    data[i] = data[i].replace(tzinfo=dt.timezone.utc)
                        else:
                            data = self._iodavar.readNPArray.int64()
                    else:
                        data = self._iodavar.readNPArray.int64()
            elif (varType.getClass() == ioda.TypeClass.Float):
                if (varTypeSize == 4):
                    # float
                    data = self._iodavar.readNPArray.float()
                    data[np.abs(data) > 9e36] = np.nan # undefined values
                elif (varTypeSize == 8):
                    # float
                    data = self._iodavar.readNPArray.double()
                    data[np.abs(data) > 9e36] = np.nan # undefined values
            elif (varType.getClass() == ioda.TypeClass.String):
                # string
                data = self._iodavar.readVector.str() # ioda cannot read str to datetime...
                # convert to datetimes if applicable
                if "datetime" in self._varstr:
                    data = self._str_to_datetime(data)
            else:
                raise TypeError(f"Variable {self._varstr} type not supported")

            # reshape if it is a 2D array with second dim size 1
            try:
                if data.shape[1] == 1:
                    data = data.flatten()
            except (IndexError, AttributeError):
                pass
            return data

        def _str_to_datetime(self, datain):
            # comes as a list of strings, need to
            # make into datetime objects
            datetimes = [dt.datetime.strptime(x, "%Y-%m-%dT%H:%M:%SZ") for x in datain]
            return np.array(datetimes)
