//! Test: Runs through setting up an output trigger on line 1 configured to
//!       output when exposure starts.
#include "acquire.h"
#include "device/hal/device.manager.h"
#include "device/props/components.h"
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

uint8_t
select_digital_line(const AcquireRuntime* runtime, const char* name)
{
    AcquirePropertyMetadata meta = {};
    OK(acquire_get_configuration_metadata(runtime, &meta));
    {
        const auto* info = &meta.video[0].camera.digital_lines;
        for (int i = 0; i < info->line_count; ++i)
            if (strcmp(info->names[i], name) == 0)
                return i;
    }
Error:
    return -1;
}

int
setup(AcquireRuntime* runtime)
{
    AcquireProperties props = {};

    const DeviceManager* dm = 0;
    CHECK(dm = acquire_device_manager(runtime));

#define SIZED(s) s, sizeof(s) - 1
    DEVOK(device_manager_select(dm,
                                DeviceKind_Camera,
                                SIZED("Hamamatsu C15440-20UP.*"),
                                &props.video[0].camera.identifier));
    DEVOK(device_manager_select(dm,
                                DeviceKind_Storage,
                                SIZED("Trash"),
                                &props.video[0].storage.identifier));
#undef SIZED

    props.video[0].max_frame_count = 10;
    props.video[0].camera.settings.output_triggers.exposure = {
        .enable = 1,
        .line = 1,
        .kind = Signal_Output,
        .edge = TriggerEdge_Rising
    };
    OK(acquire_configure(runtime, &props));
    // Expect that "Timing 1" is line id 1.
    // select_digital_line() must be called after setting the camera.
    CHECK(select_digital_line(runtime, "Timing 1") == 1);
    return 1;
Error:
    return 0;
}

int
main()
{
    int ecode = 0;
    AcquireRuntime* runtime = 0;
    CHECK(runtime = acquire_init(reporter));
    CHECK(setup(runtime));
    OK(acquire_start(runtime));
    OK(acquire_stop(runtime));
Finalize:
    acquire_shutdown(runtime);
    return ecode;
Error:
    ecode = 1;
    goto Finalize;
}