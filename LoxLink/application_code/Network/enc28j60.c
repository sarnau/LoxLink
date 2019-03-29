#include "enc28j60.h"
#include "stm32f1xx_hal.h"

/***
 *  STM32 pin configuration for the SPI and the CS gpio
 ***/
// The SPI pins are fixed by the STM32 chip
#define ENC28J60_SCK_Pin GPIO_PIN_5
#define ENC28J60_SCK_GPIO_Port GPIOA
#define ENC28J60_MISO_Pin GPIO_PIN_6
#define ENC28J60_MISO_GPIO_Port GPIOA
#define EBC27J60_MOSI_Pin GPIO_PIN_7
#define EBC27J60_MOSI_GPIO_Port GPIOA

// Chip Select is done via a specific pin
#define ENC28J60_CS_Pin GPIO_PIN_4
#define ENC28J60_CS_GPIO_Port GPIOA

/***
 *  Default initialization values for Phy configuration registers
 ***/
// LEDA : Display transmit activity (stretchable)
// LEDB : Display receive activity (stretchable)
// + Stretchable LED events will cause lengthened LED pulses
#define PHLCON_DEFAULT_VALUE ((1 << PHLCON_LACFG_SHIFT) | (2 << PHLCON_LBCFG_SHIFT) | PHLCON_STRCH)

/***
 *  Ethernet buffer organization
 ***/
#define ENC28J60_BUFSIZE (0x2000) // constant for the hardware
#define ENC28J60_RXSIZE (0x1A00)  // 6.5kb RX buffer
#define ENC28J60_TXSIZE (0x0600)  // 1.5kb TX buffer

#define ENC28J60_RXBEGIN (0) // has to be zero (Rev. B4 Silicon Errata)
#define ENC28J60_RXEND (ENC28J60_RXSIZE - 1)
#define ENC28J60_TXBEGIN (ENC28J60_BUFSIZE - 1 - ENC28J60_TXSIZE)
#define ENC28J60_TXEND (ENC28J60_BUFSIZE - 1)

/***
 *  variables
 ***/
uint8_t sENC28J60_current_bank = 0;

/***
 *  ENC28J60 Operations
 ***/
#define ENC28J60_READ_CTRL_REG (0 << 5)
#define ENC28J60_READ_BUF_MEM (1 << 5)
#define ENC28J60_WRITE_CTRL_REG (2 << 5)
#define ENC28J60_WRITE_BUF_MEM (3 << 5)
#define ENC28J60_BIT_FIELD_SET (4 << 5)
#define ENC28J60_BIT_FIELD_CLR (5 << 5)

#define ENC28J60_SOFT_RESET_COMMAND ((7 << 5) | 0x1F)
#define ENC28J60_READ_BUF_MEM_COMMAND (ENC28J60_READ_BUF_MEM | 0x1A)
#define ENC28J60_WRITE_BUF_MEM_COMMAND (ENC28J60_WRITE_BUF_MEM | 0x1A)

#define ENC28J60_COMMON_REGOFFSET 0x1B

/***
 *  TABLE 3-1: ENC28J60 CONTROL REGISTER MAP
 ***/
#define REG_MASK 0x1F
#define BANK_MASK 0x60
#define SPRD_MASK 0x80
#define BANK_REG(bank, reg) (((bank) << 5) & BANK_MASK) | (reg & REG_MASK)

// Shared registers between all banks
#define EIE 0x1B   // 8 bit - Ethernet interrupt enable register
#define EIR 0x1C   // 8 bit - Ethernet interrupt request (flag) register
#define ESTAT 0x1D // 8 bit - Ethernet status register
#define ECON2 0x1E // 8 bit - Ethernet control register 2
#define ECON1 0x1F // 8 bit - Ethernet control register 1

