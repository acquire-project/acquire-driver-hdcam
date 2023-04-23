#include "dcam.getset.h"
#include "dcam.prelude.h"
#include "dcam.error.h"

#include "logger.h"

int
prop_read_i32(HDCAM h, int32_t prop_id, int32_t* out, const char* prop_name)
{
    double v;
    DCAM(dcamprop_getvalue(h, prop_id, &v));
    *out = (int32_t)v;
    return 1;
Error:
    LOG("Failed to read %s", prop_name);
    return 0;
}

int
prop_write_i32(HDCAM hdcam,
               int32_t prop_id,
               int32_t* value,
               const char* prop_name)
{
    double v = *value;
    DCAM(dcamprop_setgetvalue(hdcam, prop_id, &v, 0));
    *value = (int32_t)v;
    return 1;
Error:
    LOG("Failed to write %s", prop_name);
    return 0;
}

int
prop_read_u32(HDCAM h, int32_t prop_id, uint32_t* out, const char* prop_name)
{
    double v;
    DCAM(dcamprop_getvalue(h, prop_id, &v));
    *out = (uint32_t)v;
    return 1;
Error:
    LOG("Failed to read %s", prop_name);
    return 0;
}

int
prop_write_u32(HDCAM hdcam,
               int32_t prop_id,
               uint32_t* value,
               const char* prop_name)
{
    double v = *value;
    DCAM(dcamprop_setgetvalue(hdcam, prop_id, &v, 0));
    *value = (uint32_t)v;
    return 1;
Error:
    LOG("Failed to write %s", prop_name);
    return 0;
}

int
prop_read_f32(HDCAM h,
              int32_t prop_id,
              float scale,
              float* out,
              const char* prop_name)
{
    double v;
    DCAM(dcamprop_getvalue(h, prop_id, &v));
    *out = (float)(v * scale);
    return 1;
Error:
    LOG("Failed to read %s", prop_name);
    return 0;
}

int
prop_write_f32(HDCAM hdcam,
               int32_t prop_id,
               float scale,
               float* value,
               const char* prop_name)
{
    double v = *value * scale;
    DCAM(dcamprop_setgetvalue(hdcam, prop_id, &v, 0));
    *value = (float)(v / (double)scale);
    return 1;
Error:
    LOG("Failed to write %s", prop_name);
    return 0;
}

int
prop_read_f64(HDCAM h, int32_t prop_id, double* out, const char* prop_name)
{
    DCAM(dcamprop_getvalue(h, prop_id, out));
    return 1;
Error:
    LOG("Failed to read %s", prop_name);
    return 0;
}

int
prop_write_f64(HDCAM hdcam, int32_t prop_id, double v, const char* prop_name)
{
    DCAM(dcamprop_setvalue(hdcam, prop_id, v));
    return 1;
Error:
    LOG("Failed to write %s", prop_name);
    return 0;
}

int
array_prop_read_i32(HDCAM h,
                    int32_t prop_id,
                    uint32_t index,
                    int32_t* out,
                    const char* prop_name)
{
    int32 base, step;
    {
        DCAMPROP_ATTR attr = {
            .cbSize = sizeof(attr),
            .iProp = prop_id,
        };
        DCAM(dcamprop_getattr(h, &attr));
        base = attr.iProp_ArrayBase;
        step = attr.iPropStep_Element;
    }
    CHECK(prop_read_i32(h, base + index * step, out, prop_name));
    return 1;
Error:
    LOG("Failed to read %s[%d]", prop_name, index);
    return 0;
}

int
array_prop_rw_i32(HDCAM h,
                  int32_t prop_id,
                  uint32_t index,
                  int32_t* value,
                  const char* prop_name)
{
    int32 base, step;
    {
        DCAMPROP_ATTR attr = {
            .cbSize = sizeof(attr),
            .iProp = prop_id,
        };
        DCAM(dcamprop_getattr(h, &attr));
        base = attr.iProp_ArrayBase;
        step = attr.iPropStep_Element;
    }
    CHECK(prop_write_i32(h, base + index * step, value, prop_name));
    return 1;
Error:
    LOG("Failed to write %s[%d]", prop_name, index);
    return 0;
}

int
array_prop_write_i32(HDCAM h,
                     int32_t prop_id,
                     uint32_t index,
                     int32_t value,
                     const char* prop_name)
{
    return array_prop_rw_i32(h, prop_id, index, &value, prop_name);
}

int
array_prop_read_f64(HDCAM h,
                    int32_t prop_id,
                    uint32_t index,
                    double* out,
                    const char* prop_name)
{
    int32 base, step;
    {
        DCAMPROP_ATTR attr = {
            .cbSize = sizeof(attr),
            .iProp = prop_id,
        };
        DCAM(dcamprop_getattr(h, &attr));
        base = attr.iProp_ArrayBase;
        step = attr.iPropStep_Element;
    }
    CHECK(prop_read_f64(h, base + index * step, out, prop_name));
    return 1;
Error:
    LOG("Failed to read %s[%d]", prop_name, index);
    return 0;
}

int
array_prop_write_f64(HDCAM h,
                     int32_t prop_id,
                     uint32_t index,
                     double value,
                     const char* prop_name)
{
    int32 base, step;
    {
        DCAMPROP_ATTR attr = {
            .cbSize = sizeof(attr),
            .iProp = prop_id,
        };
        DCAM(dcamprop_getattr(h, &attr));
        base = attr.iProp_ArrayBase;
        step = attr.iPropStep_Element;
    }
    CHECK(prop_write_f64(h, base + index * step, value, prop_name));
    return 1;
Error:
    LOG("Failed to write %s[%d]", prop_name, index);
    return 0;
}
