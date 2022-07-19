//
//  LoxBusTreeRoomComfortSensor.cpp
//

#include "LoxBusTreeRoomComfortSensor.hpp"

#include "global_functions.hpp"
#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <string.h>

#include <__cross_studio_io.h>

static UART_HandleTypeDef huart2;
static I2C_HandleTypeDef hi2c2;
static DMA_HandleTypeDef hdma_i2c2_rx;
static DMA_HandleTypeDef hdma_i2c2_tx;

static unsigned char getCo2Cmd[]= {0xFE,0x04,0x00,0x03,0x00,0x01,0xD5,0xC5};
//static unsigned char initTmpCmd[]= {0xBE,0x08,0x00};
//static unsigned char readTmpCmd[]= {0xAC,0x33,0x00};
unsigned char rxBuffer[7]={0,0,0,0,0,0,0};
unsigned char gChar;
uint8_t I2Cbuf[8];
uint8_t I2CexpectBytesNum=1;
uint8_t I2CtestState=0;
uint8_t rxCounter = 0;
uint8_t expectBytesNum = 0;
bool expectReply=false;
#define SENSORUPDATEINTERVAL 1*60*1000
#define CO2TIMEOUT 1000

bool configReceived=false;

/***
 *  Constructor
 ***/
LoxBusTreeRoomComfortSensor::LoxBusTreeRoomComfortSensor(LoxCANBaseDriver &driver, uint32_t serial, eAliveReason_t alive)
  : LoxBusTreeDevice(driver, serial, eDeviceType_t_RoomComfortSensorTree, 0, 11010706, 1, sizeof(config), &config, alive) {
}

void LoxBusTreeRoomComfortSensor::ConfigUpdate(void) {
  //debug_printf("unknownA = %d\n", config.updateIntervalMinutes);
  //debug_printf("unknownB = %d\n", config.unknownB);
}

void LoxBusTreeRoomComfortSensor::ConfigLoadDefaults(void) {
}

void LoxBusTreeRoomComfortSensor::Startup(void) {
  //init dummy measured values
  this->co2ppmValue=1200;
  this->humidityPercentValue=66600;
  this->temperatureValue=66600;

  //init timers
  this->lastValueSendTime=0;
  this->lastCO2UpdateTime=SENSORUPDATEINTERVAL;
  this->config.updateIntervalMinutes=8;
  expectReply=false;

  //init median filter
  for(int i;i<5;i++){
    this->co2ppmValueArray[i]=(i+1)*100;
  }
  memcpy(this->co2ppmValueArray_sorted,this->co2ppmValueArray,sizeof(co2ppmValueArray));
  this->co2ArrayCounter=0;
  
  SimpleInitUart();
  //debug_printf("UART init OK\n");
  //SimpleInitI2C();
  //debug_printf("I2C init OK\n");
}

void LoxBusTreeRoomComfortSensor::Timer10ms(void) {
  LoxNATExtension::Timer10ms();
  this->lastValueSendTime+=10;
  this->lastCO2UpdateTime+=10;

  if ((this->lastValueSendTime >= this->config.updateIntervalMinutes*60*1000) && (this->state == eDeviceState_online)){
    SendValues();
    this->lastValueSendTime=0;
  }

  if (this->lastCO2UpdateTime >= SENSORUPDATEINTERVAL && !expectReply) {
    //get CO2 update from S8
    RequestCO2update();
  }
  
  //Full modbus message received
  if (expectReply && (rxCounter==expectBytesNum)){
    if (ProcessCO2message()) this->lastCO2UpdateTime=0;
    else lastCO2UpdateTime=SENSORUPDATEINTERVAL-CO2TIMEOUT;//get another value in 1sec
  }

  //Monitor for modbus timeout
  if (lastCO2UpdateTime>SENSORUPDATEINTERVAL+CO2TIMEOUT){
    expectReply=false;
    lastCO2UpdateTime=SENSORUPDATEINTERVAL;//get another value in 1sec
    //debug_printf("Response timeout from S8\n");
  }
  
}

