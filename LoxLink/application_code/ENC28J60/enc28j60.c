#include "enc28j60.h"

void debug_print_buffer_eth(const void *data, size_t size, const char *header) {
  if (size > 64)
    size = 64;
  const int LineLength = 16;
  const uint8_t *dp = (const uint8_t *)data;
  int match = 0;
  // to
  if(dp[0] == 0x11 && dp[0+1] == 0x11 && dp[0+2] == 0x11 && dp[0+3] == 0x11 && dp[0+4] == 0x11 && dp[0+5] == 0x11)
    match++;
  // from
  if(dp[6] == 0x11 && dp[6+1] == 0x11 && dp[6+2] == 0x11 && dp[6+3] == 0x11 && dp[6+4] == 0x11 && dp[6+5] == 0x11)
    match++;
  if(!match)
    return;
  for (int loffset = 0; loffset < size; loffset += LineLength) {
    if (header)
      printf("%s ", header);
    printf("%04x : ", loffset);
    for (int i = 0; i < LineLength; ++i) {
      int offset = loffset + i;
      if (offset < size) {
        printf("%02x ", dp[offset]);
      } else {
        printf("   ");
      }
    }
    putchar(' ');
    for (int i = 0; i < LineLength; ++i) {
      int offset = loffset + i;
      if (offset >= size)
        break;
      uint8_t c = dp[offset];
      if (c < 0x20 || c >= 0x7f)
        c = '.';
      putchar(c);
    }
    printf("\n");
  }
}

/***
 *  SPI
 ***/
static SPI_HandleTypeDef hspi1;

static void enc28j60_select(void) {
  HAL_GPIO_WritePin(ENC28J60_CS_GPIO_Port, ENC28J60_CS_Pin, GPIO_PIN_RESET);
}
static void enc28j60_release(void) {
  HAL_GPIO_WritePin(ENC28J60_CS_GPIO_Port, ENC28J60_CS_Pin, GPIO_PIN_SET);
}

volatile uint8_t enc28j60_current_bank = 0;
volatile uint16_t enc28j60_rxrdpt = 0;

uint8_t enc28j60_rx() {
  uint8_t spiRxData;
  HAL_SPI_Receive(&hspi1, &spiRxData, sizeof(spiRxData), 100);
  return spiRxData;
}

void enc28j60_tx(uint8_t data) {
  uint8_t spiData = data;
  HAL_SPI_Transmit(&hspi1, &spiData, sizeof(spiData), 100);
}

// Generic SPI read command
uint8_t enc28j60_read_op(uint8_t cmd, uint8_t adr) {
  enc28j60_select();
  enc28j60_tx(cmd | (adr & ENC28J60_ADDR_MASK));
  if (adr & 0x80)  // throw out dummy byte
    enc28j60_rx(); // when reading MII/MAC register
  uint8_t data = enc28j60_rx();
  enc28j60_release();
  return data;
}

// Generic SPI write command
void enc28j60_write_op(uint8_t cmd, uint8_t adr, uint8_t data) {
  enc28j60_select();
  enc28j60_tx(cmd | (adr & ENC28J60_ADDR_MASK));
  enc28j60_tx(data);
  enc28j60_release();
}

// Initiate software reset
void enc28j60_soft_reset() {
  enc28j60_select();
  enc28j60_tx(ENC28J60_SPI_SC);
  enc28j60_release();

  enc28j60_current_bank = 0;
  HAL_Delay(1); // Wait until device initializes
}

/*
 * Memory access
 */

// Set register bank
void enc28j60_set_bank(uint8_t adr) {
  if ((adr & ENC28J60_ADDR_MASK) < ENC28J60_COMMON_CR) {
    uint8_t bank = (adr >> 5) & 0x03; //BSEL1|BSEL0=0x03
    if (bank != enc28j60_current_bank) {
      enc28j60_write_op(ENC28J60_SPI_BFC, ECON1, 0x03);
      enc28j60_write_op(ENC28J60_SPI_BFS, ECON1, bank);
      enc28j60_current_bank = bank;
    }
  }
}

