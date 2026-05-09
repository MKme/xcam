#include <Arduino.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <esp_jpg_decode.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <soc/rtc_cntl_reg.h>
#include <driver/i2c.h>
#include <IotWebConf.h>
#include <IotWebConfTParameter.h>
#include <OV2640.h>
#include <ESPmDNS.h>
#include <rtsp_server.h>
#include <lookup_camera_effect.h>
#include <lookup_camera_frame_size.h>
#include <lookup_camera_gainceiling.h>
#include <lookup_camera_wb_mode.h>
#include <format_duration.h>
#include <format_number.h>
#include <moustache.h>
#include <motion_alert.h>
#include <settings.h>
#include <math.h>
#include <time.h>

// HTML files
extern const char index_html_min_start[] asm("_binary_html_index_min_html_start");

auto param_group_camera = iotwebconf::ParameterGroup("camera", "Camera settings");
auto param_frame_duration = iotwebconf::Builder<iotwebconf::UIntTParameter<unsigned long>>("fd").label("Frame duration (ms)").defaultValue(DEFAULT_FRAME_DURATION).min(10).build();
auto param_frame_size = iotwebconf::Builder<iotwebconf::SelectTParameter<sizeof(frame_sizes[0])>>("fs").label("Frame size").optionValues((const char *)&frame_sizes).optionNames((const char *)&frame_sizes).optionCount(sizeof(frame_sizes) / sizeof(frame_sizes[0])).nameLength(sizeof(frame_sizes[0])).defaultValue(DEFAULT_FRAME_SIZE).build();
auto param_jpg_quality = iotwebconf::Builder<iotwebconf::UIntTParameter<byte>>("q").label("JPG quality").defaultValue(DEFAULT_JPEG_QUALITY).min(1).max(100).build();
auto param_brightness = iotwebconf::Builder<iotwebconf::IntTParameter<int>>("b").label("Brightness").defaultValue(DEFAULT_BRIGHTNESS).min(-2).max(2).build();
auto param_contrast = iotwebconf::Builder<iotwebconf::IntTParameter<int>>("c").label("Contrast").defaultValue(DEFAULT_CONTRAST).min(-2).max(2).build();
auto param_saturation = iotwebconf::Builder<iotwebconf::IntTParameter<int>>("s").label("Saturation").defaultValue(DEFAULT_SATURATION).min(-2).max(2).build();
auto param_special_effect = iotwebconf::Builder<iotwebconf::SelectTParameter<sizeof(camera_effects[0])>>("e").label("Effect").optionValues((const char *)&camera_effects).optionNames((const char *)&camera_effects).optionCount(sizeof(camera_effects) / sizeof(camera_effects[0])).nameLength(sizeof(camera_effects[0])).defaultValue(DEFAULT_EFFECT).build();
auto param_whitebal = iotwebconf::Builder<iotwebconf::CheckboxTParameter>("wb").label("White balance").defaultValue(DEFAULT_WHITE_BALANCE).build();
auto param_awb_gain = iotwebconf::Builder<iotwebconf::CheckboxTParameter>("awbg").label("Automatic white balance gain").defaultValue(DEFAULT_WHITE_BALANCE_GAIN).build();
auto param_wb_mode = iotwebconf::Builder<iotwebconf::SelectTParameter<sizeof(camera_wb_modes[0])>>("wbm").label("White balance mode").optionValues((const char *)&camera_wb_modes).optionNames((const char *)&camera_wb_modes).optionCount(sizeof(camera_wb_modes) / sizeof(camera_wb_modes[0])).nameLength(sizeof(camera_wb_modes[0])).defaultValue(DEFAULT_WHITE_BALANCE_MODE).build();
auto param_exposure_ctrl = iotwebconf::Builder<iotwebconf::CheckboxTParameter>("ec").label("Exposure control").defaultValue(DEFAULT_EXPOSURE_CONTROL).build();
auto param_aec2 = iotwebconf::Builder<iotwebconf::CheckboxTParameter>("aec2").label("Auto exposure (dsp)").defaultValue(DEFAULT_AEC2).build();
auto param_ae_level = iotwebconf::Builder<iotwebconf::IntTParameter<int>>("ael").label("Auto Exposure level").defaultValue(DEFAULT_AE_LEVEL).min(-2).max(2).build();
auto param_aec_value = iotwebconf::Builder<iotwebconf::IntTParameter<int>>("aecv").label("Manual exposure value").defaultValue(DEFAULT_AEC_VALUE).min(9).max(1200).build();
auto param_gain_ctrl = iotwebconf::Builder<iotwebconf::CheckboxTParameter>("gc").label("Gain control").defaultValue(DEFAULT_GAIN_CONTROL).build();
auto param_agc_gain = iotwebconf::Builder<iotwebconf::IntTParameter<int>>("agcg").label("AGC gain").defaultValue(DEFAULT_AGC_GAIN).min(0).max(30).build();
auto param_gain_ceiling = iotwebconf::Builder<iotwebconf::SelectTParameter<sizeof(camera_gain_ceilings[0])>>("gcl").label("Auto Gain ceiling").optionValues((const char *)&camera_gain_ceilings).optionNames((const char *)&camera_gain_ceilings).optionCount(sizeof(camera_gain_ceilings) / sizeof(camera_gain_ceilings[0])).nameLength(sizeof(camera_gain_ceilings[0])).defaultValue(DEFAULT_GAIN_CEILING).build();
auto param_bpc = iotwebconf::Builder<iotwebconf::CheckboxTParameter>("bpc").label("Black pixel correct").defaultValue(DEFAULT_BPC).build();
auto param_wpc = iotwebconf::Builder<iotwebconf::CheckboxTParameter>("wpc").label("White pixel correct").defaultValue(DEFAULT_WPC).build();
auto param_raw_gma = iotwebconf::Builder<iotwebconf::CheckboxTParameter>("rg").label("Gamma correct").defaultValue(DEFAULT_RAW_GAMMA).build();
auto param_lenc = iotwebconf::Builder<iotwebconf::CheckboxTParameter>("lenc").label("Lens correction").defaultValue(DEFAULT_LENC).build();
auto param_hmirror = iotwebconf::Builder<iotwebconf::CheckboxTParameter>("hm").label("Horizontal mirror").defaultValue(DEFAULT_HORIZONTAL_MIRROR).build();
auto param_vflip = iotwebconf::Builder<iotwebconf::CheckboxTParameter>("vm").label("Vertical mirror").defaultValue(DEFAULT_VERTICAL_MIRROR).build();
auto param_dcw = iotwebconf::Builder<iotwebconf::CheckboxTParameter>("dcw").label("Downsize enable").defaultValue(DEFAULT_DCW).build();
auto param_colorbar = iotwebconf::Builder<iotwebconf::CheckboxTParameter>("cb").label("Colorbar").defaultValue(DEFAULT_COLORBAR).build();

