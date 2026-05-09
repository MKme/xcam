#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "motion_alert.h"

#define CHECK(expr)                                                                 \
  do                                                                                \
  {                                                                                 \
    if (!(expr))                                                                    \
    {                                                                               \
      fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #expr);    \
      return 1;                                                                     \
    }                                                                               \
  } while (0)

static uint32_t read_u32_be(const uint8_t *p)
{
  return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

static int32_t read_i32_be(const uint8_t *p)
{
  return (int32_t)read_u32_be(p);
}

static int16_t read_i16_be(const uint8_t *p)
{
  return (int16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static int test_base64url()
{
  char out[16];
  size_t out_len = 0;

  CHECK(xcam_base64url_encode((const uint8_t *)"", 0, out, sizeof(out), &out_len));
  CHECK(strcmp(out, "") == 0);
  CHECK(out_len == 0);

  CHECK(xcam_base64url_encode((const uint8_t *)"f", 1, out, sizeof(out), &out_len));
  CHECK(strcmp(out, "Zg") == 0);
  CHECK(out_len == 2);

  CHECK(xcam_base64url_encode((const uint8_t *)"fo", 2, out, sizeof(out), &out_len));
  CHECK(strcmp(out, "Zm8") == 0);
  CHECK(out_len == 3);

  CHECK(xcam_base64url_encode((const uint8_t *)"foo", 3, out, sizeof(out), &out_len));
  CHECK(strcmp(out, "Zm9v") == 0);
  CHECK(out_len == 4);

  const uint8_t bytes[] = {0xff, 0xee, 0xdd};
  CHECK(xcam_base64url_encode(bytes, sizeof(bytes), out, sizeof(out), &out_len));
  CHECK(strcmp(out, "_-7d") == 0);
  CHECK(out_len == 4);

  CHECK(!xcam_base64url_encode((const uint8_t *)"foo", 3, out, 4, &out_len));
  return 0;
}

static int test_sentinel_payload()
{
  uint8_t payload[80];
  size_t payload_len = 0;
  CHECK(xcam_encode_sentinel_payload(
      0xdeadbeefUL,
      0x01020304UL,
      1234567,
      -7654321,
      true,
      "  XCAM-MOTION  ",
      XCAM_SENTINEL_SENSOR_RCWL_MOTION,
      1,
      payload,
      sizeof(payload),
      &payload_len));

  CHECK(payload_len == 19 + 1 + strlen("XCAM-MOTION") + 3);
  CHECK(payload[0] == XCAM_SENTINEL_VERSION);
  CHECK(payload[1] == 1);
  CHECK(read_u32_be(payload + 2) == 0x01020304UL);
  CHECK(read_i32_be(payload + 6) == 1234567);
  CHECK(read_i32_be(payload + 10) == -7654321);
  CHECK(read_u32_be(payload + 14) == 0xdeadbeefUL);
  CHECK(payload[18] == (XCAM_SENTINEL_FLAG_ALERT | XCAM_SENTINEL_FLAG_HAS_LABEL));

  size_t offset = 19;
  CHECK(payload[offset++] == strlen("XCAM-MOTION"));
  CHECK(memcmp(payload + offset, "XCAM-MOTION", strlen("XCAM-MOTION")) == 0);
  offset += strlen("XCAM-MOTION");
  CHECK(payload[offset++] == XCAM_SENTINEL_SENSOR_RCWL_MOTION);
  CHECK(read_i16_be(payload + offset) == 1);

  uint8_t no_label[32];
  size_t no_label_len = 0;
  CHECK(xcam_encode_sentinel_payload(1, 2, 3, 4, false, "   ", 2, -1, no_label, sizeof(no_label), &no_label_len));
  CHECK(no_label_len == 22);
  CHECK(no_label[18] == 0);
  CHECK(no_label[19] == 2);
  CHECK(read_i16_be(no_label + 20) == -1);
  return 0;
}

static int test_node_id_parsing()
{
  uint32_t out = 0;
  CHECK(xcam_parse_node_id("", 0x12345678UL, &out));
  CHECK(out == 0x12345678UL);
  CHECK(xcam_parse_node_id("3735928559", 0, &out));
  CHECK(out == 0xdeadbeefUL);
  CHECK(xcam_parse_node_id("0xDEADBEEF", 0, &out));
  CHECK(out == 0xdeadbeefUL);
  CHECK(xcam_parse_node_id("!deadbeef", 0, &out));
  CHECK(out == 0xdeadbeefUL);
  CHECK(!xcam_parse_node_id("0x100000000", 0, &out));
  CHECK(!xcam_parse_node_id("12x", 0, &out));
  return 0;
}

static int test_motion_scoring()
{
  const uint8_t previous[] = {10, 10, 10, 10};
  const uint8_t current[] = {10, 30, 15, 40};
  CHECK(xcam_motion_changed_percent(previous, current, 4, 10) == 50);
  CHECK(xcam_motion_hit(50, 10));
  CHECK(!xcam_motion_hit(5, 10));

  const uint8_t bytes[] = {0, 10, 20, 30, 100, 110, 120, 130};
  uint8_t digest[4] = {};
  CHECK(xcam_segment_digest(bytes, sizeof(bytes), digest, 4));
  CHECK(digest[0] == 5);
  CHECK(digest[1] == 25);
  CHECK(digest[2] == 105);
  CHECK(digest[3] == 125);
  return 0;
}

int main()
{
  CHECK(test_base64url() == 0);
  CHECK(test_sentinel_payload() == 0);
  CHECK(test_node_id_parsing() == 0);
  CHECK(test_motion_scoring() == 0);
  puts("motion_alert_host_test passed");
  return 0;
}
