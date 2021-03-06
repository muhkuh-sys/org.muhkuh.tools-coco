cmake_minimum_required(VERSION 2.8)

SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY FALSE)
SET(CPACK_PACKAGE_FILE_NAME coco-${VERSION_FULL})

SET(CPACK_GENERATOR "ZIP")
INCLUDE(CPack)
INCLUDE(../../dependencies/ant_paths.cmake)

# Look for the Doxygen package.
# If it is not installed, no Doxygen documentation was generated.
FIND_PACKAGE(Doxygen)

# Build the ivy file.
CONFIGURE_FILE(ivy.xml ivy-${VERSION_FULL}.xml)

#-----------------------------------------------------------------------------
#
# Install the 32bit files.
#
INSTALL(FILES ${COCO_WIN32_INSTALL}/modules/led_analyzer/led_analyzer.dll
        DESTINATION windows_x86
)

INSTALL(FILES ${COCO_WIN32_INSTALL}/ColorControl/ColorControl.exe
        DESTINATION windows_x86
)


#-----------------------------------------------------------------------------
#
# Install the 64bit files.
#
INSTALL(FILES ${COCO_WIN64_INSTALL}/modules/led_analyzer/led_analyzer.dll
        DESTINATION windows_amd64
)

INSTALL(FILES ${COCO_WIN64_INSTALL}/ColorControl/ColorControl.exe
        DESTINATION windows_amd64
)


#-----------------------------------------------------------------------------

IF(DOXYGEN_FOUND)
	INSTALL(DIRECTORY ${COCO_WIN64_INSTALL}/doc
	        DESTINATION common
	)
ENDIF(DOXYGEN_FOUND)


#-----------------------------------------------------------------------------

INSTALL(FILES install.xml
        DESTINATION .)

#-----------------------------------------------------------------------------

# Install the general LUA Files.
INSTALL(DIRECTORY ${LED_ANALYZER_LUA}/lua/
        DESTINATION common/${INSTALL_DIR_LUA_SCRIPTS}
)

INSTALL(DIRECTORY ${LED_ANALYZER_LUA}/example_device
        DESTINATION common
)

INSTALL(FILES ${LED_ANALYZER_LUA}/color_control_init.lua
              ${LED_ANALYZER_LUA}/detect_interfaces.lua
        DESTINATION common
)


#-----------------------------------------------------------------------------
#
# Install the folder containing the icons used for this project

INSTALL(DIRECTORY ${COCO_ICONS}
	DESTINATION common
)


#-----------------------------------------------------------------------------
#
# Filter the pom.xml file.
#
SET(POM_ARTIFACT_ID  "coco")
SET(POM_GROUP_ID     "tools.muhkuh.org")
SET(POM_VERSION      "${VERSION_FULL}")
SET(POM_PACKAGING    "zip")
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/../pom.xml pom.xml)


#-----------------------------------------------------------------------------
#
# Filter the artifacts.xml file.
#
SET(ARTIFACT_DIRECTORY    "${CMAKE_CURRENT_BINARY_DIR}")
SET(ARTIFACT_ARTIFACT_ID  "coco")
SET(ARTIFACT_GROUP_ID     "tools.muhkuh.org")
SET(ARTIFACT_VERSION      "${VERSION_FULL}")
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/../artifacts.xml ${ARTIFACT_LISTS_OUTPUT}/artifacts_coco.xml)

