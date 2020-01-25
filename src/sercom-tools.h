/**
 * @file sercom-tools.h
 * @desc A set of utility functions used in all SERCOM modes
 * @author Samuel Dewan
 * @date 2018-12-27
 * Last Author:
 * Last Edited On:
 */

#ifndef sercom_tools_h
#define sercom_tools_h

#include "global.h"

#define SERCOM_IRQ_PRIORITY     1

#define SERCOM_DMA_RX_PRIORITY  2
#define SERCOM_DMA_TX_PRIORITY  1


/**
 *  Abstraction over sercom interrupt handlers which allows a callback to be
 *  specified for each interrupt which is provided context about the SERCOM
 *  instance as well as a pointer to any other context information.
 *  This allows the same function to be used as an interrupt handler on any
 *  SERCOM instance.
 */
extern struct sercom_handler_t {
    void (*handler)(Sercom*, uint8_t, void*);
    void *state;
} sercom_handlers[SERCOM_INST_NUM];


/**
 *  Calculate the baud register value for a given baudrate.
 *
 *  @param baudrate The target baudrate
 *  @param clock The speed of the SERCOMs core clock
 *  @param baud Location where the calculated baud value will be stored
 *  @param sampr Location where the calculated sample rate setting will be
 *               stored
 *
 *  @return 0 if successful, 1 if baud rate not supported
 */
extern uint8_t sercom_calc_async_baud (const uint32_t baudrate,
                                       const uint32_t clock,
                                       volatile uint16_t *baud, uint8_t *sampr);

/**
 *  Calculate the baud register value for a given baudrate.
 *
 *  @param baudrate The target baudrate
 *  @param clock The speed of the SERCOMs core clock
 *  @param baud Location where the calculated baud value will be stored
 *
 *  @return 0 if successful, 1 if baud rate not supported
 */
extern uint8_t sercom_calc_sync_baud (const uint32_t baudrate,
                                      const uint32_t clock,
                                      volatile uint8_t *baud);

/**
 *  Get the index of a SERCOM instance from it's register address
 *
 *  @param inst A pointer to a SERCOM instances registers
 *
 *  @return The index of the SERCOM, -1 if the pointer is not a valid SERCOM
 */
extern int8_t sercom_get_inst_num (Sercom *const inst) __attribute__((const));

/**
 *  Determine the IRQ number for a given SERCOM instance number.
 *
 *  @param inst_num The index of a SERCOM instance
 *
 *  @return The IRQ number of the SERCOM instance or an invalid IRQ number if
 *          the SERCOM index does not exist
 */
static inline IRQn_Type sercom_get_irq_num (uint8_t inst_num)
{
    switch (inst_num) {
        case 0:
            return SERCOM0_IRQn;
        case 1:
            return SERCOM1_IRQn;
        case 2:
            return SERCOM2_IRQn;
        case 3:
            return SERCOM3_IRQn;
#ifdef ID_SERCOM4
        case 4:
            return SERCOM4_IRQn;
#endif
#ifdef ID_SERCOM5
        case 5:
            return SERCOM5_IRQn;
#endif
        default:
            return PERIPH_COUNT_IRQn; // Not a valid IRQ number
    }
}

/**
 *  Determine the generic clock id mask for a given SERCOM instance number.
 *
 *  @param inst_num The index of a SERCOM instance
 *
 *  @return The generic clock id mask for the given SERCOM or 0 if the instance
 *          number is not valid
 */
static inline uint32_t sercom_get_clk_id_mask (uint8_t inst_num)
{
    switch (inst_num) {
        case 0:
            return GCLK_CLKCTRL_ID_SERCOM0_CORE;
        case 1:
            return GCLK_CLKCTRL_ID_SERCOM1_CORE;
        case 2:
            return GCLK_CLKCTRL_ID_SERCOM2_CORE;
        case 3:
            return GCLK_CLKCTRL_ID_SERCOM3_CORE;
#ifdef ID_SERCOM4
        case 4:
            return GCLK_CLKCTRL_ID_SERCOM4_CORE;
#endif
#ifdef ID_SERCOM5
        case 5:
            return GCLK_CLKCTRL_ID_SERCOM5_CORE;
#endif
        default:
            return 0;
    }
}

/**
 *  Determine the power manager APBC register mask for a given SERCOM instance.
 *
 *  @param inst_num The index of a SERCOM instance
 *
 *  @return The APBC register mask for the SERCOM instance or 0 if the instance
 *          number is not valid
 */
static inline uint32_t sercom_get_pm_apb_mask (uint8_t inst_num)
{
    switch (inst_num) {
        case 0:
            return PM_APBCMASK_SERCOM0;
        case 1:
            return PM_APBCMASK_SERCOM1;
        case 2:
            return PM_APBCMASK_SERCOM2;
        case 3:
            return PM_APBCMASK_SERCOM3;
#ifdef ID_SERCOM4
        case 4:
            return PM_APBCMASK_SERCOM4;
#endif
#ifdef ID_SERCOM5
        case 5:
            return PM_APBCMASK_SERCOM5;
#endif
        default:
            return 0;
    }
}

/**
 *  Determine DMAC RX trigger id for a given SERCOM instance
 *
 *  @param inst_num The index of a SERCOM instance
 *
 *  @return The DMAC RX trigger id for the SERCOM instance or 0 if the instance
 *          number is not valid
 */
static inline uint32_t sercom_get_dma_rx_trigger (uint8_t inst_num)
{
    switch (inst_num) {
        case 0:
            return SERCOM0_DMAC_ID_RX;
        case 1:
            return SERCOM1_DMAC_ID_RX;
        case 2:
            return SERCOM2_DMAC_ID_RX;
        case 3:
            return SERCOM3_DMAC_ID_RX;
#ifdef ID_SERCOM4
        case 4:
            return SERCOM4_DMAC_ID_RX;
#endif
#ifdef ID_SERCOM5
        case 5:
            return SERCOM5_DMAC_ID_RX;
#endif
        default:
            return 0;
    }
}

/**
 *  Determine DMAC TX trigger id for a given SERCOM instance
 *
 *  @param inst_num The index of a SERCOM instance
 *
 *  @return The DMAC TX trigger id for the SERCOM instance or 0 if the instance
 *          number is not valid
 */
static inline uint32_t sercom_get_dma_tx_trigger (uint8_t inst_num)
{
    switch (inst_num) {
        case 0:
            return SERCOM0_DMAC_ID_TX;
        case 1:
            return SERCOM1_DMAC_ID_TX;
        case 2:
            return SERCOM2_DMAC_ID_TX;
        case 3:
            return SERCOM3_DMAC_ID_TX;
#ifdef ID_SERCOM4
        case 4:
            return SERCOM4_DMAC_ID_TX;
#endif
#ifdef ID_SERCOM5
        case 5:
            return SERCOM5_DMAC_ID_TX;
#endif
        default:
            return 0;
    }
}

#endif /* sercom_tools_h */
