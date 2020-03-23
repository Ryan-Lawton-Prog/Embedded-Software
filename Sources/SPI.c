/*! @file SPI.c
 *
 *  @brief I/O routines for K70 SPI interface.
 *
 *  This contains the functions for operating the SPI (serial peripheral interface) module.
 *
 *  @author Ryan Lawton (12545341) - Ashley More (12545479)
 *  @date 2019-09-30
 */
/*!
**  @addtogroup SPI_module SPI module documentation
**  @{
*/
/* MODULE SPI */

// new types
#include "SPI.h"
#include "types.h"
#include "analog.h"
#include "MK70F12.h"
#include "Cpu.h"

#define SPI_FRAME_SIZE 16
#define NUM_DIVIDERS 4
#define NUM_SCALARS 16

typedef struct {
  uint8_t PDT;
  uint8_t DT;
} delayVal_t;

static bool Master;

static float Abs(float num);
static void setBaudRate(uint32_t baudRate);
static void setDelay(uint32_t value, uint32_t moduleClk);
static uint32_t calcDelay(delayVal_t values, uint32_t moduleClk);

static uint8_t baudRateDividers[NUM_DIVIDERS] = {2,3,5,7};
static uint32_t baudRateScalars[NUM_SCALARS] = {2,4,6,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768};
static const uint32_t DELAY_TIME = 5000;



bool SPI_Init(const TSPIModule* const aSPIModule, const uint32_t moduleClock)
{

  SIM_SCGC5 |= SIM_SCGC5_PORTD_MASK; // Enable clock gates
  SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK;
  SIM_SCGC3 |= SIM_SCGC3_DSPI2_MASK;

  PORTD_PCR11 |= PORT_PCR_MUX(2);  // Enable port signals
  PORTD_PCR12 |= PORT_PCR_MUX(2);
  PORTD_PCR13 |= PORT_PCR_MUX(2);
  PORTD_PCR14 |= PORT_PCR_MUX(2);
  PORTD_PCR15 |= PORT_PCR_MUX(2);

  PORTE_PCR5 |= PORT_PCR_MUX(1);  // Set pins as outputs
  PORTE_PCR27 |= PORT_PCR_MUX(1);

  GPIOE_PDDR |= (1<<5);
  GPIOE_PDDR |= (1<<27);

  SPI2_MCR |= SPI_MCR_HALT_MASK; // Disable SPI Comms

  // Setting MCR Fixed Fields
  SPI2_MCR &= ~SPI_MCR_DCONF_MASK;
  SPI2_MCR |= SPI_MCR_FRZ_MASK;
  SPI2_MCR &= ~SPI_MCR_MTFE_MASK;
  SPI2_MCR &= ~SPI_MCR_PCSSE_MASK;
  SPI2_MCR &= ~SPI_MCR_ROOE_MASK;
  SPI2_MCR |= SPI_MCR_PCSIS(1);  // ** Set to 1
  SPI2_MCR &= ~SPI_MCR_DOZE_MASK;
  SPI2_MCR &= ~SPI_MCR_MDIS_MASK;
  SPI2_MCR |= SPI_MCR_DIS_TXF_MASK;
  SPI2_MCR |= SPI_MCR_DIS_RXF_MASK;

  if (aSPIModule->continuousClock)
    SPI2_MCR |= SPI_MCR_CONT_SCKE_MASK; // Set Synchronous Clock
  else
    SPI2_MCR &= ~SPI_MCR_CONT_SCKE_MASK; // Set Asynchronous Clock



  // Setting up for Operation Conditions
  if (aSPIModule->isMaster)
  {
    Master = true;

    SPI2_MCR |= SPI_MCR_MSTR_MASK; // Set SPR Master
    SPI2_CTAR0 |= SPI_CTAR_FMSZ(0x0F);

    if (aSPIModule->inactiveHighClock)
      SPI2_MCR |= SPI_MCR_PCSIS_MASK; // Set Inactive High
    else
      SPI2_MCR &= ~SPI_MCR_PCSIS_MASK; // Set Inactive Low

    if (aSPIModule->changedOnLeadingClockEdge)
      SPI2_CTAR0 |= SPI_CTAR_CPHA_MASK; // Set to Change on Leading Edge
    else
      SPI2_CTAR0 &= ~SPI_CTAR_CPHA_MASK; // Set to Capture on Leading Edge

    if (aSPIModule->LSBFirst)
      SPI2_CTAR0 |= SPI_CTAR_LSBFE_MASK; // Set LSB First
    else
      SPI2_CTAR0 &= ~SPI_CTAR_LSBFE_MASK; // Set MSB First

    setBaudRate(aSPIModule->baudRate);
    setDelay(DELAY_TIME, moduleClock);
  }
  else
  {
    Master = false;

    SPI2_MCR &= ~SPI_MCR_MSTR_MASK; // Set SPR Slave
    SPI0_CTAR0_SLAVE |= SPI_CTAR_SLAVE_FMSZ(SPI_FRAME_SIZE-1); // Set Frame Size is equal to number set + 1

    if (aSPIModule->inactiveHighClock)
      SPI2_MCR |= SPI_MCR_PCSIS_MASK; // Set Inactive High
    else
      SPI2_MCR &= ~SPI_MCR_PCSIS_MASK; // Set Inactive Low

    if (aSPIModule->changedOnLeadingClockEdge)
      SPI2_CTAR0_SLAVE |= SPI_CTAR_SLAVE_CPHA_MASK; // Set to Change on Leading Edge
    else
      SPI2_CTAR0_SLAVE &= ~SPI_CTAR_SLAVE_CPHA_MASK; // Set to Capture on Leading Edge
  }


  SPI2_SR &= ~SPI_SR_EOQF_MASK;

  SPI2_SR |= SPI_SR_TFFF_MASK;

  SPI2_MCR &= ~SPI_MCR_HALT_MASK; // Enable SPI Comms

  return true;
}


 
void SPI_SelectSlaveDevice(const uint8_t slaveAddress)
{
  switch (slaveAddress) // Switches port allocation based on slave address selected
  {
    case 4: // 100 = 4, first bit it alway enabled
      GPIOE_PDOR |= (0 << 5);
      GPIOE_PDOR |= (0 << 27);
      break;
    case 5: // 101 = 5
      GPIOE_PDOR |= (1 << 5);
      GPIOE_PDOR |= (0 << 27);
      break;
    case 6: // 110 = 6
      GPIOE_PDOR |= (0 << 5);
      GPIOE_PDOR |= (1 << 27);
      break;
    case 7: // 111 = 7
      GPIOE_PDOR |= (1 << 5);
      GPIOE_PDOR |= (1 << 27);
      break;
    default:
      return;
  }
}