auto param_group_motion = iotwebconf::ParameterGroup("motion", "Motion alert settings");
auto param_motion_enabled = iotwebconf::Builder<iotwebconf::CheckboxTParameter>("me").label("Enable camera motion alerts").defaultValue(DEFAULT_MOTION_ENABLED).build();
auto param_motion_bridge_url = iotwebconf::Builder<iotwebconf::TextTParameter<128>>("mbu").label("XTOC MANET bridge URL").defaultValue(DEFAULT_MOTION_BRIDGE_URL).build();
auto param_motion_label = iotwebconf::Builder<iotwebconf::TextTParameter<48>>("ml").label("Sentinel label").defaultValue(DEFAULT_MOTION_LABEL).build();
auto param_motion_node_id = iotwebconf::Builder<iotwebconf::TextTParameter<24>>("mnid").label("Sentinel node ID").defaultValue(DEFAULT_MOTION_NODE_ID).build();
auto param_motion_latitude = iotwebconf::Builder<iotwebconf::FloatTParameter>("mlat").label("Sentinel latitude").defaultValue(DEFAULT_MOTION_LATITUDE).min(-90.0f).max(90.0f).step(0.00001f).build();
auto param_motion_longitude = iotwebconf::Builder<iotwebconf::FloatTParameter>("mlon").label("Sentinel longitude").defaultValue(DEFAULT_MOTION_LONGITUDE).min(-180.0f).max(180.0f).step(0.00001f).build();
auto param_motion_sensor_type = iotwebconf::Builder<iotwebconf::UIntTParameter<byte>>("mst").label("Sentinel motion sensor type").defaultValue(DEFAULT_MOTION_SENSOR_TYPE).min(1).max(2).build();
auto param_motion_sample_interval = iotwebconf::Builder<iotwebconf::UIntTParameter<unsigned long>>("msi").label("Motion sample interval (ms)").defaultValue(DEFAULT_MOTION_SAMPLE_INTERVAL).min(250).max(10000).build();
auto param_motion_pixel_delta = iotwebconf::Builder<iotwebconf::UIntTParameter<byte>>("mdt").label("Motion pixel delta").defaultValue(DEFAULT_MOTION_PIXEL_DELTA).min(1).max(128).build();
auto param_motion_area_threshold = iotwebconf::Builder<iotwebconf::UIntTParameter<byte>>("mat").label("Motion area threshold (%)").defaultValue(DEFAULT_MOTION_AREA_THRESHOLD).min(1).max(100).build();
auto param_motion_consecutive_hits = iotwebconf::Builder<iotwebconf::UIntTParameter<byte>>("mch").label("Motion consecutive hits").defaultValue(DEFAULT_MOTION_CONSECUTIVE_HITS).min(1).max(10).build();
auto param_motion_cooldown = iotwebconf::Builder<iotwebconf::UIntTParameter<unsigned long>>("mcd").label("Motion alert cooldown (ms)").defaultValue(DEFAULT_MOTION_COOLDOWN).min(1000).max(3600000).build();
auto param_motion_http_timeout = iotwebconf::Builder<iotwebconf::UIntTParameter<unsigned long>>("mto").label("Motion bridge timeout (ms)").defaultValue(DEFAULT_MOTION_HTTP_TIMEOUT).min(250).max(10000).build();

// Camera
OV2640 cam;
// DNS Server
DNSServer dnsServer;
// RTSP Server
std::unique_ptr<rtsp_server> camera_server;
// Web server
WebServer web_server(80);

String build_default_thing_name()
{
  char thing_name[32];
  snprintf(
      thing_name,
      sizeof(thing_name),
      "%s-%06lx",
      WIFI_SSID,
      static_cast<unsigned long>(ESP.getEfuseMac() & 0xFFFFFF));
  return String(thing_name);
}

String get_device_hostname(const char *thing_name)
{
  const auto hostname = WiFi.getHostname();
  if (hostname != nullptr && hostname[0] != '\0')
    return String(hostname);

  auto fallback = String(thing_name);
  fallback.toLowerCase();
  fallback.replace(" ", "-");
  return fallback;
}

String build_mdns_name(const String &hostname)
{
  return hostname + ".local";
}

String build_http_url(const String &hostname, const char *path)
{
  return "http://" + build_mdns_name(hostname) + path;
}

String build_rtsp_url(const String &hostname)
{
  return "rtsp://" + build_mdns_name(hostname) + ":" + String(RTSP_PORT) + "/mjpeg/1";
}

String build_http_url(const IPAddress &ipv4, const char *path)
{
  return "http://" + ipv4.toString() + path;
}

String build_rtsp_url(const IPAddress &ipv4)
{
  return "rtsp://" + ipv4.toString() + ":" + String(RTSP_PORT) + "/mjpeg/1";
}

auto thingName = build_default_thing_name();
IotWebConf iotWebConf(thingName.c_str(), &dnsServer, &web_server, WIFI_PASSWORD, CONFIG_VERSION);

// Camera initialization result
esp_err_t camera_init_result;

static const size_t MOTION_GRID_COLS = 8;
static const size_t MOTION_GRID_ROWS = 8;
static const size_t MOTION_DIGEST_CELLS = MOTION_GRID_COLS * MOTION_GRID_ROWS;
static const uint32_t MOTION_FALLBACK_EPOCH_SECONDS = 1704067200UL;

struct MotionState
{
  bool have_baseline = false;
  uint8_t baseline[MOTION_DIGEST_CELLS] = {};
  uint8_t last_score = 0;
  uint8_t consecutive_hits = 0;
  unsigned long next_sample_ms = 0;
  unsigned long last_sample_ms = 0;
  unsigned long last_alert_ms = 0;
  uint32_t sample_count = 0;
  uint32_t alert_count = 0;
  bool last_send_ok = false;
  int last_send_code = 0;
  String last_error = "Idle";
  String last_packet = "";
};

MotionState motion_state;

template <size_t len>
void apply_text_default(iotwebconf::TextTParameter<len> &param, const char *default_value)
{
  strncpy(param.value(), default_value, len);
  param.value()[len - 1] = '\0';
}

