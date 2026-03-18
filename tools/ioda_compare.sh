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

# Allow syntax for $3 to be "test_out_file:test_ref_file"
# so that you don't have to have an exact match between
# these two files. This allows one test reference file
# to be used to check output files from multiple tests
# that are supposed to produce identical files.
file_type=$1
cmd=$2
IFS=":" read -r -a test_files <<< "$3"
tol=${4:-"0.0"}
verbose=${5:-${VERBOSE:-"N"}}
expect_error=${6:-"N"}

[[ $verbose =~ [yYtT] ]] && set -x

# Parse what we received in the $3 argument
# If there is not a ":test_ref_file" section,
# then use the file_name as the test_ref_file.
file_name="${test_files[0]}"
if (( ${#test_files[@]} > 1 ))
then
  test_ref_file="${test_files[1]}"
else
  test_ref_file="${file_name}"
fi

rc="-1"
testRefFile="Data/testinput_tier_1/test_reference/${test_ref_file}"
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
    h5diff -v testoutput/$file_name $testRefFile
    rc=${?}
    if [[ $rc != 0 ]]; then
      h5dump testoutput/$file_name
      exit 1
    fi
    ;;
  netcdf)
    $cmd && \
    nccmp testoutput/$file_name $testRefFile -d -m -g -f -S -T ${tol}
    rc=${?}
    ;;
   odb)
    $cmd && \
    odc compare testoutput/$file_name $testRefFile
    rc=${?}
    ;;
   fileExists)
    # Check if the file exists
    if [[ -f testoutput/$file_name ]]; then
        rc=0
    else
        echo "ERROR: ioda_compare.sh: File '${file_name}' does not exist when it should."
        rc=1
    fi
    ;;
   fileDoesNotExist)
    # Check if the file does not exist
    if [[ ! -f testoutput/$file_name ]]; then
        rc=0
    else
        echo "ERROR: ioda_compare.sh: File '${file_name}' exists when it should not."
        rc=1
    fi
    ;;
   *)
    echo "ERROR: ioda_compare.sh: Unrecognized file type: ${file_type}"
    rc="-2"
    ;;
esac

exit $rc