// Read register
uint8_t enc28j60_rcr(uint8_t adr) {
  enc28j60_set_bank(adr);
  return enc28j60_read_op(ENC28J60_SPI_RCR, adr);
}

// Read register pair
uint16_t enc28j60_rcr16(uint8_t adr) {
  enc28j60_set_bank(adr);
  return enc28j60_read_op(ENC28J60_SPI_RCR, adr) |
         (enc28j60_read_op(ENC28J60_SPI_RCR, adr + 1) << 8);
}

// Write register
void enc28j60_wcr(uint8_t adr, uint8_t arg) {
  enc28j60_set_bank(adr);
  enc28j60_write_op(ENC28J60_SPI_WCR, adr, arg);
}

// Write register pair
void enc28j60_wcr16(uint8_t adr, uint16_t arg) {
  enc28j60_set_bank(adr);
  enc28j60_write_op(ENC28J60_SPI_WCR, adr, arg);
  enc28j60_write_op(ENC28J60_SPI_WCR, adr + 1, arg >> 8);
}

// Clear bits in register (reg &= ~mask)
void enc28j60_bfc(uint8_t adr, uint8_t mask) {
  enc28j60_set_bank(adr);
  enc28j60_write_op(ENC28J60_SPI_BFC, adr, mask);
}

// Set bits in register (reg |= mask)
void enc28j60_bfs(uint8_t adr, uint8_t mask) {
  enc28j60_set_bank(adr);
  enc28j60_write_op(ENC28J60_SPI_BFS, adr, mask);
}

// Read Rx/Tx buffer (at ERDPT)
void enc28j60_read_buffer(uint8_t *buf, uint16_t len) {
  enc28j60_select();
  enc28j60_tx(ENC28J60_SPI_RBM);
  while (len--)
    *(buf++) = enc28j60_rx();
  enc28j60_release();
}

// Write Rx/Tx buffer (at EWRPT)
void enc28j60_write_buffer(uint8_t *buf, uint16_t len) {
  enc28j60_select();
  enc28j60_tx(ENC28J60_SPI_WBM);
  while (len--)
    enc28j60_tx(*(buf++));
  enc28j60_release();
}

// Read PHY register
uint16_t enc28j60_read_phy(uint8_t adr) {
  enc28j60_wcr(MIREGADR, adr);
  enc28j60_bfs(MICMD, MICMD_MIIRD);
  while (enc28j60_rcr(MISTAT) & MISTAT_BUSY)
    ;
  enc28j60_bfc(MICMD, MICMD_MIIRD);
  return enc28j60_rcr16(MIRD);
}

// Write PHY register
void enc28j60_write_phy(uint8_t adr, uint16_t data) {
  enc28j60_wcr(MIREGADR, adr);
  enc28j60_wcr16(MIWR, data);
  while (enc28j60_rcr(MISTAT) & MISTAT_BUSY)
    ;
}

/*
 * Init & packet Rx/Tx
 */
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

