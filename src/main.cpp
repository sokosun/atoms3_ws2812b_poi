#include <M5Unified.h>
#include <ESP32DMASPIMaster.h>
#include "../image/image1.h"

constexpr int GPIO_BUTTON = 41;
constexpr int SPI_MOSI_PIN = 8;
constexpr uint32_t RESET_BYTES = 36;
constexpr uint32_t ENCODE_BYTES = 4;
constexpr uint32_t NUM_OF_LEDs = 72;
constexpr int INTERVAL_us = 2500;
constexpr size_t BUFFER_SIZE = RESET_BYTES + 3 * ENCODE_BYTES * NUM_OF_LEDs; // '3' means RGB colors
constexpr uint32_t LED_OFF[NUM_OF_LEDs] = {0};

hw_timer_t * timer = nullptr;
ESP32DMASPI::Master spi_master;
uint8_t *dma_tx_buf;

volatile bool g_irq0 = false;
void setIRQ0() {
  g_irq0 = true;
}

// Notes
// - 3.333333MHz SPI MOSI controls WS2812B LEDs
//   = T = 0.3us
// - WS2812B LED detects following patterns as commands
//   = Reset |_____________(80+us)___________| : 266.66bits -> 288bits
//   = 0     |‾‾‾|_________| (0.3us, 0.9us)    : 4bits
//   = 1     |‾‾‾‾‾‾|______| (0.6us, 0.6us)    : 4bits
// - Each WS2812B LED requires 24bit RGB value to change colors
//   = MSB first & GRB sequence (G7, G6, ... G1, R7, R6, ..., R1, B7, B6, ... B1)
//   = SPI sends 96 bits data for each LED

// Pack 36 Bytes Reset command to 'buf'.
void PackReset(uint8_t *buf){
  for(int i=0;i<RESET_BYTES;i++){
    buf[i] = 0;
  }
}

// Pack 12 Bytes Color command to 'buf'
void PackGRB(uint8_t *buf, uint32_t rgb){
  static constexpr uint8_t lut[] = {
    0x88, // <- 00
    0x8c, // <- 01
    0xc8, // <- 10
    0xcc  // <- 11
  };
  
  buf[ 0] = lut[(rgb & 0x0000c000) >> 14];
  buf[ 1] = lut[(rgb & 0x00003000) >> 12];
  buf[ 2] = lut[(rgb & 0x00000c00) >> 10];
  buf[ 3] = lut[(rgb & 0x00000300) >>  8];
  
  buf[ 4] = lut[(rgb & 0x00c00000) >> 22];
  buf[ 5] = lut[(rgb & 0x00300000) >> 20];
  buf[ 6] = lut[(rgb & 0x000c0000) >> 18];
  buf[ 7] = lut[(rgb & 0x00030000) >> 16];
  
  buf[ 8] = lut[(rgb & 0x000000c0) >>  6];
  buf[ 9] = lut[(rgb & 0x00000030) >>  4];
  buf[10] = lut[(rgb & 0x0000000c) >>  2];
  buf[11] = lut[ rgb & 0x00000003       ];
}

void setLedColor(const uint32_t * colors)
{
  PackReset(dma_tx_buf);
  for(auto i=0; i<NUM_OF_LEDs; i++){
    PackGRB(dma_tx_buf + RESET_BYTES + i * 3 * ENCODE_BYTES, colors[i]);
  }
  spi_master.queue(dma_tx_buf, NULL, BUFFER_SIZE);
  spi_master.trigger();
}

void IRAM_ATTR timer_isr(){
  static int pos = 0;
  constexpr uint32_t image1_length = sizeof(image1) / sizeof(image1[0][0]) / NUM_OF_LEDs;

  setLedColor(image1[pos]);

  if(++pos >= image1_length){
    pos = 0;
  }
}

void setup()
{
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;
  M5.begin(cfg);

  Serial.begin(115200);
  delay(1000);
  Serial.printf("demo\r\n");

  attachInterrupt(digitalPinToInterrupt(GPIO_BUTTON), setIRQ0, FALLING);

  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &timer_isr, true);
  timerAlarmWrite(timer, INTERVAL_us, true);
  timerAlarmDisable(timer);

  dma_tx_buf = spi_master.allocDMABuffer(BUFFER_SIZE);

  spi_master.setFrequency(3333333);
  spi_master.setMaxTransferSize(BUFFER_SIZE);
  spi_master.begin(2, -1, -1, SPI_MOSI_PIN, -1); // Enable MOSI pin only
}

void loop()
{
  static bool draw = false;

  if(g_irq0){
    g_irq0 = false;
    draw = !draw;

    if(draw){
      timerAlarmEnable(timer);  
    }else{
      timerAlarmDisable(timer);
      delay(10);
      setLedColor(LED_OFF);
    }
  }

  return;
}