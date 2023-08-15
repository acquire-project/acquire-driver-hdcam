//! Test: Runs through setting up acquiring from a sub-rectangle of the sensor
#include "acquire.h"
#include "device/hal/device.manager.h"
#include "device/props/components.h"
#include "logger.h"
#include <stdio.h>
#include <string.h>

#define L (aq_logger)
#define LOG(...) L(0, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define ERR(...) L(1, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define EXPECT(e, ...)                                                         \
    do {                                                                       \
        if (!(e)) {                                                            \
            ERR(__VA_ARGS__);                                                  \
            goto Error;                                                        \
        }                                                                      \
    } while (0)
#define CHECK(e) EXPECT(e, "Expression evaluated as false: %s", #e)
#define DEVOK(e) CHECK(Device_Ok == (e))
#define OK(e) CHECK(AcquireStatus_Ok == (e))

/// Check that a==b
/// example: `ASSERT_EQ(int,"%d",42,meaning_of_life())`
#define ASSERT_EQ(T, fmt, a, b)                                                \
    do {                                                                       \
        T a_ = (T)(a);                                                         \
        T b_ = (T)(b);                                                         \
        EXPECT(a_ == b_, "Expected %s==%s but " fmt "!=" fmt, #a, #b, a_, b_); \
    } while (0)

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
    props.video[0].camera.settings.shape = { .x = 1700, .y = 512 };
    props.video[0].camera.settings.offset = { .x = 304, .y = 896 };
    OK(acquire_configure(runtime, &props));
    // Expect that the roi is what we intended.

    ASSERT_EQ(int, "%d", props.video[0].camera.settings.shape.x, 1700);
    ASSERT_EQ(int, "%d", props.video[0].camera.settings.shape.y, 512);
    ASSERT_EQ(int, "%d", props.video[0].camera.settings.offset.x, 304);
    ASSERT_EQ(int, "%d", props.video[0].camera.settings.offset.y, 896);
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