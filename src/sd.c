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
 *
 * @return The bytes that the SD card sent in response.
 */
static inline uint8_t* sd_send_cmd(uint8_t cmd, uint32_t arg, uint8_t crc,
        uint8_t* receiveBuffer, uint16_t receiveBufferLength)
{
    uint8_t transactionId;
    uint8_t sendBuffer[7];
    uint16_t sendBufferLength = sizeof(sendBuffer);

    sendBuffer[0] = cmd | 0x40; // Every command byte sent must have bit 6 set
    sendBuffer[1] = arg >> 24;
    sendBuffer[2] = arg >> 16;
    sendBuffer[3] = arg >> 8;
    sendBuffer[4] = arg;
    sendBuffer[5] = crc; // Usually 0x00 during normal (i.e. not init) operation
    // The byte below provides 8 clock cycles necessary to allow the card to
    // complete the operation according to the SD Card spec
    sendBuffer[6] = 0xFF;

    sercom_spi_start(&spi_g, inst->currentTransactionId, SD_BAUDRATE,
            SD_CS_PIN_GROUP, SD_CS_PIN_MASK, sendBuffer, sendBufferLength,
            receiveBuffer, receiveBufferLength);
    sercom_spi_clear_transaction(&spi_g, transactionId);

    return 0;
}

/**
 * write_block()
 *
 * @brief Write a single block to the SD card.
 *
 * @param blockAddr The address of the block to write to.
 * @param src A pointer to the data which will be written to the card.
 *
 * @return 0 on success, 1 on error.
 */
static inline void write_block(struct sd_desc_t *inst)
{
    uint8_t response;
    uint8_t writeBeginByte = 0xFE;
    // Blocksize +1 for writeBeginByte +2 for CRC bytes at end.
    uint16_t sendBufferLength = SD_BLOCKSIZE + 3;
    uint16_t responseLength = sizeof(response);

    init->state = WRITE_WAIT;

    // Send the single block write command with our desired address
    sd_send_cmd(CMD24, inst->blockAddr, 0x00, &response, responseLength);

    if (response != 0x00) {
        return;
    }

    // First byte sent MUST be 0xFE according to spec. (align 4th byte boundary)
    inst->data[3] = writeBeginByte;
    // 16-bit "CRC" at end of block write
    inst->data[516] = 0xAA;
    inst->data[517] = 0xFF;
    inst->data[518] = 0xFF;

    // Write the block.
    sercom_spi_start(&spi_g, inst->currentTransactionId, SD_BAUDRATE,
            SD_CS_PIN_GROUP, SD_CS_PIN_MASK, inst->data, sendBufferLength,
            &response, responseLength);
    sercom_spi_clear_transaction(&spi_g, transactionId);
}

/**
 * compare_response()
 *
 * @desc Compares a response given by the SD card to a desired response
 *
 * @param response The response to be compared
 * @param compareTo The value we are comparing the response to
 * @param size The size of the comparison
 *
 * @return 0 (false) if the two are not equal, 1 (true) otherwise
 */
static inline uint8_t compare_response(uint8_t *response, uint8_t *compareTo,
        uint16_t size)
{
    for (uint8_t i; i < size; i++) {
        if (response[i] != compareTo[i])
            return 0;
    }
    return 1;
}

/**
 * init_sd_card()
 *
 * @brief Initializes the SD card into SPI mode.
 *
 * Side effects: Changes initialized bit in instance struct
 */
