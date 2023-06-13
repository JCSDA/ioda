import datetime as dt
import numpy as np
import os
import ioda

__IodaTypeDict = {
    int: ioda.Types.int32,
    float: ioda.Types.float,
    str: ioda.Types.str,
    bool: ioda.Types.int32,
    dt.datetime: ioda.Types.datetime,
    np.dtype('float64'): ioda.Types.double,
    np.dtype('float32'): ioda.Types.float,
    np.dtype('int64'): ioda.Types.int64,
    np.dtype('int32'): ioda.Types.int32,
    np.dtype('int16'): ioda.Types.int16,
    np.dtype('int8'): ioda.Types.int16,
    np.dtype('S1'): ioda.Types.str,
    np.dtype('<U'): ioda.Types.str,
    np.dtype('M'): ioda.Types.datetime,
    np.dtype('datetime64[s]'): ioda.Types.datetime
}

def _ioda_type(data):
    '''
    Determines the ioda type of the data
    :param data: The data
    :return: Ioda type
    '''
    native_type = None
    if hasattr(data, 'dtype'):
        if data.dtype == np.dtype('object'):
            native_type = type(data[0]) if len(data) > 0 else str
        else:
            native_type = data.dtype
    elif isinstance(data, list):
        native_type = type(data[0]) if len(data) > 0 else str
    else:
        try:
            native_type = type(data)
        except:
            pass

    return __IodaTypeDict.get(native_type)

def _ioda_shape(data):
    '''
    Determines the shape of the data
    :param data:
    :return: shape list
    '''
    if hasattr(data, 'shape'):
        shape = data.shape
    elif isinstance(data, list):
        shape = [len(data)]
    else:
        shape = [1]

    return shape

# Extend Has_Attributes Class
def __write_attr(self, name, data):
    '''
    Add an attribute to the object
    :param name: name
    :param data: data
    :return:
    '''
    attr_type = _ioda_type(data)
    shape = _ioda_shape(data)

    att = self.create(name, attr_type, shape)
    if attr_type == ioda.Types.str:
        att.writeDatum.str(data)
    elif attr_type == ioda.Types.float:
        att.writeVector.float(data)
    elif attr_type == ioda.Types.double:
        att.writeVector.double(data)
    elif attr_type == ioda.Types.int64:
        att.writeVector.int64(data)
    elif attr_type == ioda.Types.int32:
        att.writeVector.int32(data)
    elif attr_type == ioda.Types.int16:
        att.writeVector.int16(data)
    else:
        raise TypeError(f"Attribute {name} type not supported.")

# Add the method add_attr to Has_Attributes
setattr(ioda._ioda_python.Has_Attributes, 'write_attr', __write_attr)


