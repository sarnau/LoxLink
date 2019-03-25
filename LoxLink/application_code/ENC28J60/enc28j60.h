#pragma once

#include "stm32f1xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

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

// Init ENC28J60
void enc28j60_init(const uint8_t *macadr);

// Snd/Rcv packets
void enc28j60_send_packet(uint8_t *data, uint16_t len);
uint16_t enc28j60_recv_packet(uint8_t *buf, uint16_t buflen);

// R/W control registers
uint8_t enc28j60_rcr(uint8_t adr);
void enc28j60_wcr(uint8_t adr, uint8_t arg);
uint16_t enc28j60_rcr16(uint8_t adr);
void enc28j60_wcr16(uint8_t adr, uint16_t arg);
void enc28j60_bfc(uint8_t adr, uint8_t mask); // Clr bits (reg &= ~mask)
void enc28j60_bfs(uint8_t adr, uint8_t mask); // Set bits (reg |= mask)

// R/W Rx/Tx buffer
void enc28j60_read_buffer(uint8_t *buf, uint16_t len);
void enc28j60_write_buffer(uint8_t *buf, uint16_t len);

// R/W PHY reg
uint16_t enc28j60_read_phy(uint8_t adr);
void enc28j60_write_phy(uint8_t adr, uint16_t data);

#define ENC28J60_BUFSIZE 0x2000
#define ENC28J60_RXSIZE 0x1A00
#define ENC28J60_BUFEND (ENC28J60_BUFSIZE - 1)

#define ENC28J60_MAXFRAME 600

#define ENC28J60_RXSTART 0
#define ENC28J60_RXEND (ENC28J60_RXSIZE - 1)
#define ENC28J60_TXSTART ENC28J60_RXSIZE

#define ENC28J60_SPI_RCR 0x00
#define ENC28J60_SPI_RBM 0x3A
#define ENC28J60_SPI_WCR 0x40
#define ENC28J60_SPI_WBM 0x7A
#define ENC28J60_SPI_BFS 0x80
#define ENC28J60_SPI_BFC 0xA0
#define ENC28J60_SPI_SC 0xFF

#define ENC28J60_ADDR_MASK 0x1F
#define ENC28J60_COMMON_CR 0x1B

/*
 * Main registers
 */

#define EIE 0x1B
#define EIR 0x1C
#define ESTAT 0x1D
#define ECON2 0x1E
#define ECON1 0x1F

// Buffer read pointer
#define ERDPTL 0x00
#define ERDPTH 0x01
#define ERDPT ERDPTL

// Buffer write pointer
#define EWRPTL 0x02
#define EWRPTH 0x03
#define EWRPT EWRPTL

// Tx packet start pointer
#define ETXSTL 0x04
#define ETXSTH 0x05
#define ETXST ETXSTL

// Tx packet end pointer
#define ETXNDL 0x06
#define ETXNDH 0x07
#define ETXND ETXNDL

// Rx FIFO start pointer
#define ERXSTL 0x08
#define ERXSTH 0x09
#define ERXST ERXSTL

// Rx FIFO end pointer
#define ERXNDL 0x0A
#define ERXNDH 0x0B
#define ERXND ERXNDL

// Rx FIFO read pointer
#define ERXRDPTL 0x0C
#define ERXRDPTH 0x0D
#define ERXRDPT ERXRDPTL

// Rx FIFO write pointer
#define ERXWRPTL 0x0E
#define ERXWRPTH 0x0F
#define ERXWRPT ERXWRPTL

// DMA source block start pointer
#define EDMASTL 0x10
#define EDMASTH 0x11
#define EDMAST EDMASTL

// DMA source block end pointer
#define EDMANDL 0x12
#define EDMANDH 0x13
#define EDMAND EDMANDL

// DMA destination pointer
#define EDMADSTL 0x14
#define EDMADSTH 0x15
#define EDMADST EDMADSTL

// DMA checksum
#define EDMACSL 0x16
#define EDMACSH 0x17
#define EDMACS EDMACSL

// Hash table registers
#define EHT0 (0x00 | 0x20)
#define EHT1 (0x01 | 0x20)
#define EHT2 (0x02 | 0x20)
#define EHT3 (0x03 | 0x20)
#define EHT4 (0x04 | 0x20)
#define EHT5 (0x05 | 0x20)
#define EHT6 (0x06 | 0x20)
#define EHT7 (0x07 | 0x20)

// Pattern match registers
#define EPMM0 (0x08 | 0x20)
#define EPMM1 (0x09 | 0x20)
#define EPMM2 (0x0A | 0x20)
#define EPMM3 (0x0B | 0x20)
#define EPMM4 (0x0C | 0x20)
#define EPMM5 (0x0D | 0x20)
#define EPMM6 (0x0E | 0x20)
#define EPMM7 (0x0F | 0x20)
#define EPMCSL (0x10 | 0x20)
#define EPMCSH (0x11 | 0x20)
#define EPMOL (0x14 | 0x20)
#define EPMOH (0x15 | 0x20)

