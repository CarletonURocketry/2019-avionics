/**
 * @file rn2483.c
 * @desc Driver for RN2483 LoRa radio
 * @author Samuel Dewan
 * @date 2019-06-07
 * Last Author:
 * Last Edited On:
 */

#include "rn2483.h"

#include <string.h>
#include <stdlib.h>

#define RN2483_ANALOG_PINS_MASK ((1<<RN2483_GPIO0)  | (1<<RN2483_GPIO1)  | \
                                 (1<<RN2483_GPIO2)  | (1<<RN2483_GPIO3)  | \
                                 (1<<RN2483_GPIO5)  | (1<<RN2483_GPIO6)  | \
                                 (1<<RN2483_GPIO7)  | (1<<RN2483_GPIO8)  | \
                                 (1<<RN2483_GPIO9)  | (1<<RN2483_GPIO10) | \
                                 (1<<RN2483_GPIO11) | (1<<RN2483_GPIO12) | \
                                 (1<<RN2483_GPIO13))
#define RN2483_PIN_SUPPORTS_ANA(x) (RN2483_ANALOG_PINS_MASK & (1<<x))

static const char* const RN2483_RSP_OK = "ok";
#define RN2483_RSP_OK_LEN 2
static const char* const RN2483_RSP_RESET_OK = "RN2483";
#define RN2483_RSP_RESET_OK_LEN 6
static const char* const RN2483_RSP_TX_OK = "radio_tx_ok";
#define RN2483_RSP_TX_OK_LEN 11
static const char* const RN2483_RSP_RX_OK = "radio_rx ";
#define RN2483_RSP_RX_OK_LEN 9
static const char* const RN2483_RSP_PAUSE_MAC = "4294967245";
#define RN2483_RSP_PAUSE_MAC_LEN 10


static const char* const RN2483_CMD_RESET = "sys reset\r\n";
static const char* const RN2483_CMD_WDT = "radio set wdt 0\r\n";
static const char* const RN2483_CMD_PAUSE_MAC = "mac pause\r\n";

static const char* const RN2483_CMD_MODE = "radio set mod lora\r\n";
static const char* const RN2483_CMD_FREQ = "radio set freq ";
static const char* const RN2483_CMD_PWR = "radio set pwr ";
static const char* const RN2483_CMD_SF = "radio set sf ";
static const char* const RN2483_CMD_CRC = "radio set crc ";
static const char* const RN2483_CMD_IQI = "radio set iqi ";
static const char* const RN2483_CMD_CR = "radio set cr ";
static const char* const RN2483_CMD_SYNC = "radio set sync ";
static const char* const RN2483_CMD_BW = "radio set bw ";

static const char* const RN2483_CMD_TX = "radio tx ";
static const char* const RN2483_CMD_RX = "radio rx ";
static const char* const RN2483_CMD_SNR = "radio get snr\r\n";

static const char* const RN2483_CMD_SET_PINMODE = "sys set pinmode ";
static const char* const RN2483_CMD_SET_PINDIG = "sys set pindig ";
static const char* const RN2483_CMD_GET_PINDIG = "sys get pindig ";
static const char* const RN2483_CMD_GET_PINANA = "sys get pinana ";

static const char* const RN2483_STR_ON = "on\r\n";
static const char* const RN2483_STR_OFF = "off\r\n";

static const char* const RN2483_STR_SF_7 = "sf7\r\n";
static const char* const RN2483_STR_SF_8 = "sf8\r\n";
static const char* const RN2483_STR_SF_9 = "sf9\r\n";
static const char* const RN2483_STR_SF_10 = "sf10\r\n";
static const char* const RN2483_STR_SF_11 = "sf11\r\n";
static const char* const RN2483_STR_SF_12 = "sf12\r\n";

static const char* const RN2483_STR_CR_4_5 = "4/5\r\n";
static const char* const RN2483_STR_CR_4_6 = "4/6\r\n";
static const char* const RN2483_STR_CR_4_7 = "4/7\r\n";
static const char* const RN2483_STR_CR_4_8 = "4/8\r\n";

