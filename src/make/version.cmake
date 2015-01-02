# Variables available after inclusion of this file:
#  ABI_VERSION    - single number indicating the ABI version of utils library.
#  VACUUM_VERSION - string in "major.minor.patch" format denoting the
#                   application version.
#  VACUUM_VERSION_MAJOR - major version number.
#  VACUUM_VERSION_MINOR - minor version number.
#  VACUUM_VERSION_PATCH - patch version number.

file(READ "${CMAKE_SOURCE_DIR}/src/make/abi_version" ABI_VERSION_TMP OFFSET 14)
string(REPLACE "\"" "" ABI_VERSION_TMP2 "${ABI_VERSION_TMP}")
string(REPLACE "\n" "" ABI_VERSION "${ABI_VERSION_TMP2}")
unset(ABI_VERSION_TMP)
unset(ABI_VERSION_TMP2)

file(READ "${CMAKE_SOURCE_DIR}/src/make/vacuum_version" VACUUM_VERSION_TMP OFFSET 17)
string(REPLACE "\"" "" VACUUM_VERSION_TMP2 "${VACUUM_VERSION_TMP}")
string(REPLACE "\n" "" VACUUM_VERSION "${VACUUM_VERSION_TMP2}")
unset(VACUUM_VERSION_TMP)
unset(VACUUM_VERSION_TMP2)

# Code below relies on the fact that lists in CMake are emulated with
# semicolon-separated strings.
string(REPLACE "." ";" SPLIT_VERSION "${VACUUM_VERSION}")
list(GET SPLIT_VERSION 0 VACUUM_VERSION_MAJOR)
list(GET SPLIT_VERSION 1 VACUUM_VERSION_MINOR)
list(GET SPLIT_VERSION 2 VACUUM_VERSION_PATCH)
unset(SPLIT_VERSION)
