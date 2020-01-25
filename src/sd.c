/**
 * @file sd.c
 * @brief Containing function implementations for communicating with the SD
 *          Card
 *
 * Sources for implementation:
 *  -- https://electronics.stackexchange.com/questions/77417
 *      ^^^ THIS IS THE BEST RESOURCE ^^^
 *  -- https://openlabpro.com/guide/interfacing-microcontrollers-with-sd-card/
 *  -- https://nerdclub-uk.blogspot.com/2012/11/how-spi-works-with-sd-card.html
 *  -- https://openlabpro.com/guide/raw-sd-readwrite-using-pic-18f4550/
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
 * sd_send_cmd_large()
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
static inline uint8_t* sd_send_cmd_large(uint8_t cmd, uint32_t arg, uint8_t crc,
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

    sercom_spi_start(&spi_g, &transactionId, SD_BAUDRATE, SD_CS_PIN_GROUP,
            SD_CS_PIN_MASK, sendBuffer, sendBufferLength, receiveBuffer,
            receiveBufferLength);
    while (!sercom_spi_transaction_done(&spi_g, transactionId));

    return 0;
}

/**
 * sd_send_cmd()
 * @brief Sets up the buffer for sending commands to the SD card and then
 * sends the data.
 *
 * @param cmd The single byte command to send to the SD card.
 * @param arg The four byte argument to send to the SD card.
 * @param crc The crc checksum to send (necessary during initialization,
 *              otherwise optional).
 *
 * @return The byte that the SD card sent in response.
 */
static inline uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    uint8_t transactionId;
    uint8_t receiveBuffer = 0x00;
    uint8_t sendBuffer[7];
    uint16_t sendBufferLength = sizeof(sendBuffer);
    uint16_t receiveBufferLength = sizeof(receiveBuffer);

    sendBuffer[0] = cmd | 0x40; // Every command byte sent must have bit 6 set
    sendBuffer[1] = arg >> 24;
    sendBuffer[2] = arg >> 16;
    sendBuffer[3] = arg >> 8;
    sendBuffer[4] = arg;
    sendBuffer[5] = crc; // Usually 0x00 during normal (i.e. not init) operation
    // The byte below provides 8 clock cycles necessary to allow the card to
    // complete the operation according to the SD Card spec
    sendBuffer[6] = 0xFF;

    sercom_spi_start(&spi_g, &transactionId, SD_BAUDRATE, SD_CS_PIN_GROUP,
            SD_CS_PIN_MASK, sendBuffer, sendBufferLength, &receiveBuffer,
            receiveBufferLength);
    while (!sercom_spi_transaction_done(&spi_g, transactionId));

    return receiveBuffer;
}

/**
 * init()
 * @brief Initializes the SD card into SPI mode.
 *
 * @return Either 0 (success) or 1 (failure)
 */
uint8_t init_sd_card(void)
{
    uint8_t oldCard = 0;
    uint8_t softResetCount = 0;
    uint8_t response = 0x00;
    uint8_t transactionId;

    // Put SD card in SPI mode
    // Buffer of all 1s as dummy data
    uint8_t sendBuffer[10];
    uint16_t sendBufferLength = sizeof(sendBuffer);
    memset(sendBuffer, 0xFF, 10);


    // Receive Buffer/Length is NULL/0 here because there is no expected
    // response
    sercom_spi_start(&spi_g, &transactionId, SD_BAUDRATE, SD_CS_PIN_GROUP,
            SD_CS_PIN_MASK, sendBuffer, sendBufferLength, NULL, 0);
    while (!sercom_spi_transaction_done(&spi_g, transactionId));

    // Repeat until soft reset successful or a reasonable number of times
    // since it is possible to not get a valid response here but have the next
    // steps work just fine
    while (response != 0x01 && softResetCount < 20) {
        response = sd_send_cmd(CMD0, 0x00000000, 0x95);
        softResetCount++;
    }

    // This CMD needs a larger response buffer as it sends back the argument in
    // addition to the regular response code
    uint8_t largeReceiveBuffer[5];
    memset(largeReceiveBuffer, 0x00, 5);
    while (largeReceiveBuffer[0] != 0x01) {
        sd_send_cmd_large(CMD8, 0x000001AA, 0x87,
                          largeReceiveBuffer, sizeof(largeReceiveBuffer));
    }

    // Apparently most cards require this to be repeated at least once
    for (uint8_t i = 0; i < 1; i++) {
        if (! oldCard) {
            response = sd_send_cmd(CMD55, 0x00000000, 0x65);
            // If this response is given, we have an old card that must use CMD1
            if (response == 0x05) {
                response = sd_send_cmd(CMD1, 0x00000000, 0xF9);
                oldCard = 1;
            }
            // Successful CMD55
            else if (response == 0x01) {
                response = sd_send_cmd(ACMD41, 0x40000000, 0x77);
                // Response should be 0x00 indicating the SD card is ready, if
                // not, initialization has failed.
                // We must keep repeating CMD55+ACMD41 until we get 0x00
                if (response != 0x00) {
                    i--;
                    continue;
                }
            }
            else {
                // Something is quite wrong, stop initializing.
                return 2;
            }
        }
        else {
            response = sd_send_cmd(CMD1, 0x00000000, 0xF9);
        }
    }
    // Set the R/W block size to 512 bytes with CMD16
    // Try 3 times, if success return 0 immediately, card is ready.
    for (uint8_t i = 0; i < 3; i++) {
        response = sd_send_cmd(CMD16, SD_BLOCKSIZE, 0xFF);
        if (response == 0x00)
            return 0;
    }
    // In all other error cases, return 1
    return 1;
}

/**
 * write_block()
 * @brief Write a single block to the SD card.
 *
 * @param blockAddr The address of the block to write to.
 * @param src A pointer to the data which will be written to the card.
 *
 * @return 0 on success, 1 on error.
 */
uint8_t write_block(uint32_t blockAddr, uint8_t* src)
{
    /* NOTE: This may be difficult to do because the specification and
     * many of the resources I found required that we wait for responses
     * and for things to complete on the SD card. Depending on how the SPI
     * driver schedules things, writes could fail because the SD card
     * expects to have exclusive control of the SPI bus when it is being
     * commanded to do something.
     */
    uint8_t transactionId;
    uint8_t response = 0xFF;
    uint16_t sendBufferLength = SD_BLOCKSIZE + 1; // +1 for writeBeginByte
    uint16_t responseLength = sizeof(response);
    uint8_t writeBeginByte = 0xFE;

    // Send CMD24 (the single block write command)
    response = sd_send_cmd(CMD24, blockAddr, 0x00);
    // If not immediately successful, exit because we can't afford keep
    // trying for a single write command.
    if (response != 0x00) {
        return 1;
    }

    // First byte sent MUST be 0xFE to enable writing of a single block.
    src[0] = writeBeginByte;

    // Write the block.
    sercom_spi_start(&spi_g, &transactionId, SD_BAUDRATE, SD_CS_PIN_GROUP,
                SD_CS_PIN_MASK, src, sendBufferLength, &response,
                responseLength);
    while (!sercom_spi_transaction_done(&spi_g, transactionId));

    // It is (apparently) mandatory to send CMD13 after every block write
    // presumably because this returns the status of the card.
    response = sd_send_cmd(CMD13, 0x00000000, 0xFF);

    // Check if write was successful.
    if (response == 0x00)
        return 0;
    else
        return 1;
}