static const char* const RN2483_STR_BW125 = "125\r\n";
static const char* const RN2483_STR_BW250 = "250\r\n";
static const char* const RN2483_STR_BW500 = "500\r\n";

static const char* const RN2483_STR_PINSTATE_HIGH = " 1\r\n";
static const char* const RN2483_STR_PINSTATE_LOW = " 0\r\n";

static const char* const RN2483_STR_PIN_MODE_DIGOUT = " digout\r\n";
static const char* const RN2483_STR_PIN_MODE_DIGIN = " digin\r\n";
static const char* const RN2483_STR_PIN_MODE_ANA = " ana\r\n";

static const char* const RN2483_PIN_NAMES[] = { "GPIO0",  "GPIO1",  "GPIO2",
                                                "GPIO3",  "GPIO4",  "GPIO5",
                                                "GPIO6",  "GPIO7",  "GPIO8",
                                                "GPIO9",  "GPIO10",  "GPIO11",
                                                "GPIO12",  "GPIO13",
                                                "UART_CTS", "UART_RTS", "TEST0",
                                                "TEST1" };


void init_rn2483 (struct rn2483_desc_t *inst, struct sercom_uart_desc_t *uart,
                  uint32_t freq, int8_t power, enum rn2483_sf spreading_factor,
                  enum rn2483_cr coding_rate, enum rn2483_bw bandwidth,
                  uint8_t send_crc, uint8_t invert_qi, uint8_t sync_byte)
{
    inst->uart = uart;
    
    
    if (freq > RN2483_FREQ_MAX) {
        inst->settings.freq = RN2483_FREQ_MAX;
    } else if (freq < RN2483_FREQ_MIN) {
        inst->settings.freq = RN2483_FREQ_MIN;
    } else {
        inst->settings.freq = freq;
    }
    
    if (power > RN2483_PWR_MAX) {
        inst->settings.power = RN2483_PWR_MAX;
    } else if (power < RN2483_PWR_MIN) {
        inst->settings.power = RN2483_PWR_MIN;
    } else {
        inst->settings.power = power;
    }
    
    inst->settings.spreading_factor = spreading_factor;
    inst->settings.coding_rate = coding_rate;
    inst->settings.bandwidth = bandwidth;
    inst->settings.crc = !!send_crc;
    inst->settings.invert_qi = !!invert_qi;
    inst->settings.sync_byte = sync_byte;
    
    // Initialize GPIO pins to inputs
    for (enum rn2483_pin pin = 0; pin < RN2483_NUM_PINS; pin++) {
        inst->pins[pin].raw = (RN2483_PIN_DESC_MODE(RN2483_PIN_MODE_INPUT) |
                               RN2483_PIN_DESC_MODE_DIRTY);
    }
    
    // Start by reseting module
    inst->state = RN2483_RESET;
    
    inst->waiting_for_line = 0;
    inst->cmd_ready = 0;
    inst->out_pos = 0;
}

/**
 *  Handle a state where a command is sent and a response is read back.
 *
 *  @param inst The RN2483 driver instance
 *  @param out_buffer The buffer from which the command will be sent, this
 *                    buffer must not change until the state is complete
 *  @param expected_response The response expected from the radio module
 *  @param compare_length The maximum number of characters to compare between
 *                        the response and expected response
 *  @param next_state The state which should be entered next
 *
 *  @return 1 if the FSM should move to the next state, 0 otherwise
 */
