
#include <vcl.h>
#include "Unit1.h"
#include "ymodem.h"
#include "string.h"

void Int2Str(uint8_t* str, int32_t intnum)
{
  uint32_t i, Div = 1000000000, j = 0, Status = 0;

  for (i = 0; i < 10; i++)
  {
    str[j++] = (intnum / Div) + 48;

    intnum = intnum % Div;
    Div /= 10;
    if ((str[j-1] == '0') & (Status == 0))
    {
      j = 0;
    }
    else
    {
      Status++;
    }
  }
}

static  int32_t Receive_Byte (uint8_t *c, uint32_t timeout)
{
  DWORD len,i;
  while (timeout-- > 0)
  {
      if(!Form1->UART_READ(c,1,&len)){
         break;
      }
      if(len == 1){
         return 0;
      }
      Sleep(2);
  }
  return -1;
}

static uint32_t Send_Byte (uint8_t c)
{
  DWORD len,i;
  for(i=0;i<25;i++){
      if(!Form1->UART_WRITE(&c,1,&len)){
         break;
      }
      if(len == 1){
         break;
      }
      Sleep(2);
  }
  return 0;
}

void Ymodem_PrepareIntialPacket(uint8_t *data, const uint8_t* fileName, uint32_t *length)
{
  uint16_t i, j;
  uint8_t file_ptr[10];
  
  /* Make first three packet */
  data[0] = SOH;
  data[1] = 0x00;
  data[2] = 0xff;
  
  /* Filename packet has valid data */
  for (i = 0; (fileName[i] != '\0') && (i < FILE_NAME_LENGTH);i++)
  {
     data[i + PACKET_HEADER] = fileName[i];
  }

  data[i + PACKET_HEADER] = 0x00;
  memset(file_ptr,0,10);
  Int2Str (file_ptr, *length);
  for (j =0, i = i + PACKET_HEADER + 1; file_ptr[j] != '\0' ; )
  {
     data[i++] = file_ptr[j++];
  }
  
  for (j = i; j < PACKET_SIZE + PACKET_HEADER; j++)
  {
    data[j] = 0;
  }
}

void Ymodem_PreparePacket(uint8_t *SourceBuf, uint8_t *data, uint8_t pktNo, uint32_t sizeBlk)
{
  uint32_t i, size, packetSize;
  uint8_t* file_ptr;
  
  /* Make first three packet */
  packetSize = sizeBlk >= PACKET_1K_SIZE ? PACKET_1K_SIZE : PACKET_SIZE;
  size = sizeBlk < packetSize ? sizeBlk :packetSize;
  if (packetSize == PACKET_1K_SIZE)
  {
     data[0] = STX;
  }
  else
  {
     data[0] = SOH;
  }
  data[1] = pktNo;
  data[2] = (~pktNo);
  file_ptr = SourceBuf;
  
  /* Filename packet has valid data */
  for (i = PACKET_HEADER; i < size + PACKET_HEADER;i++)
  {
     data[i] = *file_ptr++;
  }
  if ( size  <= packetSize)
  {
    for (i = size + PACKET_HEADER; i < packetSize + PACKET_HEADER; i++)
    {
      data[i] = 0x1A; /* EOF (0x1A) or 0x00 */
    }
  }
}

uint16_t UpdateCRC16(uint16_t crcIn, uint8_t byte)
{
  uint32_t crc = crcIn;
  uint32_t in = byte | 0x100;

  do
  {
    crc <<= 1;
    in <<= 1;
    if(in & 0x100)
      ++crc;
    if(crc & 0x10000)
      crc ^= 0x1021;
  }
  
  while(!(in & 0x10000));

  return crc & 0xffffu;
}


/**
  * @brief  Cal CRC16 for YModem Packet
  * @param  data
  * @param  length
  * @retval None
  */
uint16_t Cal_CRC16(const uint8_t* data, uint32_t size)
{
  uint32_t crc = 0;
  const uint8_t* dataEnd = data+size;

  while(data < dataEnd)
    crc = UpdateCRC16(crc, *data++);
 
  crc = UpdateCRC16(crc, 0);
  crc = UpdateCRC16(crc, 0);

  return crc&0xffffu;
}

/**
  * @brief  Cal Check sum for YModem Packet
  * @param  data
  * @param  length
  * @retval None
  */
uint8_t CalChecksum(const uint8_t* data, uint32_t size)
{
  uint32_t sum = 0;
  const uint8_t* dataEnd = data+size;

  while(data < dataEnd )
    sum += *data++;

  return (sum & 0xffu);
}

/**
  * @brief  Transmit a data packet using the ymodem protocol
  * @param  data
  * @param  length
  * @retval None
  */
void Ymodem_SendPacket(uint8_t *data, uint16_t length)
{
  uint16_t      index;
  DWORD      writelen;
  for(index=0;index<length;){
   	  writelen = (length - index);
      if(!Form1->UART_WRITE((data+index),writelen,&writelen)){
         return;
      }
      index +=writelen;
  }
}

uint8_t Ymodem_WaitForChar(uint8_t ch)
{
  uint8_t receivedC[2],i;
  for(i=0;i<20;i++){
      if(Receive_Byte(&receivedC[0], NAK_TIMEOUT) == 0){
         //Form1->ShowTestMessage("Ymodem_WaitForChar %02x %c\r\n",receivedC[0],receivedC[0]);
         if(receivedC[0] == ch){
            return 1;
         }
      }
  }
  return 0;
}