void SPI_Exchange(const uint16_t dataTx, uint16_t* const dataRx)
{
  while (!(SPI2_SR & SPI_SR_TFFF_MASK)); // Transmit FIFO Fill Flag

  SPI2_SR |= SPI_SR_TFFF_MASK; // Clear TFFF flag

  if (Master)
  {
    SPI2_PUSHR &= SPI_PUSHR_TXDATA(0);

    SPI2_PUSHR |= SPI_PUSHR_PCS(1);
    SPI2_PUSHR_SLAVE &= SPI_PUSHR_TXDATA(0);
    SPI2_PUSHR |= SPI_PUSHR_TXDATA(dataTx);
  }
  else
  {
    SPI2_PUSHR_SLAVE &= SPI_PUSHR_TXDATA(0);
    SPI2_PUSHR_SLAVE |= SPI_PUSHR_SLAVE_TXDATA(dataTx);
  }

  while (!(SPI2_SR & SPI_SR_RFDF_MASK)); // Receive FIFO Drain Flag

  *dataRx = SPI2_POPR; // RxFIFO 32-bit read accesses

  SPI2_SR |= SPI_SR_RFDF_MASK; // Clear RFDF Flag
}


/*! @brief Finds the absolute number of a value.
 *
 *  @param num is the value to be worked on.
 */
static float Abs(float num)
{
  if (num < 0)
    return num*-1;
  else
    return num;
}


/*! @brief Sets the baud rate of the module using dividors and scalars from manual.
 *
 *  @param baudRate is the defined baud rate.
 */
static void setBaudRate(uint32_t baudRate)
{
  float estimatedBaudRate = baudRate / CPU_BUS_CLK_HZ;

  uint8_t preBaud = 1 / ((float) baudRateDividers[0] * baudRateScalars[0]);
  uint8_t currentDivider = 0;
  uint8_t currentScalar = 0;

  for (uint8_t i = 0; i < NUM_DIVIDERS; i++)
  {
    for (uint8_t j = 1; j < NUM_SCALARS; j++)
    {
      uint8_t newBaud = 1 / ((float) baudRateDividers[i] * baudRateScalars[j]);

      if (Abs(estimatedBaudRate - newBaud) < Abs(estimatedBaudRate - preBaud))
      {
        currentDivider = baudRateDividers[i];
        currentScalar = baudRateScalars[j];
        preBaud = newBaud;
      }
    }
  }


  SPI2_CTAR0 &= ~SPI_CTAR_DBR_MASK;
  SPI2_CTAR0 |= SPI_CTAR_PBR(currentDivider);
  SPI2_CTAR0 |= SPI_CTAR_BR(currentScalar);
}



/*! @brief Calculates the delay to be used.
 *
 *  @param values is data to transmit.
 *  @param module_clk The module clock in Hz.
 */
static uint32_t calcDelay(delayVal_t values, uint32_t module_clk)
{
  uint8_t bestPDelay;
  switch(values.PDT)
  {
    case 0: bestPDelay = 2; break;
    case 1: bestPDelay = 3; break;
    case 2: bestPDelay = 5; break;
    case 3: bestPDelay = 7; break;
  }

  uint16_t bestDelay = values.DT * values.DT;

  return (1000000000/module_clk) * bestPDelay * bestDelay;
}



/*! @brief Sets the delay.
 *
 *  @param value.
 *  @param module_clk The module clock in Hz.
 */
static void setDelay(uint32_t value, uint32_t module_clk)
{
  delayVal_t vals = {0, 0};
  uint32_t bestVal;
  bestVal = calcDelay(vals, module_clk);

  for (uint8_t bestPDelay = 0; bestPDelay < 4; ++bestPDelay)
  {
    for (uint8_t bestDelay = 0; bestDelay < 16; ++bestDelay)
    {
      delayVal_t newValues = {bestPDelay, bestDelay};
      uint32_t delay = calcDelay(newValues, module_clk);
      if (Abs(value - delay) < Abs(value - calcDelay(vals, module_clk)) && delay >= value)
        vals = newValues;
    }
  }

  SPI2_CTAR0 |= SPI_CTAR_PDT(vals.PDT);
  SPI2_CTAR0 |= SPI_CTAR_DT(vals.DT);
}

/*!
** @}
*/
