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
    u32 meterBundle::getMeterStocks()
    {
        return meter / currMeterConfig->meterStockSize;
    }
    float meterBundle::getMeterStockRemainder()
    {
        return fmod(meter, currMeterConfig->meterStockSize);
    }
    bool meterBundle::getMeterEnabled()
    {
        return currMeterConfig != &disabledMeterConf;
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
    int meterBundle::setMeter(float meterIn)
    {
        return addMeter(meterIn - meter);
    }
    int meterBundle::setMeterStocks(u32 stocksIn)
    {
        return addMeter((stocksIn * currMeterConfig->meterStockSize) - meter);
    }
    void meterBundle::setMeterStockRemainder(float meterIn)
    {
        roundDownMeter();
        addMeter(meterIn);
    }

    void meterBundle::resetMeter()
    {
        meter = 0.0f;
    }
    void meterBundle::disableMeter()
    {
        setMeterConfig(disabledMeterConf, 1);
    }
    int meterBundle::addMeter(float meterIn)
    {
        int initialStocks = getMeterStocks();
        float currMeter = meter + meterIn;
        currMeter = MAX(currMeter, 0.0f);
        currMeter = MIN(currMeter, currMeterConfig->maxMeter);
        meter = currMeter;
        return getMeterStocks() - initialStocks;
    }
    int meterBundle::addMeterStocks(int meterStocksIn)
    {
        return addMeter(meterStocksIn * currMeterConfig->meterStockSize);
    }
    int meterBundle::roundUpMeter()
    {
        return addMeter(currMeterConfig->meterStockSize - getMeterStockRemainder());
    }
    int meterBundle::roundDownMeter()
    {
        return addMeter(-getMeterStockRemainder());
    }

    meterBundle playerMeters[fighterHooks::maxFighterCount];

    void registerHooks();
}