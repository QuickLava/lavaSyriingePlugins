#include "fighterMeters.h"

namespace fighterMeters
{
    // == playerMeterBundle ==

    const meterConfiguration disabledMeterConf = {0.0f, 1.0f};

    meterBundle::meterBundle() : meter(0.0f), currMeterConfig(&disabledMeterConf) {};
    const meterConfiguration* meterBundle::getMeterConfig()
    {
        return currMeterConfig;
    }
    float meterBundle::getMeter()
    {
        return meter;
    }
    float meterBundle::getMeterStockRemainder()
    {
        return fmod(meter, currMeterConfig->meterStockSize);
    }
    u32 meterBundle::getMeterStocks()
    {
        return meter / currMeterConfig->meterStockSize;
    }

    void meterBundle::setMeterConfig(const meterConfiguration& configIn, bool doReset)
    {
        currMeterConfig = &configIn;
        if (doReset)
        {
            resetMeter();
        }
        addMeter(0.0f);
    }
    void meterBundle::setMeter(float meterIn)
    {
        resetMeter();
        addMeter(meterIn);
    }
    void meterBundle::setMeterStockRemainder(float meterIn)
    {
        roundDownMeter();
        addMeter(meterIn);
    }
    void meterBundle::setMeterStocks(u32 stocksIn)
    {
        resetMeter();
        addMeter(stocksIn * currMeterConfig->meterStockSize);
    }

    void meterBundle::resetMeter()
    {
        meter = 0.0f;
    }
    void meterBundle::disableMeter()
    {
        currMeterConfig = &disabledMeterConf;
        resetMeter();
    }
    void meterBundle::addMeter(float meterIn)
    {
        float currMeter = meter + meterIn;
        currMeter = MAX(currMeter, 0.0f);
        currMeter = MIN(currMeter, currMeterConfig->maxMeter);
        meter = currMeter;
    }
    void meterBundle::addMeterStocks(int meterStocksIn)
    {
        addMeter(meterStocksIn * currMeterConfig->meterStockSize);
    }
    void meterBundle::roundUpMeter()
    {
        addMeter(currMeterConfig->meterStockSize - getMeterStockRemainder());
    }
    void meterBundle::roundDownMeter()
    {
        addMeter(-getMeterStockRemainder());
    }

    meterBundle playerMeters[fighterHooks::maxFighterCount];

    void registerHooks();
}