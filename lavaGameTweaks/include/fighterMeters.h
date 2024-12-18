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

        // Returns the active meter config.
        const meterConfiguration* getMeterConfig();
        // Returns the current meter total.
        float getMeter();
        // Returns the number of meter stocks (as calculated using the active meter config).
        u32 getMeterStocks();
        // Returns the amount of meter contributing towards filling the current stock (as calculated using the active meter config).
        float getMeterStockRemainder();
        // Returns true if a meter config is currently set.
        bool getMeterEnabled();

        // Setters

        // Sets the active meter config.
        void setMeterConfig(const meterConfiguration& configIn, bool doReset);
        // Sets the current meter total.
        // Returns the resulting change in meter stock count.
        int setMeter(float meterIn);
        // Sets the current number of meter stocks.
        // Returns the resulting change in meter stock count.
        int setMeterStocks(u32 stocksIn);
        // Sets the amount of meter contributing towards filling the current stock (as calculated using the active meter config).
        void setMeterStockRemainder(float meterIn);

        // Modifiers

        // Resets the current meter total to 0.
        void resetMeter();
        // Resets the current meter total to 0, then disables all meter gain by discarding the active configuration.
        void disableMeter();
        // Adds the specified amount of meter to the current total.
        // Returns the resulting change in meter stock count.
        int addMeter(float meterIn);
        // Adds meter equivalent to the specified number of stocks (as calculated using the active meter config) to the current meter total.
        // Returns the resulting change in meter stock count.
        int addMeterStocks(int meterStocksIn);
        // Adds the amount of meter necessary to fill the current stock (as calculated using the active meter config).
        // Returns the resulting change in meter stock count.
        int roundUpMeter();
        // Subtracts the amount of meter necessary to empty the current stock (as calculated using the active meter config).
        // Returns the resulting change in meter stock count.
        int roundDownMeter();
    };
    extern meterBundle playerMeters[fighterHooks::maxFighterCount];

    void registerHooks();
}

#endif
