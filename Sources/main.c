/* ###################################################################
**     Filename    : main.c
**     Project     : Lab4
**     Processor   : MK70FN1M0VMJ12
**     Version     : Driver 01.01
**     Compiler    : GNU C Compiler
**     Date/Time   : 2015-07-20, 13:27, # CodeGen: 0
**     Abstract    :
**         Main module.
**         This module contains user's application code.
**     Settings    :
**     Contents    :
**         No public methods
**
** ###################################################################*/
/*!
** @file main.c
** @version 4.0
** @brief
**         Main module.
**         This module contains user's application code.
**
** @author Ryan Lawton (12545341) - Ashley More (12545479)
** @date 2019-09-16
*/         
/*!
**  @addtogroup main_module main module documentation
**  @{
*/         
/* MODULE main */


// CPU module - contains low level hardware initialization routines
#include "Cpu.h"
#include "OS.h"
#include "Events.h"
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"
#include "packet.h"
#include "Flash.h"
#include "UART.h"
#include "LEDs.h"
#include "FTM.h"
#include "PIT.h"
#include "RTC.h"
#include "analog.h"

bool TowerInit();           // Initialize Tower
bool TowerInitFlash();      // Initialize Flash

bool TowerStartup();        // 0x04
bool TowerFlashProgram();   // 0x07
bool TowerFlashRead();      // 0x08
bool TowerVersion();        // 0x09
bool TowerProtocol();       // 0x0A
bool TowerNumber();         // 0x0B
bool TowerTime();           // 0x0C
bool TowerMode();           // 0x0D
void HandleCommand();

//Prototypes
static void InitThread(void* data);
static void PacketCheckerThread(void* data);

// Variable definitions
static uint32_t InitThreadStack[THREAD_STACK_SIZE] __attribute__ ((aligned(0x08)));            /*!< The stack for the initialization thread. */
static uint32_t PacketCheckerThreadStack[THREAD_STACK_SIZE] __attribute__ ((aligned(0x08)));   /*!< The stack for the packet checking thread. */

uint16union_t volatile *NvTowerNb; /*!< Tower Number pointer in memory */
uint16union_t volatile *NvTowerMd; /*!< Tower mode pointer in memory */

TFTMChannel PacketTimer = {
  PACKET_CHANNEL_NB,
  CPU_MCGFF_CLK_HZ_CONFIG_0,
  TIMER_FUNCTION_OUTPUT_COMPARE,
  TIMER_OUTPUT_HIGH,
  (void*) 0
};


/*! @brief Initialises the hardware, sets up to threads, and starts the OS.
 *
 */
/*lint -save  -e970 Disable MISRA rule (6.3) checking. */
int main(void)
/*lint -restore Enable MISRA rule (6.3) checking. */
{
  OS_ERROR error; /*!< Thread content */

  // Initialise low-level clocks etc using Processor Expert code
  PE_low_level_init();

  // Initialize the RTOS
  OS_Init(CPU_CORE_CLK_HZ, false);

  // Create threads using OS_ThreadCreate();
  error = OS_ThreadCreate(InitThread, // Highest priority
                          NULL,
                          &InitThreadStack[THREAD_STACK_SIZE - 1],
			  INIT_THREAD_PRIORITY);

  error = OS_ThreadCreate(PacketCheckerThread, // Lowest priority
                          NULL,
                          &PacketCheckerThreadStack[THREAD_STACK_SIZE - 1],
                          6);

  // Start multithreading - never returns!
  OS_Start();

  /*** Don't write any code pass this line, or it will be deleted during code generation. ***/
  /*** RTOS startup code. Macro PEX_RTOS_START is defined by the RTOS component. DON'T MODIFY THIS CODE!!! ***/
  #ifdef PEX_RTOS_START
    PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of RTOS startup code.  ***/
  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  for(;;){}
  /*** Processor Expert end of main routine. DON'T WRITE CODE BELOW!!! ***/
} /*** End of main routine. DO NOT MODIFY THIS TEXT!!! ***/



/*! @brief Initializes tower
 *
 *  @param data is not used but is required by the OS to create a thread.
 *  @note
 */
static void InitThread(void* data)
{
  for (;;)
  {
    OS_DisableInterrupts(); // Disable interrupts
    bool init = TowerInit();
    OS_EnableInterrupts(); // Enable interrupts

    if (init)
      TowerStartup(); // Send startup packets to PC

    OS_ThreadDelete(OS_PRIORITY_SELF); // Thread not accessed again
  }
}



/*!
 * @brief Forever checks for packets and handles them
 */
static void PacketCheckerThread(void* data)
{
  for (;;)
  {
    if (Packet_Get())
    {
      LEDs_On(LED_BLUE);
      FTM_StartTimer(&PacketTimer);
      HandleCommand();
    }
  }
}



/* END main */
/*!
** @}
*/



/*! @brief Initialize Tower Modules
 *
 *  @return bool - TRUE if all modules successfully initialized
 */