template <size_t len>
bool sanitize_text_parameter(iotwebconf::TextTParameter<len> &param, const char *default_value)
{
  bool found_null = false;
  bool valid = true;
  for (size_t i = 0; i < len; i++)
  {
    const unsigned char ch = static_cast<unsigned char>(param.value()[i]);
    if (ch == '\0')
    {
      found_null = true;
      break;
    }
    if (ch < 32 || ch == 0xff)
    {
      valid = false;
      break;
    }
  }

  if (!found_null || !valid)
  {
    apply_text_default(param, default_value);
    return false;
  }

  param.value()[len - 1] = '\0';
  return true;
}

template <typename T>
void sanitize_range(T &value, T minimum, T maximum, T fallback)
{
  if (value < minimum || value > maximum)
    value = fallback;
}

void sanitize_motion_settings()
{
  const bool bridge_text_was_valid = sanitize_text_parameter(param_motion_bridge_url, DEFAULT_MOTION_BRIDGE_URL);
  sanitize_text_parameter(param_motion_label, DEFAULT_MOTION_LABEL);
  sanitize_text_parameter(param_motion_node_id, DEFAULT_MOTION_NODE_ID);

  if (!isfinite(param_motion_latitude.value()) || param_motion_latitude.value() < -90.0f || param_motion_latitude.value() > 90.0f)
    param_motion_latitude.value() = DEFAULT_MOTION_LATITUDE;
  if (!isfinite(param_motion_longitude.value()) || param_motion_longitude.value() < -180.0f || param_motion_longitude.value() > 180.0f)
    param_motion_longitude.value() = DEFAULT_MOTION_LONGITUDE;

  sanitize_range<byte>(param_motion_sensor_type.value(), 1, 2, DEFAULT_MOTION_SENSOR_TYPE);
  sanitize_range<unsigned long>(param_motion_sample_interval.value(), 250UL, 10000UL, DEFAULT_MOTION_SAMPLE_INTERVAL);
  sanitize_range<byte>(param_motion_pixel_delta.value(), 1, 128, DEFAULT_MOTION_PIXEL_DELTA);
  sanitize_range<byte>(param_motion_area_threshold.value(), 1, 100, DEFAULT_MOTION_AREA_THRESHOLD);
  sanitize_range<byte>(param_motion_consecutive_hits.value(), 1, 10, DEFAULT_MOTION_CONSECUTIVE_HITS);
  sanitize_range<unsigned long>(param_motion_cooldown.value(), 1000UL, 3600000UL, DEFAULT_MOTION_COOLDOWN);
  sanitize_range<unsigned long>(param_motion_http_timeout.value(), 250UL, 10000UL, DEFAULT_MOTION_HTTP_TIMEOUT);

  if (!bridge_text_was_valid || strlen(param_motion_bridge_url.value()) == 0)
    param_motion_enabled.value() = DEFAULT_MOTION_ENABLED;
}

void reset_motion_detector()
{
  motion_state.have_baseline = false;
  motion_state.consecutive_hits = 0;
  motion_state.next_sample_ms = 0;
  motion_state.last_score = 0;
}

String format_hex32(uint32_t value)
{
  char buf[11];
  snprintf(buf, sizeof(buf), "0x%08lX", static_cast<unsigned long>(value));
  return String(buf);
}

uint32_t default_motion_node_id()
{
  return static_cast<uint32_t>(ESP.getEfuseMac() & 0xffffffffUL);
}

String motion_client_id()
{
  char buf[24];
  snprintf(buf, sizeof(buf), "xcam-%08lX", static_cast<unsigned long>(default_motion_node_id()));
  return String(buf);
}

String motion_label()
{
  String label(param_motion_label.value());
  label.trim();
  if (label.length() == 0)
    label = iotWebConf.getThingName();
  if (label.length() > XCAM_SENTINEL_MAX_LABEL_BYTES)
    label = label.substring(0, XCAM_SENTINEL_MAX_LABEL_BYTES);
  return label;
}

String normalized_motion_bridge_url()
{
  String url(param_motion_bridge_url.value());
  url.trim();
  while (url.endsWith("/"))
    url.remove(url.length() - 1);
  if (url.length() > 0 && !url.endsWith("/send"))
    url += "/send";
  return url;
}

bool motion_bridge_url_is_usable(const String &url)
{
  return url.startsWith("http://");
}

String json_escape(const String &value)
{
  String out;
  out.reserve(value.length() + 8);
  const char hex[] = "0123456789abcdef";
  for (size_t i = 0; i < value.length(); i++)
  {
    const unsigned char ch = static_cast<unsigned char>(value[i]);
    if (ch == '"' || ch == '\\')
    {
      out += '\\';
      out += static_cast<char>(ch);
    }
    else if (ch == '\b')
      out += "\\b";
    else if (ch == '\f')
      out += "\\f";
    else if (ch == '\n')
      out += "\\n";
    else if (ch == '\r')
      out += "\\r";
    else if (ch == '\t')
      out += "\\t";
    else if (ch < 0x20)
    {
      out += "\\u00";
      out += hex[(ch >> 4) & 0x0f];
      out += hex[ch & 0x0f];
    }
    else
      out += static_cast<char>(ch);
  }
  return out;
}

String build_motion_send_body(const String &packet)
{
  const String client_id = motion_client_id();
  const String label = motion_label();

  String body;
  body.reserve(packet.length() + client_id.length() + label.length() + 96);
  body += "{\"client\":{\"id\":\"";
  body += json_escape(client_id);
  body += "\",\"app\":\"xcom\",\"role\":\"client\",\"label\":\"";
  body += json_escape(label);
  body += "\"},\"kind\":\"packet\",\"text\":\"";
  body += json_escape(packet);
  body += "\"}";
  return body;
}

