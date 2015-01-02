file(GLOB HEADERS "*.h")

include("${CMAKE_SOURCE_DIR}/src/make/version.cmake")

file(READ "version.h" VERSION_H_ORIG)
string(REPLACE "VACUUM_VERSION" "\"${VACUUM_VERSION}\"" VERSION_H "${VERSION_H_ORIG}")
file(WRITE "${CMAKE_BINARY_DIR}/generated/definitions/version.h" "${VERSION_H}")

if (INSTALL_SDK)
	install(FILES ${HEADERS}
		DESTINATION "${INSTALL_INCLUDES}/definitions"
		COMPONENT sdk)
endif (INSTALL_SDK)
