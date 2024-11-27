#ifndef LAVA_FIGHTER_METERS_H_V1
#define LAVA_FIGHTER_METERS_H_V1

#include <cstdlib>
#include "fighterHooks.h"

namespace fighterMeters
{
    struct meterConfiguration
    {
        float maxMeter;
        float meterStockSize;
    };
    struct meterBundle
    {
    private:
        float meter;
        meterConfiguration* currMeterConfig;

    public:
        meterBundle(){};

        // Getters
        meterConfiguration* getMeterConfig();
        float getMeter();
        float getMeterStockRemainder();
        u32 getMeterStocks();

        // Setters
        void setMeterConfig(meterConfiguration* configIn, bool doReset);
        bool setMeter(float meterIn);
        bool setMeterStockRemainder(float meterIn);
        bool setMeterStocks(u32 stocksIn);

        // Modifiers
        void resetMeter();
        bool addMeter(float meterIn);
        bool addMeterStocks(int meterStocksIn);
        bool roundUpMeter();
        bool roundDownMeter();
    };
    extern meterBundle playerMeters[fighterHooks::maxFighterCount];

    void registerHooks();
}

#endif