void init_sd_card(struct sd_desc_t *inst)
{
    uint8_t oldCard = 0;
    uint8_t softResetCount = 0;
    uint8_t response = 0x00;
    uint8_t sendBuffer[10];
    uint8_t largeReceiveBuffer[5];
    uint16_t sendBufferLength = sizeof(sendBuffer);
    uint16_t receiveBufferLength = sizeof(largeReceiveBuffer);
    uint16_t responseLength = sizeof(response);
    memset(sendBuffer, 0xFF, 10);
    memset(largeReceiveBuffer, 0xFF, 5);

    init->state = INIT_WAIT;
    inst->initializing = 1;

    // Put SD card in SPI mode
    // Buffer of all 1s as dummy data
    // Receive Buffer/Length is NULL/0 here because no expected response
    sercom_spi_start(&spi_g, inst->currentTransactionId, SD_BAUDRATE,
            SD_CS_PIN_GROUP, SD_CS_PIN_MASK, sendBuffer, sendBufferLength, NULL,
            0);
    while (!sercom_spi_transaction_done(&spi_g, transactionId));
    sercom_spi_clear_transaction(&spi_g, transactionId);

    // Repeat until soft reset successful or a reasonable number of times
    // since it is possible to not get a valid response here but have the next
    // steps work just fine
    while (response != R1_IDLE_STATE && softResetCount < 20) {
        sd_send_cmd(CMD0, 0x00000000, 0x95, &response, responseLength);
        softResetCount++;
    }

    // This CMD needs a larger response buffer as it sends back the argument in
    // addition to the regular response code
    while (largeReceiveBuffer[0] != R1_IDLE_STATE) {
        sd_send_cmd(CMD8, 0x000001AA, 0x87, largeReceiveBuffer, receiveBufferLength);
    }

    // Apparently most cards require this to be repeated at least once
    // This behaviour was confirmed in practice
    for (uint8_t i = 0; i < 2; i++) {
        if (! oldCard) {
            sd_send_cmd(CMD55, 0x00000000, 0x65, &response, responseLength);
            // If this response is given, we have an old card that must use CMD1
            if (response == R1_USE_CMD1) {
                sd_send_cmd(CMD1, 0x00000000, 0xF9, &response, responseLength);
                oldCard = 1;
            }
            // Successful CMD55
            else if (response == R1_IDLE_STATE) {
                sd_send_cmd(ACMD41, 0x40000000, 0x77, &response, responseLength);
                // If this is the second iteration of the loop and it hasn't
                // been successful, then return with initialization failed
                if (i == 1 && response != R1_READY_STATE) {
                    goto failed_init;
                }
            }
            else {
                // Unexpected response/error
                goto failed_init;
            }
        }
        else {
            sd_send_cmd(CMD1, 0x00000000, 0xF9, &response, responseLength);
        }
    }

    // Turn off CRC requirement
    sd_send_cmd(CMD59, 0x00000000, 0xFF, &response, responseLength);

    // Set the R/W block size to 512 bytes with CMD16
    // Try 3 times, if success return 0 immediately, card is ready.
    // Only necessary for standard capacity cards, not for HC, XC cards
    for (uint8_t i = 0; i < 3; i++) {
        sd_send_cmd(CMD16, SD_BLOCKSIZE, 0xFF, &response, responseLength);
        if (response == R1_READY_STATE || response == R1_ILLEGAL_COMMAND) {
            inst->initialized = 1;
            inst->initializing = 0;
        }
    }
    // Otherwise, keep SD Card uninitialized
    failed_init:
    inst->initialized = 0;
    inst->initializing = 0;
}

/**
 * sd_card_service()
 *
 * @brief Checks the status of the SD card and writes a block of data if it is
 * ready.
 * TODO: Short circuit waiting for data -> add bit to struct and sd_send_cmd
 * TODO: Take all init and make individual states in cases
 * TODO: Move init stuff in main.c to above init function.
 *          Basically, stuff in init function should move here, stuff in main.c
 *          should move to init function.
 * TODO: Implement single block read function.
 */
void sd_card_service(struct sd_desc_t *inst, uint8_t *src, uint8_t action)
{
    uint8_t allZero[] = {0x00, 0x00}; // Find better name?
    uint8_t statusResponse[2];
    uint16_t statusResponseLength = sizeof(statusResponse);
    memset(statusResponse, 0xAA, 2);

    // UNSURE IF NEEDED OR HOW IT WILL BEHAVE IF QUERIED IN MIDDLE OF OPERATION
    // Send the SEND_STATUS command to check the SD card's current status.
    // Return format is R2 (2 bytes long).
    sd_send_cmd(CMD13, 0x00000000, 0xFF, statusResponse, statusResponseLength);

    switch(inst->state){
    case INIT:
        init_sd_card(inst);
        inst->state = INIT_WAIT;
        break;
    case INIT_WAIT:
        if (inst->initializing == 0) {
            inst->state = READY;
        }
        break;
    case WRITE_WAIT:
        if (sercom_spi_transaction_done(&spi_g, transactionId)) {
            inst->state = READY;
        }
        break;
    case READ_WAIT:
        if (sercom_spi_transaction_done(&spi_g, transactionId)) {
            inst->state = READY;
        }
        break;
    case READY:
        // Don't do anything if card failed to initialize
        if (inst->initialized = 0) {
            break;
        }
        if (action == SD_ACTION_WRITE) {
            write_block(inst);
        }
        else {
            read_block(inst);
        }
        break;
    }
}
