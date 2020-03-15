#ifndef SDSPI_H
#define SDSPI_H

#include "global.h"
#include "sercom-spi.h"

#define ENABLE_SD_CARD_SERVICE

// SD card pinout
/** Usually the SS pin */
#define SD_CHIP_SELECT_PIN 15
/** SPI Master Out Slave In pin */
#define SPI_MOSI_PIN 16
/** SPI Master In Slave Out pin */
#define SPI_MISO_PIN 17
/** SPI Clock pin */
#define SPI_SCK_PIN 18

// SD card errors – From Arduino SD libraries
// We actually don't use these because we're mostly checking for success and
// moving on regardless of the type of error. These are kept here in case we
// should use them in the future or in case we think it would be better to build
// a fully robust writing system.
/** timeout error for command CMD0 */
#define SD_CARD_ERROR_CMD0 0x1
/** CMD8 was not accepted - not a valid SD card*/
#define SD_CARD_ERROR_CMD8 0x2
/** card returned an error response for CMD17 (read block) */
#define SD_CARD_ERROR_CMD17 0x3
/** card returned an error response for CMD24 (write block) */
#define SD_CARD_ERROR_CMD24 0x4
/** WRITE_MULTIPLE_BLOCKS command failed */
#define SD_CARD_ERROR_CMD25 0x05
/** card returned an error response for CMD58 (read OCR) */
#define SD_CARD_ERROR_CMD58 0x06
/** SET_WR_BLK_ERASE_COUNT failed */
#define SD_CARD_ERROR_ACMD23 0x07
/** card's ACMD41 initialization process timeout */
#define SD_CARD_ERROR_ACMD41 0x08
/** card returned a bad CSR version field */
#define SD_CARD_ERROR_BAD_CSD 0x09
/** erase block group command failed */
#define SD_CARD_ERROR_ERASE 0x0A
/** card not capable of single block erase */
#define SD_CARD_ERROR_ERASE_SINGLE_BLOCK 0x0B
/** Erase sequence timed out */
#define SD_CARD_ERROR_ERASE_TIMEOUT 0x0C
/** card returned an error token instead of read data */
#define SD_CARD_ERROR_READ 0x0D
/** read CID or CSD failed */
#define SD_CARD_ERROR_READ_REG 0x0E
/** timeout while waiting for start of read data */
#define SD_CARD_ERROR_READ_TIMEOUT 0x0F
/** card did not accept STOP_TRAN_TOKEN */
#define SD_CARD_ERROR_STOP_TRAN 0x10
/** card returned an error token as a response to a write operation */
#define SD_CARD_ERROR_WRITE 0x11
/** attempt to write protected block zero */
#define SD_CARD_ERROR_WRITE_BLOCK_ZERO 0x12
/** card did not go ready for a multiple block write */
#define SD_CARD_ERROR_WRITE_MULTIPLE 0x13
/** card returned an error to a CMD13 status check after a write */
#define SD_CARD_ERROR_WRITE_PROGRAMMING 0x14
/** timeout occurred during write programming */
#define SD_CARD_ERROR_WRITE_TIMEOUT 0x15
/** incorrect rate selected */
#define SD_CARD_ERROR_SCK_RATE 0x16
//------------------------------------------------------------------------------

// SD card commands
// Once programming is complete, we should decide if it's worth it to slim down
// this list to only the commands we use or if it's a good idea to just keep
// them all in anyways.
/** GO_IDLE_STATE - init card in spi mode if CS low */
#define CMD0 0X00
/** Initialize card fallback code */
#define CMD1 0X01
/** SEND_IF_COND - verify SD Memory Card interface operating condition.*/
#define CMD8 0X08
/** SEND_CSD - read the Card Specific Data (CSD register) */
#define CMD9 0X09
/** SEND_CID - read the card identification information (CID register) */
#define CMD10 0X0A
/** SEND_STATUS - read the card status register */
#define CMD13 0X0D
/** Set block size*/
#define CMD16 0X0F
/** READ_BLOCK - read a single data block from the card */
#define CMD17 0X11
/** WRITE_BLOCK - write a single data block to the card */
#define CMD24 0X18
/** WRITE_MULTIPLE_BLOCK - write blocks of data until a STOP_TRANSMISSION */
#define CMD25 0X19
/** ERASE_WR_BLK_START - sets the address of the first block to be erased */
#define CMD32 0X20
/** ERASE_WR_BLK_END - sets the address of the last block of the continuous
  range to be erased*/