static uint8_t handle_state (struct rn2483_desc_t *inst, const char *out_buffer,
                             const char *expected_response,
                             size_t compare_length,
                             enum rn2483_state next_state)
{
    if (inst->waiting_for_line) {
        /* response received */
        inst->waiting_for_line = 0;
        // Clear output position for next transaction
        inst->out_pos = 0;
        // Clear command ready flag for next state
        inst->cmd_ready = 0;
        
        // Get the received line
        sercom_uart_get_line(inst->uart, inst->buffer, RN2483_BUFFER_LEN);
        
        if (!strncmp(inst->buffer, expected_response, compare_length)) {
            // Success! Go to next state
            inst->state = next_state;
            return 0;
        } else {
            // Something went wrong, go to failed state
            inst->state = RN2483_FAILED;
            return 2;
        }
    } else {
        /* send command */
        // Send as much of the command as we can fit in the SERCOM
        // driver's output buffer
        inst->out_pos += sercom_uart_put_string(inst->uart,
                                                (out_buffer + inst->out_pos));
        // If we have sent the whole command we need to wait for the
        // response from the radio
        inst->waiting_for_line = inst->out_pos == strlen(out_buffer);
        return 1;
    }
}


void rn2483_service (struct rn2483_desc_t *inst)
{
    if (inst->waiting_for_line && !sercom_uart_has_line(inst->uart)) {
        // waiting for a line and a new line has not yet been received
        return;
    }
    
    switch (inst->state) {
        case RN2483_RESET:
            // Handle writing of command and reception of response
            if (handle_state(inst, RN2483_CMD_RESET, RN2483_RSP_RESET_OK,
                             RN2483_RSP_RESET_OK_LEN, RN2483_WRITE_WDT)) {
                break;
            }
            /* fall through */
        case RN2483_WRITE_WDT:
            // Handle writing of command and reception of response
            if (handle_state(inst, RN2483_CMD_WDT, RN2483_RSP_OK,
                             RN2483_RSP_OK_LEN, RN2483_PAUSE_MAC)) {
                break;
            }
            /* fall through */
        case RN2483_PAUSE_MAC:
            // Handle writing of command and reception of response
            if (handle_state(inst, RN2483_CMD_PAUSE_MAC, RN2483_RSP_PAUSE_MAC,
                             RN2483_RSP_PAUSE_MAC_LEN, RN2483_WRITE_MODE)) {
                break;
            }
            /* fall through */
        case RN2483_WRITE_MODE:
            // Handle writing of command and reception of response
            if (handle_state(inst, RN2483_CMD_MODE, RN2483_RSP_OK,
                             RN2483_RSP_OK_LEN, RN2483_WRITE_FREQ)) {
                break;
            }
            /* fall through */
        case RN2483_WRITE_FREQ:
            // Update command if required
            if (!inst->cmd_ready) {
                // Create command
                strcpy(inst->buffer, RN2483_CMD_FREQ);
                size_t i = strlen(RN2483_CMD_FREQ);
                utoa(inst->settings.freq, inst->buffer + i, 10);
                i = strlen(inst->buffer);
                *(inst->buffer + i + 0) = '\r';
                *(inst->buffer + i + 1)  = '\n';
                *(inst->buffer + i + 2)  = '\0';
                inst->cmd_ready = 1;
            }
            // Handle writing of command and reception of response
            if (handle_state(inst, inst->buffer, RN2483_RSP_OK,
                             RN2483_RSP_OK_LEN, RN2483_WRITE_PWR)) {
                break;
            }
            /* fall through */
        case RN2483_WRITE_PWR:
            // Update command if required
            if (!inst->cmd_ready) {
                // Create command
                strcpy(inst->buffer, RN2483_CMD_PWR);
                size_t i = strlen(RN2483_CMD_PWR);
                itoa(inst->settings.power, inst->buffer + i, 10);
                i = strlen(inst->buffer);
                *(inst->buffer + i + 0) = '\r';
                *(inst->buffer + i + 1)  = '\n';
                *(inst->buffer + i + 2)  = '\0';
                inst->cmd_ready = 1;
            }
            // Handle writing of command and reception of response
            if (handle_state(inst, inst->buffer, RN2483_RSP_OK,
                             RN2483_RSP_OK_LEN, RN2483_WRITE_SF)) {
                break;
            }
            /* fall through */
        case RN2483_WRITE_SF:
            // Update command if required
            if (!inst->cmd_ready) {
                // Create command
                strcpy(inst->buffer, RN2483_CMD_SF);
                size_t i = strlen(RN2483_CMD_SF);
                switch (inst->settings.spreading_factor) {
                    case RN2483_SF_SF7:
                        strcpy(inst->buffer + i, RN2483_STR_SF_7);
                        break;
                    case RN2483_SF_SF8:
                        strcpy(inst->buffer + i, RN2483_STR_SF_8);
                        break;
                    case RN2483_SF_SF9:
                        strcpy(inst->buffer + i, RN2483_STR_SF_9);
                        break;
                    case RN2483_SF_SF10:
                        strcpy(inst->buffer + i, RN2483_STR_SF_10);
                        break;
                    case RN2483_SF_SF11:
                        strcpy(inst->buffer + i, RN2483_STR_SF_11);
                        break;
                    case RN2483_SF_SF12:
                        strcpy(inst->buffer + i, RN2483_STR_SF_12);
                        break;
                }
                inst->cmd_ready = 1;
            }
            // Handle writing of command and reception of response
            if (handle_state(inst, inst->buffer, RN2483_RSP_OK,
                             RN2483_RSP_OK_LEN, RN2483_WRITE_CRC)) {
                break;
            }
            /* fall through */
        case RN2483_WRITE_CRC:
            // Update command if required
            if (!inst->cmd_ready) {
                // Create command
                strcpy(inst->buffer, RN2483_CMD_CRC);
                size_t i = strlen(RN2483_CMD_CRC);
                if (inst->settings.crc) {
                    strcpy(inst->buffer + i, RN2483_STR_ON);
                } else {
                    strcpy(inst->buffer + i, RN2483_STR_OFF);
                }
                inst->cmd_ready = 1;
            }
            // Handle writing of command and reception of response
            if (handle_state(inst, inst->buffer, RN2483_RSP_OK,
                             RN2483_RSP_OK_LEN, RN2483_WRITE_IQI)) {
                break;
            }
            /* fall through */
        case RN2483_WRITE_IQI:
            // Update command if required
            if (!inst->cmd_ready) {
                // Create command
                strcpy(inst->buffer, RN2483_CMD_IQI);
                size_t i = strlen(RN2483_CMD_IQI);
                if (inst->settings.invert_qi) {
                    strcpy(inst->buffer + i, RN2483_STR_ON);
                } else {
                    strcpy(inst->buffer + i, RN2483_STR_OFF);
                }
                inst->cmd_ready = 1;
            }
            // Handle writing of command and reception of response
            if (handle_state(inst, inst->buffer, RN2483_RSP_OK,
                             RN2483_RSP_OK_LEN, RN2483_WRITE_CR)) {
                break;
            }
            /* fall through */
        case RN2483_WRITE_CR:
            // Update command if required
            if (!inst->cmd_ready) {
                // Create command
                strcpy(inst->buffer, RN2483_CMD_CR);
                size_t i = strlen(RN2483_CMD_CR);
                switch (inst->settings.coding_rate) {
                    case RN2483_CR_4_5:
                        strcpy(inst->buffer + i, RN2483_STR_CR_4_5);
                        break;
                    case RN2483_CR_4_6:
                        strcpy(inst->buffer + i, RN2483_STR_CR_4_6);
                        break;
                    case RN2483_CR_4_7:
                        strcpy(inst->buffer + i, RN2483_STR_CR_4_7);
                        break;
                    case RN2483_CR_4_8:
                        strcpy(inst->buffer + i, RN2483_STR_CR_4_8);
                        break;
                }
                inst->cmd_ready = 1;
            }
            // Handle writing of command and reception of response
            if (handle_state(inst, inst->buffer, RN2483_RSP_OK,
                             RN2483_RSP_OK_LEN, RN2483_WRITE_SYNC)) {
                break;
            }
            /* fall through */
        case RN2483_WRITE_SYNC:
            // Update command if required
            if (!inst->cmd_ready) {
                // Create command
                strcpy(inst->buffer, RN2483_CMD_SYNC);
                size_t i = strlen(RN2483_CMD_SYNC);
                utoa(inst->settings.sync_byte, inst->buffer + i, 10);
                i = strlen(inst->buffer);
                *(inst->buffer + i + 0) = '\r';
                *(inst->buffer + i + 1)  = '\n';
                *(inst->buffer + i + 2)  = '\0';
                inst->cmd_ready = 1;
            }
            // Handle writing of command and reception of response
            if (handle_state(inst, inst->buffer, RN2483_RSP_OK,
                             RN2483_RSP_OK_LEN, RN2483_WRITE_BW)) {
                break;
            }
            /* fall through */
        case RN2483_WRITE_BW:
            // Update command if required
            if (!inst->cmd_ready) {
                // Create command
                strcpy(inst->buffer, RN2483_CMD_BW);
                size_t i = strlen(RN2483_CMD_BW);
                switch (inst->settings.bandwidth) {
                    case RN2483_BW_125:
                        strcpy(inst->buffer + i, RN2483_STR_BW125);
                        break;
                    case RN2483_BW_250:
                        strcpy(inst->buffer + i, RN2483_STR_BW250);
                        break;
                    case RN2483_BW_500:
                        strcpy(inst->buffer + i, RN2483_STR_BW500);
                        break;
                }
                inst->cmd_ready = 1;
            }
            // Handle writing of command and reception of response
            if (!handle_state(inst, inst->buffer, RN2483_RSP_OK,
                              RN2483_RSP_OK_LEN, RN2483_IDLE)) {
                break;
            }
            // Prevent sleep because we are about to call allow_sleep() when we
            // fall through
            inhibit_sleep();
            /* fall through */
        case RN2483_RETURN_TO_IDLE:
            inst->state = RN2483_IDLE;
            // Returning to idle after a GPIO command, allow sleep again since
            // the command has finished
            allow_sleep();
            /* fall through */
        case RN2483_IDLE:
            // If we are otherwise idle we should check to see if there are any
            // GPIO commands that need to send
            
            /* Check if enough time has elapsed that we should mark our inputs
               dirty */
            if (RN2483_GPIO_UPDATE_PERIOD &&
                ((millis - inst->last_polled) > RN2483_GPIO_UPDATE_PERIOD)) {
                inst->last_polled = millis;
                rn2483_poll_gpio(inst);
            }
            
            /* Check for pins with dirty modes */
            for (inst->current_pin = 0; inst->current_pin < RN2483_NUM_PINS;
                    inst->current_pin++) {
                if (inst->pins[inst->current_pin].mode_dirty) {
                    break;
                }
            }
            if (inst->current_pin < RN2483_NUM_PINS) {
                // There is a pin with a dirty mode, update it
                inst->state = RN2483_SET_PIN_MODE;
                // Prevent sleep so that that this function is run again ASAP
                inhibit_sleep();
                break;
            }
            
            /* Check for pins with dirty values */
            for (inst->current_pin = 0; inst->current_pin < RN2483_NUM_PINS;
                    inst->current_pin++) {
                if (inst->pins[inst->current_pin].value_dirty) {
                    break;
                }
            }
            if (inst->current_pin < RN2483_NUM_PINS) {
                // There is a pin with a dirty value, update it
                if (inst->pins[inst->current_pin].mode ==
                        RN2483_PIN_MODE_OUTPUT) {
                    inst->state = RN2483_SET_PINDIG;
                } else {
                    inst->state = RN2483_GET_PIN_VALUE;
                }
                // Prevent sleep so that that this function is run again ASAP
                inhibit_sleep();
                break;
            }
            break;
        case RN2483_SEND:
            // Handle writing of command and reception of response
            if (!handle_state(inst, inst->buffer, RN2483_RSP_OK,
                              RN2483_RSP_OK_LEN, RN2483_SEND_WAIT)) {
                // Got first response to send command
                // Send has started, wait for the second response
                inst->waiting_for_line = 1;
            } else if (inst->state == RN2483_FAILED) {
                // Send failed, go back to idle
                inst->state = RN2483_IDLE;
            }
            break;
        case RN2483_SEND_WAIT:
            // Wait for second response and return to idle
            handle_state(inst, inst->buffer, RN2483_RSP_TX_OK,
                         RN2483_RSP_TX_OK_LEN, RN2483_IDLE);
            if (inst->state == RN2483_FAILED) {
                // Send failed, go back to idle
                inst->state = RN2483_IDLE;
            }
            break;
        case RN2483_RECEIVE:
            // Handle writing of command and reception of response
            if (!handle_state(inst, inst->buffer, RN2483_RSP_OK,
                              RN2483_RSP_OK_LEN, RN2483_RECEIVE_WAIT)) {
                // Got first response to receive command
                // Receive has started, wait for the second response
                inst->waiting_for_line = 1;
            } else if (inst->state == RN2483_FAILED) {
                // Receive failed, go back to idle
                inst->state = RN2483_IDLE;
            }
            break;
        case RN2483_RECEIVE_WAIT:
            if (handle_state(inst, inst->buffer, RN2483_RSP_RX_OK,
                             RN2483_RSP_RX_OK_LEN, RN2483_GET_SNR)) {
                if (inst->state == RN2483_FAILED) {
                    // Receive timed out
                    inst->receive_callback(inst, inst->callback_context, NULL,
                                           0, 0);
                    // go back to idle
                    inst->state = RN2483_IDLE;
                }
                break;
            }
            /* fall through */
        case RN2483_GET_SNR:
            if (!handle_state(inst, RN2483_CMD_SNR, RN2483_RSP_OK, 0,
                              RN2483_IDLE)) {
                /* Got the SNR from the radio */
                // Parse received SNR
                int8_t snr = (int8_t) strtol(inst->buffer, NULL, 10);
                
                // Parse packet
                char *s = inst->buffer + RN2483_RSP_RX_OK_LEN + 1;
                size_t str_len = strlen(s);
                uint8_t len = str_len / 2;
                uint8_t data[len];
                
                s += str_len;
                for (uint8_t i = len; i > 0; i--) {
                    // Move back to previous byte
                    s -= 2;
                    // Parse byte
                    data[i - 1] = (uint8_t) strtoul(s, NULL, 16);
                    // Place null char so that previous byte can be parsed
                    *s = '\0';
                }
                
                inst->receive_callback(inst, inst->callback_context, data, len,
                                       snr);
            }
            break;
        case RN2483_SET_PIN_MODE:
            // Update command if required
            if (!inst->cmd_ready) {
                // Create command
                strcpy(inst->buffer, RN2483_CMD_SET_PINMODE);
                size_t i = strlen(inst->buffer);
                strcpy(inst->buffer + i, RN2483_PIN_NAMES[inst->current_pin]);
                i = strlen(inst->buffer);
                switch (inst->pins[inst->current_pin].mode) {
                    case RN2483_PIN_MODE_OUTPUT:
                        strcpy(inst->buffer + i, RN2483_STR_PIN_MODE_DIGOUT);
                        break;
                    case RN2483_PIN_MODE_INPUT:
                        strcpy(inst->buffer + i, RN2483_STR_PIN_MODE_DIGIN);
                        break;
                    case RN2483_PIN_MODE_ANALOG:
                        strcpy(inst->buffer + i, RN2483_STR_PIN_MODE_ANA);
                        break;
                }
                inst->cmd_ready = 1;
                // The idle state will have prevented sleep to get here ASAP,
                // now that we are waiting for a response form the module we
                // should allow sleep again
                allow_sleep();
            }
            // Handle writing of command and reception of response
            if (handle_state(inst, inst->buffer, RN2483_RSP_OK,
                             RN2483_RSP_OK_LEN, RN2483_RETURN_TO_IDLE)) {
                // Pin's mode is now clean
                inst->pins[inst->current_pin].mode_dirty = 0;
                // Prevent sleep to get back to the idle state ASAP
                inhibit_sleep();
            }
            break;
        case RN2483_SET_PINDIG:
            // Update command if required
            if (!inst->cmd_ready) {
                // Create command
                strcpy(inst->buffer, RN2483_CMD_SET_PINDIG);
                size_t i = strlen(inst->buffer);
                strcpy(inst->buffer + i, RN2483_PIN_NAMES[inst->current_pin]);
                i = strlen(inst->buffer);
                if (inst->pins[inst->current_pin].value) {
                    strcpy(inst->buffer + i, RN2483_STR_PINSTATE_HIGH);
                } else {
                    strcpy(inst->buffer + i, RN2483_STR_PINSTATE_LOW);
                }
                inst->cmd_ready = 1;
                // The idle state will have prevented sleep to get here ASAP,
                // now that we are waiting for a response form the module we
                // should allow sleep again
                allow_sleep();
            }
            // Handle writing of command and reception of response
            if (handle_state(inst, inst->buffer, RN2483_RSP_OK,
                             RN2483_RSP_OK_LEN, RN2483_RETURN_TO_IDLE)) {
                // Pin's value is now clean
                inst->pins[inst->current_pin].value_dirty = 0;
                // Prevent sleep to get back to the idle state ASAP
                inhibit_sleep();
            }
            break;
        case RN2483_GET_PIN_VALUE:
            // Update command if required
            if (!inst->cmd_ready) {
                // Create command
                switch (inst->pins[inst->current_pin].mode) {
                    case RN2483_PIN_MODE_INPUT:
                        strcpy(inst->buffer, RN2483_CMD_GET_PINDIG);
                        break;
                    case RN2483_PIN_MODE_ANALOG:
                        strcpy(inst->buffer, RN2483_CMD_GET_PINANA);
                        break;
                    default:
                        // This should not happen, go back to idle
                        inst->state = RN2483_IDLE;
                        return;
                }
                size_t i = strlen(inst->buffer);
                strcpy(inst->buffer + i, RN2483_PIN_NAMES[inst->current_pin]);
                i = strlen(inst->buffer);
                *(inst->buffer + i + 0) = '\r';
                *(inst->buffer + i + 1)  = '\n';
                *(inst->buffer + i + 2)  = '\0';
                inst->cmd_ready = 1;
                // The idle state will have prevented sleep to get here ASAP,
                // now that we are waiting for a response form the module we
                // should allow sleep again
                allow_sleep();
            }
            // Handle writing of command and reception of response
            if (!handle_state(inst, inst->buffer, RN2483_RSP_OK, 0,
                              RN2483_RETURN_TO_IDLE)) {
                // Parse and store received value
                uint16_t value = (uint16_t) strtoul(inst->buffer, NULL, 10);
                inst->pins[inst->current_pin].value = value;
                // Pin value is no longer dirty
                inst->pins[inst->current_pin].value_dirty = 0;
                // Prevent sleep to get back to the idle state ASAP
                inhibit_sleep();
            }
            break;
        case RN2483_FAILED:
            // This should not happen
            break;
    }
}

