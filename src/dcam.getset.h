#ifndef H_ACQUIRE_DCAM_GETSET_V0
#define H_ACQUIRE_DCAM_GETSET_V0

#include <stddef.h>
#include <stdint.h>

#pragma pack(push)
#include <dcamapi4.h>
#pragma pack(pop)

int
prop_read_i32(HDCAM h, int32_t prop_id, int32_t* out, const char* prop_name);

int
prop_write_i32(HDCAM h, int32_t prop_id, int32_t* value, const char* prop_name);

int
prop_read_u32(HDCAM h, int32_t prop_id, uint32_t* out, const char* prop_name);

int
prop_write_u32(HDCAM h,
               int32_t prop_id,
               uint32_t* value,
               const char* prop_name);

int
prop_read_f32(HDCAM h,
              int32_t prop_id,
              float scale,
              float* out,
              const char* prop_name);

int
prop_write_f32(HDCAM h,
               int32_t prop_id,
               float scale,
               float* value,
               const char* prop_name);

int
prop_read_f64(HDCAM h, int32_t prop_id, double* out, const char* prop_name);

int
prop_write_f64(HDCAM h, int32_t prop_id, double v, const char* prop_name);

int
array_prop_read_i32(HDCAM h,
                    int32_t prop_id,
                    uint32_t index,
                    int32_t* out,
                    const char* prop_name);

int
array_prop_rw_i32(HDCAM h,
                  int32_t prop_id,
                  uint32_t index,
                  int32_t* value,
                  const char* prop_name);

int
array_prop_write_i32(HDCAM h,
                     int32_t prop_id,
                     uint32_t index,
                     int32_t value,
                     const char* prop_name);

int
array_prop_read_f64(HDCAM h,
                    int32_t prop_id,
                    uint32_t index,
                    double* out,
                    const char* prop_name);

int
array_prop_write_f64(HDCAM h,
                     int32_t prop_id,
                     uint32_t index,
                     double value,
                     const char* prop_name);

#endif // H_ACQUIRE_DCAM_GETSET_V0
