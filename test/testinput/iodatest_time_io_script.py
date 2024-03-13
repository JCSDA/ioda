
import numpy as np

from pyioda import ioda

def create_obs_group(varname, start, stop:int, var4:float, var5:bool=True, step=0):
    assert varname == "testVar", "varname is not empty"
    assert start == 0, "start is not 0"
    assert stop == 10, "stop is not 10"
    assert abs(var4 - 3.14159) < 0.00001, "var 4 is not 3.14159"
    assert var5 == False, "var 5 is not false"
    assert step == 0, "step is not 0"

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