// Bank 0
#define ERDPT BANK_REG(0, 0x00)   // 16 bit LSB - Read Pointer
#define EWRPT BANK_REG(0, 0x02)   // 16 bit LSB - Write Pointer
#define ETXST BANK_REG(0, 0x04)   // 16 bit LSB - TX Start
#define ETXND BANK_REG(0, 0x06)   // 16 bit LSB - TX End
#define ERXST BANK_REG(0, 0x08)   // 16 bit LSB - RX Start
#define ERXND BANK_REG(0, 0x0A)   // 16 bit LSB - RX End
#define ERXRDPT BANK_REG(0, 0x0C) // 16 bit LSB - RX RD Pointer
#define ERXWRPT BANK_REG(0, 0x0E) // 16 bit LSB - RX WR Pointer
#define EDMAST BANK_REG(0, 0x10)  // 16 bit LSB - DMA Start
#define EDMAND BANK_REG(0, 0x12)  // 16 bit LSB - DMA End
#define EDMADST BANK_REG(0, 0x14) // 16 bit LSB - DMA Destination
#define EDMACS BANK_REG(0, 0x16)  // 16 bit LSB - DMA Checksum

// Bank 1
#define ETH BANK_REG(1, 0x00)     // 8 bytes - Hash Table Bytes
#define EPMM BANK_REG(1, 0x08)    // 8 bytes - Pattern Match Mask Bytes
#define EPMCS BANK_REG(1, 0x10)   // 16 bit LSB - Pattern Match Checksum
#define EPMO BANK_REG(1, 0x14)    // 16 bit LSB - Pattern Match Offset
#define ERXFCON BANK_REG(1, 0x18) // 8 bit -
#define EPKTCNT BANK_REG(1, 0x19) // 8 bit - Ethernet Packet Count

// Bank 2
#define MACON1 BANK_REG(2, 0x00) | SPRD_MASK   // 8 bit - MAC Control 1
#define MACON3 BANK_REG(2, 0x02) | SPRD_MASK   // 8 bit - MAC Control 3
#define MACON4 BANK_REG(2, 0x03) | SPRD_MASK   // 8 bit - MAC Control 4
#define MABBIPG BANK_REG(2, 0x04) | SPRD_MASK  // 8 bit - Back-to-Back Inter-Packet Gap
#define MAIPG BANK_REG(2, 0x06) | SPRD_MASK    // 16 bit LSB - Non-Back-to-Back Inter-Packet Gap
#define MACLCON1 BANK_REG(2, 0x08) | SPRD_MASK // 8 bit - Retransmission Maximum
#define MACLCON2 BANK_REG(2, 0x09) | SPRD_MASK // 8 bit - Collision Window
#define MAMXFL BANK_REG(2, 0x0A) | SPRD_MASK   // 16 bit LSB - Maximum Frame Length
#define MICMD BANK_REG(2, 0x12) | SPRD_MASK    // 8 bit - MII command register
#define MIREGADR BANK_REG(2, 0x14) | SPRD_MASK // 8 bit - MII register address
#define MIWR BANK_REG(2, 0x16) | SPRD_MASK     // 16 bit LSB - MII Write Data
#define MIRD BANK_REG(2, 0x18) | SPRD_MASK     // 16 bit LSB - MII Read Data

// Bank 3
#define MAADR1 BANK_REG(3, 0x00) | SPRD_MASK // 8 bit - MAC Address - warning unexpected order of MAADR1-MAADR6!
#define MAADR0 BANK_REG(3, 0x01) | SPRD_MASK // 8 bit
#define MAADR3 BANK_REG(3, 0x02) | SPRD_MASK // 8 bit
#define MAADR2 BANK_REG(3, 0x03) | SPRD_MASK // 8 bit
#define MAADR5 BANK_REG(3, 0x04) | SPRD_MASK // 8 bit
#define MAADR4 BANK_REG(3, 0x05) | SPRD_MASK // 8 bit
#define EBSTSD BANK_REG(3, 0x06)             // 8 bit - Built-in Self-Test Fill Seed
#define EBSTCON BANK_REG(3, 0x07)            // 8 bit - Ethernet self-test control register
#define EBSTCS BANK_REG(3, 0x08)             // 16 bit LSB - Built-in Self-Test Checksum
#define MISTAT BANK_REG(3, 0x0A) | SPRD_MASK // 8 bit
#define EREVID BANK_REG(3, 0x12)             // 8 bit - Ethernet Revision ID
#define ECOCON BANK_REG(3, 0x15)             // 8 bit - Clock output control register
#define EFLOCON BANK_REG(3, 0x17)            // 8 bit - Ethernet flow control register
#define EPAUS BANK_REG(3, 0x18)              // 16 bit LSB - Pause Timer Value

