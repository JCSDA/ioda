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
        if  os.path.isfile(path) and self.read:
            self.iodafile = path
        elif os.path.exists(os.path.dirname(path)) and self.write:
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
        if _attr.isA2(ioda.Types.float):
            # float
            attrval = _attr.readVector.float()
        elif _attr.isA2(ioda.Types.int):
            # integer
            attrval = _attr.readVector.int()
        elif _attr.isA2(ioda.Types.str):
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
                    'nlocs', dimlen, ioda.Unlimited, dimlen)
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
        elif (NumpyDtype == np.dtype('object')):
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
            # add other elif here TODO

        def read_attr(self, attrName):
            """
              returns single value or array of values of requested attribute
            """
            _attr = self._iodavar.atts.open(attrName)
            # figure out what datatype the attribute is
            if _attr.isA2(ioda.Types.float):
                # float
                attrval = _attr.readVector.float()
            elif _attr.isA2(ioda.Types.int):
                # integer
                attrval = _attr.readVector.int()
            elif _attr.isA2(ioda.Types.str):
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
            if self._iodavar.isA2(ioda.Types.float):
                # float
                data = self._iodavar.readNPArray.float()
                data[np.abs(data) > 9e36] = np.nan # undefined values
            elif self._iodavar.isA2(ioda.Types.int):
                # integer
                data = self._iodavar.readNPArray.int()
            elif self._iodavar.isA2(ioda.Types.str):
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
