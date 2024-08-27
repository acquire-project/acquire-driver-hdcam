# Locates the Hamamatsu DCAM-SDK
#
# In general, if you install the SDK somewhere on your system environment's
# PATH, cmake will be able to find it.
#
# That is, the folder containing the "dcamsdk4" or "Hamamatsu_DCAMSDK4_v22126552"
# folders should be on the system path.
find_path(DCAMSDK_ROOT_DIR "inc/dcamapi4.h"
    HINTS ${CMAKE_SOURCE_DIR}/dcamsdk4
    DOC "Hamamatsu DCAM-SDK location"
    NO_CACHE
)


if(DCAMSDK_ROOT_DIR)
    message(STATUS "DCAM-SDK found: ${DCAMSDK_ROOT_DIR}")

    set(tgt hdcam)
    if(WIN32)
        add_library(${tgt} STATIC IMPORTED GLOBAL)
        set_target_properties(${tgt} PROPERTIES
            IMPORTED_LOCATION ${DCAMSDK_ROOT_DIR}/lib/win64/dcamapi.lib
        )
    elseif(LINUX)
        # add_library(${tgt} SHARED IMPORTED GLOBAL)
        # target_include_directories(${tgt} INTERFACE ${DCAMSDK_ROOT_DIR}/inc)
        # set_target_properties(${tgt} PROPERTIES
        #     IMPORTED_LOCATION ${DCAMSDK_ROOT_DIR}/lib/linux64/libdcamapi.so
        # )
        message(DEBUG "Linux platform uses a shared library")
    else()
        message(FATAL_ERROR "Unsupported platform")
    endif()
else()
    message(FATAL_ERROR "DCAM-SDK NOT FOUND")
endif()