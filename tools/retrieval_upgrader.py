#!/usr/bin/env python3

#
# (C) Copyright 2023 UCAR
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
#

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
    required.add_argument(
        '-a', '--apriori',
        help="is there an apriori?",
        type=bool, required=False)

    args = parser.parse_args()
    dsin = nc.Dataset(args.input, 'r')

    dsout = nc.Dataset(args.output, 'w')

    #Copy Location
    dloc = dsin.dimensions['Location']
    dsout.createDimension(dloc.name, dloc.size)

    #Create Layer and Vertice dimensions
    lak = dsin.averaging_kernel_levels
    layer = lak
    vertice = lak + 1
    dsout.createDimension('Layer', layer)
    dsout.createDimension('Vertice', vertice)

    copyvar(dsin, dsout, 'Location')

    vloc = dsin.variables['Location']
    outLayer = dsout.createVariable('Layer', vloc.datatype, 'Layer')
    outLayer.setncatts({vloc.ncattrs()[0]: layer})
    outLayer[:] = np.arange(layer,0,-1)

    outVertice = dsout.createVariable('Vertice', vloc.datatype, 'Vertice')
    outVertice.setncatts({vloc.ncattrs()[0]: vertice})
    outVertice[:] =  np.arange(vertice,0,-1)

    dsout.createGroup('MetaData')
    copyvar(dsin, dsout, 'dateTime', 'MetaData')
    copyvar(dsin, dsout, 'latitude', 'MetaData')
    copyvar(dsin, dsout, 'longitude', 'MetaData')
    #copyvar(dsin, dsout, 'quality_assurance_value', 'MetaData')

    dsout.createGroup('ObsError')
    copyvar(dsin, dsout, args.obsvar, 'ObsError')

    dsout.createGroup('ObsValue')
    copyvar(dsin, dsout, args.obsvar, 'ObsValue')

    dsout.createGroup('PreQC')
    copyvar(dsin, dsout, args.obsvar, 'PreQC')

    dsout.createGroup('RtrvlAncData')
    #Get and concatenate vectors
    if args.apriori:
        vap = dsin.groups['RtrvlAncData'].variables['apriori_term']
        outAp = dsout.createVariable('/RetrievalAncillaryData/aprioriTerm', vap.datatype, ('Location'))
        outAp.setncatts({k: vap.getncattr(k) for k in vap.ncattrs()})
        outAp[:] = vap[:]

    vak = dsin.groups['RtrvlAncData'].variables['averaging_kernel_level_1']
    outAvk = dsout.createVariable('/RetrievalAncillaryData/averagingKernel', vak.datatype, ('Location','Layer'))
    outAvk.setncatts({k: vak.getncattr(k) for k in vak.ncattrs()})

    vpr = dsin.groups['RtrvlAncData'].variables['pressure_level_1']
    outPrs = dsout.createVariable('/RetrievalAncillaryData/pressureVertice', vpr.datatype, ('Location','Vertice'))
    outPrs.setncatts({k: vpr.getncattr(k) for k in vpr.ncattrs()})

    ak2d = dsin.groups['RtrvlAncData'].variables['averaging_kernel_level_1'][:]
    pr2d = dsin.groups['RtrvlAncData'].variables['pressure_level_1'][:]

    for i in np.arange(layer-1)+2:
        si = str(i)
        nak = dsin.groups['RtrvlAncData'].variables['averaging_kernel_level_'+si][:]
        npr = dsin.groups['RtrvlAncData'].variables['pressure_level_'+si][:]

        ak2d = np.append([ak2d], [nak])
        pr2d = np.append([pr2d], [npr])

    npr = dsin.groups['RtrvlAncData'].variables['pressure_level_'+str(vertice)][:]
    pr2d = np.append([pr2d], [npr])

    ak2d = np.flip(np.reshape(ak2d, (dloc.size,layer), order='F'),axis=1)
    pr2d = np.flip(np.reshape(pr2d, (dloc.size,vertice), order='F'),axis=1)

    outAvk[:] = ak2d[:]
    outPrs[:] = pr2d[:]

    dsout.close()


if __name__ == '__main__':
    main()
