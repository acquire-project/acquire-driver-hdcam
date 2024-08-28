# Locates the Hamamatsu DCAM-SDK
#
# In general, if you install the SDK somewhere on your system environment's
# PATH, cmake will be able to find it.
#
# That is, the folder containing the "dcamsdk4" or "Hamamatsu_DCAMSDK4_v22126552"
# folders should be on the system path.
find_path(DCAMSDK_ROOT_DIR
    NAMES "dcamsdk4/inc/dcamapi4.h"
    PATH_SUFFIXES 
        "Hamamatsu_DCAMSDK4_v22126552"
        "Hamamatsu_DCAMSDK4_v24026764"
    DOC "Hamamatsu DCAM-SDK location"
    NO_CACHE
)


if(DCAMSDK_ROOT_DIR)
    message(STATUS "DCAM-SDK found: ${DCAMSDK_ROOT_DIR}")

    # only do the following if pulling libs from the SDK (windows only)
    # for other platforms one must separately install the DCAM API
    if (WIN32)
        set(tgt hdcam)
        add_library(${tgt} STATIC IMPORTED GLOBAL)
        target_include_directories(${tgt} INTERFACE ${DCAMSDK_ROOT_DIR}/dcamsdk4/inc)
        set_target_properties(${tgt} PROPERTIES
            IMPORTED_LOCATION ${DCAMSDK_ROOT_DIR}/dcamsdk4/lib/win64/dcamapi.lib
        )
    endif()
else()
    message(FATAL_ERROR "DCAM-SDK NOT FOUND")
endif()