void LoxBusTreeRoomComfortSensor::SendValues(void){
  send_digital_value(0,this->co2digitalAlarm);
  send_analog_value(0, this->temperatureValue, 0, eAnalogFormat_div_1000);
  send_analog_value(1, this->humidityPercentValue, 0, eAnalogFormat_div_1000);
  send_analog_value(2, this->co2ppmValue, 0, eAnalogFormat_mul_1);
  send_analog_value(3, this->co2analogAlarm, 0, eAnalogFormat_mul_1);
}

extern "C" void USART2_IRQHandler() {
    HAL_UART_IRQHandler(&huart2);
}

extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  //Copy received charachter into RX buffer
  if (expectReply && (rxCounter<expectBytesNum)){
    rxBuffer[rxCounter]=gChar;
    rxCounter++;
  }
  //Await new RX character
  HAL_UART_Receive_IT(&huart2,&gChar,1);
}

extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
}

//calculate modbus CRC
uint16_t LoxBusTreeRoomComfortSensor::getCRC(unsigned char *buf, uint8_t len) {  
  uint16_t crc = 0xFFFF;

  for (int pos = 0; pos < len; pos++) {
    crc ^= (unsigned int)buf[pos];    // XOR byte into least sig. byte of crc

    for (int i = 8; i != 0; i--) {    // Loop over each bit
      if ((crc & 0x0001) != 0) {      // If the LSB is set
        crc >>= 1;                    // Shift right and XOR 0xA001
        crc ^= 0xA001;
      }
      else                            // Else LSB is not set
        crc >>= 1;                    // Just shift right
    }
  }
  return crc;
}

void LoxBusTreeRoomComfortSensor::RequestCO2update(void) {
  HAL_UART_Transmit_IT(&huart2, getCo2Cmd, sizeof(getCo2Cmd));
  rxCounter=0;
  expectBytesNum=7;
  expectReply=true;
  rxBuffer[0]='\0';
}

bool LoxBusTreeRoomComfortSensor::ProcessCO2message(void){
  expectBytesNum=0;
  expectReply=false;

  uint16_t calcCRC = getCRC(rxBuffer,5);
  uint16_t rxCRC = (rxBuffer[6] <<8) | rxBuffer[5];
  
  bool crcOK=(rxCRC == calcCRC);
  if (crcOK){
    uint32_t meas = (rxBuffer[3] <<8) | rxBuffer[4];
    UpdateMedianArrays(meas);
    this->co2ppmValue = this->co2ppmValueArray_sorted[2];
    //this->co2ppmValue = (rxBuffer[3] <<8) | rxBuffer[4];
    //debug_printf("CO2val= %d ppm\n",this-> co2ppmValue);
    if(this->co2ppmValue>2500) {
      this->co2digitalAlarm=4;
      this->co2analogAlarm=3;
    }
    else if(this->co2ppmValue>=1500){
      this->co2digitalAlarm=2;
      this->co2analogAlarm=2;
    }
    else{
      this->co2digitalAlarm=1;
      this->co2analogAlarm=1;
    }
  }
  //else //debug_printf("CRC mismatch, communication error\n");

  return crcOK;
}

void LoxBusTreeRoomComfortSensor::UpdateMedianArrays(uint32_t meas){
  //add most recent meas to array, overwriting oldest value
  this->co2ppmValueArray[this->co2ArrayCounter]=meas;

  //increase counter that holds the position of the oldest value
  this->co2ArrayCounter++;
  if(this->co2ArrayCounter>4){
    this->co2ArrayCounter=0;
  }

  //create the sorted array from scratch with insertion sort algorithm
  memcpy(this->co2ppmValueArray_sorted,this->co2ppmValueArray,sizeof(co2ppmValueArray));
  uint8_t i, j;
  uint32_t key;

  for (i=1; i<5; i++){
    key= this->co2ppmValueArray_sorted[i];
    j=i-1;
    //working backwards from largest to smallest
    //shift elements of arr[0..i-1] one ahead if they are larger than key
    while (j>=0 && this->co2ppmValueArray_sorted[j]>key){
      this->co2ppmValueArray_sorted[j+1] = this->co2ppmValueArray_sorted[j];
      j=j-1;
    }
    co2ppmValueArray_sorted[j+1]=key;
  }

}