/***
 *  TABLE 3-2: ENC28J60 CONTROL REGISTER SUMMARY
 *
 *  Bitfield defines for control registers
 ***/
// EIE register - page 65
#define EIE_RXERIE 0x01
#define EIE_TXERIE 0x02
//#define EIE_WOLIE 0x04
#define EIE_TXIE 0x08
#define EIE_LINKIE 0x10
#define EIE_DMAIE 0x20
#define EIE_PKTIE 0x40
#define EIE_INTIE 0x80

// EIR register - page 66
#define EIR_RXERIF 0x01
#define EIR_TXERIF 0x02
//#define EIR_WOLIF 0x04
#define EIR_TXIF 0x08
#define EIR_LINKIF 0x10
#define EIR_DMAIF 0x20
#define EIR_PKTIF 0x40

// ESTAT register - page 64
#define ESTAT_CLKRDY 0x01
//#define ESTAT_TXABRT 0x02
#define ESTAT_TXANRT 0x02
#define ESTAT_RXBUSY 0x04
#define ESTAT_LATECOL 0x10
#define ESTAT_BUFER 0x40
#define ESTAT_INT 0x80

// ECON2 register - page 16
#define ECON2_VRPS 0x08
#define ECON2_PWRSV 0x20
#define ECON2_PKTDEC 0x40
#define ECON2_AUTOINC 0x80

// ECON1 register - page 15
#define ECON1_BSEL0 0x01
#define ECON1_BSEL1 0x02
#define ECON1_RXEN 0x04
#define ECON1_TXRTS 0x08
#define ECON1_CSUMEN 0x10
#define ECON1_DMAST 0x20
#define ECON1_RXRST 0x40
#define ECON1_TXRST 0x80

// ERXFCON register - page 48
#define ERXFCON_BCEN 0x01
#define ERXFCON_MCEN 0x02
#define ERXFCON_HTEN 0x04
#define ERXFCON_MPEN 0x08
#define ERXFCON_PMEN 0x10
#define ERXFCON_CRCEN 0x20
#define ERXFCON_ANDOR 0x40
#define ERXFCON_UCEN 0x80

// MACON1 register - page 34
#define MACON1_MARXEN 0x01
#define MACON1_PASSALL 0x02
#define MACON1_RXPAUS 0x04
#define MACON1_TXPAUS 0x08

// MACON3 register - page 35
#define MACON3_FULDPX 0x01
#define MACON3_FRMLNEN 0x02
#define MACON3_HFRMEN 0x04
#define MACON3_PHDREN 0x08
#define MACON3_TXCRCEN 0x10
#define MACON3_PADCFG0 0x20
#define MACON3_PADCFG1 0x40
#define MACON3_PADCFG2 0x80

// MACON4 register - page 36
#define MACON4_NOBKOFF 0x10
#define MACON4_BPEN 0x20
#define MACON4_DEFER 0x40

// MICMD register - page 21
#define MICMD_MIIRD 0x01
#define MICMD_MIISCAN 0x02

// MISTAT register - page 21
#define MISTAT_BUSY 0x01
#define MISTAT_SCAN 0x02
#define MISTAT_NVALID 0x04

// EBSTCON register - page 75
#define EBSTCON_PSV2 0x80
#define EBSTCON_PSV1 0x40
#define EBSTCON_PSV0 0x20
#define EBSTCON_PSEL 0x10
#define EBSTCON_TMSEL1 0x08
#define EBSTCON_TMSEL0 0x04
#define EBSTCON_TME 0x02
#define EBSTCON_BISTST 0x01

// EFLOCON register - page 56
#define EFLOCON_FULDPXS 0x04
#define EFLOCON_FCEN1 0x02
#define EFLOCON_FCEN0 0x01

/*** 
 *  Table 8-3: Receive Status Vectors
 ***/
