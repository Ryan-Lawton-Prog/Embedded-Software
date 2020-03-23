/*! @file
 *
 *  @brief Routines to access the LEDs on the TWR-K70F120M.
 *
 *  This contains the functions for operating the LEDs.
 *
 *  @author PMcL
 *  @date 2015-08-15
 */

#ifndef LEDS_H
#define LEDS_H

// new types
#include "types.h"
#include "MK70F12.h"

/*! @brief LED to pin mapping on the TWR-K70F120M
 *
 */
typedef enum
{
  LED_ORANGE = (1 << 11),  // 2048		// Port Pin = PTA11
  LED_YELLOW = (1 << 28),  // 268435456		// Port Pin = PTA28
  LED_GREEN  = (1 << 29),  // 536870912		// Port Pin = PTA29
  LED_BLUE   = (1 << 10)   // 1024		// Port Pin = PTA10
} TLED;

/*! @brief Sets up the LEDs before first use.
 *
 *  @return bool - TRUE if the LEDs were successfully initialized.
 */
bool LEDs_Init(void);
 
/*! @brief Turns an LED on.
 *
 *  @param color The color of the LED to turn on.
 *  @note Assumes that LEDs_Init has been called.
 */
void LEDs_On(const TLED color);
 
/*! @brief Turns off an LED.
 *
 *  @param color THe color of the LED to turn off.
 *  @note Assumes that LEDs_Init has been called.
 */
void LEDs_Off(const TLED color);

/*! @brief Toggles an LED.
 *
 *  @param color THe color of the LED to toggle.
 *  @note Assumes that LEDs_Init has been called.
 */
void LEDs_Toggle(const TLED color);

#endif