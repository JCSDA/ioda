#!/usr/bin/env python3

#
# (C) Copyright 2023 UCAR
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
#

'''
READ THIS!
The OMPS files in UFO-data from EMC are not on the ioda v3 format, before running this
upgrader you need to run the upgrader in build/bin/ioda-upgrade-v2-to-v3.x
ioda-upgrade-v2-to-v3.x in.nc4 out.nc4 ioda/share/ioda/yaml/validation/ObsSpace.yaml
'''

import argparse
import netCDF4 as nc
import h5py
import numpy as np
import shutil

import os

import sys

if not sys.warnoptions:
    import warnings
    warnings.simplefilter("ignore")

def copyvar(ds_in, ds_out, varstr, grpstr=None):
        if grpstr is None:
           var = ds_in.variables[varstr]
           varname = varstr
        else:
           var = ds_in.groups[grpstr].variables[varstr]
           varname = '/'+grpstr+'/'+varstr

        outVar = ds_out.createVariable(varname, var.datatype, var.dimensions)
        outVar.setncatts({k: var.getncattr(k) for k in var.ncattrs()})
        outVar[:] = var[:]


def main():

    # get command line arguments
    parser = argparse.ArgumentParser(
        description=(
            'upgrader for ioda column retrievals')
    )

    required = parser.add_argument_group(title='required arguments')
    required.add_argument(
        '-i', '--input',
        help="IODA file to be upgraded",
        type=str, required=True)
    required.add_argument(
        '-v', '--obsvar',
        help="the observable (e.g. carbonmonoxideTotal)",
        type=str, required=True)
    required.add_argument(
        '-o', '--output',
        help="IODA output file",
        type=str, required=False)

    args = parser.parse_args()
    dsin = nc.Dataset(args.input, 'r')

    dsout = nc.Dataset(args.output, 'w')

    #Copy Location
    dloc = dsin.dimensions['Location']
    dsout.createDimension(dloc.name, dloc.size)

    #Create Layer and Vertice dimensions
    # no need to layer here as there is no AK and each
    #layer can be treated independently
    vertice = 2 # this because we remove the TC values
    dsout.createDimension('Vertice', vertice)

    copyvar(dsin, dsout, 'Location')
    vloc = dsin.variables['Location']

    outVertice = dsout.createVariable('Vertice', vloc.datatype, 'Vertice')
    outVertice.setncatts({vloc.ncattrs()[0]: vertice})
    outVertice[:] =  np.arange(vertice,0,-1)

    #gsi stuff
    dsout.createGroup('GsiFinalObsError')
    copyvar(dsin, dsout, args.obsvar, 'GsiFinalObsError')
    dsout.createGroup('GsiHofX')
    copyvar(dsin, dsout, args.obsvar, 'GsiHofX')
    dsout.createGroup('GsiHofXBc')
    copyvar(dsin, dsout, args.obsvar, 'GsiHofXBc')
    dsout.createGroup('GsiUseFlag')
    copyvar(dsin, dsout, args.obsvar, 'GsiUseFlag')

    dsout.createGroup('MetaData')
    #not sure what is really needed there
    #pressure will be moved to RetrievalAncillaryData
    copyvar(dsin, dsout, 'dateTime', 'MetaData') #not camel case...
    copyvar(dsin, dsout, 'latitude', 'MetaData')
    copyvar(dsin, dsout, 'longitude', 'MetaData')
    copyvar(dsin, dsout, 'row_anomaly_index', 'MetaData')
    copyvar(dsin, dsout, 'sensorScanPosition', 'MetaData')
    copyvar(dsin, dsout, 'sequenceNumber', 'MetaData')
    copyvar(dsin, dsout, 'solarZenithAngle', 'MetaData')

    dsout.createGroup('ObsError')
    copyvar(dsin, dsout, args.obsvar, 'ObsError')

    dsout.createGroup('ObsValue')
    copyvar(dsin, dsout, args.obsvar, 'ObsValue')

    dsout.createGroup('PreQC')
    copyvar(dsin, dsout, args.obsvar, 'PreQC')

    dsout.createGroup('RtrvlAncData')
    #Get and concatenate vectors

    pre_ve = []

    pre = dsin.groups['MetaData'].variables['pressure']
    outPrs = dsout.createVariable('/RetrievalAncillaryData/pressureVertice', pre.datatype, ('Location','Vertice'))
    outPrs.setncatts({k: pre.getncattr(k) for k in pre.ncattrs()})

    pre = np.insert(pre,0,0)

    for p1,p2 in zip(pre[:-1],pre[1:]):

        if p1 < p2:
            ivert = [p1, p2]
        if p1 > p2:
            ivert = [p2, p1]

        pre_ve.append(ivert)

    outPrs[:] = pre_ve

    dsout.close()


if __name__ == '__main__':
    main()
