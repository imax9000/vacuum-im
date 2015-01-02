# Ideally, *_version files should contain just a version string. But qmake
# from Qt4 does not support reading file content into a variable, so for now
# we're writing there something that qmake is able to handle. Please, do not
# add more variables to that files. If you need a new version string for
# something - create a new file, include it here and update version.cmake.
#
# Variables available after inclusion of this file:
#  ABI_VERSION    - single number indicating the ABI version of utils library.
#  VACUUM_VERSION - string in "major.minor.patch" format denoting the
#                   application version.

include(abi_version)
include(vacuum_version)