// Wake-on-LAN interrupt registers
#define EWOLIE (0x16 | 0x20)
#define EWOLIR (0x17 | 0x20)

// Receive filters mask
#define ERXFCON (0x18 | 0x20)

// Packet counter
#define EPKTCNT (0x19 | 0x20)

// MAC control registers
#define MACON1 (0x00 | 0x40 | 0x80)
#define MACON2 (0x01 | 0x40 | 0x80)
#define MACON3 (0x02 | 0x40 | 0x80)
#define MACON4 (0x03 | 0x40 | 0x80)

// MAC Back-to-back gap
#define MABBIPG (0x04 | 0x40 | 0x80)

// MAC Non back-to-back gap
#define MAIPGL (0x06 | 0x40 | 0x80)
#define MAIPGH (0x07 | 0x40 | 0x80)

// Collision window & rexmit timer
#define MACLCON1 (0x08 | 0x40 | 0x80)
#define MACLCON2 (0x09 | 0x40 | 0x80)

// Max frame length
#define MAMXFLL (0x0A | 0x40 | 0x80)
#define MAMXFLH (0x0B | 0x40 | 0x80)
#define MAMXFL MAMXFLL

// MAC-PHY support register
#define MAPHSUP (0x0D | 0x40 | 0x80)
#define MICON (0x11 | 0x40 | 0x80)

// MII registers
#define MICMD (0x12 | 0x40 | 0x80)
#define MIREGADR (0x14 | 0x40 | 0x80)

#define MIWRL (0x16 | 0x40 | 0x80)
#define MIWRH (0x17 | 0x40 | 0x80)
#define MIWR MIWRL

#define MIRDL (0x18 | 0x40 | 0x80)
#define MIRDH (0x19 | 0x40 | 0x80)
#define MIRD MIRDL

// MAC Address
#define MAADR1 (0x00 | 0x60 | 0x80)
#define MAADR0 (0x01 | 0x60 | 0x80)
#define MAADR3 (0x02 | 0x60 | 0x80)
#define MAADR2 (0x03 | 0x60 | 0x80)
#define MAADR5 (0x04 | 0x60 | 0x80)
#define MAADR4 (0x05 | 0x60 | 0x80)

// Built-in self-test
#define EBSTSD (0x06 | 0x60)
#define EBSTCON (0x07 | 0x60)
#define EBSTCSL (0x08 | 0x60)
#define EBSTCSH (0x09 | 0x60)
#define MISTAT (0x0A | 0x60 | 0x80)

// Revision ID
#define EREVID (0x12 | 0x60)

// Clock output control register
#define ECOCON (0x15 | 0x60)

// Flow control registers
#define EFLOCON (0x17 | 0x60)
#define EPAUSL (0x18 | 0x60)
#define EPAUSH (0x19 | 0x60)

// PHY registers
#define PHCON1 0x00
#define PHSTAT1 0x01
#define PHID1 0x02
#define PHID2 0x03
#define PHCON2 0x10
#define PHSTAT2 0x11
#define PHIE 0x12
#define PHIR 0x13
#define PHLCON 0x14

// EIE
#define EIE_INTIE 0x80
#define EIE_PKTIE 0x40
#define EIE_DMAIE 0x20
#define EIE_LINKIE 0x10
#define EIE_TXIE 0x08
#define EIE_WOLIE 0x04
#define EIE_TXERIE 0x02
#define EIE_RXERIE 0x01

// EIR
#define EIR_PKTIF 0x40
#define EIR_DMAIF 0x20
#define EIR_LINKIF 0x10
#define EIR_TXIF 0x08
#define EIR_WOLIF 0x04
#define EIR_TXERIF 0x02
#define EIR_RXERIF 0x01

// ESTAT
#define ESTAT_INT 0x80
#define ESTAT_LATECOL 0x10
#define ESTAT_RXBUSY 0x04
#define ESTAT_TXABRT 0x02
#define ESTAT_CLKRDY 0x01

// ECON2
#define ECON2_AUTOINC 0x80
#define ECON2_PKTDEC 0x40
#define ECON2_PWRSV 0x20
#define ECON2_VRPS 0x08

// ECON1
#define ECON1_TXRST 0x80
#define ECON1_RXRST 0x40
#define ECON1_DMAST 0x20
#define ECON1_CSUMEN 0x10
#define ECON1_TXRTS 0x08
#define ECON1_RXEN 0x04
#define ECON1_BSEL1 0x02
#define ECON1_BSEL0 0x01

// EWOLIE
#define EWOLIE_UCWOLIE 0x80
#define EWOLIE_AWOLIE 0x40
#define EWOLIE_PMWOLIE 0x10
#define EWOLIE_MPWOLIE 0x08
#define EWOLIE_HTWOLIE 0x04
#define EWOLIE_MCWOLIE 0x02
#define EWOLIE_BCWOLIE 0x01

