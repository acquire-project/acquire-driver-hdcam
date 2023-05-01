#include "dcam.camera.h"
#include "device/props/device.h"
#include "logger.h"

#include "dcam.error.h"
#include "dcam.getset.h"
#include "dcam.prelude.h"
#include "device/kit/driver.h"
#include "device/hal/camera.h"

#include <stddef.h> // this needs to come before dcamapi4.h to define wchar_t
#include <dcamapi4.h>
#include <dcamprop.h>

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define countof(e) (sizeof(e) / sizeof((e)[0]))
#define containerof(P, T, F) ((T*)(((char*)(P)) - offsetof(T, F)))

#define MAX_CAMERAS (countof(((struct Dcam4Driver*)(0))->cameras))

//
// These define the logical index of these digital lines
//
#define LINE_EXT_TRIG 0
#define LINE_TIMING1 1
#define LINE_TIMING2 2
#define LINE_TIMING3 3
#define LINE_SOFTWARE 4

//
// HDCAM Property accessors
//
#define prop_read(type, hdcam, prop_id, out)                                   \
    prop_read_##type(hdcam, prop_id, out, #prop_id)

#define prop_write(type, hdcam, prop_id, in)                                   \
    prop_write_##type(hdcam, prop_id, in, #prop_id)

#define array_prop_read(type, hdcam, prop_id, idx, out)                        \
    array_prop_read_##type(hdcam, prop_id, idx, out, #prop_id)

#define array_prop_RW(type, hdcam, prop_id, idx, in)                           \
    array_prop_rw_##type(hdcam, prop_id, idx, in, #prop_id)

#define array_prop_write(type, hdcam, prop_id, idx, in)                        \
    array_prop_write_##type(hdcam, prop_id, idx, in, #prop_id)

#define prop_read_scaled(type, hdcam, prop_id, scale, out)                     \
    prop_read_##type(hdcam, prop_id, scale, out, #prop_id)

#define prop_write_scaled(type, hdcam, prop_id, scale, in)                     \
    prop_write_##type(hdcam, prop_id, scale, in, #prop_id)

//
// Forward declarations
//
struct Dcam4Camera*
reset_driver_and_replace_camera(struct Dcam4Camera* self);

struct image_descriptor
{
    DCAM_PIXELTYPE pixel_type;
    int32_t offset, pitch, width, height;
};

static int
set_sample_type(HDCAM hdcam, enum SampleType* value)
{
    double v;
    switch (*value) {
        case SampleType_u8:
            v = DCAM_PIXELTYPE_MONO8;
            break;
        case SampleType_u16:
            v = DCAM_PIXELTYPE_MONO16;
            break;
        default:
            ERR("Unexpected pixel type");
            return 0;
    }
    DCAM(dcamprop_setgetvalue(hdcam, DCAM_IDPROP_IMAGE_PIXELTYPE, &v, 0));
    switch ((int)v) {
        case DCAM_PIXELTYPE_MONO8:
            *value = SampleType_u8;
            break;
        case DCAM_PIXELTYPE_MONO16:
            *value = SampleType_u16;
            break;
        default:
            *value = SampleType_Unknown;
    }
    return 1;
Error:
    return 0;
}

static int
get_image_description(HDCAM h, struct image_descriptor* desc)
{
    int is_ok = 1;
    is_ok &=
      prop_read(i32, h, DCAM_IDPROP_BUFFER_TOPOFFSETBYTES, &desc->offset);
    is_ok &= prop_read(i32, h, DCAM_IDPROP_BUFFER_ROWBYTES, &desc->pitch);
    is_ok &= prop_read(i32, h, DCAM_IDPROP_IMAGE_WIDTH, &desc->width);
    is_ok &= prop_read(i32, h, DCAM_IDPROP_IMAGE_HEIGHT, &desc->height);
    is_ok &= prop_read(
      i32, h, DCAM_IDPROP_BUFFER_PIXELTYPE, (int32_t*)&desc->pixel_type);
    return is_ok;
}

