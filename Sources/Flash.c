/*!
** @file flash.c
**
**  @brief Routines for erasing and writing to the Flash.
**
**  This contains the functions needed for accessing the internal Flash.
**
** @author Ryan Lawton (12545341) - Ashley More (12545479)
** @date 2019-08-26
*/
/*!
**  @addtogroup Flash_module Flash module documentation
**  @{
*/
/* MODULE Flash */

#include "Flash.h"
#include "MK70F12.h"
#include "string.h"
#include "OS.h"

#define FCMD_ERASE_SEC 0x09LU
#define FCMD_PGM_PHRASE 0x07LU
#define MAX_ADDRESS (FLASH_DATA_END - FLASH_DATA_START)

bool WritePhrase(const uint64_t phrase);

typedef union
{
  uint64_t phrase;

  struct
  {
    union
    {
      struct
      {
        uint8_t b3;
        uint8_t b2;
        uint8_t b1;
        uint8_t b0;
        uint8_t b7;
        uint8_t b6;
        uint8_t b5;
        uint8_t b4;
      } bigEndian;
      struct
      {
	uint8_t b[MAX_ADDRESS];
      } bytes;
      struct
      {
      	uint16_t hf[MAX_ADDRESS/2];
      } halfWords;
      struct
      {
	uint32_t w[MAX_ADDRESS/4];
      } words;
    } params;
  } phraseStruct;

} PHRASE_t;

typedef union
{
  uint32_t a;

  struct
  {
    uint8_t a3;
    uint8_t a2;
    uint8_t a1;
    uint8_t null;
  } ADR;

} FCCOB_ADR_t;



bool Flash_Init(void)
{
  SIM_SCGC3 |= SIM_SCGC3_NFC_MASK; // Initialize the Flash Clock
  while (!(FTFE_FSTAT & FTFE_FSTAT_CCIF_MASK));
  return true;
}
 


bool Flash_AllocateVar(volatile void** variable, const uint8_t size)
{
  if (size == 1 || size == 2 || size == 4){
    static uint8_t flashMem = 0xFF;

    uint32_t address;
    uint8_t mask = (((1 << size) - 1) << (8 - size));

    for (address = FLASH_DATA_START; address <= (FLASH_DATA_END); address+=size)
    {
      if (mask == (mask & flashMem))
      {
	*variable = (void *) address;
	flashMem ^= mask;
	return true;
      }
      mask = mask >> size;
    }
  }

  return false;
}



bool Flash_Write32(volatile uint32_t* const address, const uint32_t data)
{
  size_t index = ((size_t) address - FLASH_DATA_START);

  if (index < 0 || index > MAX_ADDRESS) return false;
  if (index % 4 != 0) return false;

  index /= 4;

  PHRASE_t arrWords;
  arrWords.phrase = _FP(FLASH_DATA_START);
  arrWords.phraseStruct.params.words.w[index] = data;

  WritePhrase(arrWords.phrase);
  return true;
}
 


bool Flash_Write16(volatile uint16_t* const address, const uint16_t data)
{
  size_t index = ((size_t) address - FLASH_DATA_START);

  if (index < 0 || index > MAX_ADDRESS) return false;
  if (index % 2 != 0) return false;

  index /= 2;

  PHRASE_t arrHalfWords;
  arrHalfWords.phrase = _FP(FLASH_DATA_START);
  arrHalfWords.phraseStruct.params.halfWords.hf[index] = data;

  WritePhrase(arrHalfWords.phrase);
  return true;
}



bool Flash_Write8(volatile uint8_t* const address, const uint8_t data)
{
  size_t index = ((size_t) address - FLASH_DATA_START);

  if (index < 0 || index > MAX_ADDRESS) return false;

  PHRASE_t arrBytes;
  arrBytes.phrase = _FP(FLASH_DATA_START);
  arrBytes.phraseStruct.params.bytes.b[index] = data;

  WritePhrase(arrBytes.phrase);
  return true;
}



bool Flash_Erase(void)
{
  FCCOB_ADR_t fccob;

  while (!(FTFE_FSTAT & FTFE_FSTAT_CCIF_MASK));

  fccob.a = FLASH_DATA_START;
  FTFE_FCCOB0 = FCMD_ERASE_SEC;
  FTFE_FCCOB1 = fccob.ADR.a1;
  FTFE_FCCOB2 = fccob.ADR.a2;
  FTFE_FCCOB3 = fccob.ADR.a3;

  FTFE_FSTAT = FTFE_FSTAT_CCIF_MASK;
  while (!(FTFE_FSTAT & FTFE_FSTAT_CCIF_MASK));

  return true;
}



/*! @brief Writes a phrase to memory after erasing flash
 *
 *  @param phrase The phrase to write to memory
 *  @return bool - TRUE if the phrase was written to flash successfully.
 */
bool WritePhrase(const uint64_t phrase)
{
  FCCOB_ADR_t fccob;

  while (!(FTFE_FSTAT & FTFE_FSTAT_CCIF_MASK));

  if (!Flash_Erase())
  {
    return false;
  }

  fccob.a = FLASH_DATA_START;
  FTFE_FCCOB0 = FCMD_PGM_PHRASE; // FCCOB (Flash command control object byte)
  FTFE_FCCOB1 = fccob.ADR.a1;
  FTFE_FCCOB2 = fccob.ADR.a2;
  FTFE_FCCOB3 = fccob.ADR.a3;

  PHRASE_t arrBytes;
  arrBytes.phrase = phrase;
  FTFE_FCCOB4 = arrBytes.phraseStruct.params.bigEndian.b0;
  FTFE_FCCOB5 = arrBytes.phraseStruct.params.bigEndian.b1;
  FTFE_FCCOB6 = arrBytes.phraseStruct.params.bigEndian.b2;
  FTFE_FCCOB7 = arrBytes.phraseStruct.params.bigEndian.b3;
  FTFE_FCCOB8 = arrBytes.phraseStruct.params.bigEndian.b4;
  FTFE_FCCOB9 = arrBytes.phraseStruct.params.bigEndian.b5;
  FTFE_FCCOBA = arrBytes.phraseStruct.params.bigEndian.b6;
  FTFE_FCCOBB = arrBytes.phraseStruct.params.bigEndian.b7;

  FTFE_FSTAT = FTFE_FSTAT_CCIF_MASK;
  while (!(FTFE_FSTAT & FTFE_FSTAT_CCIF_MASK));

  return true;
}

/*!
** @}
*/
