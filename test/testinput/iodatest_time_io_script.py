
import numpy as np

from pyioda import ioda

def create_obs_group(var_no_type_hint_int,
                     var_no_type_hint_str,
                     var_int: int,
                     var_int_str: int,
                     var_float: float,
                     var_float_str: float,
                     var_bool: bool,
                     var_bool_str: bool):

    # make assert for all the variables
    assert var_no_type_hint_int == 0, "var_no_type_hint_int is not 0"
    assert var_no_type_hint_str == "my_str", "var_no_type_hint_str is not 'my_str'"
    assert var_int == 10, "var_int is not 10"
    assert var_int_str == 10, "var_int_str is not 10"
    assert abs(var_float - 3.14159) < 0.00001, "var_float is not 3.14159"
    assert abs(var_float_str - 3.14159) < 0.00001, "var_float_str is not 3.14159"
    assert var_bool is False, "var_bool is not False"
    assert var_bool_str is False, "var_bool_str is not False"

    varname = var_no_type_hint_str
    start = var_no_type_hint_int
    stop = var_int_str
    step = 0

    print ("Creating ObsGroup with variable: ", varname)

    if (step == 0):
        step = (stop - start) / 100

    numLocs = int((stop - start) / step)

    datetime = np.array(["2023-12-13"]*numLocs, dtype=np.dtype('datetime64[s]'))
    lat = np.linspace(-89, 89, numLocs)
    lon = np.linspace(-179, 179, numLocs)
    data = np.linspace(start, stop, numLocs)

    g = ioda.Engines.HH.createMemoryFile(name = "test.hdf5",
                                         mode = ioda.Engines.BackendCreateModes.Truncate_If_Exists)

    dims = [ioda.NewDimensionScale.int32('Location', numLocs, ioda.Unlimited, numLocs)]
    og = ioda.ObsGroup.generate(g, dims)

    p1 = ioda.VariableCreationParameters()
    p1.compressWithGZIP()
    p1.setFillValue.float(-999)

    var = g.vars.create(f'ObsVal/{varname}', ioda.Types.float, scales=[og.vars.open('Location')], params=p1)
    var.atts.create('units', ioda.Types.str).writeVector.str(['dunno'])
    var.writeNPArray.float(data)

    var = g.vars.create(f'MetaData/latitude', ioda.Types.float, scales=[og.vars.open('Location')], params=p1)
    var.atts.create('units', ioda.Types.str).writeVector.str(['degrees_north'])
    var.writeNPArray.float(lat)

    var = g.vars.create(f'MetaData/longitude', ioda.Types.float, scales=[og.vars.open('Location')], params=p1)
    var.atts.create('units', ioda.Types.str).writeVector.str(['degrees_east'])
    var.writeNPArray.float(lon)

    var = g.vars.create(f'MetaData/dateTime', ioda.Types.float, scales=[og.vars.open('Location')], params=p1)
    var.atts.create('units', ioda.Types.str).writeVector.str(["seconds since 1970-01-01T00:00:00Z"])
    var.writeNPArray.float(datetime)

    return og
