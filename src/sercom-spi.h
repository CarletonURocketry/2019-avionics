/**
 * @file sercom-spi.h
 * @desc SERCOM SPI mode driver which allows interrupt or DMA driven transfers.
 * @author Samuel Dewan
 * @date 2019-01-02
 * Last Author:
 * Last Edited On:
 */

#ifndef sercom_spi_h
#define sercom_spi_h

#include "global.h"

#include "transaction-queue.h"

#define SERCOM_SPI_TRANSACTION_QUEUE_LENGTH 16


/**
 *  State for an SPI transaction.
 */
struct sercom_spi_transaction_t {
    /** The buffer from which data is sent. */
    uint8_t *out_buffer;
    /** The buffer into which received data is placed. */
    uint8_t *in_buffer;
    /** The number of bytes to be sent. */
    uint16_t out_length;
    /** The number of bytes to be received. */
    uint16_t in_length;

    /** The number of bytes which have been sent. */
    uint16_t bytes_out;
    /** The number of bytes which have been received. */
    uint16_t bytes_in;

    /** The synchronous clock frequency for this transaction.  */
    uint32_t baudrate;

    /** The mask for the chip select pin of the peripheral. */
    uint32_t cs_pin_mask;
    /** The group index of the chip select pin for the peripheral. */
    uint8_t cs_pin_group:2;

    /** Flag set if the receive stage has been initialized. */
    uint8_t rx_started:1;
    /** Flag to indicate that there are further parts to follow within the same
        transaction. */
    uint8_t multi_part:1;
};

/**
 *  State for a SERCOM SPI driver instance
 */
struct sercom_spi_desc_t {
    /** Registers for the SERCOM hardware of this SPI instance. */
    Sercom *sercom;

    /** Frequency of the SERCOM code clock, used to calculate baud rates. */
    uint32_t core_frequency;

    /** Memory for transaction queue. */
    struct transaction_t transactions[SERCOM_SPI_TRANSACTION_QUEUE_LENGTH];
    /** Memory for transaction state information. */
    struct sercom_spi_transaction_t states[SERCOM_SPI_TRANSACTION_QUEUE_LENGTH];
    /** Queue of SPI transactions. */
    struct transaction_queue_t queue;

    /** The instance number of the SERCOM hardware of this SPI instance. */
    uint8_t sercom_instnum;

    /** Index of the DMA channel used for transmitting. */
    uint8_t tx_dma_chan:4;
    /** Index of the DMA channel used for receiving. */
    uint8_t rx_dma_chan:4;
    /** Flag which is set if DMA should be used for transmitting. */
    uint8_t tx_use_dma:1;
    /** Flag which is set if DMA should be used for receiving. */
    uint8_t rx_use_dma:1;
    /** Flag used to unsure that the service function is not executed in an
        interrupt while it is already being run in the main thread */
    uint8_t service_lock:1;
};

/**
 * Structure which describes one stage of a multi-part SPI transaction.
 */
struct sercom_spi_transaction_part_t {
    /** Buffer from which data will be sent */
    uint8_t *out_buffer;
    /** Buffer into which received data will be placed */
    uint8_t * in_buffer;
    /** Number of bytes to be sent */
    uint16_t out_length;
    /** Number of bytes to be received */
    uint16_t in_length;
    /** Baudrate */
    uint32_t baudrate;
    /** Transaction ID for this part of the transaction */
    uint8_t transaction_id;
};


/**
 *  Initialize a SERCOM instance for use as an SPI master.
 *
 *  @param descriptor The descriptor to be populated for this SPI instance.
 *  @param sercom Pointer to the SERCOM instance's registers.
 *  @param core_freq The frequency of the core clock for the SERCOM instance.
 *  @param core_clock_mask The mask for the generator to be used for the SERCOM
 *                         core clock;
 *  @param tx_dma_channel The DMA channel to be used for transmission or a
 *                        negative value for interrupt driven transmission.
 *  @param rx_dma_channel The DMA channel to be used for reception or a
 *                        negative value for interrupt driven reception.
 */
extern void init_sercom_spi(struct sercom_spi_desc_t *descriptor,
                            Sercom *sercom, uint32_t core_freq,
                            uint32_t core_clock_mask, int8_t tx_dma_channel,
                            int8_t rx_dma_channel);

/**
 *  Send and receive data on the SPI bus.
 *
 *  @param spi_inst The SPI instance to use.
 *  @param trans_id The identifier for the created transaction will be
 *                  placed here.
 *  @param baudrate The baudrate to be used for the transaction.
 *  @param cs_pin_group The group index of the chip select pin of the
 *                      peripheral.
 *  @param cs_pin_mask The mask for the chip select pin of the peripheral.
 *  @param out_buffer The buffer from which data should be sent.
 *  @param out_length The number of bytes to be sent.
 *  @param in_buffer The buffer where received data will be placed.
 *  @param in_length The number of bytes to be received.
 *
 *  @return 0 if transaction is successfully queued.
 */
extern uint8_t sercom_spi_start(struct sercom_spi_desc_t *spi_inst,
                                uint8_t *trans_id, uint32_t baudrate,
                                uint8_t cs_pin_group, uint32_t cs_pin_mask,
                                uint8_t *out_buffer, uint16_t out_length,
                                uint8_t * in_buffer, uint16_t in_length);

/**
 *  Queue and SPI transaction that required multiple parts without raising the
 *  CS line.
 *
 *  @param spi_inst The SPI instance to use
 *  @param parts Pointer to an array of descriptors for the transaction stages,
 *               the transaction_id field of each descriptor will be populated
 *  @param num_parts The number of stages in the transaction
 *  @param cs_pin_group The group index of the chip select pin of the
 *                      peripheral.
 *  @param cs_pin_mask The mask for the chip select pin of the peripheral.
 */
extern uint8_t sercom_spi_start_multi_part(struct sercom_spi_desc_t *spi_inst,
                                    struct sercom_spi_transaction_part_t *parts,
                                    uint8_t num_parts, uint8_t cs_pin_group,
                                    uint32_t cs_pin_mask);

/**
 *  Check if an SPI transaction in the queue is complete.
 *
 *  @param spi_inst The SPI instance from which the queue should be used.
 *  @param trans_id The ID of the transaction to check.
 *
 *  @return A non-zero value if the transaction has finished, 0 otherwise.
 */
extern uint8_t sercom_spi_transaction_done(struct sercom_spi_desc_t *spi_inst,
                                           uint8_t trans_id);

/**
 *  Remove an SPI transaction from the queue.
 *
 *  @param spi_inst The SPI instance from which the queue should be used.
 *  @param trans_id The ID of the transaction to clear.
 *
 *  @return 0 if transaction was successfully cleared.
 */
extern uint8_t sercom_spi_clear_transaction(struct sercom_spi_desc_t *spi_inst,
                                            uint8_t trans_id);

#endif /* sercom_spi_h */
