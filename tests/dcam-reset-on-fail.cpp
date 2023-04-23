/// Start should retry on failure.

#include "acquire.h"
#include "device/hal/device.manager.h"
#include "platform.h"
#include "logger.h"

#include <cstdio>
#include <stdexcept>

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

#define SIZED(str) str, sizeof(str) - 1

#define L (aq_logger)
#define LOG(...) L(0, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define ERR(...) L(1, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define EXPECT(e, ...)                                                         \
    do {                                                                       \
        if (!(e)) {                                                            \
            char buf[1 << 8] = { 0 };                                          \
            ERR(__VA_ARGS__);                                                  \
            snprintf(buf, sizeof(buf) - 1, __VA_ARGS__);                       \
            throw std::runtime_error(buf);                                     \
        }                                                                      \
    } while (0)
#define CHECK(e) EXPECT(e, "Expression evaluated as false: %s", #e)
#define DEVOK(e) CHECK(Device_Ok == (e))
#define AQOK(e) CHECK(AcquireStatus_Ok == (e))

int
main()
{
    AcquireRuntime* runtime = acquire_init(reporter);
    try {
        CHECK(runtime);

        const DeviceManager* dm = acquire_device_manager(runtime);
        CHECK(dm);

        AcquireProperties props = { 0 };
        AQOK(acquire_get_configuration(runtime, &props));

        DEVOK(device_manager_select(dm,
                                    DeviceKind_Camera,
                                    SIZED("C15440-20UP.*"),
                                    &props.video[0].camera.identifier));
        DEVOK(device_manager_select(dm,
                                    DeviceKind_Storage,
                                    SIZED("Trash"),
                                    &props.video[0].storage.identifier));

        props.video[0].camera.settings.binning = 1;
        props.video[0].camera.settings.pixel_type = SampleType_u16;
        props.video[0].camera.settings.shape = { .x = 2304, .y = 2304 };
        props.video[0].max_frame_count = 100;

        AQOK(acquire_configure(runtime, &props));
        AQOK(acquire_start(runtime));
        AQOK(acquire_stop(runtime));

        LOG("TURN OFF THE CAMERA NOW. Then press <Enter>.");
        // getc(stdin);
        clock_sleep_ms(0, 1000);
        LOG("RESTARTING");
        AQOK(acquire_start(runtime));
        LOG("RESTARTED");
        AQOK(acquire_stop(runtime));
        LOG("DONE (OK)");
        AQOK(acquire_shutdown(runtime));
        return 0;
    } catch (const std::runtime_error& e) {
        ERR("Runtime error: %s", e.what());

    } catch (...) {
        ERR("Uncaught exception");
    }
    return 1;
}
