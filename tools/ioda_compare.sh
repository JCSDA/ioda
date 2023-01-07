#!/bin/bash

# use h5diff, nccmp or odc to compare the output of 
#
# argument 1: what type of file to compare; hdf5, netcdf or odb
# argument 2: the command to run the ioda converter
# argument 3: the filename to test
# argument 4: tolerence for comparing values
# argument 5: verbosity
# argument 6: expect an error to occur

set -eu

file_type=$1
cmd=$2
file_name=$3
tol=${4:-"0.0"}
verbose=${5:-${VERBOSE:-"N"}}
expect_error=${6:-"N"}

[[ $verbose =~ [yYtT] ]] && set -x

rc="-1"
testRefDir="Data/testinput_tier_1/test_reference"
case $file_type in
  hdf5)
    set +e
    $cmd
    rc=${?}
    if [[ $rc != 0 ]]; then
        if [[ ${expect_error} =~ [yYtT] ]]; then
            exit 0
        else
            exit ${rc}
        fi
    fi
    h5diff -v testoutput/$file_name $testRefDir/$file_name
    rc=${?}
    if [[ $rc != 0 ]]; then
      h5dump testoutput/$file_name
      exit 1
    fi
    ;;
  netcdf)
    $cmd && \
    nccmp testoutput/$file_name $testRefDir/$file_name -d -m -g -f -S -T ${tol}
    rc=${?}
    ;;
   odb)
    $cmd && \
    odc compare testoutput/$file_name $testRefDir/$file_name
    rc=${?}
    ;;
   *)
    echo "ERROR: ioda_comp.sh: Unrecognized file type: ${file_type}"
    rc="-2"
    ;;
esac

exit $rc
