#include "dcam.camera.h"
#include "dcam.error.h"
#include "dcam.prelude.h"

#include "device/kit/driver.h"
#include "device/hal/camera.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <dcamapi4.h>

#define countof(e) (sizeof(e) / sizeof(*(e)))
#define containerof(P, T, F) ((T*)(((char*)(P)) - offsetof(T, F)))

enum DeviceStatusCode
aq_dcam_set__inner(struct Dcam4Camera* self,
                   struct CameraProperties* props,
                   int force);

static uint32_t
aq_dcam_device_count(struct Driver* self_)
{
    struct Dcam4Driver* self = containerof(self_, struct Dcam4Driver, driver);
    const uint32_t n = self->api_init.iDeviceCount;
    return n;
}

static enum DeviceStatusCode
aq_dcam_describe__inner(const struct Dcam4Driver* self,
                        struct DeviceIdentifier* ident,
                        uint64_t i)
{
    char model[64] = { 0 };
    char sn[64] = { 0 };
    ident->kind = DeviceKind_Camera;
    {
        DCAMDEV_STRING param = { .size = sizeof(param),
                                 .text = model,
                                 .textbytes = sizeof(model),
                                 .iString = DCAM_IDSTR_MODEL };
        DCAM(dcamdev_getstring((HDCAM)i, &param));
    }

    {
        DCAMDEV_STRING param = { .size = sizeof(param),
                                 .text = sn,
                                 .textbytes = sizeof(sn),
                                 .iString = DCAM_IDSTR_CAMERAID };
        DCAM(dcamdev_getstring((HDCAM)i, &param));
    }

    snprintf(ident->name, sizeof(ident->name), "Hamamatsu %s %s", model, sn);
    ident->device_id = (uint8_t)i;
    return Device_Ok;
Error:
    return Device_Err;
}

static enum DeviceStatusCode
aq_dcam_describe(const struct Driver* self_,
                 struct DeviceIdentifier* ident,
                 uint64_t i)
{

    struct Dcam4Driver* self = containerof(self_, struct Dcam4Driver, driver);
    lock_acquire(&self->lock);
    CHECK(Device_Ok == aq_dcam_describe__inner(self, ident, i));
    lock_release(&self->lock);
    return Device_Ok;
Error:
    lock_release(&self->lock);
    return Device_Err;
}

static enum DeviceStatusCode
aq_dcam_open__inner(struct Dcam4Driver* driver,
                    uint64_t device_id,
                    struct Dcam4Camera* out)
{
    HDCAM hdcam = { 0 };
    HDCAMWAIT hwait = { 0 };

    {
        DCAMDEV_OPEN p = { .size = sizeof(p), .index = (int32_t)device_id };
        DCAM(dcamdev_open(&p));
        hdcam = p.hdcam;
    }

    {
        DCAMWAIT_OPEN p = {
            .size = sizeof(p),
            .hdcam = hdcam,
        };
        DCAM(dcamwait_open(&p));
        hwait = p.hwait;
    }

    *out = (struct Dcam4Camera){
        .camera =
          (struct Camera){ .state = DeviceState_AwaitingConfiguration,
                           .set = aq_dcam_set,
                           .get = aq_dcam_get,
                           .get_meta = aq_dcam_get_metadata,
                           .get_shape = aq_dcam_get_shape,
                           .start = aq_dcam_start,
                           .stop = aq_dcam_stop,
                           .execute_trigger = aq_dcam_fire_software_trigger,
                           .get_frame = aq_dcam_get_frame },
        .hdcam = hdcam,
        .wait = hwait,
    };
    aq_dcam_get(&out->camera, &out->last_props);
    TRACE("DCAM device id: %d\tdcam: %p\thwait: %p",
          (int)device_id,
          out->hdcam,
          out->wait);
    return Device_Ok;
Error:
    return Device_Err;
}

