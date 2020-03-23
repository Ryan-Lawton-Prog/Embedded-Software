/*!
** @file commands.h
**
** @brief defines packet command definitions
**
** @author Ryan Lawton (12545341) - Ashley More (12545479)
** @date 2019-10-14
*/

#ifndef COMMANDS_H
#define COMMANDS_H


//Define tower startup command
#define GET_TOWER_STARTUP_COMM 0x04

//Define flash program command
#define GET_TOWER_FLASH_PROGRAM_COMM 0x07
#define TOWER_FLASH_PROGRAM_PARAM2 0

//Define flash read command
#define GET_TOWER_FLASH_READ_COMM 0x08
#define TOWER_FLASH_READ_PARAM2 0
#define TOWER_FLASH_READ_PARAM3 0

//Define tower version command
#define GET_TOWER_VERSION_COMM 0x09

//Define test mode command
#define TEST_MODE_COMM 0x10

//Define tariff command
#define TARIFF_COMM 0x11

//Define time 1 command
#define TIME_SEC_MINS_COMM 0x12

//Define time 2 command
#define TIME_HRS_DAYS_COMM 0x13

//Define power command
#define POWER_COMM 0x14

//Define energy command
#define ENERGY_COMM 0x15

//Define cost command
#define COST_COMM 0x16

//Define frequency command
#define SET_FREQUENCY_COMM 0x17

//Define frequency command
#define VOLTAGE_RMS_COMM 0x18

//Define frequency command
#define CURRENT_RMS_COMM 0x19

//Define frequency command
#define POWER_FACTOR_COMM 0x1A

//Define protocol mode command
#define GET_TOWER_PROTOCOL_COMM 0x0A
#define TOWER_PROTOCOL_GET 1
#define TOWER_PROTOCOL_SET 2
#define TOWER_PROTOCOL_SET_Synchronous 0
#define TOWER_PROTOCOL_SET_SYNCHRONOUS 1
#define TOWER_PROTOCOL_GET_PARAM2 0
#define TOWER_PROTOCOL_PARAM3 0

//Define tower number command
#define GET_TOWER_NUMBER_COMM 0x0B
//Packet Parameter 1 for getting the tower number
#define TOWER_NUMBER_GET 1
#define TOWER_NUMBER_GET_PARAM2 0
#define TOWER_NUMBER_GET_PARAM3 0
//Packet Parameter 1 for setting the tower number
#define TOWER_NUMBER_SET 2

//Define tower number command
#define GET_TOWER_TIME_COMM 0x0C

//Define tower mode command
#define GET_TOWER_MODE_COMM 0x0D
//Packet Parameter 1 for getting the tower number
#define TOWER_MODE_GET 1
#define TOWER_MODE_GET_PARAM2 0
#define TOWER_MODE_GET_PARAM3 0
//Packet Parameter 1 for setting the tower number
#define TOWER_MODE_SET 2

//Least significant byte of Student ID 12545341
#define SID 0x14DD

//Define "0x04 Tower startup" packet
#define TOWER_STARTUP_COMM 0x04
#define TOWER_STARTUP_PAR1 0x0
#define TOWER_STARTUP_PAR2 0x0
#define TOWER_STARTUP_PAR3 0x0

//Define "0x08 Flash Read" packet
#define TOWER_FLASH_READ_COMM 0x08
#define TOWER_FLASH_READ_PAR2 0
#define TOWER_FLASH_READ_PAR3 0

//Define "0x09 Special - Tower version" packet
#define TOWER_VERSION_COMM 0x09
#define TOWER_VERSION_V 'v'   //version 5.0
#define TOWER_VERSION_MAJ 5
#define TOWER_VERSION_MIN 0

//Define "0x0A Protocol Mode" packet
#define TOWER_PROTOCOL_COMM 0x0A
#define TOWER_PROTOCOL_PAR1 1
#define TOWER_PROTOCOL_PAR2 0
#define TOWER_PROTOCOL_Synchronous 0
#define TOWER_PROTOCOL_SYNCHRONOUS 1
#define TOWER_PROTOCOL_PAR3 0

//Define "0x0B Tower number" packet
#define TOWER_NUMBER_COMM 0x0B
#define TOWER_NUMBER_PAR1 1

//Define "0x0C Time" packet
#define TOWER_TIME_COMM 0x0C

//Define "0x0D Tower mode" packet
#define TOWER_MODE_COMM 0x0D
#define TOWER_MODE_PAR1 1

//Define "0x50 Analog Input" packet
#define TOWER_ANALOG_INPUT_COMM 0x50
#define TOWER_ANALOG_INPUT_CHANNEL 0

#define PACKET_CHANNEL_NB 0

#endif
