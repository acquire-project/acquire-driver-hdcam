aq_require(acquire-core-logger)
aq_require(acquire-core-platform)
aq_require(acquire-device-properties)
aq_require(acquire-device-kit)
aq_require(acquire-device-hal)

set(tgt acquire-dcam-cameras)
add_library(${tgt} MODULE
        dcam.camera.c
        dcam.error.h
        dcam.error.c
        dcam.getset.h
        dcam.getset.c
        dcam.prelude.h)
target_enable_simd(${tgt})
target_link_libraries(${tgt}
        hdcam
        acquire-core-platform
        acquire-core-logger
        acquire-device-kit
        acquire-device-hal
        )
set_target_properties(${tgt} PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
        )

install(TARGETS ${tgt} LIBRARY DESTINATION lib)
