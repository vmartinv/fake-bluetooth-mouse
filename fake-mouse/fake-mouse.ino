// BLE mouse fed over USB serial by ~/fake-mouse/forward (see ../README.md).
// Frame: 0xA5, buttons, dx (int16 LE), dy (int16 LE), wheel (int8).
#include <bluefruit.h>

BLEDis bledis;
BLEHidAdafruit blehid;

const uint8_t SYNC = 0xA5;
const int FRAME_LEN = 7;

uint8_t frame[FRAME_LEN];
int frameLen = 0;

uint8_t buttons = 0;
uint8_t sentButtons = 0;
int32_t pendingX = 0, pendingY = 0, pendingWheel = 0;

// Counters since the last '?' (lag diagnosis).
uint32_t statFrames = 0, statReports = 0, statSendUs = 0, statMaxSendUs = 0, statSince = 0;

static int8_t take(int32_t &pending) {
  int32_t v = constrain(pending, -127, 127);
  pending -= v;
  return v;
}

// '?' on the serial port (outside a frame) prints the BLE link parameters.
void printStatus() {
  Serial.printf("temp=%.2fC ", readCPUTemperature());
  BLEConnection *conn = Bluefruit.Connection(0);
  if (!Bluefruit.connected() || !conn) {
    Serial.println("not connected");
    return;
  }
  char peer[32] = "";
  conn->getPeerName(peer, sizeof peer - 1);
  Serial.printf("peer=%s interval=%.2fms latency=%u timeout=%ums\n", peer,
                conn->getConnectionInterval() * 1.25f, conn->getSlaveLatency(),
                conn->getSupervisionTimeout() * 10);
  uint32_t ms = millis() - statSince;
  Serial.printf("last %lums: frames=%lu (%.0f/s) reports=%lu (%.0f/s) send avg=%.2fms max=%.2fms\n",
                ms, statFrames, statFrames * 1000.0f / ms, statReports, statReports * 1000.0f / ms,
                statReports ? statSendUs / 1000.0f / statReports : 0.0f, statMaxSendUs / 1000.0f);
  statFrames = statReports = statSendUs = statMaxSendUs = 0;
  statSince = millis();
}

void readSerial() {
  while (Serial.available()) {
    uint8_t b = Serial.read();
    if (frameLen == 0 && b == '?') {
      printStatus();
      continue;
    }
    if (frameLen == 0 && b != SYNC) continue;  // resync
    frame[frameLen++] = b;
    if (frameLen < FRAME_LEN) continue;
    frameLen = 0;
    statFrames++;
    buttons = frame[1];
    pendingX += (int16_t)(frame[2] | frame[3] << 8);
    pendingY += (int16_t)(frame[4] | frame[5] << 8);
    pendingWheel += (int8_t)frame[6];
  }
}

void setup() {
  Serial.begin(115200);

  // Default HVN queue is 1: each report then waits for the previous one's ack
  // (~2 connection events, measured ~13 ms). 2 lets one report go out every event.
  Bluefruit.configPrphConn(BLE_GATT_ATT_MTU_DEFAULT, BLE_GAP_EVENT_LENGTH_DEFAULT, 2,
                           BLE_GATTC_WRITE_CMD_TX_QUEUE_SIZE_DEFAULT);
  Bluefruit.begin();
  Bluefruit.setName("Desk Mouse");
  Bluefruit.Periph.setConnInterval(6, 12);  // 7.5-15 ms; the host has the final say
  Bluefruit.setTxPower(0);  // dBm; the laptop is close, less heat than +4
  Bluefruit.autoConnLed(true);  // LED blinks while advertising, solid when connected

  bledis.setManufacturer("fake-mouse");
  bledis.setModel("Desk Mouse");
  bledis.begin();
  blehid.begin();

  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_MOUSE);
  Bluefruit.Advertising.addService(blehid);
  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);
  Bluefruit.Advertising.setFastTimeout(30);
  Bluefruit.Advertising.start(0);
}

void loop() {
  readSerial();
  if (!Bluefruit.connected()) {
    pendingX = pendingY = pendingWheel = 0;  // don't replay stale motion on reconnect
    sentButtons = buttons;
    return;
  }
  if (!pendingX && !pendingY && !pendingWheel && buttons == sentButtons) return;
  // mouseReport blocks until the radio has room, so motion arriving meanwhile is
  // summed into the next report instead of queueing up.
  uint32_t t0 = micros();
  blehid.mouseReport(buttons, take(pendingX), take(pendingY), take(pendingWheel));
  uint32_t us = micros() - t0;
  statReports++;
  statSendUs += us;
  statMaxSendUs = max(statMaxSendUs, us);
  sentButtons = buttons;
}
