/*!
** @file PIT.c
**
**  @brief Routines for controlling Periodic Interrupt Timer (PIT) on the TWR-K70F120M.
**
**  This contains the functions for operating the periodic interrupt timer (PIT).
**
** @author Ryan Lawton (12545341) - Ashley More (12545479)
** @date 2019-08-26
*/
/*!
**  @addtogroup PIT_module PIT module documentation
**  @{
*/
/* MODULE PIT */

// new types
#include "macros.h"
#include "PIT.h"
#include "analog.h"
#include "MK70F12.h"
#include "PE_Types.h"
#include "OS.h"
#include "LEDs.h"
#include "packet.h"

// Prototypes
static void PIT0Thread(void* data);
static uint32_t PIT_moduleClk;

// Variable definitions
static uint32_t PIT0ThreadStack[THREAD_STACK_SIZE] __attribute__ ((aligned(0x08)));             /*!< The stack for the PIT thread. */
OS_ECB *PIT0_Semaphore;

bool PIT_Init(const uint32_t moduleClk)
{
  PIT0_Semaphore = OS_SemaphoreCreate(0); //Create PIT Semaphore for Channel 0

  OS_ERROR error;             /*!< Thread content */

  error = OS_ThreadCreate(PIT0Thread, // 5th highest priority
                          NULL,
                          &PITThreadStack[THREAD_STACK_SIZE - 1],
			  PIT0_THREAD_PRIORITY);

  PIT_moduleClk = moduleClk;

  SIM_SCGC6 |= SIM_SCGC6_PIT_MASK; // Enable clock gating to SCGC6 = System Gated Clock Control Register 6

  PIT_MCR &= ~PIT_MCR_MDIS_MASK;
  PIT_MCR = PIT_MCR_FRZ_MASK;  // Freeze debug timer
  PIT_Enable(true);

  PIT_TCTRL0 |= PIT_TCTRL_TIE_MASK; //Enable Interrupts

  NVICICPR2 =  (1 << (68 % 32)); //IRQ = 68, Initialize PIT Interrupts
  NVICISER2 =  (1 << (68 % 32));

  return true;
}



void PIT_Set(const uint32_t period, const bool restart)
{
  uint32_t freqHz = 1e9 / period;
  uint32_t cycleCount = PIT_moduleClk / freqHz;
  uint32_t triggerVal = cycleCount - 1;

  PIT_LDVAL0 = PIT_LDVAL_TSV(triggerVal);

  if (restart)
  {
    PIT_Enable(false);
    PIT_Enable(true);
  }
}



void PIT_Enable(const bool enable)
{
  if (enable)
    PIT_TCTRL0 |= PIT_TCTRL_TEN_MASK;
  else
    PIT_TCTRL0 &= ~ PIT_TCTRL_TEN_MASK;
}



void __attribute__ ((interrupt)) PIT_ISR(void)
{

  OS_ISREnter(); // Start of servicing interrupt

  PIT_TFLG0 |= PIT_TFLG_TIF_MASK; // Acknowledge Interrupt

  OS_SemaphoreSignal(PIT0_Semaphore); // Signal PIT thread to tell it can run
  OS_ISRExit(); // End of servicing interrupt
}



/*! @brief Thread that looks after interrupts made by the periodic interrupt timer.
 *
 *  @param data Thread parameter.
 *  @note Assumes that semaphores are created and communicate properly.
 */
static void PIT0Thread(void* data)
{
  for (;;)
  {
    OS_SemaphoreWait(PIT0_Semaphore,0);

    for (uint8_t channelNb = 0; channelNb < ANALOG_NB_INPUTS; channelNb++)
    {
      if (Synchronous)
      {
        Analog_Get(channelNb);
        Packet_Put(TOWER_ANALOG_INPUT_COMM, channelNb, Analog_Input[channelNb].value.s.Lo, Analog_Input[channelNb].value.s.Hi);
      }
      else
        if (Analog_Get(channelNb))
      	  if (Analog_Input[channelNb].value.l != Analog_Input[channelNb].oldValue.l)
    	    Packet_Put(TOWER_ANALOG_INPUT_COMM, channelNb, Analog_Input[channelNb].value.s.Lo, Analog_Input[channelNb].value.s.Hi);
    }
  }
}



/*!
** @}
*/
