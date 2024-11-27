#include "fighterMeters.h"

namespace fighterMeters
{
    // == playerMeterBundle ==

    meterConfiguration* meterBundle::getMeterConfig()
    {
        return currMeterConfig;
    }
    float meterBundle::getMeter()
    {
        float result = 0.0f;

        if (currMeterConfig != NULL)
        {
            result = meter;
        }

        return result;
    }
    float meterBundle::getMeterStockRemainder()
    {
        float result = 0.0f;

        if (currMeterConfig != NULL)
        {
            result = fmod(meter, currMeterConfig->meterStockSize);
        }

        return result;
    }
    u32 meterBundle::getMeterStocks()
    {
        u32 result = 0x00;

        if (currMeterConfig != NULL)
        {
            result = meter / currMeterConfig->meterStockSize;
        }

        return result;
    }

    void meterBundle::setMeterConfig(meterConfiguration* configIn, bool doReset)
    {
        currMeterConfig = configIn;
        if (doReset)
        {
            resetMeter();
        }
        addMeter(0.0f);
    }
    bool meterBundle::setMeter(float meterIn)
    {
        bool result = 0;

        if (currMeterConfig != NULL)
        {
            resetMeter();
            addMeter(meterIn);
        }

        return result;
    }
    bool meterBundle::setMeterStockRemainder(float meterIn)
    {
        bool result = 0;

        if (currMeterConfig != NULL)
        {
            roundDownMeter();
            addMeter(meterIn);
        }

        return result;
    }
    bool meterBundle::setMeterStocks(u32 stocksIn)
    {
        bool result = 0;

        if (currMeterConfig != NULL)
        {
            resetMeter();
            addMeter(stocksIn * currMeterConfig->meterStockSize);
        }

        return result;
    }

    void meterBundle::resetMeter()
    {
        meter = 0.0f;
    }
    bool meterBundle::addMeter(float meterIn)
    {
        bool result = 0;

        if (currMeterConfig != NULL)
        {
            meter += meterIn;
            meter = MAX(meter, 0.0f);
            meter = MIN(meter, currMeterConfig->maxMeter);
        }

        return result;
    }
    bool meterBundle::addMeterStocks(int meterStocksIn)
    {
        bool result = 0;

        if (currMeterConfig != NULL)
        {
            addMeter(meterStocksIn * currMeterConfig->meterStockSize);
        }

        return result;
    }
    bool meterBundle::roundUpMeter()
    {
        bool result = 0;

        if (currMeterConfig != NULL)
        {
            addMeter(currMeterConfig->meterStockSize - getMeterStockRemainder());
        }

        return result;
    }
    bool meterBundle::roundDownMeter()
    {
        bool result = 0;

        if (currMeterConfig != NULL)
        {
            addMeter(-getMeterStockRemainder());
        }

        return result;
    }

    meterBundle playerMeters[fighterHooks::maxFighterCount];


    void registerHooks();
}