void LoxBusTreeRoomComfortSensor::SimpleInitUart(void){
  //setup simple UART implementation
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_USART2_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  /**USART2 GPIO Configuration    
  PA2     ------> USART2_TX
  PA3     ------> USART2_RX 
  */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  
  huart2.Instance        = USART2;
  huart2.Init.BaudRate   = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits   = UART_STOPBITS_1;
  huart2.Init.Parity     = UART_PARITY_NONE;
  huart2.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
  huart2.Init.Mode       = UART_MODE_TX_RX;
  
  if (HAL_UART_Init(&huart2) != HAL_OK){
    //debug_printf("UART2 init failed\n");
  }

  /* USART2 interrupt Init */
  HAL_NVIC_SetPriority(USART2_IRQn, 15, 0);
  HAL_NVIC_EnableIRQ(USART2_IRQn);

  HAL_UART_Receive_IT(&huart2,&gChar,1);
}

void LoxBusTreeRoomComfortSensor::SimpleInitI2C(void){
  //setup simple I2C implementation
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_I2C2_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  /**I2C2 GPIO Configuration    
  PB10     ------> I2C2_SCL
  PB11     ------> I2C2_SDA 
  */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_11;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  
  __HAL_RCC_I2C2_FORCE_RESET();
  __HAL_RCC_I2C2_RELEASE_RESET();
  

  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 50000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_16_9;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  
  if (HAL_I2C_Init(&hi2c2) != HAL_OK){
    //debug_printf("I2C2 init failed\n");
  }
  /* I2C2 interrupt Init */
  HAL_NVIC_SetPriority(I2C2_ER_IRQn,13,0);
  HAL_NVIC_SetPriority(I2C2_EV_IRQn,13,0);
  //debug_printf("try I2C IRQen\n");
  HAL_NVIC_EnableIRQ(I2C2_ER_IRQn);
  HAL_NVIC_EnableIRQ(I2C2_EV_IRQn);

  /* DMA controller initialization */
  __HAL_RCC_DMA1_CLK_ENABLE();
  
  hdma_i2c2_rx.Instance = DMA1_Channel5; //RM0008 Table 78
  hdma_i2c2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_i2c2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_i2c2_rx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_i2c2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_i2c2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_i2c2_rx.Init.Mode = DMA_NORMAL;
  hdma_i2c2_rx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
  
  if(HAL_DMA_Init(&hdma_i2c2_rx) != HAL_OK){
    //debug_printf("RX DMA init failed\n");
  }
  __HAL_LINKDMA(&hi2c2,hdmarx,hdma_i2c2_rx);
  
  hdma_i2c2_tx.Instance = DMA1_Channel4;  //RM0008 Table 78
  hdma_i2c2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdma_i2c2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_i2c2_tx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_i2c2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_i2c2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_i2c2_tx.Init.Mode = DMA_NORMAL;
  hdma_i2c2_tx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
  
  if(HAL_DMA_Init(&hdma_i2c2_tx) != HAL_OK){
    //debug_printf("TX DMA init failed\n");
  }
  __HAL_LINKDMA(&hi2c2,hdmatx,hdma_i2c2_tx);

  /* Enable DMA interrupts */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn,14,0);
  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn,14,0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
  

}

extern "C" void I2C2_EV_IRQHandler(){
    HAL_I2C_EV_IRQHandler(&hi2c2);
}
extern "C" void I2C2_ER_IRQHandler(){
    HAL_I2C_ER_IRQHandler(&hi2c2);
}
extern "C" void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) {
  //debug_printf("I2C TX OK\n");
}
extern "C" void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) {
  //debug_printf("I2C RX OK\n");
  for(int8_t i=0;i<I2CexpectBytesNum;i++){
    //debug_printf("RX data %d: 0x%x\n",i,I2Cbuf[i]);
    }
}
extern "C" void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
  //debug_printf("I2C transfer failed\n");
}
extern "C" void HAL_I2C_AbortCpltCallback(I2C_HandleTypeDef *hi2c){
  //debug_printf("I2C transfer abort\n");
}
extern "C" void DMA1_Channel5_IRQHandler(){
    HAL_DMA_IRQHandler(&hdma_i2c2_rx);
}
extern "C" void DMA1_Channel4_IRQHandler(){
    HAL_DMA_IRQHandler(&hdma_i2c2_tx);
}
