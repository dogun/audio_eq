#include <driver/i2s.h>
#include <WiFi.h>

#define RATE 48000

/**
   config i2s start
*/

static const i2s_port_t i2s_num = I2S_NUM_0; // i2s port number

static const i2s_config_t i2s_config = {
  .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),
  .sample_rate = RATE,
  .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
  .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
  .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_I2S),
  .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
  .dma_buf_count = 8,
  .dma_buf_len = 1024,
  .use_apll = true,
  .tx_desc_auto_clear = true,
  //.mclk_multiple=256,
  //.fixed_mclk = 24576000,
  //.fixed_mclk = RATE * 256,
};

static const i2s_pin_config_t pin_config = {
  .mck_io_num = 0,
  .bck_io_num = 27,
  .ws_io_num = 26,
  .data_out_num = 25,
  .data_in_num = 23
};
//end i2s config

/**
   eq start
*/
#define MAX_EQ_COUNT 32
#define PREAMP -5.0
static const double TWOPI = (2.0 * M_PI);
static const double PREAMPF = pow(10.0, PREAMP / 20.0);

typedef struct t_biquad t_biquad;
struct t_biquad {
  double a0, a1, a2, a3, a4;
  double x1, x2, y1, y2;
  double cf, bw, gain;
};

typedef struct eq_config eq_config;
struct eq_config {
  double cf, bw, gain;
};

static eq_config eq_r[MAX_EQ_COUNT];
int eq_len_r;

static eq_config eq_l[MAX_EQ_COUNT];
int eq_len_l;

static t_biquad r_biquads[MAX_EQ_COUNT];
static t_biquad l_biquads[MAX_EQ_COUNT];

/**
   len = 实际数据长度
*/
static inline void _apply_biquads(int32_t* src, int32_t* dst, size_t len, double vol) {
  int i, di;

  if (len % 2 != 0) {
    Serial.print("buffer len error:");
    Serial.println(len);
    return;
  }

  for (i = 0; i < len; i = i + 2) {
    double l_s = (src[i] >> 8) * PREAMPF * vol;
    double r_s = (src[i + 1] >> 8) * PREAMPF * vol;

    double l_f = l_s;
    double r_f = r_s;

    for (di = 0; di < eq_len_l; ++di) {
      l_f = _apply_biquads_one(l_s, &(l_biquads[di]));
      l_s = l_f;
    }
    for (di = 0; di < eq_len_r; ++di) {
      r_f = _apply_biquads_one(r_s, &(r_biquads[di]));
      r_s = r_f;
    }

    dst[i] = ((int32_t)l_f) << 8;
    dst[i + 1] = ((int32_t)r_f) << 8;
  }
}

static inline double _apply_biquads_one(double s, t_biquad *b) {
  //Serial.print(b->cf); Serial.print(" "); Serial.print(b->bw); Serial.print(" "); Serial.print(b->gain);
  double f =
    s * b->a0 \
    + b->a1 * b->x1 \
    + b->a2 * b->x2 \
    - b->a3 * b->y1 \
    - b->a4 * b->y2;
  b->x2 = b->x1;
  b->x1 = s;
  b->y2 = b->y1;
  b->y1 = f;

  //Serial.print(" 1:"); Serial.print(s); Serial.print("   2:"); Serial.println((int32_t)f);
  return f;
}

/**
   See 'Audio EQ Cookbook' for more information
*/
static void _mk_biquad(double dbgain, double cf, double bw, t_biquad *b) {
  if (b == NULL) {
    Serial.println("biquad NULL");
    return;
  }

  double A = pow(10.0, dbgain / 40.0);
  double omega = TWOPI * cf / RATE;
  double sn = sin(omega);
  double cs = cos(omega);
  double alpha = sn * sinh(M_LN2 / 2.0f * bw * omega / sn);

  double alpha_m_A = alpha * A;
  double alpha_d_A = alpha / A;

  double b0 = 1.0 + alpha_m_A;
  double b1 = -2.0 * cs;
  double b2 = 1.0 - alpha_m_A;
  double a0 = 1.0 + alpha_d_A;
  double a1 = b1;
  double a2 = 1.0 - alpha_d_A;

  b->a0 = b0 / a0;
  b->a1 = b1 / a0;
  b->a2 = b2 / a0;
  b->a3 = a1 / a0;
  b->a4 = a2 / a0;

  b->x1 = 0.0;
  b->x2 = 0.0;
  b->y1 = 0.0;
  b->y2 = 0.0;

  b->cf = cf;
  b->bw = bw;
  b->gain = dbgain;
}

/**
   wifi config
*/
const char* ssid     = "ESP32-DSP-AP";
const char* password = "123456789";