static enum DeviceStatusCode
aq_dcam_open(struct Driver* self_, uint64_t device_id, struct Device** out)
{
    struct Dcam4Camera* camera = 0;
    CHECK(out);
    *out = 0;
    CHECK(self_);
    struct Dcam4Driver* driver = containerof(self_, struct Dcam4Driver, driver);
    CHECK(device_id < countof(driver->cameras));
    CHECK(camera = (struct Dcam4Camera*)malloc(sizeof(struct Dcam4Camera)));
    lock_init(&camera->lock);
    CHECK(Device_Ok == aq_dcam_open__inner(driver, device_id, camera));
    driver->cameras[device_id] = camera;
    *out = &camera->camera.device;
    return Device_Ok;
Error:
    if (camera)
        free(camera);
    return Device_Err;
}

static void
aq_dcam_close__inner(struct Dcam4Driver* driver, struct Dcam4Camera* self)
{

    DWRN(dcamwait_close(self->wait));
    DWRN(dcamdev_close(self->hdcam));

    self->camera.state = DeviceState_Closed;
}

static enum DeviceStatusCode
aq_dcam_close(struct Driver* driver, struct Device* in)
{
    struct Dcam4Driver* dcam_driver =
      containerof(driver, struct Dcam4Driver, driver);
    struct Dcam4Camera* self =
      containerof(in, struct Dcam4Camera, camera.device);
    lock_acquire(&dcam_driver->lock);

    WARN(Device_Ok == aq_dcam_stop(&self->camera));

    lock_acquire(&self->lock);
    aq_dcam_close__inner(dcam_driver, self);
    dcam_driver->cameras[self->camera.device.identifier.device_id] = 0;
    lock_release(&self->lock);

    lock_release(&dcam_driver->lock);
    free(self);
    return Device_Ok;
}

static enum DeviceStatusCode
aq_dcam_shutdown_(struct Driver* self_)
{
    CHECK(self_);
    struct Dcam4Driver* self = containerof(self_, struct Dcam4Driver, driver);
    lock_acquire(&self->lock);
    for (int i = 0; i < countof(self->cameras); ++i) {
        if (self->cameras[i])
            camera_close(&self->cameras[i]->camera);
    }
    DWRN(dcamapi_uninit());
    lock_release(&self->lock);

    memset(self, 0, sizeof(*self));
    free(self);

    return Device_Ok;
Error:
    return Device_Err;
}

// forward declarations for reset_driver_and_replace_camera()

// FIXME: (nclack) Manual maint of ABI compatibility required. see loader.c
struct Loader
{
    struct Driver driver;
    struct Driver* inner;
    HMODULE hmodule;
};

// static enum DeviceStatusCode
// aq_dcam_open__inner(struct Dcam4Driver* driver,
//                     uint64_t device_id,
//                     struct Dcam4Camera* out);
// static enum DeviceStatusCode
// aq_dcam_describe(const struct Driver* self_,
//                  struct DeviceIdentifier* ident,
//                  uint64_t i);
// static void
// aq_dcam_close__inner(struct Dcam4Driver* driver, struct Dcam4Camera* self);
// static enum DeviceStatusCode
// aq_dcam_describe__inner(const struct Dcam4Driver* self,
//                         struct DeviceIdentifier* ident,
//                         uint64_t i);

