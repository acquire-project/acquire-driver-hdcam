# Locates the Hamamatus DCAM-SDK
#
# In general, if you install the SDK somewhere on your system environment's
# PATH, cmake will be able to find it.
#
# That is, the folder containing the "dcamsdk4" or "Hamamatsu_DCAMSDK4_v22126552"
# folders should be on the system path.
find_path(DCAMSDK_ROOT_DIR
    NAMES "dcamsdk4/inc/dcamapi4.h"
    PATH_SUFFIXES "Hamamatsu_DCAMSDK4_v22126552"
    DOC "Hamamatsu DCAM-SDK location"
    NO_CACHE
)

if(DCAMSDK_ROOT_DIR)
    message(STATUS "DCAM-SDK found: ${DCAMSDK_ROOT_DIR}")

    set(tgt hdcam)
    add_library(${tgt} STATIC IMPORTED GLOBAL)
    target_include_directories(${tgt} INTERFACE ${DCAMSDK_ROOT_DIR}/dcamsdk4/inc)
    set_target_properties(${tgt} PROPERTIES
        IMPORTED_LOCATION ${DCAMSDK_ROOT_DIR}/dcamsdk4/lib/win64/dcamapi.lib
    )
else()
    message(STATUS "DCAM-SDK NOT FOUND")
endif()