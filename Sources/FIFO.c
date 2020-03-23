/*!
** @file fifo.c
**
** @brief Routines to implement a fifo buffer.
**
** This contains the structure and "methods" for accessing a byte-wide fifo.
**
** @author Ryan Lawton (12545341) - Ashley More (12545479)
** @date 2019-08-12
*/
/*!
**  @addtogroup fifo_module fifo module documentation
**  @{
*/
/* MODULE fifo */

#include "fifo.h"
#include "PE_Types.h"
#include "Cpu.h"
#include "OS.h"


bool fifo_Init(TFIFO * const fifo)
{
  fifo->Start = 0;
  fifo->End = 0;
  fifo->NbBytes = 0;
  fifo->BufferSemaphore = OS_SemaphoreCreate(1);
  fifo->FIFOSpaceSemaphore = OS_SemaphoreCreate(FIFO_SIZE);
  fifo->FIFODataSemaphore = OS_SemaphoreCreate(0); //Create Semaphore to wait on space available
}



bool fifo_Put(TFIFO * const fifo, const uint8_t data)
{
  OS_SemaphoreWait(fifo->FIFOSpaceSemaphore, 0);
  OS_SemaphoreWait(fifo->BufferSemaphore, 0);

  bool success = false;

  fifo->Buffer[fifo->End] = data;
  fifo->NbBytes++;
  fifo->End++;
  if (fifo->End == FIFO_SIZE-1)
  {
    fifo->End = 0; // Checks if End of buffer has overflowed
    success = true;
  }

  OS_SemaphoreSignal(fifo->BufferSemaphore);
  OS_SemaphoreSignal(fifo->FIFODataSemaphore);

  return success;
}



bool fifo_Get(TFIFO * const fifo, uint8_t * const dataPtr)
{
  OS_SemaphoreWait(fifo->FIFODataSemaphore, 0);
  OS_SemaphoreWait(fifo->BufferSemaphore, 0);

  bool success = false;

  *dataPtr = fifo->Buffer[fifo->Start];

  fifo->Start++;
  fifo->NbBytes--;

  if (fifo->Start == FIFO_SIZE-1)  // Checks if Start of buffer has overflowed
  {
    fifo->Start = 0;
    success = true;
  }

  OS_SemaphoreSignal(fifo->BufferSemaphore);
  OS_SemaphoreSignal(fifo->FIFOSpaceSemaphore);

  return success;
}

/*!
** @}
*/
