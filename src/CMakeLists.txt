if (NOT TARGET acquire-core-libs)
    add_subdirectory(../acquire-common/acquire-core-libs ${CMAKE_CURRENT_BINARY_DIR}/acquire-core-libs)
endif ()

set(tgt acquire-driver-hdcam)

if (TARGET hdcam)
    add_library(${tgt} MODULE
            dcam.camera.c
            dcam.error.h
            dcam.error.c
            dcam.getset.h
            dcam.getset.c
            dcam.prelude.h
            dcam.driver.c
            dcam.camera.h)
    target_link_libraries(${tgt}
            acquire-core-platform
            acquire-core-logger
            acquire-device-kit
            acquire-device-hal
            )
    if(WIN32)
        message(STATUS "Linking static libs provided by SDK")
        target_link_libraries(${tgt} hdcam)
    endif()
else ()
    add_library(${tgt} MODULE
            unsupported.driver.c
            )

    target_link_libraries(${tgt}
            acquire-core-logger
            acquire-device-kit
            )
endif ()

target_include_directories(${tgt} PRIVATE ${DCAMSDK_ROOT_DIR}/dcamsdk4/inc)

target_enable_simd(${tgt})
set_target_properties(${tgt} PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
        )
install(TARGETS ${tgt} LIBRARY DESTINATION lib)