enum rn2483_operation_result rn2483_send (struct rn2483_desc_t *inst,
                                          uint8_t const *data, uint8_t length)
{
    if (inst->state != RN2483_IDLE) {
        return RN2483_OP_BUSY;
    } else if (length > ((RN2483_BUFFER_LEN - (strlen(RN2483_CMD_TX) + 2))
                         / 2)) {
        // Message is too large to be sent
        return RN2483_OP_TOO_LONG;
    }
    
    // Create command
    strcpy(inst->buffer, RN2483_CMD_TX);
    char *insert_point = inst->buffer + strlen(RN2483_CMD_TX);
    
    for (uint8_t i = 0; i < length; i++) {
        if (data[i] < 16) {
            *insert_point = '0';
            utoa(data[i], insert_point + 1, 16);
        } else {
            utoa(data[i], insert_point, 16);
        }
        insert_point = inst->buffer + strlen(inst->buffer);
    }
    
    *insert_point = '\r';
    *(insert_point + 1) = '\n';
    *(insert_point + 2) = '\0';
    inst->cmd_ready = 1;
    
    // Set state and start sending command
    inst->state = RN2483_SEND;
    rn2483_service(inst);
    
    return RN2483_OP_SUCCESS;
}

enum rn2483_operation_result rn2483_receive (struct rn2483_desc_t *inst,
                                             uint32_t window_size,
                                             rn2483_recv_callback callback,
                                             void *context)
{
    if (inst->state != RN2483_IDLE) {
        return RN2483_OP_BUSY;
    }
    
    // Store receive callback
    inst->receive_callback = callback;
    inst->callback_context = context;
    
    // Create command
    strcpy(inst->buffer, RN2483_CMD_RX);
    size_t i = strlen(RN2483_CMD_RX);
    utoa(window_size, inst->buffer + i, 10);
    i = strlen(inst->buffer);
    *(inst->buffer + i + 0) = '\r';
    *(inst->buffer + i + 1)  = '\n';
    *(inst->buffer + i + 2)  = '\0';
    inst->cmd_ready = 1;
    
    // Set state and start receive command
    inst->state = RN2483_RECEIVE;
    rn2483_service(inst);
    
    return RN2483_OP_SUCCESS;
}