#define RECV_STAT_RECEIVE_VLAN_TYPE_DETECTED 0x4000
#define RECV_STAT_RECEIVE_UNKNOWN_OPCODE 0x2000
#define RECV_STAT_RECEIVE_PAUSE_CONTROL_FRAME 0x1000
#define RECV_STAT_RECEIVE_CONTROL_FRAME 0x0800
#define RECV_STAT_DRIBBLE_NIBBLE 0x0400
#define RECV_STAT_RECEIVE_BROADCAST_PACKET 0x0200
#define RECV_STAT_RECEIVE_MULTICAST_PACKET 0x0100
#define RECV_STAT_RECEIVED_OK 0x0080
#define RECV_STAT_LENGTH_OUT_OF_RANGE 0x0040
#define RECV_STAT_LENGTH_CHECK_ERROR 0x0020
#define RECV_STAT_CRC_ERROR 0x0010
#define RECV_STAT_CARRIER_EVENT_PREVIOUSLY_SEEN 0x0004
#define RECV_STAT_LONG_EVENT 0x0001

/***
 *  Table 3-3: ENC28J60 PHY REGISTER SUMMARY
 ***/
#define PHCON1 0x00  // PHY control register 1
#define PHSTAT1 0x01 // Physical layer status register
#define PHID1 0x02   // PHY Identifier
#define PHID2 0x03   // PHY Identifier
#define PHCON2 0x10  // PHY control register 2
#define PHSTAT2 0x11 // Physical layer status register 2
#define PHIE 0x12    // PHY interrupt enable register
#define PHIR 0x13    // PHY interrupt request (flag) register
#define PHLCON 0x14  // PHY module LED control register

// PHCON1 register - page 61
#define PHCON1_PDPXMD 0x0100
#define PHCON1_PPWRSV 0x0800
#define PHCON1_PLOOPBK 0x4000
#define PHCON1_PRST 0x8000

// PHSTAT1 register - page 23
#define PHSTAT1_JBSTAT 0x0002
#define PHSTAT1_LLSTAT 0x0004
#define PHSTAT1_PHDPX 0x0800
#define PHSTAT1_PFDPX 0x1000

// PHCON2 register - page 61
#define PHCON2_HDLDIS 0x0100
#define PHCON2_JABBER 0x0400
#define PHCON2_TXDIS 0x2000
#define PHCON2_FRCLNK 0x4000

// PHSTAT2 register - page 24
#define PHSTAT2_PLRITY 0x0020
#define PHSTAT2_DPXSTAT 0x0200
#define PHSTAT2_LSTAT 0x0400
#define PHSTAT2_COLSTAT 0x0800
#define PHSTAT2_RXSTAT 0x1000
#define PHSTAT2_TXSTAT 0x2000

// PHIE register - page 67
#define PHIE_PGEIE 0x0002
#define PHIE_PLNKIE 0x0010

// PHIR register - page 67
#define PHIR_PGIF 0x0004
#define PHIR_PLNKIF 0x0010

// PHLCON register - page 9
#define PHLCON_STRCH 0x0002
#define PHLCON_LFRQ0 0x0004
#define PHLCON_LFRQ1 0x0008
// LEDB Configuration bits
#define PHLCON_LBCFG_SHIFT 4
#define PHLCON_LBCFG0 0x0010
#define PHLCON_LBCFG1 0x0020
#define PHLCON_LBCFG2 0x0040
#define PHLCON_LBCFG3 0x0080
// LEDA Configuration bits
#define PHLCON_LACFG_SHIFT 8
#define PHLCON_LACFG0 0x0100
#define PHLCON_LACFG1 0x0200
#define PHLCON_LACFG2 0x0400
#define PHLCON_LACFG3 0x0800

/***
 *  SPI API
 ***/
static SPI_HandleTypeDef hspi1;

static void ENC28j60_chip_select(void) {
  HAL_GPIO_WritePin(ENC28J60_CS_GPIO_Port, ENC28J60_CS_Pin, GPIO_PIN_RESET);
}
static void ENC28j60_chip_deselect(void) {
  HAL_GPIO_WritePin(ENC28J60_CS_GPIO_Port, ENC28J60_CS_Pin, GPIO_PIN_SET);
}

static uint8_t ENC28J60_spi_read() {
  uint8_t spiRxData;
  HAL_SPI_Receive(&hspi1, &spiRxData, sizeof(spiRxData), 100);
  return spiRxData;
}

static void ENC28J60_spi_write(uint8_t data) {
  uint8_t spiData = data;
  HAL_SPI_Transmit(&hspi1, &spiData, sizeof(spiData), 100);
}

/***
 *  ENC28J60 communication API
 ***/
