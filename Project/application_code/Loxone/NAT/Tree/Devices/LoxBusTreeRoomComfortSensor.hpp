//
//  LoxBusTreeRoomComfortSensor.hpp
//
//  This is a user-supplied version for this sensor. It uses a
//  SenseAir S8 NDIR CO2 sensor and is used behind Tree-Touch
//  switches so temperature and humidity are dummy values and
//  not used in Loxone config.
//

#ifndef LoxBusTreeRoomComfortSensor_hpp
#define LoxBusTreeRoomComfortSensor_hpp

#include "LoxBusTreeDevice.hpp"


//findme
//#include "stm32f10x.h"

#include "stm32f1xx_hal_dma.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_tim.h"
#include "stm32f1xx_hal_i2c.h"

class __attribute__((__packed__)) tTreeRoomComfortSensorConfig : public tConfigHeader {
public:
  uint32_t updateIntervalMinutes;
  uint32_t unknownB;
private:
  tConfigHeaderFiller filler;
};

class LoxBusTreeRoomComfortSensor : public LoxBusTreeDevice {
  tTreeRoomComfortSensorConfig config;

private:
  uint32_t lastValueSendTime;
  uint32_t lastCO2UpdateTime;

  uint32_t co2ppmValue;
  uint32_t humidityPercentValue;
  uint32_t temperatureValue;
  uint32_t co2digitalAlarm;
  uint32_t co2analogAlarm;

  uint32_t co2ppmValueArray[5];
  uint32_t co2ppmValueArray_sorted[5];
  uint8_t co2ArrayCounter;


  virtual void ConfigUpdate(void);
  virtual void ConfigLoadDefaults(void);

  virtual void SendValues();
  virtual void SimpleInitUart();
  virtual void SimpleInitI2C();
  virtual uint16_t getCRC(unsigned char *buf, uint8_t len);
  virtual void RequestCO2update();
  virtual bool ProcessCO2message();
  virtual void UpdateMedianArrays(uint32_t meas);


public:
  LoxBusTreeRoomComfortSensor(LoxCANBaseDriver &driver, uint32_t serial, eAliveReason_t alive);

  virtual void Startup(void);
  virtual void Timer10ms(void);
};

#endif /* LoxBusTreeRoomComfortSensor_hpp */