bool TowerInit()
{
  bool initSuccess = true;
  const uint32_t baudRate = 115200;
  const uint32_t moduleClock = CPU_BUS_CLK_HZ;

  if (Packet_Init(baudRate, moduleClock))
    if (Flash_Init())
      if (TowerInitFlash())
        if (LEDs_Init())
          if (PIT_Init(moduleClock))
            if (FTM_Init())
              if (RTC_Init())
        	if (Analog_Init(moduleClock))
                  initSuccess = true;

  FTM_Set(&PacketTimer);
  PIT_Set(1e7, false);

  Synchronous = false;

  if (initSuccess)
    LEDs_On(LED_ORANGE);
  return initSuccess;
}



/*! @brief Initialize flash memory locations and write to them if blank
 *
 *  @return bool - TRUE if flash is set and written to successfully.
 *  @note Assumes that TowerInit() has been called.
 */
bool TowerInitFlash()
{
  if (Flash_AllocateVar((volatile void **) &NvTowerNb, sizeof(uint16union_t)));
  if (Flash_AllocateVar((volatile void **) &NvTowerMd, sizeof(uint16union_t)));

  if ((*NvTowerNb).s.Lo == 0xFF && (*NvTowerNb).s.Hi == 0xFF)
    Flash_Write16((uint16_t *) NvTowerNb, SID);
  if ((*NvTowerMd).s.Lo == 0xFF && (*NvTowerMd).s.Hi == 0xFF)
    Flash_Write16((uint16_t *) NvTowerMd, 0x1);

  return true;
}



/*! @brief Send Tower Startup Packets to PC
 *
 *  @return bool - TRUE if packets are sent successfully.
 *  @note Assumes that PACKET_Init has been called.
 */
bool TowerStartup()
{
  if (Packet_Put(TOWER_STARTUP_COMM, TOWER_STARTUP_PAR1, TOWER_STARTUP_PAR2, TOWER_STARTUP_PAR3))
    if (Packet_Put(TOWER_VERSION_COMM, TOWER_VERSION_V, TOWER_VERSION_MAJ, TOWER_VERSION_MIN))
      if (Packet_Put(TOWER_PROTOCOL_COMM, TOWER_PROTOCOL_PAR1, Synchronous, TOWER_PROTOCOL_PAR1))
	if (Packet_Put(TOWER_NUMBER_COMM, TOWER_NUMBER_PAR1, (*NvTowerNb).s.Lo, (*NvTowerNb).s.Hi))
	  if (Packet_Put(TOWER_MODE_COMM, TOWER_MODE_PAR1, (*NvTowerMd).s.Lo, (*NvTowerMd).s.Hi))
  	    return true;
  return false;
}



/*! @brief Writes a byte to flash memory
 *
 *  @return bool - TRUE if flash successfully written to.
 *  @note Assumes that TowerInit is successfully called.
 */
bool TowerFlashProgram()
{
  if (Packet_Parameter1 >= 0 && Packet_Parameter1 <= 7 && Packet_Parameter2 == 0)
  {
    uint8_t * pByte = (uint8_t *)(FLASH_DATA_START + Packet_Parameter1);
    return Flash_Write8((uint8_t volatile *) pByte, Packet_Parameter3);
  }
  if (Packet_Parameter1 == 8)
    return Flash_Erase();
  return false;
}



/*! @brief Reads in a specified byte from flash memory
 *
 *  @return bool - TRUE if packet is sent successfully.
 *  @note Assumes that TowerInit is successfully called.
 */
bool TowerFlashRead()
{
  if (Packet_Parameter1 >= 0 && Packet_Parameter1 <= 7 && Packet_Parameter23 == 0)
  {
    uint8_t * const pByte;
    *pByte = _FB(FLASH_DATA_START + Packet_Parameter1);
    return Packet_Put(TOWER_FLASH_READ_COMM, Packet_Parameter1, TOWER_FLASH_READ_PAR2, *pByte);
  }
  return false;
}



/*! @brief Send Tower Version Packet to PC
 *
 *  @return bool - TRUE if packet are sent successfully.
 *  @note Assumes that PACKET_Init has been called.
 */
bool TowerVersion()
{
  if (Packet_Put(TOWER_VERSION_COMM, TOWER_VERSION_V, TOWER_VERSION_MAJ, TOWER_VERSION_MIN))
    return true;
  return false;
}



bool TowerProtocol()
{
  if (TOWER_PROTOCOL_GET == Packet_Parameter1 && TOWER_PROTOCOL_GET_PARAM2 == Packet_Parameter2 && TOWER_PROTOCOL_PARAM3 == Packet_Parameter3)
  {
    if (Packet_Put(TOWER_PROTOCOL_COMM, TOWER_PROTOCOL_PAR1, Synchronous, TOWER_PROTOCOL_PAR3))
      return true;
  }
  else if (TOWER_PROTOCOL_SET == Packet_Parameter1 && TOWER_PROTOCOL_PARAM3 == Packet_Parameter3)
  {
    if (TOWER_PROTOCOL_SET_Synchronous == Packet_Parameter2)
    {
      Synchronous = false;
      return true;
    }
    else if (TOWER_PROTOCOL_SET_SYNCHRONOUS == Packet_Parameter2)
    {
      Synchronous = true;
      return true;
    }
  }
  return false;
}



