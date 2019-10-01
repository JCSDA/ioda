GNU:[![AWS-gnu](https://codebuild.us-east-1.amazonaws.com/badges?uuid=eyJlbmNyeXB0ZWREYXRhIjoianB5bGN4NkkyRFBlcjZCWGRSQityVDIvVVBkZGQvQW9ITnplZzQ4SzJ4cU9iY3lBVkkxQ2NxRkIwNEdnY1pvZnIvWHZzQU9QMis2dnV0M25iU2JmeWhNPSIsIml2UGFyYW1ldGVyU3BlYyI6IlJ5VklGbis4UjhPK1pFUTAiLCJtYXRlcmlhbFNldFNlcmlhbCI6MX0%3D&branch=master)](https://console.aws.amazon.com/codesuite/codebuild/projects/automated-testing-ioda-gnu/history?region=us-east-1)
INTEL:[![AWS-intel](https://codebuild.us-east-1.amazonaws.com/badges?uuid=eyJlbmNyeXB0ZWREYXRhIjoiZElWZDBVSlhzY1AydnpBZzB5MUxKRFdIdExyYm1WME9MNGxKWE9GMmV1RCtIZEtiTkIweGdOU1RXTmwzRitKWVZqZ00rQ2hqY2dJRStNNDVMU01aK1pZPSIsIml2UGFyYW1ldGVyU3BlYyI6IlBMWmlucEhvTHBHejhmMEkiLCJtYXRlcmlhbFNldFNlcmlhbCI6MX0%3D&branch=master)](https://console.aws.amazon.com/codesuite/codebuild/projects/automated-testing-ioda-intel/history?region=us-east-1)

# IODA

JEDI Interface for Observation Data Access

## ODB support is now available

ODB is a observation database implementation from ECMWF.
The IODA implementation using ODB is in a prototype state so the capability, at this time, is limited.
This is just the beginning and we will be actively developing the ODB interface.

As an example, the radiosonde observation type has can now use netcdf, ODB1 or ODB2 file formats for input observation data.

Instructions for compiling ufo-bundle (which should transfer relatively easily to other bundles) are on the JEDI wiki: 
* https://wiki.ucar.edu/display/JEDI/Building+ufo-bundle+with+ODB+support
