#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define XCAM_SENTINEL_VERSION 1
#define XCAM_SENTINEL_MAX_LABEL_BYTES 32
#define XCAM_SENTINEL_FLAG_ALERT 0x01
#define XCAM_SENTINEL_FLAG_HAS_LABEL 0x04
#define XCAM_SENTINEL_SENSOR_RCWL_MOTION 1
#define XCAM_SENTINEL_SENSOR_PIR_MOTION 2

static inline uint8_t xcam_clamp_u8(int32_t value)
{
  if (value < 0)
    return 0;
  if (value > 255)
    return 255;
  return (uint8_t)value;
}

static inline int16_t xcam_clamp_i16(int32_t value)
{
  if (value < -32768)
    return -32768;
  if (value > 32767)
    return 32767;
  return (int16_t)value;
}

static inline void xcam_write_u32_be(uint8_t *dst, uint32_t value)
{
  dst[0] = (uint8_t)((value >> 24) & 0xff);
  dst[1] = (uint8_t)((value >> 16) & 0xff);
  dst[2] = (uint8_t)((value >> 8) & 0xff);
  dst[3] = (uint8_t)(value & 0xff);
}

static inline void xcam_write_i32_be(uint8_t *dst, int32_t value)
{
  xcam_write_u32_be(dst, (uint32_t)value);
}

static inline void xcam_write_i16_be(uint8_t *dst, int16_t value)
{
  uint16_t encoded = (uint16_t)value;
  dst[0] = (uint8_t)((encoded >> 8) & 0xff);
  dst[1] = (uint8_t)(encoded & 0xff);
}