String generate_xtoc_packet_id()
{
  static const char alphabet[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
  char out[9];
  for (size_t i = 0; i < 8; i++)
    out[i] = alphabet[random(0, sizeof(alphabet) - 1)];
  out[8] = '\0';
  return String(out);
}

int month_index(const char *month)
{
  static const char names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
  for (int i = 0; i < 12; i++)
  {
    if (strncmp(month, names + i * 3, 3) == 0)
      return i + 1;
  }
  return 1;
}

int64_t days_from_civil(int year, unsigned month, unsigned day)
{
  year -= month <= 2;
  const int era = (year >= 0 ? year : year - 399) / 400;
  const unsigned yoe = static_cast<unsigned>(year - era * 400);
  const unsigned doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return static_cast<int64_t>(era) * 146097 + static_cast<int64_t>(doe) - 719468;
}

uint32_t build_epoch_seconds()
{
  char month_text[4] = {};
  int day = 1;
  int year = 2024;
  int hour = 0;
  int minute = 0;
  int second = 0;
  sscanf(__DATE__, "%3s %d %d", month_text, &day, &year);
  sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second);

  const int month = month_index(month_text);
  const int64_t days = days_from_civil(year, static_cast<unsigned>(month), static_cast<unsigned>(day));
  const int64_t seconds_since_epoch = days * 86400 + hour * 3600 + minute * 60 + second;
  if (seconds_since_epoch <= 0 || seconds_since_epoch > 0xffffffffLL)
    return MOTION_FALLBACK_EPOCH_SECONDS;
  return static_cast<uint32_t>(seconds_since_epoch);
}

uint32_t motion_unix_minutes()
{
  const time_t now = time(nullptr);
  if (now > 1609459200)
    return static_cast<uint32_t>(now / 60);

  const uint64_t estimated = static_cast<uint64_t>(build_epoch_seconds()) + millis() / 1000UL;
  return static_cast<uint32_t>(estimated / 60ULL);
}

struct MotionJpegDigestContext
{
  const uint8_t *jpg = nullptr;
  size_t jpg_len = 0;
  uint16_t scaled_width = 1;
  uint16_t scaled_height = 1;
  uint32_t sums[MOTION_DIGEST_CELLS] = {};
  uint16_t counts[MOTION_DIGEST_CELLS] = {};
};

size_t motion_jpg_reader(void *arg, size_t index, uint8_t *buf, size_t len)
{
  MotionJpegDigestContext *ctx = static_cast<MotionJpegDigestContext *>(arg);
  if (ctx == nullptr || ctx->jpg == nullptr || buf == nullptr || index >= ctx->jpg_len)
    return 0;

  const size_t remaining = ctx->jpg_len - index;
  const size_t to_copy = len < remaining ? len : remaining;
  memcpy(buf, ctx->jpg + index, to_copy);
  return to_copy;
}

bool motion_jpg_writer(void *arg, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t *data)
{
  MotionJpegDigestContext *ctx = static_cast<MotionJpegDigestContext *>(arg);
  if (ctx == nullptr || data == nullptr || w == 0 || h == 0)
    return true;

  for (uint16_t yy = 0; yy < h; yy++)
  {
    const uint16_t py = y + yy;
    uint8_t row = static_cast<uint8_t>((static_cast<uint32_t>(py) * MOTION_GRID_ROWS) / ctx->scaled_height);
    if (row >= MOTION_GRID_ROWS)
      row = MOTION_GRID_ROWS - 1;

    for (uint16_t xx = 0; xx < w; xx++)
    {
      const uint16_t px = x + xx;
      uint8_t col = static_cast<uint8_t>((static_cast<uint32_t>(px) * MOTION_GRID_COLS) / ctx->scaled_width);
      if (col >= MOTION_GRID_COLS)
        col = MOTION_GRID_COLS - 1;

      const size_t pixel = (static_cast<size_t>(yy) * w + xx) * 3;
      const uint8_t r = data[pixel];
      const uint8_t g = data[pixel + 1];
      const uint8_t b = data[pixel + 2];
      const uint8_t luma = static_cast<uint8_t>(((uint16_t)r * 77 + (uint16_t)g * 150 + (uint16_t)b * 29) >> 8);
      const size_t cell = static_cast<size_t>(row) * MOTION_GRID_COLS + col;
      ctx->sums[cell] += luma;
      if (ctx->counts[cell] < 0xffff)
        ctx->counts[cell]++;
    }
  }

  return true;
}

bool compute_motion_digest_from_jpeg(const uint8_t *jpg, size_t jpg_len, int width, int height, uint8_t *out)
{
  if (jpg == nullptr || jpg_len == 0 || out == nullptr)
    return false;

  MotionJpegDigestContext ctx;
  ctx.jpg = jpg;
  ctx.jpg_len = jpg_len;
  ctx.scaled_width = static_cast<uint16_t>(max(1, (width + 7) / 8));
  ctx.scaled_height = static_cast<uint16_t>(max(1, (height + 7) / 8));

  const esp_err_t result = esp_jpg_decode(jpg_len, JPG_SCALE_8X, motion_jpg_reader, motion_jpg_writer, &ctx);
  if (result != ESP_OK)
    return false;

  bool has_samples = false;
  for (size_t i = 0; i < MOTION_DIGEST_CELLS; i++)
  {
    if (ctx.counts[i] > 0)
    {
      out[i] = static_cast<uint8_t>(ctx.sums[i] / ctx.counts[i]);
      has_samples = true;
    }
    else
      out[i] = 0;
  }

  return has_samples;
}

bool compute_motion_digest(const uint8_t *jpg, size_t jpg_len, int width, int height, uint8_t *out)
{
  if (compute_motion_digest_from_jpeg(jpg, jpg_len, width, height, out))
    return true;
  return xcam_segment_digest(jpg, jpg_len, out, MOTION_DIGEST_CELLS);
}

bool build_motion_packet(String &packet)
{
  uint32_t node_id = 0;
  if (!xcam_parse_node_id(param_motion_node_id.value(), default_motion_node_id(), &node_id))
  {
    motion_state.last_error = "Invalid Sentinel node ID";
    return false;
  }

  const int32_t lat_e5 = static_cast<int32_t>(lroundf(param_motion_latitude.value() * 100000.0f));
  const int32_t lon_e5 = static_cast<int32_t>(lroundf(param_motion_longitude.value() * 100000.0f));
  const uint8_t sensor_type = xcam_clamp_u8(param_motion_sensor_type.value());
  const String label = motion_label();

  uint8_t payload[19 + 1 + XCAM_SENTINEL_MAX_LABEL_BYTES + 3];
  size_t payload_len = 0;
  if (!xcam_encode_sentinel_payload(
          node_id,
          motion_unix_minutes(),
          lat_e5,
          lon_e5,
          true,
          label.c_str(),
          sensor_type,
          1,
          payload,
          sizeof(payload),
          &payload_len))
  {
    motion_state.last_error = "Unable to encode Sentinel payload";
    return false;
  }

  char encoded[80];
  size_t encoded_len = 0;
  if (!xcam_base64url_encode(payload, payload_len, encoded, sizeof(encoded), &encoded_len))
  {
    motion_state.last_error = "Unable to encode Sentinel payload text";
    return false;
  }

  packet = "X1.11.C.";
  packet += generate_xtoc_packet_id();
  packet += ".1/1.";
  packet += encoded;
  return true;
}