class ObsSpace:

    def __repr__(self):
        return f"ObsSpace({self.name},{self.iodafile})"

    def __str__(self):
        return f"IODA ObsSpace Object - {self.name}"

    def __init__(self, path, dim_dict=None, mode='r', name="NoName", iodalayout=0):
        self.name = name
        self.epoch = dt.datetime(1970, 1, 1, tzinfo=dt.timezone.utc)
        if mode not in ['r', 'w', 'rw']:
            raise TypeError(f"{mode} not one of 'r','w','rw'")
        self.write = True if 'w' in mode else False
        self.read = True if 'r' in mode else False
        # It's possible that path actually is just a file name (without a leading path)
        # when writing to the current working directory. We want to check in the write case
        # that the directory where we want the output to be created exists. If there is no
        # leading path then we are okay since we are in that directory running this script.
        if os.path.isfile(path) and self.read:
            self.iodafile = path
        elif (os.path.basename(path) == path or \
              os.path.exists(os.path.dirname(path))) and \
              self.write:
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
                        if key == 'Location':
                            _tmpvar.writeNPArray.int64(value)
                        else:
                            _tmpvar.writeNPArray.int32(value)
            else:
                raise TypeError("dim_dict is not defined")

    def _def_classvars(self):
        # define some variables
        self.dimensions = self.obsgroup.vars.list()
        self.groups = self.file.listGroups(recurse=True)
        self.attrs = self.obsgroup.atts.list()
        self.nlocs = len(self.obsgroup.vars.open('Location').readVector.int())
        self.variables = self.file.listVars(recurse=True)
        self.nvars = len(self.variables)

    def set_epoch(self, epochDateTime):
        """
          Set datetime epoch value: default is Jan 1, 1970 0Z
        """
        self.epoch = epochDateTime

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

    def write_attr(self, name, attrVal):
        '''
        Write an attribute to the obsgroup
        :param name: Attribute name
        :param attrVal: Attribute data
        :return:
        '''
        self.obsgroup.atts.write_attr(name, attrVal)

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
            if key == 'Location':
                _locs_var = ioda.NewDimensionScale.int64(
                    'Location', dimlen, ioda.Unlimited, min(dimlen, 10000))
                _dim_list.append(_locs_var)
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
                   dim_list=['Location'], fillval=None):
        if groupname:
            assert '/' not in varname, f'Error create_var: {varname} cannot contain "/" if ' \
                                       f'"groupname" is specified.'
            varname = f"{groupname}/{varname}"

        # If the variable is datetime and it is not already in the native int64
        # form, then change the dtype to 'M' which is the numpy datetime object type.
        if varname.lower() == 'metadata/datetime' and \
           dtype != np.dtype('int64') and \
           dtype != np.dtype('datetime64[s]'):
            dtype = np.dtype('M')

        # Get the ioda type for the variable
        var_type = _ioda_type(np.array([], dtype=dtype))

        # Make the parameter for the variable
        fparams = self._p1
        if fillval:
            fparams = ioda.VariableCreationParameters()
            fparams.compressWithGZIP()
            fparams = self.setFillValue(fparams, var_type, fillval)

        newVar = self.file.vars.create(varname,
                                       var_type,
                                       scales=[self.obsgroup.vars.open(dim) for dim in dim_list],
                                       params=fparams)

        # If this was a type of datetime, add the epoch string in the units attribute
        if (var_type == ioda.Types.datetime):
            epochstr = "seconds since " + self.epoch.strftime("%Y-%m-%dT%H:%M:%SZ")
            newVar.atts.create('units', ioda.Types.str, [1]).writeDatum.str(epochstr)

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
            if isinstance(value, dt.datetime):
                params.setFillValue.duration(value - self.epoch)
            elif hasattr(value, 'dtype') and value.dtype == np.dtype('datetime64[s]'):
                params.setFillValue.int64(value.astype('int64'))
        elif datatype == ioda.Types.duration:
            params.setFillValue.duration(value)
        # add other elif here TODO
        return params

    def Variable(self, varname, groupname=None):
        return self._Variable(self, varname, groupname)

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
            datatype = _ioda_type(npArray)
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
                if isinstance(npArray[0], dt.datetime):
                    if not npArray[0].tzinfo:
                        for i in range(len(npArray)):
                            npArray[i] = npArray[i].replace(tzinfo=dt.timezone.utc)
                    self._iodavar.writeVector.datetime(npArray)
                elif npArray.dtype == np.dtype('datetime64[s]'):
                    self._iodavar.writeNPArray.int64(npArray.astype('int64'))
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

        def write_attr(self, name, data):
            '''
            Write an attribute to the variable
            :param name:
            :param data:
            '''
            self._iodavar.atts.write_attr(name, data)


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
                    data[np.abs(data) > 9e36] = np.nan  # undefined values
                elif (varTypeSize == 8):
                    # float
                    data = self._iodavar.readNPArray.double()
                    data[np.abs(data) > 9e36] = np.nan  # undefined values
            elif (varType.getClass() == ioda.TypeClass.String):
                # string
                data = self._iodavar.readVector.str()  # ioda cannot read str to datetime...
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