static uint8_t ENC28J60_readOp(uint8_t cmd, uint8_t adr) {
  ENC28j60_chip_select();
  ENC28J60_spi_write(cmd | (adr & REG_MASK));
  if (adr & 0x80)        // throw out dummy byte
    ENC28J60_spi_read(); // when reading MII/MAC register
  uint8_t data = ENC28J60_spi_read();
  ENC28j60_chip_deselect();
  return data;
}

// Generic SPI write command
static void ENC28J60_writeOp(uint8_t cmd, uint8_t adr, uint8_t data) {
  ENC28j60_chip_select();
  ENC28J60_spi_write(cmd | (adr & REG_MASK));
  ENC28J60_spi_write(data);
  ENC28j60_chip_deselect();
}

// Initiate software reset
void ENC28J60_soft_reset() {
  ENC28j60_chip_select();
  ENC28J60_spi_write(ENC28J60_SOFT_RESET_COMMAND);
  ENC28j60_chip_deselect();
  sENC28J60_current_bank = 0;
  // The CLKRDY does not work. See Rev. B4 Silicon Errata point. Just wait.
  //while (!(ENC28J60_readOp(ENC28J60_READ_CTRL_REG, ESTAT) & ESTAT_CLKRDY))
  //  ;
  HAL_Delay(10);
}

/***
 *  ENC28J60 API
 ***/
void ENC28J60_select_bank(uint8_t adr) {
  if ((adr & REG_MASK) < ENC28J60_COMMON_REGOFFSET) {
    uint8_t bank = (adr >> 5) & 0x03; //BSEL1|BSEL0=0x03
    if (bank != sENC28J60_current_bank) {
      ENC28J60_writeOp(ENC28J60_BIT_FIELD_CLR, ECON1, 0x03);
      ENC28J60_writeOp(ENC28J60_BIT_FIELD_SET, ECON1, bank);
      sENC28J60_current_bank = bank;
    }
  }
}

static uint8_t ENC28J60_readReg8(uint8_t adr) {
  ENC28J60_select_bank(adr);
  return ENC28J60_readOp(ENC28J60_READ_CTRL_REG, adr);
}

static uint16_t ENC28J60_readReg16(uint8_t adr) {
  ENC28J60_select_bank(adr);
  return ENC28J60_readOp(ENC28J60_READ_CTRL_REG, adr) | (ENC28J60_readOp(ENC28J60_READ_CTRL_REG, adr + 1) << 8);
}

static void ENC28J60_writeReg8(uint8_t adr, uint8_t arg) {
  ENC28J60_select_bank(adr);
  ENC28J60_writeOp(ENC28J60_WRITE_CTRL_REG, adr, arg);
}

static void ENC28J60_writeReg16(uint8_t adr, uint16_t arg) {
  ENC28J60_select_bank(adr);
  ENC28J60_writeOp(ENC28J60_WRITE_CTRL_REG, adr, arg);
  ENC28J60_writeOp(ENC28J60_WRITE_CTRL_REG, adr + 1, arg >> 8);
}

static void ENC28J60_bitfieldClear(uint8_t adr, uint8_t mask) {
  ENC28J60_select_bank(adr);
  ENC28J60_writeOp(ENC28J60_BIT_FIELD_CLR, adr, mask);
}

static void ENC28J60_bitfieldSet(uint8_t adr, uint8_t mask) {
  ENC28J60_writeOp(ENC28J60_BIT_FIELD_SET, adr, mask);
}

static void ENC28J60_readBuffer(void *buf, uint16_t len) {
  ENC28j60_chip_select();
  ENC28J60_spi_write(ENC28J60_READ_BUF_MEM_COMMAND);
  uint8_t *p = (uint8_t *)buf;
  while (len--)
    *(p++) = ENC28J60_spi_read();
  ENC28j60_chip_deselect();
}

static void ENC28J60_writeBuffer(const void *buf, uint16_t len) {
  ENC28j60_chip_select();
  ENC28J60_spi_write(ENC28J60_WRITE_BUF_MEM_COMMAND);
  uint8_t *p = (uint8_t *)buf;
  while (len--)
    ENC28J60_spi_write(*(p++));
  ENC28j60_chip_deselect();
}

