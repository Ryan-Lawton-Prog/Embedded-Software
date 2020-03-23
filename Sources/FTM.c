/*!
** @file FTM.c
**
**  @brief Routines for setting up the FlexTimer module (FTM) on the TWR-K70F120M.
**
**  This contains the functions for operating the FlexTimer module (FTM).
**
** @author Ryan Lawton (12545341) - Ashley More (12545479)
** @date 2019-09-02
*/
/*!
**  @addtogroup FTM_module FTM module documentation
**  @{
*/
/* MODULE FTM */

#include "macros.h"
#include "FTM.h"
#include "LEDs.h"
#include "MK70F12.h"
#include "OS.h"
#include "PE_Types.h"
#include "commands.h"

#define CHANNELS 8
#define FREQUENCY_CLOCK 2

// Prototypes
static void FTM0Thread(void* data);

// Variable Declarations
static uint32_t FTMThreadStack[THREAD_STACK_SIZE] __attribute__ ((aligned(0x08)));             /*!< The stack for the RTC thread. */
OS_ECB *FTM_Semaphore;

bool FTM_Init()
{
  FTM_Semaphore = OS_SemaphoreCreate(0); //Create FTM Semaphore

  OS_ERROR error;             /*!< Thread content */

  error = OS_ThreadCreate(FTM0Thread, // 4th highest priority
                          NULL,
                          &FTMThreadStack[THREAD_STACK_SIZE - 1],
			  FTM_THREAD_PRIORITY);

  SIM_SCGC6 |= SIM_SCGC6_FTM0_MASK; // Enable clock gating to SCGC6 = System Gated Clock Control Register 6

  FTM0_CNTIN = ~FTM_CNTIN_INIT_MASK;

  FTM0_MOD = FTM_MOD_MOD_MASK; // Initializes FTM Counter

  FTM0_CNT = ~FTM_CNT_COUNT_MASK; // Checks count

  FTM0_SC |= FTM_SC_CLKS(FREQUENCY_CLOCK); // Enable FTM overflow interrupts

  FTM0_MODE |= FTM_MODE_FTMEN_MASK;

  NVICICPR1 = (1<<(62 % 32)); //IRQ = 62, Initialize FTM0 Interrupts
  NVICISER1 = (1<<(62 % 32));

  return true;
}



bool FTM_Set(const TFTMChannel* const aFTMChannel)
{
  if (aFTMChannel->timerFunction == TIMER_FUNCTION_INPUT_CAPTURE)
  {
    // Set Mode to Input Capture
    FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_MSA_MASK; // set 0
    FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_MSB_MASK; // set 0
  }
  else
  {
    // Set Mode to Output Compare
    FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_MSB_MASK; // set 0
    FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_MSA_MASK;  // set 1
  }

  switch (aFTMChannel->ioType.inputDetection)
  {
    case 1:  //Capture on rising edge
      FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_ELSB_MASK;
      FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_ELSA_MASK;
      break;

    case 2:  // Capture on falling edge
      FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_ELSB_MASK;
      FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_ELSA_MASK;
      break;

    case 3:  // Capture on rising or falling edge
      FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_ELSB_MASK;
      FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_ELSA_MASK;
      break;

    default: // Pin not used for FTM
      FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_ELSB_MASK;
      FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_ELSA_MASK;
      break;
  }
  return true;
}



bool FTM_StartTimer(const TFTMChannel* const aFTMChannel)
{
  if (aFTMChannel->channelNb < CHANNELS)
  {
    if (aFTMChannel->timerFunction == TIMER_FUNCTION_OUTPUT_COMPARE)
    {
      FTM0_CnV(aFTMChannel->channelNb) = FTM0_CNT + aFTMChannel->delayCount;

      if (FTM0_CnSC(aFTMChannel->channelNb) & FTM_CnSC_CHF_MASK) // Clear channel flag if required
        FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_CHF_MASK;

      FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_CHIE_MASK; // Enables Interrupts

      return true;
    }
  }
  return false;
}



void __attribute__ ((interrupt)) FTM0_ISR(void)
{
  OS_ISREnter(); // Start of servicing interrupt
  uint8_t channelNb;

  for (channelNb = 0; channelNb < CHANNELS; channelNb++)
  {
    if ((FTM0_CnSC(channelNb) & FTM_CnSC_CHIE_MASK) && (FTM0_CnSC(channelNb) & FTM_CnSC_CHF_MASK))
    {
      FTM0_CnSC(channelNb) &= ~FTM_CnSC_CHF_MASK;
      FTM0_CnSC(channelNb) &= ~FTM_CnSC_CHIE_MASK;

      OS_SemaphoreSignal(FTM_Semaphore); // Signal RTC thread to update clock value
    }
  }
  OS_ISRExit(); // End of servicing interrupt
}

/*! @brief Thread that looks after interrupts made by the flexible timer module.
 *
 *  @note Assumes that semaphores are created and communicate properly.
 */
static void FTM0Thread(void* data)
{
  for (;;)
  {
    OS_SemaphoreWait(FTM_Semaphore,0);
    LEDs_Off(LED_BLUE);
  }
}

/*!
** @}
*/