/*! @brief Send Tower Number to PC or if Packet Param1 is 2 set Tower Number
 *
 *  @return bool - TRUE if packets are sent successfully.
 *  @note Assumes that PACKET_Init has been called.
 */
bool TowerNumber()
{
  if (Packet_Parameter1 == TOWER_NUMBER_GET && Packet_Parameter2 == TOWER_NUMBER_GET_PARAM2 && Packet_Parameter3 == TOWER_NUMBER_GET_PARAM3)
  {
    if (Packet_Put(TOWER_NUMBER_COMM, TOWER_NUMBER_PAR1, (*NvTowerNb).s.Lo, (*NvTowerNb).s.Hi))
      return true;
  }
  else if (Packet_Parameter1 == TOWER_NUMBER_SET)
  {
    return Flash_Write16((uint16_t *) NvTowerNb, Packet_Parameter23);
  }
  return false;
}



/*! @brief Send Tower Time to PC
 *
 *  @return bool - TRUE if packets are sent successfully.
 *  @note Assumes that RTC_Init has been called.
 */
bool TowerTime()
{
  uint8_t hours, minutes, seconds;
  if (Packet_Parameter1 >= 0 && Packet_Parameter1 < 24 && Packet_Parameter2 >= 0 && Packet_Parameter2 < 60 && Packet_Parameter3 >= 0 && Packet_Parameter3 < 60)
  {
    RTC_Set(Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
    RTC_Get(&hours, &minutes, &seconds);
    if (Packet_Put(TOWER_TIME_COMM, hours, minutes, seconds))
      return true;
  }
  return false;
}



/*! @brief Send Tower Mode to PC or if Packet Param1 is 2 set Tower Mode
 *
 *  @return bool - TRUE if packets are sent successfully.
 *  @note Assumes that PACKET_Init has been called.
 */
bool TowerMode()
{
  if (Packet_Parameter1 == TOWER_MODE_GET && Packet_Parameter2 == TOWER_MODE_GET_PARAM2 && Packet_Parameter3 == TOWER_MODE_GET_PARAM3)
  {
    if (Packet_Put(TOWER_MODE_COMM, TOWER_MODE_PAR1, (*NvTowerMd).s.Lo, (*NvTowerMd).s.Hi))
      return true;
  }
  else if (Packet_Parameter1 == TOWER_NUMBER_SET)
  {
    return Flash_Write16((uint16_t *) NvTowerMd, Packet_Parameter23);
  }
  return false;
}



/*! @brief Handle incoming valid packets
 *
 *  @return bool - TRUE if packets are sent successfully.
 *  @note Assumes that PACKET_Init has been called and that we have a successful packet.
 */
void HandleCommand()
{
  bool packetSuccess = false;
  switch (Packet_Command & ~PACKET_ACK_MASK) // Removes acknowledgment mask from command
  {
    case GET_TOWER_STARTUP_COMM:
      packetSuccess = TowerStartup();
      break;

    case GET_TOWER_FLASH_PROGRAM_COMM:
      packetSuccess = TowerFlashProgram();
      break;

    case GET_TOWER_FLASH_READ_COMM:
      packetSuccess = TowerFlashRead();
      break;

    case GET_TOWER_VERSION_COMM:
      packetSuccess = TowerVersion();
      break;

    case GET_TOWER_PROTOCOL_COMM:
      packetSuccess = TowerProtocol();
      break;

    case GET_TOWER_NUMBER_COMM:
      packetSuccess = TowerNumber();
      break;

    case GET_TOWER_TIME_COMM:
      packetSuccess = TowerTime();
      break;

    case GET_TOWER_MODE_COMM:
      packetSuccess = TowerMode();
      break;

    default:
      break;
  }

  if (Packet_Command & PACKET_ACK_MASK) // Checks if acknowledgment mask is set
  {
    uint8_t maskedPacket = 0;
    if (packetSuccess)
      maskedPacket = Packet_Command | PACKET_ACK_MASK; 	// If packet was transmitted successfully
							// Acknowledgment bit should be a 1
    else
      maskedPacket = Packet_Command & ~PACKET_ACK_MASK; // If packet was transmitted unsuccessfully
                                                        // Acknowledgment bit should be a 0

    Packet_Put(maskedPacket, Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
  }
}

/*
** ###################################################################
**
**     This file was created by Processor Expert 10.5 [05.21]
**     for the Freescale Kinetis series of microcontrollers.
**
** ###################################################################
*/