static enum SampleType
to_sample_type(DCAM_PIXELTYPE p)
{
    switch (p) {
        case DCAM_PIXELTYPE_MONO8:
            return SampleType_u8;
        case DCAM_PIXELTYPE_MONO16:
            return SampleType_u16;

            // The following are unsupported

        case DCAM_PIXELTYPE_MONO12:
        case DCAM_PIXELTYPE_MONO12P:
        case DCAM_PIXELTYPE_RGB24:
        case DCAM_PIXELTYPE_RGB48:
        case DCAM_PIXELTYPE_BGR24:
        case DCAM_PIXELTYPE_BGR48:
        case DCAM_PIXELTYPE_NONE:
        default:
            return SampleType_Unknown;
    }
}

static enum TriggerEdge
to_trigger_edge(uint64_t p)
{
    switch (p) {
        case DCAMPROP_TRIGGERPOLARITY__POSITIVE:
            return TriggerEdge_Rising;
        case DCAMPROP_TRIGGERPOLARITY__NEGATIVE:
            return TriggerEdge_Falling;
        default:
            return TriggerEdge_NotApplicable;
    }
}

static int
set_readout_speed(HDCAM h)
{
    DCAM(dcamprop_setvalue(
      h, DCAM_IDPROP_READOUTSPEED, DCAMPROP_READOUTSPEED__FASTEST));
    DCAM(dcamprop_setvalue(h,
                           DCAM_IDPROP_SENSORMODE,
                           DCAMPROP_SENSORMODE__PROGRESSIVE)); // lightsheet
    return 1;
Error:
    return 0;
}

/// @return first enabled trigger or 0 if none
static struct Trigger*
select_trigger(struct CameraProperties* settings)
{
    size_t n_triggers =
      sizeof(settings->input_triggers) / sizeof(struct Trigger);
    struct Trigger* triggers = (struct Trigger*)&settings->input_triggers;
    for (int i = 0; i < n_triggers; ++i) {
        if (triggers[i].enable)
            return triggers + i;
    }
    return 0;
}

static int
set_input_triggering(HDCAM h, struct CameraProperties* settings)
{
    int use_software_trigger = 0;
    {
        struct Trigger* event = select_trigger(settings);
        if (!event) {
            // None are enabled
            DCAM(dcamprop_setvalue(
              h, DCAM_IDPROP_TRIGGER_MODE, DCAMPROP_TRIGGER_MODE__NORMAL));
            DCAM(dcamprop_setvalue(
              h, DCAM_IDPROP_TRIGGERSOURCE, DCAMPROP_TRIGGERSOURCE__INTERNAL));
        } else {
            use_software_trigger = (event->line == LINE_SOFTWARE);
            CHECK(event->kind == Signal_Input);
            if (event == &settings->input_triggers.acquisition_start) {
                DCAM(dcamprop_setvalue(
                  h, DCAM_IDPROP_TRIGGER_MODE, DCAMPROP_TRIGGER_MODE__START));
                DCAM(dcamprop_setvalue(
                  h, DCAM_IDPROP_TRIGGERACTIVE, DCAMPROP_TRIGGERACTIVE__EDGE));
            } else if (event == &settings->input_triggers.frame_start) {
                DCAM(dcamprop_setvalue(
                  h, DCAM_IDPROP_TRIGGER_MODE, DCAMPROP_TRIGGER_MODE__NORMAL));
                DCAM(dcamprop_setvalue(
                  h, DCAM_IDPROP_TRIGGERACTIVE, DCAMPROP_TRIGGERACTIVE__EDGE));
            } else if (event == &settings->input_triggers.exposure) {
                DCAM(dcamprop_setvalue(
                  h, DCAM_IDPROP_TRIGGER_MODE, DCAMPROP_TRIGGER_MODE__NORMAL));
                DCAM(dcamprop_setvalue(
                  h, DCAM_IDPROP_TRIGGERACTIVE, DCAMPROP_TRIGGERACTIVE__LEVEL));
            } else {
                ERR("Tried to set an unsupported input triggering mode.");
            }

            DCAM(
              dcamprop_setvalue(h,
                                DCAM_IDPROP_TRIGGERPOLARITY,
                                event->edge == TriggerEdge_Falling
                                  ? DCAMPROP_TRIGGERENABLE_POLARITY__NEGATIVE
                                  : DCAMPROP_TRIGGERENABLE_POLARITY__POSITIVE));

            DCAM(dcamprop_setvalue(h,
                                   DCAM_IDPROP_TRIGGERSOURCE,
                                   use_software_trigger
                                     ? DCAMPROP_TRIGGERSOURCE__SOFTWARE
                                     : DCAMPROP_TRIGGERSOURCE__EXTERNAL));
        }
    }

    DCAM(dcamprop_setvalue(
      h, DCAM_IDPROP_TRIGGER_CONNECTOR, DCAMPROP_TRIGGER_CONNECTOR__BNC));
    DCAM(dcamprop_setvalue(
      h,
      DCAM_IDPROP_TRIGGER_GLOBALEXPOSURE,
      DCAMPROP_TRIGGER_GLOBALEXPOSURE__DELAYED)); // Rolling shutter
    DCAM(dcamprop_setvalue(h, DCAM_IDPROP_TRIGGERDELAY, 0));
    return 1;
Error:
    return 0;
}