#define CMD33 0X21
/** ERASE - erase all previously selected blocks */
#define CMD38 0X26
/** APP_CMD - escape for application specific command */
#define CMD55 0X37
/** READ_OCR - read the OCR register of a card */
#define CMD58 0X3A
/** CRC_ON_OFF - turn the requirement to send the CRC with a command on/off */
#define CMD59 0X3B
/** SET_WR_BLK_ERASE_COUNT - Set the number of write blocks to be
   pre-erased before writing */
#define ACMD23 0X17
/** SD_SEND_OP_COMD - Sends host capacity support information and
  activates the card's initialization process */
#define ACMD41 0X29
//------------------------------------------------------------------------------

/** status for card in the ready state */
#define R1_READY_STATE 0X00
/** status for card in the idle state */
#define R1_IDLE_STATE 0X01
/** status bit for illegal command */
#define R1_ILLEGAL_COMMAND 0X04
/** old card response to CMD55 */
#define R1_USE_CMD1 0X05
/** start data token for read or write single block*/
#define DATA_START_BLOCK 0XFE
/** stop token for write multiple blocks*/
#define STOP_TRAN_TOKEN 0XFD
/** start data token for write multiple blocks*/
#define WRITE_MULTIPLE_TOKEN 0XFC
/** mask for data response tokens after a write block operation */
#define DATA_RES_MASK 0X1F
/** write data accepted token */
#define DATA_RES_ACCEPTED 0X05

// SD Card block size
#define SD_BLOCKSIZE 0x00000200

// Actions
#define SD_ACTION_WRITE 0x00
#define SD_ACTION_READ 0x01

enum sd_state {
    SD_INIT,
    SD_SPI_MODE_WAIT,
    SD_SOFT_RESET_WAIT,
    SD_CMD8_WAIT,
    SD_CMD55_WAIT,
    SD_CMD1_WAIT,
    SD_ACMD41_WAIT,
    SD_CMD59_WAIT,
    SD_CMD16_WAIT,
    SD_READY,
    SD_WRITE_WAIT,
    SD_READ_WAIT,
    SD_FAILED
}

enum sd_action {
    SD_ACTION_WRITE,
    SD_ACTION_READ
}

struct sd_desc_t {
    /** SPI instance used to communicate with device */
    struct sercom_spi_desc_t *spi_inst;
    /** Currently address block for read/write **/
    uint32_t blockAddr;
    /** Data to write, or data from read **/
    /** 512 + 1 start byte + 2 crc bytes + 3 alignment bytes: **/
    /** 3 Unused Bytes | 0xFE | 512 Bytes of Data | 0xFF 0xFF **/
    char data[518];
    /** Transaction ID of currently executing SPI transaction**/
    uint8_t currentTransactionId;
    /** Responses which also send back the argument (e.g. CMD8)**/
    uint8_t argumentResponse[5];
    /** Responses which send back two bytes (e.g. writes)**/
    uint8_t doubleByteResponse[2];
    /** Most Responses, only one byte**/
    uint8_t singleByteResponse;
    /** Identifying the card as old or not **/
    uint8_t oldCard:1;
    /** Current state of the SD card driver**/
    enum sd_state state;
    enum sd_action action;
}

extern void init_sd_card(struct sd_desc_t *inst);
extern void sd_card_service(struct sd_desc_t *inst, uint8_t *src, uint8_t action);

#endif