/**
  * @brief  Transmit a file using the ymodem protocol
  * @param  buf: Address of the first byte
  * @retval The size of the file
  */
uint8_t Ymodem_Transmit (uint8_t *buf, const uint8_t* sendFileName, uint32_t sizeFile)
{
  
  uint8_t packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD];
  uint8_t filename[FILE_NAME_LENGTH];
  uint8_t *buf_ptr, tempCheckSum;
  uint16_t tempCRC;
  uint16_t blkNumber;
  uint8_t  CRC16_F = 0;
  uint32_t size = 0, pktSize,i;
  Form1->ShowTestMessage("\r\nYmodem:Transmit start\r\n");
  for (i = 0; i < (FILE_NAME_LENGTH - 1); i++)
  {
    filename[i] = sendFileName[i];
  }
  CRC16_F = 1;
    
  /* Prepare first block */
  Ymodem_PrepareIntialPacket(&packet_data[0], filename, &sizeFile);
  
  /* Send CRC or Check Sum based on CRC16_F */
  if (CRC16_F)
  {
      tempCRC = Cal_CRC16(&packet_data[3], PACKET_SIZE);
      packet_data[PACKET_SIZE + PACKET_HEADER + 0] = (uint8_t)((tempCRC >> 8) & 0xFF);
      packet_data[PACKET_SIZE + PACKET_HEADER + 1] = (uint8_t)((tempCRC >> 0) & 0xFF);
      Ymodem_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER + 2);
  }
  else
  {
      tempCheckSum = CalChecksum (&packet_data[3], PACKET_SIZE);
      packet_data[PACKET_SIZE + PACKET_HEADER + 0] = tempCheckSum;
      Ymodem_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER + 1);
  }
  if (!Ymodem_WaitForChar(ACK))
  {
    Form1->ShowTestMessage("Ymodem: Wait for Ack error\r\n");
    return -1;
  }
  if (!Ymodem_WaitForChar(CRC16))
  {
    Form1->ShowTestMessage("Ymodem: Wait for 'C' error\r\n");
    return -1;
  }
  buf_ptr = buf;
  size = sizeFile;
  blkNumber = 0x01;
  /* Here 1024 bytes package is used to send the packets */
  
  
  /* Resend packet if NAK  for a count of 10 else end of communication */
  while (size)
  {
    // Prepare next packet
    Ymodem_PreparePacket(buf_ptr, &packet_data[0], blkNumber, size);
    // Send next packet
    if (size >= PACKET_1K_SIZE)
    {
      pktSize = PACKET_1K_SIZE;
    }
    else
    {
      pktSize = PACKET_SIZE;
    }
    if (CRC16_F)
    {
       tempCRC = Cal_CRC16(&packet_data[3], pktSize);
       packet_data[pktSize + PACKET_HEADER + 0] = (uint8_t)((tempCRC >> 8) & 0xFF);
       packet_data[pktSize + PACKET_HEADER + 1] = (uint8_t)((tempCRC >> 0) & 0xFF);
       Ymodem_SendPacket(packet_data, pktSize + PACKET_HEADER + 2);
    }
    else
    {
       tempCheckSum = CalChecksum (&packet_data[3], pktSize);
       packet_data[PACKET_SIZE + PACKET_HEADER + 0] = tempCheckSum;
       Ymodem_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER + 1);
    }
    // Wait for Ack
    if (!Ymodem_WaitForChar(ACK))
    {
       Form1->ShowTestMessage("Ymodem: Send data packet error\r\n");
       return -1;
    }
    if(size > pktSize)
    {
       buf_ptr += pktSize;  
       size -= pktSize;
       if(blkNumber == (USER_FLASH_SIZE/1024))
       {
          return 0xFF; /*  error */
       }
       else
       {
          blkNumber++;
       }
    }
    else
    {
       buf_ptr += pktSize;
       size = 0;
    }
    Form1->ProgressBar1->Position = ((100*(sizeFile - size))/sizeFile);
  }

  Send_Byte(EOT);
  if (!Ymodem_WaitForChar(ACK))
  {
    Form1->ShowTestMessage("Ymodem: Send EOT Error\r\n");
    return -1;
  }

  packet_data[0] = SOH;
  packet_data[1] = 0;
  packet_data [2] = 0xFF;

  for (i = PACKET_HEADER; i < (PACKET_SIZE + PACKET_HEADER); i++)
  {
     packet_data [i] = 0x00;
  }
  
  //Send Packet
  Ymodem_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER);

  // Send CRC or Check Sum based on CRC16_F
  tempCRC = Cal_CRC16(&packet_data[3], PACKET_SIZE);
  Send_Byte(tempCRC >> 8);
  Send_Byte(tempCRC & 0xFF);
  if(!Ymodem_WaitForChar(ACK))
  {
    Form1->ShowTestMessage("Ymodem: Send last packet error\r\n");
    return -1;
  }

  Form1->ShowTestMessage("Ymodem: file Transmitted successfully\r\n");
  return 0; /* file transmitted successfully */
}