static int
set_output_trigger__handler(HDCAM h,
                            const struct Trigger* trigger,
                            int* used_lines,
                            int32_t kind,
                            int32_t source)
{
    if (trigger->enable) {
        uint8_t line = trigger->line;
        uint8_t edge = trigger->edge == TriggerEdge_Falling
                         ? DCAMPROP_OUTPUTTRIGGER_POLARITY__NEGATIVE
                         : DCAMPROP_OUTPUTTRIGGER_POLARITY__POSITIVE;
        used_lines[line] = 1;
        array_prop_write(i32, h, DCAM_IDPROP_OUTPUTTRIGGER_KIND, line, kind);

        array_prop_write(
          i32, h, DCAM_IDPROP_OUTPUTTRIGGER_SOURCE, line, source);

        DCAM(array_prop_write(
          i32, h, DCAM_IDPROP_OUTPUTTRIGGER_POLARITY, line, edge));
    }
    return 1;
Error:
    return 0;
}

static int
set_output_triggering(HDCAM h, struct CameraProperties* settings)
{
    int used_lines[3] = { 0 };

    if (settings->output_triggers.exposure.enable) {
        CHECK(
          set_output_trigger__handler(h,
                                      &settings->output_triggers.exposure,
                                      used_lines,
                                      DCAMPROP_OUTPUTTRIGGER_KIND__EXPOSURE,
                                      DCAMPROP_OUTPUTTRIGGER_SOURCE__EXPOSURE));
    }

    if (settings->output_triggers.frame_start.enable) {
        CHECK(
          set_output_trigger__handler(h,
                                      &settings->output_triggers.frame_start,
                                      used_lines,
                                      DCAMPROP_OUTPUTTRIGGER_KIND__PROGRAMABLE,
                                      DCAMPROP_OUTPUTTRIGGER_SOURCE__VSYNC));
    }

    if (settings->output_triggers.trigger_wait.enable) {
        CHECK(set_output_trigger__handler(
          h,
          &settings->output_triggers.trigger_wait,
          used_lines,
          DCAMPROP_OUTPUTTRIGGER_KIND__TRIGGERREADY,
          DCAMPROP_OUTPUTTRIGGER_SOURCE__READOUTEND));
    }

    // set unused lines low
    for (int i = 0; i < countof(used_lines); ++i) {
        array_prop_write(i32,
                         h,
                         DCAM_IDPROP_OUTPUTTRIGGER_KIND,
                         i,
                         DCAMPROP_OUTPUTTRIGGER_KIND__LOW);
    }

    return 1;
Error:
    return 0;
}

static int
read_prop_capabilities_(struct Property* out,
                        HDCAM h,
                        int32_t prop_id,
                        float unit_conversion,
                        const char* prop_name)
{
    DCAMPROP_ATTR attr = { .cbSize = sizeof(attr),
                           .iProp = prop_id,
                           .option = 0 };
    DCAM(dcamprop_getattr(h, &attr));
    *out = (struct Property){
        .low = unit_conversion * (float)attr.valuemin,
        .high = unit_conversion * (float)attr.valuemax,
        .writable =
          (attr.attribute & DCAMPROP_ATTR_WRITABLE) == DCAMPROP_ATTR_WRITABLE,
    };
    return 1;
Error:
    LOG("Failed to get attributes for property %s", prop_name);
    return 0;
}

