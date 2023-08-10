#include "acquire.h"
#include "device/hal/device.manager.h"
#include "logger.h"

#include <stdio.h>

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

#define CASE(e)                                                                \
    case e:                                                                    \
        return #e

static const char*
signal_io_kind_to_string(SignalIOKind v)
{
    switch (v) {
        CASE(Signal_Input);
        CASE(Signal_Output);
        default:
            return "(invalid)";
    }
}
static const char*
trigger_edge_to_string(TriggerEdge v)
{
    switch (v) {
        CASE(TriggerEdge_Rising);
        CASE(TriggerEdge_Falling);
        CASE(TriggerEdge_AnyEdge);
        CASE(TriggerEdge_LevelHigh);
        CASE(TriggerEdge_LevelLow);
        CASE(TriggerEdge_NotApplicable);
        default:
            return "(invalid)";
    }
}

#undef CASE

int
main()
{
    int ecode = 1;
    AcquireRuntime* runtime = 0;
    AcquireProperties props = {};
    AcquirePropertyMetadata metadata = {};

    CHECK(runtime = acquire_init(reporter));
    OK(acquire_get_configuration(runtime, &props));

    // Select devices
    {
        const DeviceManager* dm;
#define SIZED(s) s, sizeof(s) - 1
        CHECK(dm = acquire_device_manager(runtime));
        DEVOK(device_manager_select(dm,
                                    DeviceKind_Camera,
                                    SIZED("Hamamatsu C15440-20UP.*"),
                                    &props.video[0].camera.identifier));
        DEVOK(device_manager_select(dm,
                                    DeviceKind_Storage,
                                    SIZED("Trash"),
                                    &props.video[0].storage.identifier));
#undef SIZED
    }
    OK(acquire_configure(runtime, &props));

    // Configure
    {
        props.video[0].max_frame_count = 1;
        props.video[0].frame_average_count = 0;
        props.video[0].camera.settings.pixel_type = SampleType_u16;
        props.video[0].camera.settings.shape = { .x = 640, .y = 480 };
        props.video[0].camera.settings.binning = 2;
        props.video[0].camera.settings.input_triggers = {
            .frame_start = { .enable=1, .line=1,},
        };

        OK(acquire_configure(runtime, &props));
    }

    acquire_get_configuration_metadata(runtime, &metadata);

    printf("Digital Lines\n");
    {
        const auto* info = &metadata.video[0].camera.digital_lines;
        for (int i = 0; i < info->line_count; ++i)
            printf("\t[%2d] %18s\n", i, info->names[i]);
    }

    printf("Input Events\n");
    {
        const int n_triggers =
          sizeof(props.video[0].camera.settings.input_triggers) /
          sizeof(Trigger);
        const char* event_names[] = {
            "AcquisitionStart",
            "FrameStart",
            "ExposureStart",
        };
        for (int stream_id = 0; stream_id < countof(props.video); ++stream_id) {
            if (props.video[stream_id].camera.identifier.kind !=
                DeviceKind_None) {
                printf("Video %d\n", stream_id);
                for (int i_trigger = 0; i_trigger < n_triggers; ++i_trigger) {
                    const Trigger* t =
                      ((const struct Trigger*)&props.video[stream_id]
                         .camera.settings.input_triggers) +
                      i_trigger;
                    printf("\t[%2d] %18s %8s %2d %20s %20s\n",
                           i_trigger,
                           t->enable ? metadata.video[stream_id]
                                         .camera.digital_lines.names[t->line]
                                     : "",
                           t->enable ? "enabled" : "",
                           t->line,
                           event_names[i_trigger],
                           t->enable ? trigger_edge_to_string(t->edge) : "");
                }
            }
        }
    }

    printf("Output Events\n");
    {
        const int n_triggers =
          sizeof(props.video[0].camera.settings.output_triggers) /
          sizeof(Trigger);
        const char* event_names[] = {
            "Exposure",
            "FrameStart",
            "TriggerWait",
        };
        for (int stream_id = 0; stream_id < countof(props.video); ++stream_id) {
            if (props.video[stream_id].camera.identifier.kind !=
                DeviceKind_None) {
                printf("Video %d\n", stream_id);
                for (int i_trigger = 0; i_trigger < n_triggers; ++i_trigger) {
                    const Trigger* t =
                      ((const struct Trigger*)&props.video[stream_id]
                         .camera.settings.output_triggers) +
                      i_trigger;
                    printf("\t[%2d] %18s %8s %2d %20s %20s\n",
                           i_trigger,
                           t->enable ? metadata.video[stream_id]
                                         .camera.digital_lines.names[t->line]
                                     : "",
                           t->enable ? "enabled" : "",
                           t->line,
                           event_names[i_trigger],
                           t->enable ? trigger_edge_to_string(t->edge) : "");
                }
            }
        }
    }

    ecode = 0;
Finalize:
    acquire_shutdown(runtime);
    return ecode;
Error:
    ecode = 1;
    goto Finalize;
}
