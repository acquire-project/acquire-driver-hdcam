
#ifndef H_ACQUIRE_DRIVER_HDCAM_DCAM_CAMERA_V0
#define H_ACQUIRE_DRIVER_HDCAM_DCAM_CAMERA_V0

#include "device/props/camera.h"
#include "device/kit/camera.h"
#include "device/kit/driver.h"
#include "platform.h"

#include <stddef.h> // must come before dcamapi4.h
#include <dcamapi4.h>

#ifdef __cplusplus
extern "C"
{
#endif

    struct Dcam4Camera
    {
        struct Camera camera;
        HDCAM hdcam;
        HDCAMWAIT wait;
        struct CameraProperties last_props;
        struct lock lock;
    };

    struct Dcam4Driver
    {
        struct Driver driver;
        DCAMAPI_INIT api_init;
        struct Dcam4Camera* cameras[2];
        struct lock lock;
    };

    enum DeviceStatusCode aq_dcam_set(struct Camera*,
                                      struct CameraProperties* settings);
    enum DeviceStatusCode aq_dcam_get(const struct Camera*,
                                      struct CameraProperties* settings);
    enum DeviceStatusCode aq_dcam_get_metadata(
      const struct Camera*,
      struct CameraPropertyMetadata* meta);
    enum DeviceStatusCode aq_dcam_get_shape(const struct Camera*,
                                            struct ImageShape* shape);
    enum DeviceStatusCode aq_dcam_start(struct Camera*);
    enum DeviceStatusCode aq_dcam_stop(struct Camera*);

    enum DeviceStatusCode aq_dcam_fire_software_trigger(struct Camera*);

    enum DeviceStatusCode aq_dcam_get_frame(struct Camera*,
                                            void* im,
                                            size_t* nbytes,
                                            struct ImageInfo* info);

#ifdef __cplusplus
};
#endif

#endif // H_ACQUIRE_DRIVER_HDCAM_DCAM_CAMERA_V0