int
aq_dcam_get_metadata__inner(const struct Dcam4Camera* self,
                            struct CameraPropertyMetadata* metadata)
{
    int is_ok = 1;

#define READ_PROP_META(meta, prop, unit_conversion)                            \
    read_prop_capabilities_(meta, self->hdcam, prop, unit_conversion, #prop)

    is_ok &= READ_PROP_META(&metadata->exposure_time_us,
                            DCAM_IDPROP_EXPOSURETIME,
                            1e6f /*[usec/sec]*/);

    is_ok &= READ_PROP_META(&metadata->line_interval_us,
                            DCAM_IDPROP_INTERNAL_LINEINTERVAL,
                            1e6f /*[usec/sec]*/);

    is_ok &= READ_PROP_META(&metadata->binning, DCAM_IDPROP_BINNING, 1.0f);

    is_ok &= READ_PROP_META(
      &metadata->readout_direction, DCAM_IDPROP_READOUT_DIRECTION, 1.0f);

    is_ok &=
      READ_PROP_META(&metadata->offset.x, DCAM_IDPROP_SUBARRAYHPOS, 1.0f);
    is_ok &=
      READ_PROP_META(&metadata->offset.y, DCAM_IDPROP_SUBARRAYVPOS, 1.0f);
    is_ok &=
      READ_PROP_META(&metadata->shape.x, DCAM_IDPROP_SUBARRAYHSIZE, 1.0f);
    is_ok &=
      READ_PROP_META(&metadata->shape.y, DCAM_IDPROP_SUBARRAYVSIZE, 1.0f);

#undef READ_PROP_META

    metadata->supported_pixel_types =
      (1 << SampleType_u8) | (1 << SampleType_u16);

    // Triggering
    // Names for trigger lines are taken from Orca Fusion manual
    metadata->digital_lines =
      (struct CameraPropertyMetadataDigitalLineMetadata){
          .line_count = 5,
          .names = {
            [LINE_EXT_TRIG] = "Ext.Trig",
            [LINE_TIMING1] = "Timing 1",
            [LINE_TIMING2] = "Timing 2",
            [LINE_TIMING3] = "Timing 3",
            [LINE_SOFTWARE] = "Software",
          },
      };
    metadata->triggers = (struct CameraPropertiesTriggerMetadata){
        .acquisition_start = { .input = 1, .output = 0 },
        .exposure = { .input = 1, .output = 1 },
        .frame_start = { .input = 1, .output = 1 },
    };
    return is_ok;
}

enum DeviceStatusCode
aq_dcam_get_metadata(const struct Camera* self_,
                     struct CameraPropertyMetadata* metadata)
{
    struct Dcam4Camera* self = containerof(self_, struct Dcam4Camera, camera);
    lock_acquire(&self->lock);
    int is_ok = aq_dcam_get_metadata__inner(self, metadata);
    lock_release(&self->lock);
    return is_ok ? Device_Ok : Device_Err;
}

static void
clamp_uint32_t(uint32_t* v, float low_, float high_)
{
    const uint32_t low = (uint32_t)low_;
    const uint32_t high = (uint32_t)high_;
    if (low > *v)
        *v = low;
    else if (*v > high)
        *v = high;
}

static void
clamp_uint8_t(uint8_t* v, float low_, float high_)
{
    const uint8_t low = (uint8_t)low_;
    const uint8_t high = (uint8_t)high_;
    if (low > *v)
        *v = low;
    else if (*v > high)
        *v = high;
}

static void
clamp_float(float* v, float low, float high)
{
    if (low > *v)
        *v = low;
    else if (*v > high)
        *v = high;
}

enum DeviceStatusCode
aq_dcam_set__inner(struct Dcam4Camera* self,
                   struct CameraProperties* props,
                   int force)
{
    HDCAM hdcam = self->hdcam;

    {
        struct CameraPropertyMetadata metadata;
        aq_dcam_get_metadata__inner(self, &metadata);
#define CLAMP(type, field)                                                     \
    clamp_##type(&props->field, metadata.field.low, metadata.field.high);
        CLAMP(uint8_t, binning);
#undef CLAMP
    }

#define IS_CHANGED(prop) (force || (self->last_props.prop != props->prop))

    int is_ok = 1;

    // pixel type
    if (IS_CHANGED(pixel_type)) {
        is_ok &= set_sample_type(hdcam, &props->pixel_type);
    }

    // binning
    if (IS_CHANGED(binning)) {
        dcamprop_setvalue(
          hdcam, DCAM_IDPROP_BINNING_INDEPENDENT, DCAMPROP_MODE__OFF);
        int32_t v = props->binning;
        is_ok &= prop_write(i32, hdcam, DCAM_IDPROP_BINNING, &v);
        props->binning = (uint8_t)v;
    }

    // readout direction
    if (IS_CHANGED(readout_direction)) {
        double value = 0;
        switch (props->readout_direction) {
            case Direction_Backward:
                value = DCAMPROP_READOUT_DIRECTION__BACKWARD;
                break;
            case Direction_Forward:
                value = DCAMPROP_READOUT_DIRECTION__FORWARD;
                break;
            default:
                ERR("Unrecognized readout direction value (%d). Using "
                    "FORWARD.",
                    props->readout_direction);
                value = DCAMPROP_READOUT_DIRECTION__FORWARD;
        }
        dcamprop_setvalue(hdcam, DCAM_IDPROP_READOUT_DIRECTION, value);
    }

    {
        // Binning changes a bunch of stuff so re-read the capabilities
        struct CameraPropertyMetadata metadata;
        aq_dcam_get_metadata__inner(self, &metadata);
#define CLAMP(type, field)                                                     \
    clamp_##type(&props->field, metadata.field.low, metadata.field.high);
        CLAMP(uint32_t, offset.x);
        CLAMP(uint32_t, offset.y);
        CLAMP(uint32_t, shape.x);
        CLAMP(uint32_t, shape.y);
        CLAMP(float, exposure_time_us);
        CLAMP(float, line_interval_us);
#undef CLAMP
    }

    // roi
    if (IS_CHANGED(offset.x) || IS_CHANGED(offset.y) || IS_CHANGED(shape.x) ||
        IS_CHANGED(shape.y)) {
        dcamprop_setvalue(hdcam, DCAM_IDPROP_SUBARRAYMODE, DCAMPROP_MODE__ON);
        is_ok &=
          prop_write(u32, hdcam, DCAM_IDPROP_SUBARRAYHPOS, &props->offset.x);
        is_ok &=
          prop_write(u32, hdcam, DCAM_IDPROP_SUBARRAYVPOS, &props->offset.y);
        is_ok &=
          prop_write(u32, hdcam, DCAM_IDPROP_SUBARRAYHSIZE, &props->shape.x);
        is_ok &=
          prop_write(u32, hdcam, DCAM_IDPROP_SUBARRAYVSIZE, &props->shape.y);
    }

    // exposure
    is_ok &= set_readout_speed(hdcam);
    if (IS_CHANGED(exposure_time_us) || IS_CHANGED(line_interval_us)) {
        is_ok &= prop_write_scaled(f32,
                                   hdcam,
                                   DCAM_IDPROP_INTERNAL_LINEINTERVAL,
                                   1e-6f,
                                   &props->line_interval_us);
        is_ok &= prop_write_scaled(f32,
                                   hdcam,
                                   DCAM_IDPROP_EXPOSURETIME,
                                   1e-6f,
                                   &props->exposure_time_us);
    }
#undef IS_CHANGED

    if (memcmp(&props->input_triggers,
               &self->last_props.input_triggers,
               sizeof(props->input_triggers)) != 0) {
        is_ok &= set_input_triggering(hdcam, props);
        is_ok &= set_output_triggering(hdcam, props);
    }
    if (is_ok)
        self->last_props = *props;

    return is_ok ? Device_Ok : Device_Err;
}

enum DeviceStatusCode
aq_dcam_set(struct Camera* self_, struct CameraProperties* props)
{
    struct Dcam4Camera* self = containerof(self_, struct Dcam4Camera, camera);
    lock_acquire(&self->lock);
    enum DeviceStatusCode result = aq_dcam_set__inner(self, props, 0);
    lock_release(&self->lock);
    return result;
}

static struct Trigger*
as_camera_trigger_output_event(
  int source,
  int kind,
  struct camera_properties_output_triggers_s* output_triggers)
{
    if (kind == DCAMPROP_OUTPUTTRIGGER_KIND__PROGRAMABLE &&
        source == DCAMPROP_OUTPUTTRIGGER_SOURCE__VSYNC) {
        return &output_triggers->frame_start;
    }
    if (kind == DCAMPROP_OUTPUTTRIGGER_KIND__EXPOSURE) {
        return &output_triggers->exposure;
    }
    if (kind == DCAMPROP_OUTPUTTRIGGER_KIND__TRIGGERREADY) {
        return &output_triggers->trigger_wait;
    }
    return 0;
}

static enum TriggerEdge
as_camera_trigger_edge(int p)
{
    switch (p) {
        case DCAMPROP_TRIGGERPOLARITY__POSITIVE:
            return TriggerEdge_Rising;
        case DCAMPROP_TRIGGERPOLARITY__NEGATIVE:
            return TriggerEdge_Falling;
        default:
            return TriggerEdge_NotApplicable;
    }
}

static int
query_input_triggering(struct Dcam4Camera* self, struct CameraProperties* props)
{
    // There's only 1 input line.
    // Query the trigger source. "internal" is disabled, "external" means
    // the external trigger is enabled, and "software" means software
    // triggering is enabled.

    //  Trigger lines
    //  0: Ext.Trig - in
    //  1: TIMING 1 - out
    //  2: TIMING 2 - out
    //  3: TIMING 3 - out
    //  4: Software - in

    // zero init
    memset(&props->input_triggers, 0, sizeof(props->input_triggers));

    // Identify the trigger event
    struct Trigger* event = 0;
    {
        double mode, trigger_active;
        DCAM(dcamprop_getvalue(
          self->hdcam, DCAM_IDPROP_TRIGGERACTIVE, &trigger_active));
        DCAM(dcamprop_getvalue(self->hdcam, DCAM_IDPROP_TRIGGER_MODE, &mode));
        switch ((int)mode) {
            case DCAMPROP_TRIGGER_MODE__NORMAL:
                if (trigger_active == DCAMPROP_TRIGGERACTIVE__LEVEL) {
                    event = &props->input_triggers.exposure;
                } else {
                    event = &props->input_triggers.frame_start;
                }
                break;
            case DCAMPROP_TRIGGER_MODE__START:
                WARN(trigger_active == DCAMPROP_TRIGGERACTIVE__EDGE);
                event = &props->input_triggers.acquisition_start;
                break;
            default:;
        }
    }
    EXPECT(event, "Could not identify the trigger event.");

    {
        double source = 0.0;
        DCAM(
          dcamprop_getvalue(self->hdcam, DCAM_IDPROP_TRIGGERSOURCE, &source));
        switch ((int)source) {
            case DCAMPROP_TRIGGERSOURCE__EXTERNAL:
                event->enable = 1;
                event->line = 0; // Ext.Trig
                break;
            case DCAMPROP_TRIGGERSOURCE__SOFTWARE:
                event->enable = 1;
                event->line = LINE_SOFTWARE;
                break;
            default:;
        }
    }

    // Input trigger edge
    {
        double polarity = 0.0;
        DCAM(dcamprop_getvalue(
          self->hdcam, DCAM_IDPROP_TRIGGERPOLARITY, &polarity));
        event->edge = as_camera_trigger_edge((int)polarity);
    }
    return 1;
Error:
    return 0;
}

static int
query_output_triggering(struct Dcam4Camera* self,
                        struct CameraProperties* props)
{
    memset(&props->output_triggers, 0, sizeof(props->output_triggers));