bool send_motion_packet(const String &packet)
{
  const String url = normalized_motion_bridge_url();
  if (url.length() == 0)
  {
    motion_state.last_error = "Bridge URL not configured";
    return false;
  }
  if (!motion_bridge_url_is_usable(url))
  {
    motion_state.last_error = "Bridge URL must start with http://";
    return false;
  }

  WiFiClient client;
  HTTPClient http;
  http.setTimeout(param_motion_http_timeout.value());
  if (!http.begin(client, url))
  {
    motion_state.last_error = "Unable to open bridge URL";
    return false;
  }

  http.addHeader("Content-Type", "application/json");
  const int code = http.POST(build_motion_send_body(packet));
  http.end();

  motion_state.last_send_code = code;
  motion_state.last_send_ok = code >= 200 && code < 300;
  if (!motion_state.last_send_ok)
  {
    motion_state.last_error = String("Bridge POST failed: ") + code;
    return false;
  }

  motion_state.last_error = "";
  return true;
}

String motion_age(unsigned long since_ms)
{
  if (since_ms == 0)
    return "Never";
  return format_duration((millis() - since_ms) / 1000);
}

String motion_send_status()
{
  if (motion_state.last_send_code == 0)
    return motion_state.last_error.length() > 0 ? motion_state.last_error : "Not sent";
  return String(motion_state.last_send_ok ? "OK " : "Failed ") + motion_state.last_send_code;
}

void motion_detector_loop()
{
  if (!param_motion_enabled.value())
    return;

  const String bridge_url = normalized_motion_bridge_url();
  if (bridge_url.length() == 0)
  {
    motion_state.last_error = "Bridge URL not configured";
    return;
  }
  if (!motion_bridge_url_is_usable(bridge_url))
  {
    motion_state.last_error = "Bridge URL must start with http://";
    return;
  }
  if (camera_init_result != ESP_OK)
  {
    motion_state.last_error = "Camera is not initialized";
    return;
  }
  if (iotWebConf.getState() != iotwebconf::NetworkState::OnLine)
  {
    motion_state.last_error = "Network is offline";
    return;
  }

  const unsigned long now = millis();
  if (motion_state.next_sample_ms != 0 && static_cast<long>(now - motion_state.next_sample_ms) < 0)
    return;
  motion_state.next_sample_ms = now + param_motion_sample_interval.value();

  cam.run();
  const uint8_t *frame = cam.getfb();
  const size_t frame_len = cam.getSize();
  if (frame == nullptr || frame_len == 0)
  {
    motion_state.last_error = "Unable to obtain camera frame";
    return;
  }

  uint8_t digest[MOTION_DIGEST_CELLS] = {};
  if (!compute_motion_digest(frame, frame_len, cam.getWidth(), cam.getHeight(), digest))
  {
    motion_state.last_error = "Unable to compute motion digest";
    return;
  }

  motion_state.sample_count++;
  motion_state.last_sample_ms = now;

  if (!motion_state.have_baseline)
  {
    memcpy(motion_state.baseline, digest, MOTION_DIGEST_CELLS);
    motion_state.have_baseline = true;
    motion_state.last_error = "";
    return;
  }

  motion_state.last_score = xcam_motion_changed_percent(
      motion_state.baseline,
      digest,
      MOTION_DIGEST_CELLS,
      param_motion_pixel_delta.value());
  memcpy(motion_state.baseline, digest, MOTION_DIGEST_CELLS);

  if (xcam_motion_hit(motion_state.last_score, param_motion_area_threshold.value()))
  {
    if (motion_state.consecutive_hits < 255)
      motion_state.consecutive_hits++;
  }
  else
  {
    motion_state.consecutive_hits = 0;
  }

  if (motion_state.consecutive_hits < param_motion_consecutive_hits.value())
    return;

  if (motion_state.last_alert_ms != 0 && now - motion_state.last_alert_ms < param_motion_cooldown.value())
    return;

  String packet;
  if (!build_motion_packet(packet))
    return;

  motion_state.last_packet = packet;
  motion_state.last_alert_ms = now;
  motion_state.alert_count++;
  motion_state.consecutive_hits = 0;
  send_motion_packet(packet);
}