static inline bool xcam_is_space(char ch)
{
  return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

static inline size_t xcam_trimmed_label(const char *label, const char **start)
{
  if (start != NULL)
    *start = "";
  if (label == NULL)
    return 0;

  const char *begin = label;
  while (*begin != '\0' && xcam_is_space(*begin))
    begin++;

  const char *end = begin;
  while (*end != '\0')
    end++;
  while (end > begin && xcam_is_space(*(end - 1)))
    end--;

  size_t len = (size_t)(end - begin);
  if (len > XCAM_SENTINEL_MAX_LABEL_BYTES)
    len = XCAM_SENTINEL_MAX_LABEL_BYTES;
  if (start != NULL)
    *start = begin;
  return len;
}

static inline bool xcam_encode_sentinel_payload(
    uint32_t node_id,
    uint32_t unix_minutes,
    int32_t lat_e5,
    int32_t lon_e5,
    bool alert,
    const char *label,
    uint8_t sensor_type,
    int16_t sensor_value,
    uint8_t *out,
    size_t out_cap,
    size_t *out_len)
{
  if (out_len != NULL)
    *out_len = 0;

  const char *label_start = "";
  size_t label_len = xcam_trimmed_label(label, &label_start);
  const size_t total_len = 19 + (label_len > 0 ? 1 + label_len : 0) + 3;
  if (out == NULL || out_cap < total_len)
    return false;

  out[0] = XCAM_SENTINEL_VERSION;
  out[1] = 1;
  xcam_write_u32_be(out + 2, unix_minutes);
  xcam_write_i32_be(out + 6, lat_e5);
  xcam_write_i32_be(out + 10, lon_e5);
  xcam_write_u32_be(out + 14, node_id);
  out[18] = (uint8_t)((alert ? XCAM_SENTINEL_FLAG_ALERT : 0) |
                      (label_len > 0 ? XCAM_SENTINEL_FLAG_HAS_LABEL : 0));

  size_t offset = 19;
  if (label_len > 0)
  {
    out[offset++] = (uint8_t)label_len;
    memcpy(out + offset, label_start, label_len);
    offset += label_len;
  }

  out[offset++] = sensor_type;
  xcam_write_i16_be(out + offset, sensor_value);
  offset += 2;

  if (out_len != NULL)
    *out_len = offset;
  return true;
}

static inline size_t xcam_base64url_encoded_len(size_t len)
{
  size_t blocks = len / 3;
  size_t rem = len % 3;
  return blocks * 4 + (rem == 0 ? 0 : rem + 1);
}

static inline bool xcam_base64url_encode(
    const uint8_t *data,
    size_t len,
    char *out,
    size_t out_cap,
    size_t *out_len)
{
  static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

  if (out_len != NULL)
    *out_len = 0;
  if ((data == NULL && len > 0) || out == NULL)
    return false;

  const size_t encoded_len = xcam_base64url_encoded_len(len);
  if (out_cap <= encoded_len)
    return false;

  size_t i = 0;
  size_t j = 0;
  while (i + 3 <= len)
  {
    uint32_t n = ((uint32_t)data[i] << 16) | ((uint32_t)data[i + 1] << 8) | data[i + 2];
    out[j++] = alphabet[(n >> 18) & 0x3f];
    out[j++] = alphabet[(n >> 12) & 0x3f];
    out[j++] = alphabet[(n >> 6) & 0x3f];
    out[j++] = alphabet[n & 0x3f];
    i += 3;
  }

  if (len - i == 1)
  {
    uint32_t n = (uint32_t)data[i] << 16;
    out[j++] = alphabet[(n >> 18) & 0x3f];
    out[j++] = alphabet[(n >> 12) & 0x3f];
  }
  else if (len - i == 2)
  {
    uint32_t n = ((uint32_t)data[i] << 16) | ((uint32_t)data[i + 1] << 8);
    out[j++] = alphabet[(n >> 18) & 0x3f];
    out[j++] = alphabet[(n >> 12) & 0x3f];
    out[j++] = alphabet[(n >> 6) & 0x3f];
  }

  out[j] = '\0';
  if (out_len != NULL)
    *out_len = j;
  return true;
}

static inline bool xcam_parse_node_id(const char *text, uint32_t fallback, uint32_t *out)
{
  if (out == NULL)
    return false;

  if (text == NULL)
  {
    *out = fallback;
    return true;
  }

  const char *p = text;
  while (*p != '\0' && xcam_is_space(*p))
    p++;
  if (*p == '\0')
  {
    *out = fallback;
    return true;
  }

  int base = 10;
  if (*p == '!')
  {
    base = 16;
    p++;
  }
  else if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))
  {
    base = 16;
    p += 2;
  }

  uint64_t acc = 0;
  bool any = false;
  while (*p != '\0' && !xcam_is_space(*p))
  {
    uint8_t digit;
    if (*p >= '0' && *p <= '9')
      digit = (uint8_t)(*p - '0');
    else if (*p >= 'a' && *p <= 'f')
      digit = (uint8_t)(10 + *p - 'a');
    else if (*p >= 'A' && *p <= 'F')
      digit = (uint8_t)(10 + *p - 'A');
    else
      return false;

    if (digit >= base)
      return false;
    any = true;
    acc = acc * (uint64_t)base + digit;
    if (acc > 0xffffffffULL)
      return false;
    p++;
  }

  while (*p != '\0')
  {
    if (!xcam_is_space(*p))
      return false;
    p++;
  }

  if (!any)
    return false;

  *out = (uint32_t)acc;
  return true;
}

static inline uint8_t xcam_motion_changed_percent(
    const uint8_t *previous_cells,
    const uint8_t *current_cells,
    size_t cell_count,
    uint8_t delta_threshold)
{
  if (previous_cells == NULL || current_cells == NULL || cell_count == 0)
    return 0;

  size_t changed = 0;
  for (size_t i = 0; i < cell_count; i++)
  {
    int delta = (int)previous_cells[i] - (int)current_cells[i];
    if (delta < 0)
      delta = -delta;
    if ((uint8_t)delta >= delta_threshold)
      changed++;
  }

  return (uint8_t)((changed * 100 + cell_count / 2) / cell_count);
}

static inline bool xcam_motion_hit(uint8_t changed_percent, uint8_t area_threshold_percent)
{
  return changed_percent >= area_threshold_percent;
}

static inline bool xcam_segment_digest(
    const uint8_t *bytes,
    size_t len,
    uint8_t *cells,
    size_t cell_count)
{
  if (bytes == NULL || len == 0 || cells == NULL || cell_count == 0)
    return false;

  for (size_t i = 0; i < cell_count; i++)
  {
    size_t start = (i * len) / cell_count;
    size_t end = ((i + 1) * len) / cell_count;
    if (end <= start)
      end = start + 1;
    if (end > len)
      end = len;

    uint32_t sum = 0;
    size_t count = 0;
    for (size_t j = start; j < end; j++)
    {
      sum += bytes[j];
      count++;
    }
    cells[i] = count > 0 ? (uint8_t)(sum / count) : 0;
  }

  return true;
}
