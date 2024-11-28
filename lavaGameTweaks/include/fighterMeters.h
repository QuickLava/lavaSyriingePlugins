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
        const meterConfiguration* currMeterConfig;

    public:

        meterBundle();

        // Getters
        const meterConfiguration* getMeterConfig();
        float getMeter();
        float getMeterStockRemainder();
        u32 getMeterStocks();

        // Setters
        void setMeterConfig(const meterConfiguration& configIn, bool doReset);
        void setMeter(float meterIn);
        void setMeterStockRemainder(float meterIn);
        void setMeterStocks(u32 stocksIn);

        // Modifiers
        void resetMeter();
        void disableMeter();
        void addMeter(float meterIn);
        void addMeterStocks(int meterStocksIn);
        void roundUpMeter();
        void roundDownMeter();
    };
    extern meterBundle playerMeters[fighterHooks::maxFighterCount];

    void registerHooks();
}

#endif
