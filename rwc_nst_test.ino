/*

Module:  rwc_nst_test.ino

Function:
  Auto-configured raw test example, for Adafruit Feather M0 LoRa

Copyright notice and License:
  See LICENSE file accompanying this project.

Author:
    Matthijs Kooijman  2015
    Terry Moore, MCCI Corporation	2019

*/

/*******************************************************************************
 * Portions Copyright (c) 2015 Matthijs Kooijman
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 * This example transmits data on hardcoded channel and receives data
 * when not transmitting. Running this sketch on two nodes should allow
 * them to communicate.
 *******************************************************************************/

#include <lmic.h>
#include <hal/hal.h>
#include <arduino_lmic_hal_boards.h>

#include <SPI.h>

#include <stdarg.h>
#include <stdio.h>

// see README.md; here are the parameters for the tests.
#define RX_TIMEOUT  5000        // wait 5 seconds before giving up
#define TX_INTERVAL 2000        // milliseconds
#define RX_RSSI_INTERVAL 100    // milliseconds

// edit this to change the number of uplinks per tx test run.
constexpr unsigned kTxTestCount = 3;

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in arduino-lmoc/project_config/lmic_project_config.h,
// otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

// set true while a test is running to lock out terminal.
bool fBusy = false;
// counter of packets to tx.
unsigned gTxCount = 0;
// counter of received packets last test
unsigned gRxCount;

// this gets callled by the library but we choose not to display any info;
// and no action is required.
void onEvent (ev_t ev) {
}

extern "C" {
void lmic_printf(const char *fmt, ...);
};

void lmic_printf(const char *fmt, ...) {
  if (! Serial.dtr())
    return;

  char buf[256];
  va_list ap;

  va_start(ap, fmt);
  (void) vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
  va_end(ap);

  // in case we overflowed:
  buf[sizeof(buf) - 1] = '\0';
  if (Serial.dtr()) Serial.print(buf);
}

osjob_t txjob;
osjob_t timeoutjob;
static void tx_func (osjob_t* job);

// Transmit the given string and call the given function afterwards
void tx(const char *str, osjobcb_t func) {
  // the radio is probably in RX mode; stop it.
  os_radio(RADIO_RST);
  // wait a bit so the radio can come out of RX mode
  delay(1);

  // prepare data
  LMIC.dataLen = 0;
  while (*str)
    LMIC.frame[LMIC.dataLen++] = *str++;

  // set completion function.
  LMIC.osjob.func = func;

  // start the transmission
  os_radio(RADIO_TX);
  Serial.print(".");
}

// Enable rx mode and call func when a packet is received
void rx(osjobcb_t func) {
  LMIC.osjob.func = func;
  LMIC.rxtime = os_getTime(); // RX _now_
  // Enable "continuous" RX (e.g. without a timeout, still stops after
  // receiving a packet)
  os_radio(RADIO_RXON);
}

static void rxtimeout_func(osjob_t *job) {
  // the radio is probably in RX mode; stop it.
  os_radio(RADIO_RST);
  // wait a bit so the radio can come out of RX mode
  delay(1);

  digitalWrite(LED_BUILTIN, LOW); // off
  os_clearCallback(&LMIC.osjob);
  Serial.println("end RX test");
  fBusy = false;
}

void startRx()
  {
  Serial.print("\nStart RX test: capturing raw downlink for "); Serial.print(RX_TIMEOUT / 1000.0); Serial.println("seconds.");
  Serial.print("At RWC5020, select NST>Signal Generator, then Run.");

  digitalWrite(LED_BUILTIN, HIGH); // on

  // Timeout RX (i.e. update led status) after 3 periods without RX
  os_setTimedCallback(&timeoutjob, os_getTime() + ms2osticks(RX_TIMEOUT), rxtimeout_func);

  // reset rx counter
  gRxCount = 0;
  rx(rxdone_func);
  fBusy = true;
  }

static void rxdone_func (osjob_t* job) {
  // Blink once to confirm reception and then keep the led on
  digitalWrite(LED_BUILTIN, LOW); // off

  if (LMIC.dataLen > 0)
    ++gRxCount;

  Serial.print(".");

  // Blink once to confirm reception and then keep the led on
  rx(rxdone_func);

  delay(10);
  digitalWrite(LED_BUILTIN, HIGH); // on
}

static void txdone_func (osjob_t* job) {
  tx_func(job);
}

// log text to USART and toggle LED
static void tx_func (osjob_t* job) {
  if (gTxCount > 0)
    {
    // say hello
    tx("Hello, world!", txdone_func);
    --gTxCount;
    }
  else
    {
    Serial.println("\ntx test complete");
    fBusy = false;
    }
}

