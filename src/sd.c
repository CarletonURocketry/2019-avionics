/**
 * @file sd.c
 * @desc Module for writing data to an SD Card
 *
 * Sources for implementation:
 *  -- https://electronics.stackexchange.com/questions/77417
 *  -- https://openlabpro.com/guide/interfacing-microcontrollers-with-sd-card/
 *  -- https://nerdclub-uk.blogspot.com/2012/11/how-spi-works-with-sd-card.html
 *  -- https://openlabpro.com/guide/raw-sd-readwrite-using-pic-18f4550/
 *  -- SD Card Physical Layer Spec (see repo wiki)
 *      ^^^ THIS IS THE BEST RESOURCE ^^^
 *
 *  **IMPORTANT NOTE:** If using an older card, the initialization steps must be
 *  executed while the microprocessor/controller is running at a slower clock
 *  rate (100-400 KHz). Newer cards can withstand MHz clocks but older ones will
 *  complain. After initialization is complete, the clock speed may be switched
 *  to a higher one.
 *
 * Supposedly, the proper way to initalize a card over SPI is to:
 *      1. Set the clock speed to 400kHz or less if old card
 *      2. Hold the CS line low and send 80 clock pulses (with bytes 0xFF)
 *      3. Send the "soft reset" command CMD0
 *      4. Wait for the card to respond "ok" with the value 0x01 (0xFF is also
 *          acceptable and indicates the card was in a strange state)
 *      5. Initialize the card:
 *          5a. Send CMD55 followed by ACMD41, if response is 0x05, this is an
 *              old card and CMD1 must be used (step 5b). If response 0x01 for
 *              CMD55 then continue, if response 0x00 for ACMD41 then continue,
 *              if response 0x01 for ACMD41 then repeat this step.
 *          5b. Send in the "initialize card" command CMD1 and repeat this until
 *              the card responds with 0x00.
 *      6. Set sector size using CMD16 with parameter 512
 *      7. Turn off CRC requirement by sending CMD59
 *      8. Next time the card responds with "ok" value it is ready
 *      9. Ramp up clock speed back to normal if step 1 was necessary.
 *
 *  We should also assert CS low at least before and after each CMD is sent
 *  since SD cards are selfish and may assume it's the only SPI device selected
 *  all the time.
 */

#include <string.h>
#include "config.h"
#include "sd.h"

/**
 * sd_send_cmd()
 *
 * @brief Sets up the buffer for sending commands to the SD card and then
 * sends the data. This is the version of the function which receives 5 bytes of
 * data for the commands which need this
 *
 * @param cmd The single byte command to send to the SD card.
 * @param arg The four byte argument to send to the SD card.
 * @param crc The crc checksum to send (necessary during initialization,
 *              otherwise optional).
 * @param receiveBuffer The buffer that will contain the bytes sent back from
 *              the SD card in response to a command.
 * @param receiveBufferLength The lenght, in bytes, of the receiving buffer.
 * @param sd_desc_t The instance of the sd card struct
 */
static inline void sd_send_cmd(uint8_t cmd, uint32_t arg, uint8_t crc,
        uint8_t* receiveBuffer, uint16_t receiveBufferLength,
        struct sd_desc_t *inst)
{
    uint8_t sendBuffer[7];
    uint16_t sendBufferLength = sizeof(sendBuffer);

    sendBuffer[0] = cmd | 0x40; // Every command byte sent must have bit 6 set
    sendBuffer[1] = arg >> 24;
    sendBuffer[2] = arg >> 16;
    sendBuffer[3] = arg >> 8;
    sendBuffer[4] = arg;
    sendBuffer[5] = crc;
    sendBuffer[6] = 0xFF; // 8 clock cycles to allow card to process command

    sercom_spi_start(inst->spi_inst, &inst->currentTransactionId, SD_BAUDRATE,
            SD_CS_PIN_GROUP, SD_CS_PIN_MASK, sendBuffer, sendBufferLength,
            receiveBuffer, receiveBufferLength);
}

/**
 * write_block()
 *
 * @brief Write a single block to the SD card.
 *
 * @param sd_desc_t The instance of the sd card struct
 */
static inline void write_block(struct sd_desc_t *inst)
{
    uint8_t writeBeginByte = 0xFE;

    // First byte sent MUST be 0xFE according to spec. (align 4th byte boundary)
    inst->data[3] = writeBeginByte;
    // 16-bit "CRC" at end of block write
    inst->data[515] = 0xAA;
    inst->data[516] = 0xFF;
    inst->data[517] = 0xFF;

    // Write the block.
    sercom_spi_start(inst->spi_inst, &inst->currentTransactionId, SD_BAUDRATE,
            SD_CS_PIN_GROUP, SD_CS_PIN_MASK, inst->data, sizeof(inst->data),
            &inst->singleByteResponse, 1);
    sercom_spi_clear_transaction(inst->spi_inst, inst->currentTransactionId);
}

