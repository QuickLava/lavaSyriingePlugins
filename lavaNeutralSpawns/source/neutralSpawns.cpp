#include <os/OSError.h>
#include <sy_core.h>
#include <OS/OSCache.h>
#include <modules.h>
#include <st/stage.h>
#include <gm/gm_global.h>
#include <ft/ft_manager.h>
#include <mt/mt_prng.h>
#include <logUtils.h>

namespace lavaNeutralSpawns {
    const bool doRandomTeamSideSwaps = true;
    const bool doRandomTeammatePosSwaps = true;
    const char outputTag[] = "[lavaNeutralSpawns] ";
    void doNeutralSpawns()
    {
        gmGlobalModeMelee* modeMelee = g_GameGlobal->m_modeMelee;
        if (modeMelee->m_meleeInitData.m_isTeams && modeMelee->m_meleeInitData.m_numPlayers == 0x4)
        {
            register Stage* stagePtr; 
            asm
            {
                mr stagePtr, r31;
            }

            Vec3f stageSpawnLocs[4];
            Vec3f* targetVec = stageSpawnLocs;
            for (u32 i = 0; i < 4; i++)
            {
                stagePtr->getFighterStartPos(targetVec, i);
                OSReport_N("%sSpawn #%d: {%.02f, %.02f, %.02f}\n", outputTag, i, targetVec->m_x, targetVec->m_y, targetVec->m_z);
                targetVec++;
            }

            u8 tempVar;
            u8 indexA = 0x00;
            u8 indexB = 0x01;
            u8 indexC = 0x02;
            u8 indexD = 0x03;
            if (stageSpawnLocs[indexA].m_x > stageSpawnLocs[indexB].m_x) { tempVar = indexA; indexA = indexB; indexB = tempVar; }
            if (stageSpawnLocs[indexC].m_x > stageSpawnLocs[indexD].m_x) { tempVar = indexC; indexC = indexD; indexD = tempVar; }
            if (stageSpawnLocs[indexA].m_x > stageSpawnLocs[indexC].m_x) { tempVar = indexA; indexA = indexC; indexC = tempVar; }
            if (stageSpawnLocs[indexB].m_x > stageSpawnLocs[indexD].m_x) { tempVar = indexB; indexB = indexD; indexD = tempVar; }
            if (stageSpawnLocs[indexB].m_x > stageSpawnLocs[indexC].m_x) { tempVar = indexB; indexB = indexC; indexC = tempVar; }

            u32 randNum = randi(7);
            if (doRandomTeamSideSwaps && randNum & 0b001)
            {
                tempVar = indexA; indexA = indexD; indexD = tempVar;
                tempVar = indexB; indexB = indexC; indexC = tempVar;
                OSReport_N("%sDid Random Team Side Swap!\n", outputTag);
            }
            if (doRandomTeammatePosSwaps && randNum & 0b010)
            {
                tempVar = indexA; indexA = indexB; indexB = tempVar;
                OSReport_N("%sDid Random Left-Side Player Swap!\n", outputTag);
            }
            if (doRandomTeammatePosSwaps && randNum & 0b100)
            {
                tempVar = indexC; indexC = indexD; indexD = tempVar;
                OSReport_N("%sDid Random Right-Side Player Swap!\n", outputTag);
            }

            u8 sortedSpawnLocIndices[4];
            sortedSpawnLocIndices[0] = indexA;
            sortedSpawnLocIndices[1] = indexB;
            sortedSpawnLocIndices[2] = indexC;
            sortedSpawnLocIndices[3] = indexD;

            for (u32 i = 0; i < 4; i++)
            {
                targetVec = stageSpawnLocs + sortedSpawnLocIndices[i];
                OSReport_N("%sSpawn #%d: {%.02f, %.02f, %.02f}\n", outputTag, i, targetVec->m_x, targetVec->m_y, targetVec->m_z);
            }

            gmPlayerInitData* playerData = modeMelee->m_playersInitData;
            s8 firstTeam = playerData->m_teamNo;
            u32 matchingCount = 0x00;
            for (u32 i = 0; i < 4; i++)
            {
                u32 targetIndex;
                if (playerData->m_teamNo == firstTeam)
                {
                    targetIndex = matchingCount;
                    matchingCount++;
                }
                else
                {
                    targetIndex = 3 - (i - matchingCount);
                }
                playerData->m_startPointIdx = sortedSpawnLocIndices[targetIndex];
                playerData++;
            }

            playerData = modeMelee->m_playersInitData;
            for (u32 i = 0; i < 0x4; i++)
            {
                targetVec = stageSpawnLocs + playerData->m_startPointIdx;
                OSReport_N("%sPlayer #%d [Team %d]: {%.02f, %.02f, %.02f}\n", outputTag, i, playerData->m_teamNo, targetVec->m_x, targetVec->m_y, targetVec->m_z);
                playerData++;
            }
        }
    }

    void Init()
    {
        // Note: 0x8070AA14 is SORA_MELEE base address
        // 0x8092EF5C: 0x24 bytes into symbol "createStagePositions/[Stage]/stage.o"
        SyringeCore::syInlineHookRel(0x224548, reinterpret_cast<void*>(doNeutralSpawns), Modules::SORA_MELEE);
    }

    void Destroy()
    {
        OSReport("Goodbye\n");
    }
}