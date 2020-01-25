/**
 * @file tc.h
 * @desc Timer Counter Driver
 * @author Samuel Dewan
 * @date 2019-04-27
 * Last Author:
 * Last Edited On:
 */

#ifndef tc_h
#define tc_h

#include "global.h"

/**
 *  Initilize a Timer Counter to generate events at a given period and start it.
 *
 *  @param tc The Timer Counter instance to be initilized
 *  @param period The period in milliseconds with which events should be
 *                generated
 *  @param clock_mask Mask for the Generic Clock Generator which should provide
 *                    the Generic Clock for the Timer Counter
 *  @param clock_freq The frequency of the Generic Clock Generator for the Timer
 *                    Counter
 *
 *  @return 0 if successfull
 */
extern uint8_t init_tc_periodic_event (Tc *tc, uint32_t period,
                                       uint32_t clock_mask,
                                       uint32_t clock_freq);

/**
 *  Get the EVSYS event generator ID for a Timer Counter's overflow event.
 *
 *  @param tc The Timer Counter for which the event ID should be found
 *
 *  @return The event generator ID for the Timer Counter's overflow event
 */
extern uint8_t tc_get_evsys_gen_ovf_id (Tc *tc) __attribute__((const));

#endif /* tc_h */