void enc28j60_init(const uint8_t *macadr) {
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
    while (1) {
    }
  }

  __HAL_RCC_GPIOA_CLK_ENABLE();

  // Configure GPIO pin : PtPin
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = ENC28J60_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ENC28J60_CS_GPIO_Port, &GPIO_InitStruct);
  enc28j60_release();

  // Reset ENC28J60
  enc28j60_soft_reset();

  // Setup Rx/Tx buffer
  enc28j60_wcr16(ERXST, ENC28J60_RXSTART);
  enc28j60_wcr16(ERXRDPT, ENC28J60_RXSTART);
  enc28j60_wcr16(ERXND, ENC28J60_RXEND);
  enc28j60_rxrdpt = ENC28J60_RXSTART;

  // Setup MAC
  enc28j60_wcr(MACON1, MACON1_TXPAUS |                                     // Enable flow control
                         MACON1_RXPAUS | MACON1_MARXEN);                   // Enable MAC Rx
  enc28j60_wcr(MACON2, 0);                                                 // Clear reset
  enc28j60_wcr(MACON3, MACON3_PADCFG0 |                                    // Enable padding,
                         MACON3_TXCRCEN | MACON3_FRMLNEN | MACON3_FULDPX); // Enable crc & frame len chk
  enc28j60_wcr16(MAMXFL, ENC28J60_MAXFRAME);
  enc28j60_wcr(MABBIPG, 0x15); // Set inter-frame gap
  enc28j60_wcr(MAIPGL, 0x12);
  enc28j60_wcr(MAIPGH, 0x0c);
  enc28j60_wcr(MAADR5, macadr[0]); // Set MAC address
  enc28j60_wcr(MAADR4, macadr[1]);
  enc28j60_wcr(MAADR3, macadr[2]);
  enc28j60_wcr(MAADR2, macadr[3]);
  enc28j60_wcr(MAADR1, macadr[4]);
  enc28j60_wcr(MAADR0, macadr[5]);

  // Setup PHY
  enc28j60_write_phy(PHCON1, PHCON1_PDPXMD); // Force full-duplex mode
  enc28j60_write_phy(PHCON2, PHCON2_HDLDIS); // Disable loopback
  enc28j60_write_phy(PHLCON, PHLCON_LACFG2 | // Configure LED ctrl
                               PHLCON_LBCFG2 | PHLCON_LBCFG1 | PHLCON_LBCFG0 |
                               PHLCON_LFRQ0 | PHLCON_STRCH);

  // Enable Rx packets
  enc28j60_bfs(ECON1, ECON1_RXEN);
}

void enc28j60_send_packet(uint8_t *data, uint16_t len) {
  debug_print_buffer_eth(data, len, "S:");
  while (enc28j60_rcr(ECON1) & ECON1_TXRTS) {
    // TXRTS may not clear - ENC28J60 bug. We must reset
    // transmit logic in cause of Tx error
    if (enc28j60_rcr(EIR) & EIR_TXERIF) {
      enc28j60_bfs(ECON1, ECON1_TXRST);
      enc28j60_bfc(ECON1, ECON1_TXRST);
    }
  }

  enc28j60_wcr16(EWRPT, ENC28J60_TXSTART);
  enc28j60_write_buffer((uint8_t *)"\x00", 1);
  enc28j60_write_buffer(data, len);

  enc28j60_wcr16(ETXST, ENC28J60_TXSTART);
  enc28j60_wcr16(ETXND, ENC28J60_TXSTART + len);

  enc28j60_bfs(ECON1, ECON1_TXRTS); // Request packet send
}

uint16_t enc28j60_recv_packet(uint8_t *buf, uint16_t buflen) {
  uint16_t len = 0;

  if (enc28j60_rcr(EPKTCNT)) {
    enc28j60_wcr16(ERDPT, enc28j60_rxrdpt);

    enc28j60_read_buffer((uint8_t *)&enc28j60_rxrdpt, sizeof(enc28j60_rxrdpt));
    uint16_t rxlen;
    enc28j60_read_buffer((uint8_t *)&rxlen, sizeof(rxlen));
    uint16_t status;
    enc28j60_read_buffer((uint8_t *)&status, sizeof(status));

    if (status & 0x80) //success
    {
      len = rxlen - 4; //throw out crc
      if (len > buflen)
        len = buflen;
      enc28j60_read_buffer(buf, len);
    }

    // Set Rx read pointer to next packet
    uint16_t temp = (enc28j60_rxrdpt - 1) & ENC28J60_BUFEND;
    enc28j60_wcr16(ERXRDPT, temp);

    // Decrement packet counter
    enc28j60_bfs(ECON2, ECON2_PKTDEC);
  }
  return len;
}