// EWOLIR
#define EWOLIR_UCWOLIF 0x80
#define EWOLIR_AWOLIF 0x40
#define EWOLIR_PMWOLIF 0x10
#define EWOLIR_MPWOLIF 0x08
#define EWOLIR_HTWOLIF 0x04
#define EWOLIR_MCWOLIF 0x02
#define EWOLIR_BCWOLIF 0x01

// ERXFCON
#define ERXFCON_UCEN 0x80
#define ERXFCON_ANDOR 0x40
#define ERXFCON_CRCEN 0x20
#define ERXFCON_PMEN 0x10
#define ERXFCON_MPEN 0x08
#define ERXFCON_HTEN 0x04
#define ERXFCON_MCEN 0x02
#define ERXFCON_BCEN 0x01

// MACON1
#define MACON1_LOOPBK 0x10
#define MACON1_TXPAUS 0x08
#define MACON1_RXPAUS 0x04
#define MACON1_PASSALL 0x02
#define MACON1_MARXEN 0x01

// MACON2
#define MACON2_MARST 0x80
#define MACON2_RNDRST 0x40
#define MACON2_MARXRST 0x08
#define MACON2_RFUNRST 0x04
#define MACON2_MATXRST 0x02
#define MACON2_TFUNRST 0x01

// MACON3
#define MACON3_PADCFG2 0x80
#define MACON3_PADCFG1 0x40
#define MACON3_PADCFG0 0x20
#define MACON3_TXCRCEN 0x10
#define MACON3_PHDRLEN 0x08
#define MACON3_HFRMEN 0x04
#define MACON3_FRMLNEN 0x02
#define MACON3_FULDPX 0x01

// MACON4
#define MACON4_DEFER 0x40
#define MACON4_BPEN 0x20
#define MACON4_NOBKOFF 0x10
#define MACON4_LONGPRE 0x02
#define MACON4_PUREPRE 0x01

// MAPHSUP
#define MAPHSUP_RSTINTFC 0x80
#define MAPHSUP_RSTRMII 0x08

// MICON
#define MICON_RSTMII 0x80

// MICMD
#define MICMD_MIISCAN 0x02
#define MICMD_MIIRD 0x01

// EBSTCON
#define EBSTCON_PSV2 0x80
#define EBSTCON_PSV1 0x40
#define EBSTCON_PSV0 0x20
#define EBSTCON_PSEL 0x10
#define EBSTCON_TMSEL1 0x08
#define EBSTCON_TMSEL0 0x04
#define EBSTCON_TME 0x02
#define EBSTCON_BISTST 0x01

// MISTAT
#define MISTAT_NVALID 0x04
#define MISTAT_SCAN 0x02
#define MISTAT_BUSY 0x01

// ECOCON
#define ECOCON_COCON2 0x04
#define ECOCON_COCON1 0x02
#define ECOCON_COCON0 0x01

// EFLOCON
#define EFLOCON_FULDPXS 0x04
#define EFLOCON_FCEN1 0x02
#define EFLOCON_FCEN0 0x01

// PHCON1
#define PHCON1_PRST 0x8000
#define PHCON1_PLOOPBK 0x4000
#define PHCON1_PPWRSV 0x0800
#define PHCON1_PDPXMD 0x0100

// PHSTAT1
#define PHSTAT1_PFDPX 0x1000
#define PHSTAT1_PHDPX 0x0800
#define PHSTAT1_LLSTAT 0x0004
#define PHSTAT1_JBSTAT 0x0002

// PHCON2
#define PHCON2_FRCLNK 0x4000
#define PHCON2_TXDIS 0x2000
#define PHCON2_JABBER 0x0400
#define PHCON2_HDLDIS 0x0100

// PHSTAT2
#define PHSTAT2_TXSTAT 0x2000
#define PHSTAT2_RXSTAT 0x1000
#define PHSTAT2_COLSTAT 0x0800
#define PHSTAT2_LSTAT 0x0400
#define PHSTAT2_DPXSTAT 0x0200
#define PHSTAT2_PLRITY 0x0010

// PHIE
#define PHIE_PLNKIE 0x0010
#define PHIE_PGEIE 0x0002

// PHIR
#define PHIR_PLNKIF 0x0010
#define PHIR_PGIF 0x0004

// PHLCON
#define PHLCON_LACFG3 0x0800
#define PHLCON_LACFG2 0x0400
#define PHLCON_LACFG1 0x0200
#define PHLCON_LACFG0 0x0100
#define PHLCON_LBCFG3 0x0080
#define PHLCON_LBCFG2 0x0040
#define PHLCON_LBCFG1 0x0020
#define PHLCON_LBCFG0 0x0010
#define PHLCON_LFRQ1 0x0008
#define PHLCON_LFRQ0 0x0004
#define PHLCON_STRCH 0x0002

#ifdef __cplusplus
}
#endif