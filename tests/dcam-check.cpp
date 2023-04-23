#include "acquire.h"
#include "platform.h"
#include "device/hal/device.manager.h"
#include "logger.h"

#include <stdio.h>
#include <string.h>

#define L (aq_logger)
#define LOG(...) L(0, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define ERR(...) L(1, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define CHECK(e)                                                               \
    do {                                                                       \
        if (!(e)) {                                                            \
            ERR("Expression evaluated as false: " #e);                         \
            goto Error;                                                        \
        }                                                                      \
    } while (0)
#define DEVOK(e) CHECK(Device_Ok == (e))
#define OK(e) CHECK(AcquireStatus_Ok == (e))

#define countof(e) (sizeof(e) / sizeof(*(e)))

void
reporter(int is_error,
         const char* file,
         int line,
         const char* function,
         const char* msg)
{
    fprintf(is_error ? stderr : stdout,
            "%s%s(%d) - %s: %s\n",
            is_error ? "ERROR " : "",
            file,
            line,
            function,
            msg);
}

int
main()
{

    int ecode = 1;
    AcquireRuntime* runtime = 0;
    AcquireProperties props = {};

    CHECK(runtime = acquire_init(reporter));
    OK(acquire_get_configuration(runtime, &props));

    // Select devices
    {
        const DeviceManager* dm = 0;
        struct
        {
            DeviceKind kind;
            const char* name;
            size_t bytes_of_name;
            DeviceIdentifier* id;
        } query[] = {
#define SIZED(s) s, sizeof(s) - 1
            { DeviceKind_Camera,
              SIZED("C15440-20UP.*"),
              &props.video[0].camera.identifier },
            {
              DeviceKind_Storage,
              SIZED("Trash"),
              &props.video[0].storage.identifier,
            },
#undef SIZED
        };

        CHECK(dm = acquire_device_manager(runtime));
        for (int i = 0; i < countof(query); ++i) {
            DEVOK(device_manager_select(dm,
                                        query[i].kind,
                                        query[i].name,
                                        query[i].bytes_of_name,
                                        query[i].id));
        }
    }

    // Configure
    {
        props.video[0].max_frame_count = 1;
        props.video[0].frame_average_count = 0;
        props.video[0].camera.settings.pixel_type = SampleType_u16;
        props.video[0].camera.settings.shape = { .x = 640, .y = 480 };
        props.video[0].camera.settings.binning = 2;
        OK(acquire_configure(runtime, &props));
    }

    // Run
    {
        OK(acquire_start(runtime));
        clock_sleep_ms(0, 1000);
        OK(acquire_stop(runtime));
    }

#if 0
    // Check acquired data
    {
        struct VideoFrame* frames;
        size_t bytes_of_frames;
        OK(acquire_map_read(runtime, &frames, &bytes_of_frames));
        CHECK(frames);
        CHECK(bytes_of_frames == 640 * 480 * 2);
    }
#endif

    ecode = 0;
Finalize:
    acquire_shutdown(runtime);
    return ecode;
Error:
    ecode = 1;
    goto Finalize;
}