void handle_root()
{
  log_v("Handle root");
  // Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
    return;

  // Wifi Modes
  const char *wifi_modes[] = {"NULL", "STA", "AP", "STA+AP"};
  auto ipv4 = WiFi.getMode() == WIFI_MODE_AP ? WiFi.softAPIP() : WiFi.localIP();
  auto ipv6 = WiFi.getMode() == WIFI_MODE_AP ? WiFi.softAPIPv6() : WiFi.localIPv6();
  auto hostname = get_device_hostname(iotWebConf.getThingName());
  auto mdns_name = build_mdns_name(hostname);
  auto access_point = WiFi.getMode() == WIFI_MODE_AP ? WiFi.softAPSSID() : WiFi.SSID();

  auto initResult = esp_err_to_name(camera_init_result);
  if (initResult == nullptr)
    initResult = "Unknown reason";

  moustache_variable_t substitutions[] = {
      // Version / CPU
      {"AppTitle", APP_TITLE},
      {"AppVersion", APP_VERSION},
      {"BoardType", BOARD_NAME},
      {"ThingName", iotWebConf.getThingName()},
      {"SDKVersion", ESP.getSdkVersion()},
      {"ChipModel", ESP.getChipModel()},
      {"ChipRevision", String(ESP.getChipRevision())},
      {"CpuFreqMHz", String(ESP.getCpuFreqMHz())},
      {"CpuCores", String(ESP.getChipCores())},
      {"FlashSize", format_memory(ESP.getFlashChipSize(), 0)},
      {"HeapSize", format_memory(ESP.getHeapSize())},
      {"PsRamSize", format_memory(ESP.getPsramSize(), 0)},
      // Diagnostics
      {"Uptime", String(format_duration(millis() / 1000))},
      {"FreeHeap", format_memory(ESP.getFreeHeap())},
      {"MaxAllocHeap", format_memory(ESP.getMaxAllocHeap())},
      {"NumRTSPSessions", camera_server != nullptr ? String(camera_server->num_connected()) : "RTSP server disabled"},
      // Network
      {"HostName", hostname},
      {"MdnsName", mdns_name},
      {"MacAddress", WiFi.macAddress()},
      {"AccessPoint", access_point},
      {"SignalStrength", String(WiFi.RSSI())},
      {"WifiMode", wifi_modes[WiFi.getMode()]},
      {"IPv4", ipv4.toString()},
      {"IPv6", ipv6.toString()},
      {"NetworkState.ApMode", String(iotWebConf.getState() == iotwebconf::NetworkState::ApMode)},
      {"NetworkState.OnLine", String(iotWebConf.getState() == iotwebconf::NetworkState::OnLine)},
      {"HttpUrlMdns", build_http_url(hostname, "/")},
      {"HttpUrlIpv4", build_http_url(ipv4, "/")},
      {"RtspUrlMdns", build_rtsp_url(hostname)},
      {"RtspUrlIpv4", build_rtsp_url(ipv4)},
      {"StreamUrlMdns", build_http_url(hostname, "/stream")},
      {"StreamUrlIpv4", build_http_url(ipv4, "/stream")},
      {"SnapshotUrlMdns", build_http_url(hostname, "/snapshot")},
      {"SnapshotUrlIpv4", build_http_url(ipv4, "/snapshot")},
      {"RestartUrlMdns", build_http_url(hostname, "/restart")},
      {"RestartUrlIpv4", build_http_url(ipv4, "/restart")},
      // Camera
      {"FrameSize", String(param_frame_size.value())},
      {"FrameDuration", String(param_frame_duration.value())},
      {"FrameFrequency", String(1000.0 / param_frame_duration.value(), 1)},
      {"JpegQuality", String(param_jpg_quality.value())},
      {"CameraInitialized", String(camera_init_result == ESP_OK)},
      {"CameraInitResult", String(camera_init_result)},
      {"CameraInitResultText", initResult},
      // Settings
      {"Brightness", String(param_brightness.value())},
      {"Contrast", String(param_contrast.value())},
      {"Saturation", String(param_saturation.value())},
      {"SpecialEffect", String(param_special_effect.value())},
      {"WhiteBal", String(param_whitebal.value())},
      {"AwbGain", String(param_awb_gain.value())},
      {"WbMode", String(param_wb_mode.value())},
      {"ExposureCtrl", String(param_exposure_ctrl.value())},
      {"Aec2", String(param_aec2.value())},
      {"AeLevel", String(param_ae_level.value())},
      {"AecValue", String(param_aec_value.value())},
      {"GainCtrl", String(param_gain_ctrl.value())},
      {"AgcGain", String(param_agc_gain.value())},
      {"GainCeiling", String(param_gain_ceiling.value())},
      {"Bpc", String(param_bpc.value())},
      {"Wpc", String(param_wpc.value())},
      {"RawGma", String(param_raw_gma.value())},
      {"Lenc", String(param_lenc.value())},
      {"HMirror", String(param_hmirror.value())},
      {"VFlip", String(param_vflip.value())},
      {"Dcw", String(param_dcw.value())},
      {"ColorBar", String(param_colorbar.value())},
      // Motion alerts
      {"MotionEnabled", String(param_motion_enabled.value())},
      {"MotionBridgeUrl", normalized_motion_bridge_url().length() > 0 ? normalized_motion_bridge_url() : "Not configured"},
      {"MotionBridgeUsable", String(motion_bridge_url_is_usable(normalized_motion_bridge_url()))},
      {"MotionNeedsBridge", String(param_motion_enabled.value() && !motion_bridge_url_is_usable(normalized_motion_bridge_url()))},
      {"MotionLabel", motion_label()},
      {"MotionNodeId", format_hex32(default_motion_node_id())},
      {"MotionConfiguredNodeId", strlen(param_motion_node_id.value()) > 0 ? String(param_motion_node_id.value()) : "Default MAC-derived"},
      {"MotionLatitude", String(param_motion_latitude.value(), 5)},
      {"MotionLongitude", String(param_motion_longitude.value(), 5)},
      {"MotionSensorType", String(param_motion_sensor_type.value())},
      {"MotionSampleInterval", String(param_motion_sample_interval.value())},
      {"MotionPixelDelta", String(param_motion_pixel_delta.value())},
      {"MotionAreaThreshold", String(param_motion_area_threshold.value())},
      {"MotionConsecutiveHitsTarget", String(param_motion_consecutive_hits.value())},
      {"MotionConsecutiveHits", String(motion_state.consecutive_hits)},
      {"MotionCooldown", String(param_motion_cooldown.value())},
      {"MotionHttpTimeout", String(param_motion_http_timeout.value())},
      {"MotionSampleCount", String(motion_state.sample_count)},
      {"MotionAlertCount", String(motion_state.alert_count)},
      {"MotionLastScore", String(motion_state.last_score)},
      {"MotionLastSample", motion_age(motion_state.last_sample_ms)},
      {"MotionLastAlert", motion_age(motion_state.last_alert_ms)},
      {"MotionLastSend", motion_send_status()},
      {"MotionLastError", motion_state.last_error.length() > 0 ? motion_state.last_error : "None"},
      // RTSP
      {"RtspPort", String(RTSP_PORT)}};

  web_server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  auto html = moustache_render(index_html_min_start, substitutions);
  web_server.send(200, "text/html", html);
}

#ifdef FLASH_LED_GPIO
void handle_flash()
{
  log_v("handle_flash");
  // If no value present, use off, otherwise convert v to integer. Depends on analog resolution for max value
  auto v = web_server.hasArg("v") ? web_server.arg("v").toInt() : 0;
  // If conversion fails, v = 0
  analogWrite(FLASH_LED_GPIO, v);

  web_server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  web_server.send(200);
}
#endif

void handle_snapshot()
{
  log_v("handle_snapshot");
  if (camera_init_result != ESP_OK)
  {
    web_server.send(404, "text/plain", "Camera is not initialized");
    return;
  }

  // Remove old images stored in the frame buffer
  auto frame_buffers = CAMERA_CONFIG_FB_COUNT;
  while (frame_buffers--)
    cam.run();

  auto fb_len = cam.getSize();
  auto fb = (const char *)cam.getfb();
  if (fb == nullptr)
  {
    web_server.send(404, "text/plain", "Unable to obtain frame buffer from the camera");
    return;
  }

  web_server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  web_server.setContentLength(fb_len);
  web_server.send(200, "image/jpeg", "");
  web_server.sendContent(fb, fb_len);
}