// application entry point
void setup() {
  // delay(3000) makes recovery from botched images much easier, as it
  // gives the host time to break in to start a download. Without it,
  // you get to the crash before the host can break in.
  delay(3000);

  // even after the delay, we wait for the host to open the port. operator
  // bool(Serial) just checks dtr(), and it tosses in a 10ms delay.
  while(! Serial.dtr())
        /* wait for the PC */;

  Serial.begin(115200);
  Serial.println("Starting rwc_nst_test");

  pinMode(LED_BUILTIN, OUTPUT);

  // initialize runtime env
  // don't die mysteriously; die noisily.
  const lmic_pinmap *pPinMap = Arduino_LMIC::GetPinmap_ThisBoard();

  if (pPinMap == nullptr) {
    pinMode(LED_BUILTIN, OUTPUT);
    for (;;) {
      // flash lights, sleep.
      for (int i = 0; i < 5; ++i) {
        digitalWrite(LED_BUILTIN, 1);
        delay(100);
        digitalWrite(LED_BUILTIN, 0);
        delay(900);
      }
      Serial.println(F("board not known to library; add pinmap or update getconfig_thisboard.cpp"));
    }
  }

  os_init_ex(pPinMap);

  // Set up these settings once, and use them for both TX and RX
#ifdef ARDUINO_ARCH_STM32
  LMIC_setClockError(10*65536/100);
#endif

#if defined(CFG_eu868)
  // Use a frequency in the g3 which allows 10% duty cycling.
  LMIC.freq = 869525000;
  // Use a medium spread factor. This can be increased up to SF12 for
  // better range, but then, the interval should be (significantly)
  // raised to comply with duty cycle limits as well.
  LMIC.datarate = DR_SF9;
  // Maximum TX power
  LMIC.txpow = 27;
#elif defined(CFG_us915)
  // make it easier for test, by pull the parameters up to the top of the
  // block. Ideally, we'd use the serial port to drive this; or have
  // a voting protocol where one side is elected the controller and
  // guides the responder through all the channels, powers, ramps
  // the transmit power from min to max, and measures the RSSI and SNR.
  // Even more amazing would be a scheme where the controller could
  // handle multiple nodes; in that case we'd have a way to do
  // production test and qualification. However, using an RWC5020A
  // is a much better use of development time.

  // set fDownlink true to use a downlink channel; false
  // to use an uplink channel. Generally speaking, uplink
  // is more interesting, because you can prove that gateways
  // *should* be able to hear you.
  const static bool fDownlink = false;

  // the downlink channel to be used.
  const static uint8_t kDownlinkChannel = 0;

  // the uplink channel to be used.
  const static uint8_t kUplinkChannel = 0;

  // this is automatically set to the proper bandwidth in kHz,
  // based on the selected channel.
  uint32_t uBandwidth;

  if (! fDownlink)
        {
        if (kUplinkChannel < 64)
                {
                LMIC.freq = US915_125kHz_UPFBASE +
                            kUplinkChannel * US915_125kHz_UPFSTEP;
                uBandwidth = 125;
                }
        else
                {
                LMIC.freq = US915_500kHz_UPFBASE +
                            (kUplinkChannel - 64) * US915_500kHz_UPFSTEP;
                uBandwidth = 500;
                }
        }
  else
        {
        // downlink channel
        LMIC.freq = US915_500kHz_DNFBASE +
                    kDownlinkChannel * US915_500kHz_DNFSTEP;
        uBandwidth = 500;
        }

  // Use a suitable spreading factor
  if (uBandwidth < 500)
        LMIC.datarate = US915_DR_SF7;         // DR4
  else
        LMIC.datarate = US915_DR_SF12CR;      // DR8

  // default tx power for US: 21 dBm
  LMIC.txpow = 21;
#elif defined(CFG_au921)
  // make it easier for test, by pull the parameters up to the top of the
  // block. Ideally, we'd use the serial port to drive this; or have
  // a voting protocol where one side is elected the controller and
  // guides the responder through all the channels, powers, ramps
  // the transmit power from min to max, and measures the RSSI and SNR.
  // Even more amazing would be a scheme where the controller could
  // handle multiple nodes; in that case we'd have a way to do
  // production test and qualification. However, using an RWC5020A
  // is a much better use of development time.

  // set fDownlink true to use a downlink channel; false
  // to use an uplink channel. Generally speaking, uplink
  // is more interesting, because you can prove that gateways
  // *should* be able to hear you.
  const static bool fDownlink = false;

  // the downlink channel to be used.
  const static uint8_t kDownlinkChannel = 3;

  // the uplink channel to be used.
  const static uint8_t kUplinkChannel = 8 + 3;

  // this is automatically set to the proper bandwidth in kHz,
  // based on the selected channel.
  uint32_t uBandwidth;

  if (! fDownlink)
        {
        if (kUplinkChannel < 64)
                {
                LMIC.freq = AU921_125kHz_UPFBASE +
                            kUplinkChannel * AU921_125kHz_UPFSTEP;
                uBandwidth = 125;
                }
        else
                {
                LMIC.freq = AU921_500kHz_UPFBASE +
                            (kUplinkChannel - 64) * AU921_500kHz_UPFSTEP;
                uBandwidth = 500;
                }
        }
  else
        {
        // downlink channel
        LMIC.freq = AU921_500kHz_DNFBASE +
                    kDownlinkChannel * AU921_500kHz_DNFSTEP;
        uBandwidth = 500;
        }

  // Use a suitable spreading factor
  if (uBandwidth < 500)
        LMIC.datarate = AU921_DR_SF7;         // DR4
  else
        LMIC.datarate = AU921_DR_SF12CR;      // DR8

  // default tx power for AU: 30 dBm
  LMIC.txpow = 30;
#elif defined(CFG_as923)
// make it easier for test, by pull the parameters up to the top of the
// block. Ideally, we'd use the serial port to drive this; or have
// a voting protocol where one side is elected the controller and
// guides the responder through all the channels, powers, ramps
// the transmit power from min to max, and measures the RSSI and SNR.
// Even more amazing would be a scheme where the controller could
// handle multiple nodes; in that case we'd have a way to do
// production test and qualification. However, using an RWC5020A
// is a much better use of development time.
  const static uint8_t kChannel = 0;
  uint32_t uBandwidth;

  LMIC.freq = AS923_F1 + kChannel * 200000;
  uBandwidth = 125;

  // Use a suitable spreading factor
  if (uBandwidth == 125)
    LMIC.datarate = AS923_DR_SF7;         // DR7
  else
    LMIC.datarate = AS923_DR_SF7B;        // DR8

  // default tx power for AS: 21 dBm
  LMIC.txpow = 16;

  if (LMIC_COUNTRY_CODE == LMIC_COUNTRY_CODE_JP)
    {
    LMIC.lbt_ticks = us2osticks(AS923JP_LBT_US);
    LMIC.lbt_dbmax = AS923JP_LBT_DB_MAX;
    }
#elif defined(CFG_kr920)
// make it easier for test, by pull the parameters up to the top of the
// block. Ideally, we'd use the serial port to drive this; or have
// a voting protocol where one side is elected the controller and
// guides the responder through all the channels, powers, ramps
// the transmit power from min to max, and measures the RSSI and SNR.
// Even more amazing would be a scheme where the controller could
// handle multiple nodes; in that case we'd have a way to do
// production test and qualification. However, using an RWC5020A
// is a much better use of development time.
  const static uint8_t kChannel = 0;
  uint32_t uBandwidth;

  LMIC.freq = KR920_F1 + kChannel * 200000;
  uBandwidth = 125;

  LMIC.datarate = KR920_DR_SF7;         // DR7
  // default tx power for KR: 14 dBm
  LMIC.txpow = KR920_TX_EIRP_MAX_DBM;
  if (LMIC.freq < KR920_F14DBM)
    LMIC.txpow = KR920_TX_EIRP_MAX_DBM_LOW;

  LMIC.lbt_ticks = us2osticks(KR920_LBT_US);
  LMIC.lbt_dbmax = KR920_LBT_DB_MAX;
#elif defined(CFG_in866)
// make it easier for test, by pull the parameters up to the top of the
// block. Ideally, we'd use the serial port to drive this; or have
// a voting protocol where one side is elected the controller and
// guides the responder through all the channels, powers, ramps
// the transmit power from min to max, and measures the RSSI and SNR.
// Even more amazing would be a scheme where the controller could
// handle multiple nodes; in that case we'd have a way to do
// production test and qualification. However, using an RWC5020A
// is a much better use of development time.
  const static uint8_t kChannel = 0;
  uint32_t uBandwidth;

  LMIC.freq = IN866_F1 + kChannel * 200000;
  uBandwidth = 125;

  LMIC.datarate = IN866_DR_SF7;         // DR7
  // default tx power for IN: 30 dBm
  LMIC.txpow = IN866_TX_EIRP_MAX_DBM;
#else
# error Unsupported LMIC regional configuration.
#endif


  // disable RX IQ inversion
  LMIC.noRXIQinversion = true;

  // This sets CR 4/5, BW125 (except for EU/AS923 DR_SF7B, which uses BW250)
  LMIC.rps = updr2rps(LMIC.datarate);

  Serial.print("Frequency: "); Serial.print(LMIC.freq / 1000000);
            Serial.print("."); Serial.print((LMIC.freq / 100000) % 10);
            Serial.print("MHz");
  Serial.print("  LMIC.datarate: "); Serial.print(LMIC.datarate);
  Serial.print("  LMIC.txpow: "); Serial.println(LMIC.txpow);
  Serial.print("===> enter T for Tx text, R for Rx test, C for Rx count: ");
  Serial.flush();

}

void loop() {
  // execute scheduled jobs and events
  os_runloop_once();

  if (fBusy)
    return;

  if (Serial.available() == 0)
    return;

  int const c = Serial.read();
  switch (c)
    {
  case 'T': case 't':
    startTx();
    break;

  case 'R': case 'r':
    startRx();
    break;

  case 'C': case 'c':
    sendRxCount();
    break;

  default:
    Serial.println("\nUnrecognized");
    break;
    }
}

void startTx()
  {
  Serial.println("\nTransmit test: RWC5020A should be in NST > Signal Analyzer mode");
  // setup initial job
  gTxCount = kTxTestCount;
  os_setCallback(&txjob, tx_func);
  fBusy = true;
  }

void sendRxCount()
  {
  Serial.println();
  Serial.print("RxCount:"); Serial.println(gRxCount);
  Serial.flush();
  }