static uint16_t ENC28J60_readPhy16(uint8_t adr) {
  ENC28J60_writeReg8(MIREGADR, adr);
  ENC28J60_bitfieldSet(MICMD, MICMD_MIIRD);
  while (ENC28J60_readReg8(MISTAT) & MISTAT_BUSY)
    ;
  ENC28J60_bitfieldClear(MICMD, MICMD_MIIRD);
  return ENC28J60_readReg16(MIRD);
}

static void ENC28J60_writePhy16(uint8_t adr, uint16_t data) {
  ENC28J60_writeReg8(MIREGADR, adr);
  ENC28J60_writeReg16(MIWR, data);
  while (ENC28J60_readReg8(MISTAT) & MISTAT_BUSY)
    ;
}

/***
 *  Callbacks from HAL to initialize/deinitialize the SPI
 ***/
void HAL_SPI_MspInit(SPI_HandleTypeDef *spiHandle) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if (spiHandle->Instance == SPI1) {
    // SPI1 clock enable
    __HAL_RCC_SPI1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**SPI1 GPIO Configuration    
    PA5     ------> SPI1_SCK
    PA6     ------> SPI1_MISO
    PA7     ------> SPI1_MOSI 
    */
    GPIO_InitStruct.Pin = ENC28J60_SCK_Pin | EBC27J60_MOSI_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = ENC28J60_MISO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(ENC28J60_MISO_GPIO_Port, &GPIO_InitStruct);
  }
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef *spiHandle) {
  if (spiHandle->Instance == SPI1) {
    // Peripheral clock disable
    __HAL_RCC_SPI1_CLK_DISABLE();

    /**SPI1 GPIO Configuration    
    PA5     ------> SPI1_SCK
    PA6     ------> SPI1_MISO
    PA7     ------> SPI1_MOSI 
    */
    HAL_GPIO_DeInit(GPIOA, ENC28J60_SCK_Pin | ENC28J60_MISO_Pin | EBC27J60_MOSI_Pin);
  }
}

/***
 *  Setup the ENC28J60
 ***/
void ENC28J60_init(const uint8_t *macadr) {

  // Initialize SPI
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK) {
    return;
  }

  __HAL_RCC_GPIOA_CLK_ENABLE();

  // Configure GPIO pin : PtPin
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = ENC28J60_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ENC28J60_CS_GPIO_Port, &GPIO_InitStruct);
  ENC28j60_chip_deselect();

  ENC28J60_soft_reset();

  // Setup Ethernet RX/TX buffer
  ENC28J60_writeReg16(ERXST, ENC28J60_RXBEGIN);
  ENC28J60_writeReg16(ERXRDPT, ENC28J60_RXBEGIN);
  ENC28J60_writeReg16(ERXND, ENC28J60_RXEND);

  // Setup MAC
  ENC28J60_writeReg8(MACON1, MACON1_TXPAUS | MACON1_RXPAUS | MACON1_MARXEN);                    // Enable RX and flow control
  ENC28J60_writeReg8(MACON3, MACON3_PADCFG0 | MACON3_TXCRCEN | MACON3_FRMLNEN | MACON3_FULDPX); // Enable padding for short frames, CRC, frame length check and full-duplex
  ENC28J60_writeReg16(MAMXFL, ENC28J60_MAX_FRAMELEN);
  ENC28J60_writeReg8(MABBIPG, 0x15);     // Back-to-Back Inter-Packet Gap register (values: 0x15 = full-duplex, 0x12 = half-duplex)
  ENC28J60_writeReg16(MAIPG, 0x0c12);    // default values, see "MAC Initialization Settings" in the EN28J60 spec
  ENC28J60_writeReg8(MAADR5, macadr[0]); // Set MAC address
  ENC28J60_writeReg8(MAADR4, macadr[1]);
  ENC28J60_writeReg8(MAADR3, macadr[2]);
  ENC28J60_writeReg8(MAADR2, macadr[3]);
  ENC28J60_writeReg8(MAADR1, macadr[4]);
  ENC28J60_writeReg8(MAADR0, macadr[5]);

  // Setup PHY
  ENC28J60_writePhy16(PHCON1, PHCON1_PDPXMD);        // PHY operates in Full-Duplex mode
  ENC28J60_writePhy16(PHCON2, PHCON2_HDLDIS);        // Disable loopback
  ENC28J60_writePhy16(PHLCON, PHLCON_DEFAULT_VALUE); // LED configuration

  // Enable RX packets
  ENC28J60_select_bank(ECON1);
  ENC28J60_bitfieldSet(EIE, EIE_INTIE|EIE_PKTIE);
  ENC28J60_bitfieldSet(ECON1, ECON1_RXEN);
}