WiFiServer server(80);
void setup_wifi() {
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: "); Serial.println(IP);
  server.begin();
  Serial.println("setup wifi ok");
}

String header;
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

void print_html(WiFiClient client, String eq_l, String eq_r) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println();

  client.println("<!DOCTYPE html><html>");
  client.println("<head><title>ESP32 DSP Server</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  client.println("<link rel=\"icon\" href=\"data:,\">");
  client.println("</head>");
  client.println("<body><h1>ESP32 DSP Server</h1>");
  client.println("<form action=\"\" method=\"post\">");
  client.println("<textarea style=\"width:200px;height:100px;\" name=\"eq_l\">"); client.println(eq_l); client.println("</textarea>");
  client.println("<p />");
  client.println("<textarea style=\"width:200px;height:100px;\" name=\"eq_r\">"); client.println(eq_r); client.println("</textarea>");
  client.println("<p />");
  client.println("<input type=\"submit\" />");
  client.println("</form></body></html>");
  client.println();
}

void process_http() {
  WiFiClient client = server.available();
  if (client) {
    currentTime = millis();
    previousTime = currentTime;
    String currentLine = "";
    Serial.println("New Client.");
    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        header += c;
        if (c == '\n' && currentLine.length() == 0) { //接受header结束
            print_html(client, "", "");
            break;
        } else if(c == '\n') {
          currentLine = "";
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    header = "";

    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}


//==================================================================
/**
   main start
*/
void setup() {
  Serial.begin(115200);

  i2s_driver_install(i2s_num, &i2s_config, 0, NULL);        // ESP32 will allocated resources to run I2S
  i2s_set_pin(i2s_num, &pin_config);                        // Tell it the pins you will be using

  //init eq
  //init eq
  _mk_biquad(4, 2000, 2, &(l_biquads[eq_len_l++]));

  _mk_biquad(4, 200, 2, &(r_biquads[eq_len_r++]));
  _mk_biquad(-3, 300, 2, &(r_biquads[eq_len_r++]));
  _mk_biquad(4, 400, 2, &(r_biquads[eq_len_r++]));
  _mk_biquad(4, 500, 2, &(r_biquads[eq_len_r++]));
  
  _mk_biquad(-3, 600, 2, &(r_biquads[eq_len_r++]));
  _mk_biquad(4, 700, 2, &(r_biquads[eq_len_r++]));
  _mk_biquad(4, 800, 2, &(r_biquads[eq_len_r++]));
  _mk_biquad(-3, 900, 2, &(r_biquads[eq_len_r++]));
  _mk_biquad(4, 1000, 2, &(r_biquads[eq_len_r++]));
  _mk_biquad(4, 2000, 2, &(r_biquads[eq_len_r++]));
  _mk_biquad(-3, 3000, 2, &(r_biquads[eq_len_r++]));
  /*
  _mk_biquad(4, 4000, 2, &(r_biquads[eq_len_r++]));
  _mk_biquad(4, 5000, 2, &(r_biquads[eq_len_r++]));
  _mk_biquad(-3, 6000, 2, &(r_biquads[eq_len_r++]));
  _mk_biquad(4, 7000, 2, &(r_biquads[eq_len_r++]));
  _mk_biquad(4, 8000, 2, &(r_biquads[eq_len_r++]));
  _mk_biquad(-3, 9000, 2, &(r_biquads[eq_len_r++]));
  _mk_biquad(4, 10000, 2, &(r_biquads[eq_len_r++]));
  _mk_biquad(4, 20000, 2, &(r_biquads[eq_len_r++]));
  _mk_biquad(-3, 30000, 2, &(r_biquads[eq_len_r++]));
  _mk_biquad(4, 40000, 2, &(r_biquads[eq_len_r++]));
  _mk_biquad(4, 50000, 2, &(r_biquads[eq_len_r++]));
  _mk_biquad(-3, 60000, 2, &(r_biquads[eq_len_r++]));
  _mk_biquad(4, 70000, 2, &(r_biquads[eq_len_r++]));
  _mk_biquad(4, 80000, 2, &(r_biquads[eq_len_r++]));
  _mk_biquad(-3, 90000, 2, &(r_biquads[eq_len_r++]));
  _mk_biquad(4, 100000, 2, &(r_biquads[eq_len_r++]));
  */

  ESP_LOGW("MAIN", "setup audio ok ok");

  //setup_wifi();
}


void loop() {
  size_t bytes_read = 0;
  size_t buf_size = 32;
  int32_t data[buf_size];
  i2s_read(i2s_num, (char*)data, buf_size * 4, &bytes_read, 100);
  _apply_biquads(data, data, buf_size, 1);
  i2s_write(i2s_num, (char*)data, bytes_read, &bytes_read, 100);

  //process_http();
}
