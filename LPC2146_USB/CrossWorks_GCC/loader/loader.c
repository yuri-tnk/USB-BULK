/*****************************************************************************
 * Loader for Philips LPC210X                                                *
 *                                                                           *
 * Copyright (c) 2001, 2002, 2003 Rowley Associates Limited.                 *
 *                                                                           *
 * This file may be distributed under the terms of the License Agreement     *
 * provided with this software.                                              *
 *                                                                           *
 * THIS FILE IS PROVIDED AS IS WITH NO WARRANTY OF ANY KIND, INCLUDING THE   *
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. *
 *****************************************************************************/

#include "../loader/loader.h"

#define IAP_LOCATION 0x7FFFFFF1

#define IAP_CMD_PREPARE_SECTORS_FOR_WRITE_OPERATION 50
#define IAP_CMD_COPY_RAM_TO_FLASH 51
#define IAP_CMD_ERASE_SECTORS 52
#define IAP_CMD_BLANK_CHECK_SECTORS 53
#define IAP_CMD_READ_PART_ID 54
#define IAP_CMD_READ_BOOT_CODE_VERSION 55
#define IAP_CMD_COMPARE 56

#define CMD_SUCCESS 0
#define INVALID_COMMAND 1
#define SRC_ADDR_ERROR 2
#define DST_ADDR_ERROR 3
#define SRC_ADDR_NOT_MAPPED 4
#define DST_ADDR_NOT_MAPPED 5
#define COUNT_ERROR 6
#define INVALID_SECTOR 7
#define SECTOR_NOT_BLANK 8
#define SECTOR_NOT_PREPARED_FOR_WRITE_OPERATION 9
#define COMPARE_ERROR 10
#define BUSY 11
#define PARAM_ERROR 12
#define ADDR_ERROR 13
#define ADDR_NOT_MAPPED 14
#define CMD_LOCKED 15
#define INVALID_CODE 16
#define INVALID_BAUD_RATE 17
#define INVALID_STOP_BIT 18

#define FLASH_START_ADDRESS   0x00000000
#define FLASH_INVALID_ADDRESS (FLASH_START_ADDRESS - 1)

#define BLOCK_SIZE 0x1000

#include "__armlib.h"
#include "targets/LPC210x.h"

//-- Added by Yuri Tiomkin, 01.03.2006
#define PARTID_LPC2101 0x0004FF11


#define PARTID_LPC2104 0xFFF0FF12
#define PARTID_LPC2105 0xFFF0FF22
#define PARTID_LPC2106 0xFFF0FF32

#define PARTID_LPC2114 0x0101FF12
#define PARTID_LPC2119 0x0201FF12

#define PARTID_LPC2124 0x0101FF13
#define PARTID_LPC2129 0x0201FF13
#define PARTID_LPC2194 0x0301FF13

// #define PARTID_LPC2210 0x0301FF12
// #define PARTID_LPC2290 0x0301FF12

#define PARTID_LPC2212 0x0401FF12
#define PARTID_LPC2214 0x0601FF13
#define PARTID_LPC2292 0x0401FF13
#define PARTID_LPC2294 0x0501FF13
#define PARTID_LPC2131 0x0002FF01
#define PARTID_LPC2132 0x0002FF11
#define PARTID_LPC2134 0x0002FF12
#define PARTID_LPC2136 0x0002FF23
#define PARTID_LPC2138 0x0002FF25


//-- Added by Yuri Tiomkin, 08.04.2006
#define PARTID_LPC2146 0x0402FF23

typedef struct
{
  unsigned int n;
  unsigned int size;
} sectdef_t;

//-- Added by Yuri Tiomkin, 01.03.2006
static const sectdef_t geometry0[] =
{
  /* 15 * 4K Sectors */
  { 15, 0x00001000 },
  /* Terminator */
  {0, 0}
};


static const sectdef_t geometry1[] =
{
  /* 15 * 8K Sectors */
  { 15, 0x00002000 },
  /* Terminator */
  {0, 0}
};