/// @brief Attempt to reset the driver and re-initialize the camera.
/// @note This only gets called after there's been a problem with a camera.
///       Other cameras might still be live.
struct Dcam4Camera*
reset_driver_and_replace_camera(struct Dcam4Camera* self)
{
    uint8_t driver_mutex_acquired = 0;
    CHECK(self);
    // Get the driver pointer
    // FIXME: (nclack) GROSS - This depends on ABI compatibility with the
    //        Loader struct which isn't strictly constrained.

    struct Loader* loader =
      containerof(self->camera.device.driver, struct Loader, driver);
    struct Dcam4Driver* driver =
      containerof(loader->inner, struct Dcam4Driver, driver);
    size_t ncameras =
      min(driver->api_init.iDeviceCount, countof(driver->cameras));
    char self_name[256] = { 0 };
    memcpy(self_name,
           self->camera.device.identifier.name,
           strlen(self->camera.device.identifier.name));

    // try to stop all the associated cameras
    for (int i = 0; i < ncameras; ++i) {
        camera_stop(&driver->cameras[i]->camera);
    }

    // save properties
    struct CameraProperties saved_props[countof(driver->cameras)] = { 0 };
    for (int device_idx = 0; device_idx < ncameras; ++device_idx) {
        struct Dcam4Camera* cam = driver->cameras[device_idx];
        if (cam) // camera is opened
            saved_props[device_idx] = cam->last_props;
    }

    // close and shutdown

    LOG("Shutting down the driver");
    lock_acquire(&driver->lock);
    driver_mutex_acquired = 1;
    {
        for (int i = 0; i < ncameras; ++i) {
            if (driver->cameras[i])
                aq_dcam_close__inner(driver, driver->cameras[i]);
        }
        DWRN(dcamapi_uninit());
        driver->api_init = (DCAMAPI_INIT){ 0 };
    }

    // reinitialize the driver
    {
        int retries = 10;
        while (retries-- > 0) {
            const float dt_ms = 10000.0f;
            LOG("Waiting %3.1f sec before attempting to restart DCAM (%d "
                "remaining attempts).",
                dt_ms * 1e-3f,
                retries);
            clock_sleep_ms(0, 10000);

            LOG("Attempting DCAM restart...");
            driver->api_init = (DCAMAPI_INIT){ .size = sizeof(DCAMAPI_INIT) };
            {
                DCAMERR ecode = dcamapi_init(&driver->api_init);
                if (!DISFAIL(ecode))
                    break;
                LOG("Failed to restart DCAM. %s", dcam_error_to_string(ecode));
            }
            DWRN(dcamapi_uninit());
        }
        CHECK(retries > 0);
    }
    ncameras = min(driver->api_init.iDeviceCount, countof(driver->cameras));

    // reopen all cameras and reset their properties
    for (int32_t device_id = 0; device_id < ncameras; ++device_id) {
        if (driver->cameras[device_id]) { // camera was previously opened
            CHECK(Device_Ok == aq_dcam_open__inner(driver,
                                                   device_id,
                                                   driver->cameras[device_id]));
            driver->cameras[device_id]->camera.device.driver = &loader->driver;
            CHECK(Device_Ok ==
                  aq_dcam_describe__inner(
                    driver,
                    &driver->cameras[device_id]->camera.device.identifier,
                    device_id));

            switch (aq_dcam_set__inner(driver->cameras[device_id],
                                       &saved_props[device_id],
                                       1 /*?force*/)) {
                case Device_Ok:
                    driver->cameras[device_id]->camera.state =
                      DeviceState_Armed;
                    break;
                case Device_Err:
                    driver->cameras[device_id]->camera.state =
                      DeviceState_AwaitingConfiguration;
                    break;
            }
        }
    }

    // find the camera with the matching name
    for (uint32_t i = 0; i < ncameras; ++i) {
        if (0 == strncmp(self_name,
                         driver->cameras[i]->camera.device.identifier.name,
                         sizeof(self_name))) {
            *self = *driver->cameras[i];
            return driver->cameras[i];
        }
    }
    lock_release(&driver->lock);
    return self;
Error:
    if (driver_mutex_acquired)
        lock_release(&driver->lock);
    return 0;
}

acquire_export struct Driver*
acquire_driver_init_v0(acquire_reporter_t reporter)
{
    struct Dcam4Driver* self = 0;
    logger_set_reporter(reporter);
    CHECK(self = (struct Dcam4Driver*)malloc(sizeof(*self)));

    *self = (struct Dcam4Driver){
        .driver = { .device_count = aq_dcam_device_count,
          .describe = aq_dcam_describe,
          .open = aq_dcam_open,
          .close = aq_dcam_close,
          .shutdown = aq_dcam_shutdown_, },
    };
    lock_init(&self->lock);

    self->api_init.size = sizeof(self->api_init);

    {
        DCAMERR dcam_init_result = dcamapi_init(&self->api_init);
        if (dcam_init_result == DCAMERR_NOCAMERA) {
            free(self);
            return 0;
        }

        if (DISFAIL(dcam_init_result)) {
            free(self);
            LOG("Warning: DCAMAPI failed to initialize.");
            return 0;
        }
    }

    return &self->driver;
Error:
    DWRN(dcamapi_uninit());
    return 0;
}