static inline void read_block(struct sd_desc_t *inst)
{
    return;
}

/**
 * init_sd_card()
 *
 * @brief Initializes the SD card.
 *
 * @param sd_desc_t The instance of the sd card struct
 */
void init_sd_card(struct sd_desc_t *inst)
{
    gpio_set_pin_mode(GPIO_7, GPIO_PIN_OUTPUT_TOTEM);
    gpio_set_output(GPIO_7, 1);

    memset(&inst->data, 0x00, 518);
    memset(&inst->argumentResponse, 0x00, 5);
    memset(&inst->doubleByteResponse, 0x00, 2);
    memset(&inst->singleByteResponse, 0x00, 1);

    inst->blockAddr = 0x00000000;
    inst->spi_inst = &spi_g;
    inst->currentTransactionId = 0x00;

    inst->oldCard = 0;

    // DEBUG
    inst->data[1]  = 'H';
    inst->data[2]  = 'E';
    inst->data[3]  = 'L';
    inst->data[4]  = 'L';
    inst->data[5]  = 'O';
    inst->data[6]  = ' ';
    inst->data[7]  = 'W';
    inst->data[8]  = 'O';
    inst->data[9]  = 'R';
    inst->data[10] = 'L';
    inst->data[11] = 'D';
    inst->data[12] = '!';
    // DEBUG

    inst->state = SD_SPI_MODE_WAIT;
}

/**
 * sd_card_service()
 *
 * @brief Checks the status of the SD card and writes a block of data if it is
 * ready.
 *
 * @param sd_desc_t The instance of the sd card struct
 *
 * TODO: Implement single block read function.
 */
