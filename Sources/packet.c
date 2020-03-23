/*!
** @file packet.c
**
** @brief Routines to implement packet encoding and decoding for the serial port.
**
** This contains the functions for implementing the "Tower to PC Protocol" 5-byte packets.
**
** @author Ryan Lawton (12545341) - Ashley More (12545479)
** @date 2019-08-12
*/
/*!
**  @addtogroup packet_module packet module documentation
**  @{
*/
/* MODULE packet */

#include "packet.h"
#include "PE_Types.h"
#include "Cpu.h"
#include "OS.h"

TPacket Packet;

const uint8_t PACKET_ACK_MASK = 0x80u;
uint8_t PacketPosition = 0;

OS_ECB *PacketSemaphore;



bool Packet_Init(const uint32_t baudRate, const uint32_t moduleClk)
{
  PacketSemaphore = OS_SemaphoreCreate(1);
  return UART_Init(baudRate, moduleClk);
}



bool Packet_Get(void)
{
  //EnterCritical();
  uint8_t packetData;

  if (UART_InChar(&packetData))
  {
    switch (PacketPosition)
    {
      case 0: // Set first byte of packet as the command
	Packet_Command = packetData;
	PacketPosition++; // Record of incoming packet byte
	//ExitCritical();
	return false;
	break;

      case 1: // Set second byte of packet as Param1
	Packet_Parameter1 = packetData;
	PacketPosition++;
	//ExitCritical();
	return false;
	break;

      case 2: // Set third byte of packet as Param2
	Packet_Parameter2 = packetData;
	PacketPosition++;
	//ExitCritical();
	return false;
	break;

      case 3: // Set fourth byte of packet as Param3
	Packet_Parameter3 = packetData;
	PacketPosition++;
	//ExitCritical();
	return false;
	break;

      case 4: // Set fifth byte of packet as the checksum
	Packet_Checksum = packetData;
	// Check if checksum is valid for provided packet
	if (Packet_Checksum == (uint8_t) (Packet_Command ^ Packet_Parameter1 ^ Packet_Parameter2 ^ Packet_Parameter3))
	{
	  PacketPosition = 0;
	  //ExitCritical();
	  return true;
	}
	Packet_Command = Packet_Parameter1;
	Packet_Parameter1 = Packet_Parameter2;
	Packet_Parameter2 = Packet_Parameter3;
	Packet_Parameter3 = Packet_Checksum;
	//ExitCritical();
	return false;
	break;

      default:
	PacketPosition = 0;
	break;
    }
  }

  //ExitCritical();
  return false;
}



bool Packet_Put(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3)
{
  //EnterCritical();
  OS_SemaphoreWait(PacketSemaphore, 0);

  // Attempt to enter provided parameters into TxFIFO
  if (UART_OutChar(command))
    if (UART_OutChar(parameter1))
      if (UART_OutChar(parameter2))
        if (UART_OutChar(parameter3))
          if (UART_OutChar(command ^ parameter1 ^ parameter2 ^ parameter3))
          {
            //ExitCritical();
            //return true;
          }
  //ExitCritical();
  OS_SemaphoreSignal(PacketSemaphore);
  return true;
}

/*!
** @}
*/