void rn2483_poll_gpio(struct rn2483_desc_t *inst)
{
    for (enum rn2483_pin pin = 0; pin < RN2483_NUM_PINS; pin++) {
        // Mark value dirty if pin has been explicitly set as an
        // input
        if (((inst->pins[pin].mode == RN2483_PIN_MODE_INPUT) ||
             (inst->pins[pin].mode == RN2483_PIN_MODE_ANALOG)) &&
            (inst->pins[pin].mode_explicit)) {
            inst->pins[pin].value_dirty = 1;
        }
    }
}

uint8_t rn2483_poll_gpio_in_progress(struct rn2483_desc_t *inst)
{
    for (enum rn2483_pin pin = 0; pin < RN2483_NUM_PINS; pin++) {
        // If pin is an input and is being polled return 1
        if (((inst->pins[pin].mode == RN2483_PIN_MODE_INPUT) ||
             (inst->pins[pin].mode == RN2483_PIN_MODE_ANALOG)) &&
            (inst->pins[pin].value_dirty)) {
            return 1;
        }
    }
    return 0;
}

uint8_t rn2483_set_pin_mode(struct rn2483_desc_t *inst, enum rn2483_pin pin,
                            enum rn2483_pin_mode mode)
{
    // If the mode has not changed, don't bother sending command to radio
    if (inst->pins[pin].mode == mode) {
        return 0;
    }
    
