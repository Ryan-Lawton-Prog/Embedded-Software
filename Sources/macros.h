/*
 * macros.h
 *
 *  Created on: 21 Oct 2019
 *      Author: amore
 */

#ifndef SOURCES_MACROS_H_
#define SOURCES_MACROS_H_



/************************************************************
 *  Threads
 ************************************************************/

// Arbitrary thread stack size - big enough for stacking of interrupts and OS use.
#define THREAD_STACK_SIZE 100

// Thread Priorities
#define INIT_THREAD_PRIORITY	0
#define UART_RX_THREAD_PRIORITY	1
#define UART_TX_THREAD_PRIORITY	2
#define FTM_THREAD_PRIORITY	3
#define PIT0_THREAD_PRIORITY	4
#define RTC_THREAD_PRIORITY	5

/************************************************************
 *  TARIFF VALUES
 ************************************************************/
// Time of use
#define PEAK_PERIOD_START	1400
#define PEAK_PERIOD_END		2000
#define SHOULDER1_PERIOD_START	0700
#define SHOULDER1_PERIOD_END	1400
#define SHOULDER2_PERIOD_START	2000
#define SHOULDER2_PERIOD_END	2200

//

/************************************************************
 *  DEM CONSTANTS
 ************************************************************/

#define SAMPLE_PERIOD_NUM	16
#define SAMPLE_FREQUENCY	50



#endif /* SOURCES_MACROS_H_ */
