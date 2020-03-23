/*!
** @file UART.c
**
** @brief I/O routines for UART communications on the TWR-K70F120M.
**
** This contains the functions for operating the UART (serial port).
**
** @author Ryan Lawton (12545341) - Ashley More (12545479)
** @date 2019-08-12
*/
/*!
**  @addtogroup UART_module UART module documentation
**  @{
*/
/* MODULE UART */

#include "UART.h"
#include "PE_Types.h"
#include "Cpu.h"
#include "OS.h"

// Prototypes
static void RxThread(void* data);
static void TxThread(void* data);

// Variable Declarations
static uint32_t TxThreadStack[THREAD_STACK_SIZE] __attribute__ ((aligned(0x08))); /*!< The stack for the transmit thread. */
static uint32_t RxThreadStack[THREAD_STACK_SIZE] __attribute__ ((aligned(0x08))); /*!< The stack for the receive thread */
static OS_ECB *TxSemaphore; /*!< Binary semaphore for signaling that data transmission */
static OS_ECB *RxSemaphore;  /*!< Binary semaphore for signaling receiving of data */

static TFIFO RxFIFO;
static TFIFO TxFIFO;

bool UART_Init(const uint32_t baudRate, const uint32_t moduleClk)
{
  RxSemaphore = OS_SemaphoreCreate(0); // Receive semaphore initialized to 0
  TxSemaphore = OS_SemaphoreCreate(0); // Transmit semaphore initialized to 0

  OS_ERROR error;             /*!< Thread content */

  error = OS_ThreadCreate(RxThread, // 2nd highest priority thread
                          NULL,
                          &RxThreadStack[THREAD_STACK_SIZE - 1],
			  UART_RX_THREAD_PRIORITY);

  error = OS_ThreadCreate(TxThread, // 3rd highest priority thread
                          NULL,
                          &TxThreadStack[THREAD_STACK_SIZE - 1],
			  UART_TX_THREAD_PRIORITY);


  if (baudRate == 0) return false; // Check baud rate value is valid

  uint8_t brfa; // Baud rate fine adjust
  uint16union_t sbr; // Used to store final baudRate value

  SIM_SCGC4 |= SIM_SCGC4_UART2_MASK; // Enables UART2 Module
  SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK; // Enables the routing of Port E

  PORTE_PCR16 |= PORT_PCR_MUX(3); // Multiplexes port 16 & 17
  PORTE_PCR17 |= PORT_PCR_MUX(3);

  UART2_C2 &= ~UART_C2_TE_MASK; // Disable UART Transmitter and Receiver
  UART2_C2 &= ~UART_C2_RE_MASK;

  FIFO_Init(&RxFIFO); // Initialize the Receive and Transmit FIFO
  FIFO_Init(&TxFIFO);

  sbr.l = moduleClk / (baudRate * 16); // Calculating Baud Rate Setting
  brfa = (uint8_t) ((moduleClk*2) / baudRate) % 32; // Calculate Baud Rate Final Value

  UART2_BDH = sbr.s.Hi; // Configure the Baud Rate Settings
  UART2_BDL = sbr.s.Lo; // Most significant bit needs to be set first

  UART2_C4 |= UART_C4_BRFA_MASK; // Prepare the brfa register
  UART2_C4 &= brfa; // Set UART2's brfa

  UART2_C2 |= UART_C2_TE_MASK; // Enable UART Transmitter and Receiver
  UART2_C2 |= UART_C2_RE_MASK;

  UART2_C2 |= UART_C2_RIE_MASK;

  NVICICPR1 = (1 << (49 % 32)); //IRQ = 49, Initialize UART2 Interrupts
  NVICISER1 = (1 << (49 % 32));

  return true;
}
 


bool UART_InChar(uint8_t* const dataPtr)
{
  return FIFO_Get(&RxFIFO, dataPtr);
}
 


bool UART_OutChar(const uint8_t data)
{

  bool transmitted;

  UART2_C2 &= ~UART_C2_TIE_MASK; //disable
  transmitted = FIFO_Put(&TxFIFO, data);
  UART2_C2 |= UART_C2_TIE_MASK; //enable

  return transmitted;
}



/*! @brief Thread that looks after Transmitting data.
 *
 *  @param data Thread parameter.
 *  @note Assumes that semaphores are created and communicate properly.
 */
static void TxThread(void* data)
{
  static uint8_t txData;
  for (;;)
  {
    OS_SemaphoreWait(TxSemaphore,0);

    if (UART2_S1 & UART_S1_TDRE_MASK)
      {
        if (FIFO_Get(&TxFIFO, &txData))
        {
          UART2_D = txData;
	  UART2_C2 |= UART_C2_TIE_MASK;
        }
	else
          UART2_C2 &= ~UART_C2_TIE_MASK;
      }
  }
}



/*! @brief Thread that looks after Receiving data.
 *
 *  @param data Thread parameter.
 *  @note Assumes that semaphores are created and communicate properly.
 */
static void RxThread(void* data)
{
  for (;;)
  {
    OS_SemaphoreWait(RxSemaphore,0);
    FIFO_Put(&RxFIFO, UART2_D);
    UART2_C2 |= UART_C2_RIE_MASK;
  }
}



void __attribute__ ((interrupt)) UART_ISR(void)
{
  OS_ISREnter(); // Start of servicing interrupt

  if (UART2_S1 & UART_C2_RIE_MASK)
    if (UART2_S1 & UART_S1_RDRF_MASK)
    {
      UART2_C2 &= ~UART_C2_RIE_MASK;
      OS_SemaphoreSignal(RxSemaphore); // Signal receive thread
    }

  if (UART2_C2 & UART_C2_TIE_MASK)
  {
      UART2_C2 &= ~UART_C2_TIE_MASK; // Transmit interrupt disabled
      OS_SemaphoreSignal(TxSemaphore); // Signal receive thread
  }

  OS_ISRExit(); // End of servicing interrupt
}

/*!
** @}
*/