    // Check that mode is valid for pin
    if ((mode == RN2483_PIN_MODE_ANALOG) && !RN2483_PIN_SUPPORTS_ANA(pin)) {
        return 1;
    }
    
    // Update pin mode in cache, reset the pin's value to 0, and mark the pin's
    // mode has having been explicitly set.
    inst->pins[pin].raw = (RN2483_PIN_DESC_VALUE(0) |
                           RN2483_PIN_DESC_MODE(mode) |
                           RN2483_PIN_DESC_MODE_DIRTY |
                           RN2483_PIN_DESC_VALUE_DIRTY |
                           RN2483_PIN_DESC_MODE_EXP);
    
    // Run service to start sending command to radio if possible
    rn2483_service(inst);
    
    return 0;
}

enum rn2483_pin_mode rn2483_get_pin_mode(struct rn2483_desc_t *inst,
                                         enum rn2483_pin pin)
{
    return inst->pins[pin].mode;
}

void rn2483_set_output(struct rn2483_desc_t *inst, enum rn2483_pin pin,
                       uint8_t value)
{
    // If the value has not changed, don't bother sending command to radio
    if (inst->pins[pin].value == value) {
        return;
    }
    
    // Update pin value in cache
    inst->pins[pin].value = value;
    inst->pins[pin].value_dirty = 1;
    // Run service to start sending command to radio if possible
    rn2483_service(inst);
}

void rn2483_toggle_output(struct rn2483_desc_t *inst, enum rn2483_pin pin)
{
    // Update pin value in cache
    inst->pins[pin].value = !inst->pins[pin].value;
    inst->pins[pin].value_dirty = 1;
    // Run service to start sending command to radio if possible
    rn2483_service(inst);
}