//
int ENC28J60_isLinkUp() {
  return (ENC28J60_readPhy16(PHSTAT2) & PHSTAT2_LSTAT) != 0; // Link is up and has been up continously since PHSTAT1 was last read
}

void ENC28J60_sendPacket(const void *data, uint16_t len) {
  while (ENC28J60_readReg8(ECON1) & ECON1_TXRTS) {
    // TXRTS may not clear - ENC28J60 bug. We must reset transmit logic in cause of TX error
    if (ENC28J60_readReg8(EIR) & EIR_TXERIF) {
      ENC28J60_bitfieldSet(ECON1, ECON1_TXRST);
      ENC28J60_bitfieldClear(ECON1, ECON1_TXRST);
    }
  }
  // write pointer to the beginning of the TX buffer
  ENC28J60_writeReg16(EWRPT, ENC28J60_TXBEGIN);
  ENC28J60_writeOp(ENC28J60_WRITE_BUF_MEM_COMMAND, 0, 0x00); // per-packet control byte (0x00 = use MACON3 settings)
  ENC28J60_writeBuffer(data, len);                           // copy the packet into the TX buffer
  ENC28J60_writeReg16(ETXST, ENC28J60_TXBEGIN);              // beginning of the package
  ENC28J60_writeReg16(ETXND, ENC28J60_TXBEGIN + len);        // end of the package
  ENC28J60_bitfieldSet(ECON1, ECON1_TXRTS);                  // trigger the transmission

  // wait until transmission has finished; referring to the data sheet and
  // to the errata (Errata Issue 13; Example 1) you only need to wait until either
  // TXIF or TXERIF gets set; however this leads to hangs; apparently Microchip
  // realized this and in later implementations of their tcp/ip stack they introduced
  // a counter to avoid hangs; of course they didn't update the errata sheet
  int timeout = 0;
  while ((ENC28J60_readReg8(EIR) & (EIR_TXIF | EIR_TXERIF)) == 0 && timeout++ <= 1000U)
    ;
  if (!(ENC28J60_readReg8(EIR) & EIR_TXERIF) && timeout < 1000U) {
    return; // success
  }

  // cancel previous transmission if stuck
  ENC28J60_bitfieldClear(ECON1, ECON1_TXRTS);
}

uint16_t ENC28J60_receivePacket(void *buf, uint16_t buflen) {
  static uint16_t sCurrentRXPointer = ENC28J60_RXBEGIN;
  static int sRequestSet = 0;
  if (sRequestSet) {
    if (sCurrentRXPointer == 0) {
      ENC28J60_writeReg16(ERXRDPT, ENC28J60_RXEND); // advance the read pointer
    } else {
      ENC28J60_writeReg16(ERXRDPT, sCurrentRXPointer - 1); // advance the read pointer
    }
    sRequestSet = 0;
  }
  uint16_t len = 0;
  if (ENC28J60_readReg8(EPKTCNT)) {                // Ethernet Packet Count != 0, which means that we did receive at least one package
    ENC28J60_writeReg16(ERDPT, sCurrentRXPointer); // set the read pointer to the last position
    struct {                                       // each package has a 6 byte header
      uint16_t nextPacketPointer;
      uint16_t packageLength;
      uint16_t statusFlags;
    } header;
    ENC28J60_readBuffer(&header, sizeof(header));
    sCurrentRXPointer = header.nextPacketPointer;
    if (header.statusFlags & RECV_STAT_RECEIVED_OK) {
      len = header.packageLength - 4; // ignore 4 bytes of CRC
      if (len > buflen)
        len = buflen;
      ENC28J60_readBuffer(buf, len); // transfer the package into our buffer
    }
    sRequestSet = 1;
    ENC28J60_bitfieldSet(ECON2, ECON2_PKTDEC); // decrement the package counter
  }
  return len;
}