static const sectdef_t geometry2[] =
{
  /* 8 * 8K Sectors */
  { 8, 0x00002000 },
  /* 2 * 64K Sectors */
  { 2, 0x00010000 },
  /* 8 * 8K Sectors */
  { 8, 0x00002000 },
  /* Terminator */
  {0, 0}
};

static const sectdef_t geometry3[] =
{
  /* 8 * 4K Sectors */
  { 8, 0x00001000 },
  /* 14 * 32K Sectors */
  { 14, 0x00008000 },
  /* 5 * 4K Sectors */
  { 5, 0x00001000 },
  /* Terminator */
  {0, 0}
};

static const sectdef_t *geometry = geometry1;

typedef volatile unsigned int *reg32_t;

typedef void (*IAP)(unsigned long [], unsigned long[]);

static unsigned char blockData[BLOCK_SIZE] __attribute__ ((section (".noload")));
static unsigned int currentBlockAddress = FLASH_INVALID_ADDRESS;
static unsigned int processorClockFrequency;

static unsigned long
iapCommand(unsigned long cmd, unsigned long p0, unsigned long p1, unsigned long p2, unsigned long p3, unsigned long *r0)
{
  static IAP iapEntry = (IAP)IAP_LOCATION;
  unsigned long command[5] = {cmd, p0, p1, p2, p3};
  unsigned long result[2];
  iapEntry(command, result);
  if (r0)
    *r0 = result[1];
  return result[0];
}

static int
isErased(unsigned int address, unsigned int length)
{
  while (address & 3)
    {
      if (*(unsigned char *)address != 0xFF)
        return 0;
      length--;
      address++;
    }
  while (length > 4)
    {
      if (*(unsigned long *)address != 0xFFFFFFFF)
        return 0;
      length -= 4;
      address += 4;
    }
  while (length)
    {
      if (*(unsigned char *)address != 0xFF)
        return 0;
      length--;
      address++;
    }
  return 1;
}

static int
getSectorNumber(unsigned int address)
{
  unsigned int sectorStartAddress = FLASH_START_ADDRESS;
  const sectdef_t *i;
  int n = 0;
  for (i = geometry; i->n; ++i)
    {
      int j;
      for (j = i->n; j; --j)
        {
          unsigned int sectorEndAddress = sectorStartAddress + i->size;
          if (address >= sectorStartAddress && address < sectorEndAddress)
            return n;
          sectorStartAddress = sectorEndAddress;
          ++n;
        }
    }
  return -1;
}

static unsigned int
getNumberOfSectors(void)
{
  const sectdef_t *i;
  int n = 0;
  for (i = geometry; i->n; ++i)
    n += i->n;
  return n;
}

static int
writeCurrentBlock()
{
  if (currentBlockAddress != FLASH_INVALID_ADDRESS)
    {
      int sectorNumber = getSectorNumber(currentBlockAddress);
      if (sectorNumber != -1)
        {
          if (iapCommand(IAP_CMD_PREPARE_SECTORS_FOR_WRITE_OPERATION, sectorNumber, sectorNumber, 0, 0, 0) != CMD_SUCCESS)
            return 0;
          if (iapCommand(IAP_CMD_COPY_RAM_TO_FLASH, currentBlockAddress, (unsigned long)blockData, BLOCK_SIZE, processorClockFrequency / 1000, 0) != CMD_SUCCESS)
            return 0;
        }
      currentBlockAddress = FLASH_INVALID_ADDRESS;
    }
  return 1;
}

static int
setOscillatorFrequency(unsigned int freq)
{
  processorClockFrequency = freq * (PLLCON & 1 ? (PLLCFG & 0xF) + 1 : 1);
}

