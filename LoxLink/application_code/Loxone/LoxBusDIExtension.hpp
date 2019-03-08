//
//  LoxBusDIExtension.hpp
//
//  Created by Markus Fritze on 06.03.19.
//  Copyright © 2019 Markus Fritze. All rights reserved.
//

#ifndef LoxBusDIExtension_hpp
#define LoxBusDIExtension_hpp

#include "LoxNATExtension.hpp"

class tDIExtensionConfig : public tConfigHeader {
public:
    uint32_t frequencyInputsBitmask; // bit is set, if an input is used as a frequency counter
private:
    tConfigHeaderFiller filler;
};

class LoxBusDIExtension : public LoxNATExtension {
    uint32_t hardwareBitmask;
    uint32_t lastBitmaskTime;
    uint32_t lastFrequencyTime;
    tDIExtensionConfig config;

    virtual void ConfigUpdate(void);
    virtual void SendValues();

public:
    LoxBusDIExtension(LoxCANDriver& driver, uint32_t serial);

    virtual void Timer10ms(void);
};

#endif /* LoxBusDIExtension_hpp */