void sd_card_service(struct sd_desc_t *inst)
{
    static uint8_t repeatedCMD = 0;
    static uint8_t spiSendBuffer[10];

    switch(inst->state){
    case SD_FAILED:
        break;

    case SD_INIT:
        memset(spiSendBuffer, 0xFF, 10);
        uint16_t spiSendBufferLength = sizeof(spiSendBuffer);

        sercom_spi_start(inst->spi_inst, &inst->currentTransactionId,
                SD_BAUDRATE, SD_CS_PIN_GROUP, SD_CS_PIN_MASK, spiSendBuffer,
                spiSendBufferLength, NULL, 0);
        inst->state = SD_SPI_MODE_WAIT;

        break;

    case SD_SPI_MODE_WAIT:
        if (sercom_spi_transaction_done(inst->spi_inst,
                inst->currentTransactionId)) {
            sercom_spi_clear_transaction(inst->spi_inst,
                    inst->currentTransactionId);
            sd_send_cmd(CMD0, 0x00000000, 0x95, &inst->singleByteResponse, 1,
                    inst);
            inst->state = SD_SOFT_RESET_WAIT;
        }
        break;

    case SD_SOFT_RESET_WAIT:
        if (sercom_spi_transaction_done(inst->spi_inst,
                inst->currentTransactionId)) {
            sercom_spi_clear_transaction(inst->spi_inst,
                    inst->currentTransactionId);
            if (inst->singleByteResponse != R1_IDLE_STATE) {
                inst->state = SD_FAILED;
                break;
            }
            sd_send_cmd(CMD8, 0x000001AA, 0x87, inst->argumentResponse, 5, inst);
            inst->state = SD_CMD8_WAIT;
        }
        break;

    case SD_CMD8_WAIT:
        if (sercom_spi_transaction_done(inst->spi_inst,
                inst->currentTransactionId)) {
            sercom_spi_clear_transaction(inst->spi_inst,
                    inst->currentTransactionId);
            if (inst->argumentResponse[0] != R1_IDLE_STATE) {
                inst->state = SD_FAILED;
                break;
            }
            sd_send_cmd(CMD55, 0x00000000, 0x65, &inst->singleByteResponse, 1,
                    inst);
            inst->state = SD_CMD55_WAIT;
        }
        break;

    case SD_CMD55_WAIT:
        if (sercom_spi_transaction_done(inst->spi_inst,
                inst->currentTransactionId)) {
            sercom_spi_clear_transaction(inst->spi_inst,
                    inst->currentTransactionId);
            if (inst->singleByteResponse == R1_USE_CMD1) {
                sd_send_cmd(CMD1, 0x00000000, 0xF9, &inst->singleByteResponse,
                        1, inst);
                inst->state = SD_CMD1_WAIT;
                break;
            }
            else if (inst->singleByteResponse != R1_IDLE_STATE) {
                inst->state = SD_FAILED;
                break;
            }
            sd_send_cmd(ACMD41, 0x40000000, 0x77, &inst->singleByteResponse, 1,
                    inst);
            inst->state = SD_ACMD41_WAIT;
        }
        break;

    case SD_CMD1_WAIT:
        if (sercom_spi_transaction_done(inst->spi_inst,
                inst->currentTransactionId)) {
            sercom_spi_clear_transaction(inst->spi_inst,
                    inst->currentTransactionId);
            if (repeatedCMD == 0) {
                sd_send_cmd(CMD1, 0x00000000, 0xF9, &inst->singleByteResponse,
                        1, inst);
                repeatedCMD = 1;
                inst->state = SD_CMD1_WAIT;
                break;
            }
            else if (repeatedCMD == 1
                    && inst->singleByteResponse != R1_IDLE_STATE) {
                inst->state = SD_FAILED;
                break;
            }
            sd_send_cmd(CMD59, 0x00000000, 0xFF, &inst->singleByteResponse, 1,
                    inst);
            inst->state = SD_CMD59_WAIT;
        }
        break;

    case SD_ACMD41_WAIT:
        if (sercom_spi_transaction_done(inst->spi_inst,
                inst->currentTransactionId)) {
            sercom_spi_clear_transaction(inst->spi_inst,
                    inst->currentTransactionId);
            if (repeatedCMD == 0) {
                sd_send_cmd(CMD55, 0x00000000, 0x65, &inst->singleByteResponse,
                        1, inst);
                repeatedCMD = 1;
                inst->state = SD_CMD55_WAIT;
                break;
            }
            else if (repeatedCMD == 1
                    && inst->singleByteResponse != R1_IDLE_STATE) {
                inst->state = SD_FAILED;
                break;
            }
            sd_send_cmd(CMD59, 0x00000000, 0xFF, &inst->singleByteResponse, 1,
                    inst);
            inst->state = SD_CMD59_WAIT;
        }
        break;

    case SD_CMD59_WAIT:
        if (sercom_spi_transaction_done(inst->spi_inst,
                inst->currentTransactionId)) {
            sercom_spi_clear_transaction(inst->spi_inst,
                    inst->currentTransactionId);
            sd_send_cmd(CMD16, SD_BLOCKSIZE, 0xFF, &inst->singleByteResponse, 1,
                    inst);
            inst->state = SD_CMD16_WAIT;
        }
        break;

    case SD_CMD16_WAIT:
        if (sercom_spi_transaction_done(inst->spi_inst,
                inst->currentTransactionId)) {
            sercom_spi_clear_transaction(inst->spi_inst,
                    inst->currentTransactionId);
            if (!( inst->singleByteResponse == R1_IDLE_STATE
                    || inst->singleByteResponse == R1_ILLEGAL_COMMAND)) {
                inst->state = SD_FAILED;
                break;
            }
            inst->state = SD_READY;
        }
        break;

    case SD_CMD24_WAIT:
        if (sercom_spi_transaction_done(inst->spi_inst,
                inst->currentTransactionId)) {
            sercom_spi_clear_transaction(inst->spi_inst,
                    inst->currentTransactionId);
            if (inst->singleByteResponse != R1_READY_STATE) {
                inst->state = SD_WRITE_FAILED;
                break;
            }
            write_block(inst);
            inst->state = SD_WRITE_WAIT;
        }
        break;

    case SD_WRITE_WAIT:
        if (sercom_spi_transaction_done(inst->spi_inst,
                    inst->currentTransactionId)) {
            sercom_spi_clear_transaction(inst->spi_inst,
                    inst->currentTransactionId);
            inst->state = SD_READY;
        }
        break;

    case SD_READ_WAIT:
        if (sercom_spi_transaction_done(inst->spi_inst,
                    inst->currentTransactionId)) {
            sercom_spi_clear_transaction(inst->spi_inst,
                    inst->currentTransactionId);
            inst->state = SD_READY;
        }
        break;

    case SD_READY:
        if (inst->action == SD_ACTION_WRITE) {
            sd_send_cmd(CMD24, inst->blockAddr, 0x00, &inst->singleByteResponse,
                    1, inst);
            inst->state = SD_CMD24_WAIT;
        }
        else if (inst->action == SD_ACTION_READ){
            read_block(inst);
            inst->state = SD_READ_WAIT;
        }
        break;

    case SD_WRITE_FAILED:
        inst->state = SD_READY;
        break;

    case SD_READ_FAILED:
        inst->state = SD_READY;
        break;
    }
}