void
loaderBegin()
{
  unsigned long partID;
  /* Set default value for oscillator frequency, this may overridden from host
     by setting a loader parameter. */
  setOscillatorFrequency(10000000);
  iapCommand(IAP_CMD_READ_PART_ID, 0, 0, 0 ,0, &partID);
  switch (partID)
    {
//-- Added by Yuri Tiomkin, 01.03.2006
      case PARTID_LPC2101:
        geometry = geometry0;
        break;
      case PARTID_LPC2104:
      case PARTID_LPC2105:
      case PARTID_LPC2106:
      case PARTID_LPC2114:
      case PARTID_LPC2119:
        geometry = geometry1;
        break;
      case PARTID_LPC2124:
      case PARTID_LPC2212:
      case PARTID_LPC2214:
      case PARTID_LPC2129:
      case PARTID_LPC2194:
      case PARTID_LPC2292:
      case PARTID_LPC2294:
        geometry = geometry2;
        break;
      case PARTID_LPC2131:
      case PARTID_LPC2132:
      case PARTID_LPC2134:
      case PARTID_LPC2136:
      case PARTID_LPC2138:
//-- Added by Yuri Tiomkin, 08.04.2006
      case PARTID_LPC2146:
        geometry = geometry3;
        break;
    }
}

void
loaderEnd()
{
}

void
loaderFlushWrites()
{
  writeCurrentBlock();
}

int
loaderPoke(unsigned char *address, unsigned int length)
{
  while (length)
    {
      unsigned long data = loaderReadWord();
      int i;
      for(i = 4; i && length; --i)
        {
          unsigned int blockAddress = ((unsigned int)address) & (~(BLOCK_SIZE - 1));
          if (blockAddress != currentBlockAddress)
            {
              writeCurrentBlock();
              memcpy(blockData, blockAddress, BLOCK_SIZE);
              currentBlockAddress = blockAddress;
            }
          blockData[(unsigned int)address - currentBlockAddress] = (unsigned char)data;
          data >>= 8;
          --length;
          ++address;
        }
    }
  return 1;
}

int
loaderMemset(unsigned char *address, unsigned int length,  unsigned char c)
{
  while (length)
    {
      int offset, frag;
      unsigned int blockAddress = ((unsigned int)address) & (~(BLOCK_SIZE - 1));
      if (blockAddress != currentBlockAddress)
        {
          writeCurrentBlock();
          memcpy(blockData, blockAddress, BLOCK_SIZE);
          currentBlockAddress = blockAddress;
        }
      offset = (unsigned int)address - currentBlockAddress;
      frag = BLOCK_SIZE - offset;
      if (length < frag)
        frag = length;
      memset(blockData + offset, c, frag);
      address += frag;
      length -= frag;
    }
  return 1;
}

int
loaderErase(unsigned char *address, unsigned int length)
{
  int sectorStartNumber, sectorEndNumber;
  sectorStartNumber = getSectorNumber((unsigned int)address);
  sectorEndNumber = getSectorNumber(((unsigned int)address) + length - 1);
  if (sectorStartNumber != -1 && sectorEndNumber != -1)
    {
      iapCommand(IAP_CMD_PREPARE_SECTORS_FOR_WRITE_OPERATION, sectorStartNumber, sectorEndNumber, 0, 0, 0);
      iapCommand(IAP_CMD_ERASE_SECTORS, sectorStartNumber, sectorEndNumber, processorClockFrequency / 1000, 0, 0);
    }
  return 1;
}

int
loaderEraseAll()
{
  unsigned int numberOfSectors = getNumberOfSectors();
  iapCommand(IAP_CMD_PREPARE_SECTORS_FOR_WRITE_OPERATION, 0, numberOfSectors - 1, 0, 0, 0);
  iapCommand(IAP_CMD_ERASE_SECTORS, 0, numberOfSectors - 1, processorClockFrequency / 1000, 0, 0);
  return 1;
}

int
loaderSetParameter(unsigned int parameter, unsigned int value)
{
  unsigned int result = 1;
  switch (parameter)
    {
      case 0:
        setOscillatorFrequency(value);
        break;
      default:
        break;
    }
  return result;
}