    {
        uint32_t output_line_count = 0;
        prop_read(u32,
                  self->hdcam,
                  DCAM_IDPROP_NUMBEROF_OUTPUTTRIGGERCONNECTOR,
                  &output_line_count);
        // Among other things, more than 1 output line means
        // the OUTPUTTRIGGER props are array props
        EXPECT(output_line_count == 3, "Expected 3 output lines.");
    }

    // Iterate over output lines, and assign events
    for (int id = 0; id < 3; ++id) {
        struct Trigger* event = 0;
        {
            int32_t kind = 0, source = 0;
            CHECK(array_prop_read(
              i32, self->hdcam, DCAM_IDPROP_OUTPUTTRIGGER_KIND, id, &kind));
            CHECK(array_prop_read(
              i32, self->hdcam, DCAM_IDPROP_OUTPUTTRIGGER_SOURCE, id, &source));
            event = as_camera_trigger_output_event(
              source, kind, &props->output_triggers);
        }
        if (event) {
            // line `id` seems to be selected for `event`
            int32_t polarity = 0;
            CHECK(array_prop_read(i32,
                                  self->hdcam,
                                  DCAM_IDPROP_OUTPUTTRIGGER_POLARITY,
                                  id,
                                  &polarity));
            *event = (struct Trigger){
                .enable = 1,
                .line = id,
                .edge = to_trigger_edge(polarity),
                .kind = Signal_Output,
            };
        }
    }
    return 1;
Error:
    return 0;
}

enum DeviceStatusCode
aq_dcam_get(const struct Camera* self_, struct CameraProperties* props)
{
    struct Dcam4Camera* self = containerof(self_, struct Dcam4Camera, camera);
    lock_acquire(&self->lock);
    int is_ok = 1;

    // roi
    is_ok &=
      prop_read(u32, self->hdcam, DCAM_IDPROP_SUBARRAYHPOS, &props->offset.x);
    is_ok &=
      prop_read(u32, self->hdcam, DCAM_IDPROP_SUBARRAYVPOS, &props->offset.y);
    is_ok &=
      prop_read(u32, self->hdcam, DCAM_IDPROP_SUBARRAYHSIZE, &props->shape.x);
    is_ok &=
      prop_read(u32, self->hdcam, DCAM_IDPROP_SUBARRAYVSIZE, &props->shape.y);

    // pixel type
    {
        double v;
        DCAM(dcamprop_getvalue(self->hdcam, DCAM_IDPROP_BUFFER_PIXELTYPE, &v));
        props->pixel_type = to_sample_type((DCAM_PIXELTYPE)v);
    }

    // binning
    {
        int32_t binning = props->binning;
        is_ok &= prop_read(i32, self->hdcam, DCAM_IDPROP_BINNING, &binning);
        props->binning = binning;
    }

    // Exposure
    is_ok &= prop_read_scaled(f32,
                              self->hdcam,
                              DCAM_IDPROP_EXPOSURETIME,
                              1e6f,
                              &props->exposure_time_us);

    // Line interval time
    is_ok &= prop_read_scaled(f32,
                              self->hdcam,
                              DCAM_IDPROP_INTERNAL_LINEINTERVAL,
                              1e6f,
                              &props->line_interval_us);

    // Readout mode
    {
        double value = 0.0;
        DCAM(dcamprop_getvalue(
          self->hdcam, DCAM_IDPROP_READOUT_DIRECTION, &value));
        switch ((int)value) {
            case DCAMPROP_READOUT_DIRECTION__FORWARD:
                props->readout_direction = Direction_Forward;
                break;
            case DCAMPROP_READOUT_DIRECTION__BACKWARD:
                props->readout_direction = Direction_Backward;
                break;
            case DCAMPROP_READOUT_DIRECTION__BYTRIGGER:
                ERR("Unsupported readout direction \"BYTRIGGER\"");
                props->readout_direction = Direction_Unknown;
                break;
            case DCAMPROP_READOUT_DIRECTION__DIVERGE:
                ERR("Unsupported readout direction \"DIVERGE\"");
                props->readout_direction = Direction_Unknown;
                break;
            default:
                ERR("Unknown readout direction.");
                props->readout_direction = Direction_Unknown;
        }
    }

    CHECK(query_input_triggering(self, props));
    CHECK(query_output_triggering(self, props));

    self->last_props = *props;
    lock_release(&self->lock);
    return is_ok ? Device_Ok : Device_Err;
Error:
    lock_release(&self->lock);
    return Device_Err;
}

enum DeviceStatusCode
aq_dcam_get_shape(const struct Camera* self_, struct ImageShape* shape)
{
    struct Dcam4Camera* self = containerof(self_, struct Dcam4Camera, camera);
    lock_acquire(&self->lock);
    struct image_descriptor desc = { 0 };
    CHECK(get_image_description(self->hdcam, &desc));

    memset(shape, 0, sizeof(*shape));
    *shape =
      (struct ImageShape){ .dims = { .channels = 1,
                                     .width = desc.width,
                                     .height = desc.height,
                                     .planes = 1, },
                           .strides = { .channels = 1,
                                        .width = 1,
                                        .height = desc.width,
                                        .planes = desc.width * desc.height, },
                           .type = to_sample_type(desc.pixel_type), };
    lock_release(&self->lock);
    return Device_Ok;
Error:
    lock_release(&self->lock);
    return Device_Err;
}

enum DeviceStatusCode
aq_dcam_start(struct Camera* self_)
{
    if (!self_) {
        ERR("Expected non-NULL camera pointer.");
        return Device_Err;
    }
    struct Dcam4Camera* self = containerof(self_, struct Dcam4Camera, camera);
    lock_acquire(&self->lock);
    int retries = 2;
    while (retries-- > 0) {
        TRACE("DCAM: Alloc framebuffers and start");
        DCAM(dcambuf_alloc(self->hdcam, 10));
        DCAM(dcamcap_start(self->hdcam, DCAMCAP_START_SEQUENCE));
        break;
    Error : {
        if (retries <= 0)
            goto Fail;
        LOG("Attempting to reset driver");
        struct Dcam4Camera* tmp = 0;
        lock_release(&self->lock);
        if ((tmp = reset_driver_and_replace_camera(self))) {
            self = tmp;
        }
        lock_acquire(&self->lock);
    } // end error block
    } // end while(retries-->0)
    lock_release(&self->lock);
    return Device_Ok;
Fail:
    lock_release(&self->lock);
    return Device_Err;
}

enum DeviceStatusCode
aq_dcam_stop(struct Camera* self_)
{
    struct Dcam4Camera* self = containerof(self_, struct Dcam4Camera, camera);
    lock_acquire(&self->lock);
    DWRN(dcamwait_abort(self->wait));
    DWRN(dcamcap_stop(self->hdcam));
    DWRN(dcambuf_release(self->hdcam, 0));
    lock_release(&self->lock);
    return Device_Ok;
}

enum DeviceStatusCode
aq_dcam_fire_software_trigger(struct Camera* self_)
{
    struct Dcam4Camera* self = containerof(self_, struct Dcam4Camera, camera);
    DCAM(dcamcap_firetrigger(self->hdcam, 0));
    return Device_Ok;
Error:
    return Device_Err;
}

enum DeviceStatusCode
aq_dcam_get_frame(struct Camera* self_,
                  void* im,
                  size_t* nbytes,
                  struct ImageInfo* info_)
{
    struct Dcam4Camera* self = containerof(self_, struct Dcam4Camera, camera);
    DCAMWAIT_START p = {
        .size = sizeof(p),
        .eventmask = (int32)DCAMWAIT_CAPEVENT_FRAMEREADY,
        .timeout = (int32)DCAMWAIT_TIMEOUT_INFINITE,
    };
    lock_acquire(&self->lock);

