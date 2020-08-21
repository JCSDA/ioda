GNU:[![AWS-gnu](https://codebuild.us-east-1.amazonaws.com/badges?uuid=eyJlbmNyeXB0ZWREYXRhIjoianB5bGN4NkkyRFBlcjZCWGRSQityVDIvVVBkZGQvQW9ITnplZzQ4SzJ4cU9iY3lBVkkxQ2NxRkIwNEdnY1pvZnIvWHZzQU9QMis2dnV0M25iU2JmeWhNPSIsIml2UGFyYW1ldGVyU3BlYyI6IlJ5VklGbis4UjhPK1pFUTAiLCJtYXRlcmlhbFNldFNlcmlhbCI6MX0%3D&branch=develop)](https://console.aws.amazon.com/codesuite/codebuild/projects/automated-testing-ioda-gnu/history?region=us-east-1)
INTEL:[![AWS-intel](https://codebuild.us-east-1.amazonaws.com/badges?uuid=eyJlbmNyeXB0ZWREYXRhIjoiZElWZDBVSlhzY1AydnpBZzB5MUxKRFdIdExyYm1WME9MNGxKWE9GMmV1RCtIZEtiTkIweGdOU1RXTmwzRitKWVZqZ00rQ2hqY2dJRStNNDVMU01aK1pZPSIsIml2UGFyYW1ldGVyU3BlYyI6IlBMWmlucEhvTHBHejhmMEkiLCJtYXRlcmlhbFNldFNlcmlhbCI6MX0%3D&branch=develop)](https://console.aws.amazon.com/codesuite/codebuild/projects/automated-testing-ioda-intel/history?region=us-east-1)
CLANG:[![AWS-clang](https://codebuild.us-east-1.amazonaws.com/badges?uuid=eyJlbmNyeXB0ZWREYXRhIjoiZm5QTHR4YkJqMkkyVUl2Y1VlNGhlTWpSSTJFTGZLZVl5YU1wUVJDYlBpVWo2c2R1YWxtS3lEVWFDOENwTTBmbG9vNTZrK1lBbWJ4MmlpZjlLSWlmWlZrPSIsIml2UGFyYW1ldGVyU3BlYyI6IkdOQVhsQVhadDA2SStIRFYiLCJtYXRlcmlhbFNldFNlcmlhbCI6MX0%3D&branch=develop)](https://console.aws.amazon.com/codesuite/codebuild/projects/automated-testing-ioda-clang/history?region=us-east-1)
CodeCov:[![codecov](https://codecov.io/gh/JCSDA/ioda/branch/develop/graph/badge.svg?token=HyOBuLKWzo)](https://codecov.io/gh/JCSDA/ioda)

# IODA

JEDI Interface for Observation Data Access

## ODC support is now available

ODC is an API from ECMWF that gives access to ODB2 (observation data) files.
The ODC libraries have been installed in the containers, and are available through the jedi-stack build process.
If the ODC libraries are found in your environment, then the ODC interface will be built into IODA.
If the ODC libraries are not found, then the ODC interface will be omitted from IODA.