#define STREAM_CONTENT_BOUNDARY "123456789000000000000987654321"

void handle_stream()
{
  log_v("handle_stream");
  if (camera_init_result != ESP_OK)
  {
    web_server.send(404, "text/plain", "Camera is not initialized");
    return;
  }

  log_v("starting streaming");
  // Blocks further handling of HTTP server until stopped
  char size_buf[12];
  auto client = web_server.client();
  client.write("HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: multipart/x-mixed-replace; boundary=" STREAM_CONTENT_BOUNDARY "\r\n");
  while (client.connected())
  {
    client.write("\r\n--" STREAM_CONTENT_BOUNDARY "\r\n");
    cam.run();
    client.write("Content-Type: image/jpeg\r\nContent-Length: ");
    sprintf(size_buf, "%d\r\n\r\n", cam.getSize());
    client.write(size_buf);
    client.write(cam.getfb(), cam.getSize());
  }

  log_v("client disconnected");
  client.stop();
  log_v("stopped streaming");
}

void handle_restart()
{
	log_v("handle_restart");
	WiFi.disconnect(false, true);
	ESP.restart();
}

esp_err_t initialize_camera()
{
  log_v("initialize_camera");

  log_i("Frame size: %s", param_frame_size.value());
  auto frame_size = lookup_frame_size(param_frame_size.value());
  log_i("JPEG quality: %d", param_jpg_quality.value());
  auto jpeg_quality = param_jpg_quality.value();
  log_i("Frame duration: %d ms", param_frame_duration.value());
  const camera_config_t camera_config = {
      .pin_pwdn = CAMERA_CONFIG_PIN_PWDN,         // GPIO pin for camera power down line
      .pin_reset = CAMERA_CONFIG_PIN_RESET,       // GPIO pin for camera reset line
      .pin_xclk = CAMERA_CONFIG_PIN_XCLK,         // GPIO pin for camera XCLK line
      .pin_sccb_sda = CAMERA_CONFIG_PIN_SCCB_SDA, // GPIO pin for camera SDA line
      .pin_sccb_scl = CAMERA_CONFIG_PIN_SCCB_SCL, // GPIO pin for camera SCL line
      .pin_d7 = CAMERA_CONFIG_PIN_Y9,             // GPIO pin for camera D7 line
      .pin_d6 = CAMERA_CONFIG_PIN_Y8,             // GPIO pin for camera D6 line
      .pin_d5 = CAMERA_CONFIG_PIN_Y7,             // GPIO pin for camera D5 line
      .pin_d4 = CAMERA_CONFIG_PIN_Y6,             // GPIO pin for camera D4 line
      .pin_d3 = CAMERA_CONFIG_PIN_Y5,             // GPIO pin for camera D3 line
      .pin_d2 = CAMERA_CONFIG_PIN_Y4,             // GPIO pin for camera D2 line
      .pin_d1 = CAMERA_CONFIG_PIN_Y3,             // GPIO pin for camera D1 line
      .pin_d0 = CAMERA_CONFIG_PIN_Y2,             // GPIO pin for camera D0 line
      .pin_vsync = CAMERA_CONFIG_PIN_VSYNC,       // GPIO pin for camera VSYNC line
      .pin_href = CAMERA_CONFIG_PIN_HREF,         // GPIO pin for camera HREF line
      .pin_pclk = CAMERA_CONFIG_PIN_PCLK,         // GPIO pin for camera PCLK line
      .xclk_freq_hz = CAMERA_CONFIG_CLK_FREQ_HZ,  // Frequency of XCLK signal, in Hz. EXPERIMENTAL: Set to 16MHz on ESP32-S2 or ESP32-S3 to enable EDMA mode
      .ledc_timer = CAMERA_CONFIG_LEDC_TIMER,     // LEDC timer to be used for generating XCLK
      .ledc_channel = CAMERA_CONFIG_LEDC_CHANNEL, // LEDC channel to be used for generating XCLK
      .pixel_format = PIXFORMAT_JPEG,             // Format of the pixel data: PIXFORMAT_ + YUV422|GRAYSCALE|RGB565|JPEG
      .frame_size = frame_size,                   // Size of the output image: FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
      .jpeg_quality = jpeg_quality,               // Quality of JPEG output. 0-63 lower means higher quality
      .fb_count = CAMERA_CONFIG_FB_COUNT,         // Number of frame buffers to be allocated. If more than one, then each frame will be acquired (double speed)
      .fb_location = CAMERA_CONFIG_FB_LOCATION,   // The location where the frame buffer will be allocated
      .grab_mode = CAMERA_GRAB_LATEST,            // When buffers should be filled
#if CONFIG_CAMERA_CONVERTER_ENABLED
      conv_mode = CONV_DISABLE, // RGB<->YUV Conversion mode
#endif
      .sccb_i2c_port = SCCB_I2C_PORT // If pin_sccb_sda is -1, use the already configured I2C bus by number
  };

  return cam.init(camera_config);
}

void update_camera_settings()
{
  auto camera = esp_camera_sensor_get();
  if (camera == nullptr)
  {
    log_e("Unable to get camera sensor");
    return;
  }

  camera->set_brightness(camera, param_brightness.value());
  camera->set_contrast(camera, param_contrast.value());
  camera->set_saturation(camera, param_saturation.value());
  camera->set_special_effect(camera, lookup_camera_effect(param_special_effect.value()));
  camera->set_whitebal(camera, param_whitebal.value());
  camera->set_awb_gain(camera, param_awb_gain.value());
  camera->set_wb_mode(camera, lookup_camera_wb_mode(param_wb_mode.value()));
  camera->set_exposure_ctrl(camera, param_exposure_ctrl.value());
  camera->set_aec2(camera, param_aec2.value());
  camera->set_ae_level(camera, param_ae_level.value());
  camera->set_aec_value(camera, param_aec_value.value());
  camera->set_gain_ctrl(camera, param_gain_ctrl.value());
  camera->set_agc_gain(camera, param_agc_gain.value());
  camera->set_gainceiling(camera, lookup_camera_gainceiling(param_gain_ceiling.value()));
  camera->set_bpc(camera, param_bpc.value());
  camera->set_wpc(camera, param_wpc.value());
  camera->set_raw_gma(camera, param_raw_gma.value());
  camera->set_lenc(camera, param_lenc.value());
  camera->set_hmirror(camera, param_hmirror.value());
  camera->set_vflip(camera, param_vflip.value());
  camera->set_dcw(camera, param_dcw.value());
  camera->set_colorbar(camera, param_colorbar.value());
}