    CHECK(self_->state == DeviceState_Running);
    TRACE("DCAM device id: %d\tdcam: %p\thwait: %p",
          (int)self->camera.device.identifier.device_id,
          self->hdcam,
          self->wait);
    {
        DCAMERR dcamwait_start_result = dcamwait_start(self->wait, &p);
        if (dcamwait_start_result == DCAMERR_ABORT) {
            *nbytes = 0;
            LOG("CAMERA ABORT");
            return Device_Ok;
        }
        DCAM(dcamwait_start_result);
    }

    {
        struct image_descriptor d;
        CHECK(get_image_description(self->hdcam, &d));
        DCAMBUF_FRAME frame = {
            .size = sizeof(frame),
            .iFrame = -1,
            .buf = im,
            .rowbytes = d.pitch,
            .width = d.width,
            .height = d.height,
        };
        DCAM(dcambuf_copyframe(self->hdcam, &frame));
        *nbytes = (size_t)frame.rowbytes * frame.height;

        info_->hardware_frame_id = (uint64_t)frame.framestamp;
        info_->hardware_timestamp = (uint64_t)(1e6 * frame.timestamp.sec) +
                                    (uint64_t)frame.timestamp.microsec;
    }

    lock_release(&self->lock);
    return Device_Ok;
Error:
    lock_release(&self->lock);
    return Device_Err;
}