void start_rtsp_server()
{
  log_v("start_rtsp_server");
  camera_server = std::unique_ptr<rtsp_server>(new rtsp_server(cam, param_frame_duration.value(), RTSP_PORT));
  // Add RTSP service to mDNS
  // HTTP is already set by iotWebConf
  MDNS.addService("rtsp", "tcp", RTSP_PORT);
}

void on_connected()
{
  log_v("on_connected");
  // Start the RTSP Server if initialized
  if (camera_init_result == ESP_OK)
    start_rtsp_server();
  else
    log_e("Not starting RTSP server: camera not initialized");
}

void on_config_saved()
{
  log_v("on_config_saved");
  sanitize_motion_settings();
  reset_motion_detector();
  update_camera_settings();
}

void setup()
{
  // Disable brownout
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  randomSeed(esp_random() ^ micros());
#ifdef CAMERA_POWER_GPIO
  pinMode(CAMERA_POWER_GPIO, OUTPUT);
  digitalWrite(CAMERA_POWER_GPIO, CAMERA_POWER_ON_LEVEL);
#endif

#ifdef USER_LED_GPIO
  pinMode(USER_LED_GPIO, OUTPUT);
  digitalWrite(USER_LED_GPIO, !USER_LED_ON_LEVEL);
#endif

#ifdef FLASH_LED_GPIO
  pinMode(FLASH_LED_GPIO, OUTPUT);
  // Set resolution to 8 bits
  analogWriteResolution(8);
  // Turn flash led off
  analogWrite(FLASH_LED_GPIO, 0);
#endif

#ifdef ARDUINO_USB_CDC_ON_BOOT
  // Delay for USB to connect/settle
  delay(5000);
#endif

  log_i("Core debug level: %d", CORE_DEBUG_LEVEL);
  log_i("CPU Freq: %d Mhz, %d core(s)", getCpuFrequencyMhz(), ESP.getChipCores());
  log_i("Free heap: %d bytes", ESP.getFreeHeap());
  log_i("SDK version: %s", ESP.getSdkVersion());
  log_i("Board: %s", BOARD_NAME);
  log_i("Starting " APP_TITLE "...");

  if (CAMERA_CONFIG_FB_LOCATION == CAMERA_FB_IN_PSRAM && !psramInit())
    log_e("Failed to initialize PSRAM");

  param_group_camera.addItem(&param_frame_duration);
  param_group_camera.addItem(&param_frame_size);
  param_group_camera.addItem(&param_jpg_quality);
  param_group_camera.addItem(&param_brightness);
  param_group_camera.addItem(&param_contrast);
  param_group_camera.addItem(&param_saturation);
  param_group_camera.addItem(&param_special_effect);
  param_group_camera.addItem(&param_whitebal);
  param_group_camera.addItem(&param_awb_gain);
  param_group_camera.addItem(&param_wb_mode);
  param_group_camera.addItem(&param_exposure_ctrl);
  param_group_camera.addItem(&param_aec2);
  param_group_camera.addItem(&param_ae_level);
  param_group_camera.addItem(&param_aec_value);
  param_group_camera.addItem(&param_gain_ctrl);
  param_group_camera.addItem(&param_agc_gain);
  param_group_camera.addItem(&param_gain_ceiling);
  param_group_camera.addItem(&param_bpc);
  param_group_camera.addItem(&param_wpc);
  param_group_camera.addItem(&param_raw_gma);
  param_group_camera.addItem(&param_lenc);
  param_group_camera.addItem(&param_hmirror);
  param_group_camera.addItem(&param_vflip);
  param_group_camera.addItem(&param_dcw);
  param_group_camera.addItem(&param_colorbar);
  iotWebConf.addParameterGroup(&param_group_camera);

  param_group_motion.addItem(&param_motion_enabled);
  param_group_motion.addItem(&param_motion_bridge_url);
  param_group_motion.addItem(&param_motion_label);
  param_group_motion.addItem(&param_motion_node_id);
  param_group_motion.addItem(&param_motion_latitude);
  param_group_motion.addItem(&param_motion_longitude);
  param_group_motion.addItem(&param_motion_sensor_type);
  param_group_motion.addItem(&param_motion_sample_interval);
  param_group_motion.addItem(&param_motion_pixel_delta);
  param_group_motion.addItem(&param_motion_area_threshold);
  param_group_motion.addItem(&param_motion_consecutive_hits);
  param_group_motion.addItem(&param_motion_cooldown);
  param_group_motion.addItem(&param_motion_http_timeout);
  iotWebConf.addParameterGroup(&param_group_motion);

  iotWebConf.getApTimeoutParameter()->visible = true;
  iotWebConf.setConfigSavedCallback(on_config_saved);
  iotWebConf.setWifiConnectionCallback(on_connected);
#ifdef USER_LED_GPIO
  iotWebConf.setStatusPin(USER_LED_GPIO, USER_LED_ON_LEVEL);
#endif
  iotWebConf.init();
  sanitize_motion_settings();

  // Try to initialize 3 times
  for (auto i = 0; i < 3; i++)
  {
    camera_init_result = initialize_camera();
    if (camera_init_result == ESP_OK)
    {
      update_camera_settings();
      break;
    }

    esp_camera_deinit();
    log_e("Failed to initialize camera. Error: 0x%0x. Frame size: %s, frame rate: %d ms, jpeg quality: %d", camera_init_result, param_frame_size.value(), param_frame_duration.value(), param_jpg_quality.value());
    delay(500);
  }

  // Set up required URL handlers on the web server
  web_server.on("/", HTTP_GET, handle_root);
  web_server.on("/config", []
                { iotWebConf.handleConfig(); });
  // Camera snapshot
  web_server.on("/snapshot", HTTP_GET, handle_snapshot);
  // Camera stream
  web_server.on("/stream", HTTP_GET, handle_stream);
#ifdef FLASH_LED_GPIO
  // Flash led
  web_server.on("/flash", HTTP_GET, handle_flash);
#endif
  // ESP restart
  web_server.on("/restart", HTTP_GET, handle_restart);
  web_server.onNotFound([]()
                        { iotWebConf.handleNotFound(); });
}

void loop()
{
  iotWebConf.doLoop();

  if (camera_server)
    camera_server->doLoop();

  motion_detector_loop();
}
