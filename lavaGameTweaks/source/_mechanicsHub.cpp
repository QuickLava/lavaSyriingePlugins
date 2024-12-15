#include "_mechanicsHub.h"

namespace mechHub
{
    Vec3f gfxFaceScreenRotVec = { 0.0f, -90.0f * radianConvConstant, 0.0f };
    Vec3f zeroVec = { 0.0f, 0.0f, 0.0f };
    Vec3f gfxFlattenSclVec = { 2.0f, 2.0f, 0.1f };

    u32 indexBuffer[lid__COUNT] = {};

    char outputTag[] = "[lavaMechHub] ";
    const char addonShortName[] = "MECH_HUB";

    const u32 totalMechanicsCount = amid__COUNT + pmid__COUNT;
    u8 mechanicEnabledMasks[totalMechanicsCount + 1] = {};
    u8* const mechanicsDisabledMask = mechanicEnabledMasks + totalMechanicsCount;
    u8* const activeMechanicEnabledMasks = mechanicEnabledMasks;
    u8* const passiveMechanicEnabledMasks = mechanicEnabledMasks + amid__COUNT;

    u8 const passiveMechanicP1ToggleLineIDs[pmid__COUNT] = { lid_MAGIC_SERIES_TOGGLE_P1 };

    bool getFlagForPlayer(register u8 flagByte, register u32 playerNo)
    {
        register bool result;

        asm
        {
            subfic result, playerNo, 0x20;
            rlwnm result, flagByte, result, 0x1F, 0x1F;
        }

        return result;
    }
    u8 setFlagForPlayer(register u8 flagByte, register u32 playerNo, register bool stateIn)
    {
        register u8 result;

        asm
        {
            li r0, 1;
            rlwnm r0, r0, playerNo, 0x18, 0x1F;
            andc result, flagByte, r0;
            cmplwi stateIn, 0x00;
            b exit;
            or result, flagByte, r0;
        exit:
        }

        return result;
    }
    u8 toggleFlagForPlayer(register u8 flagByte, register u32 playerNo)
    {
        register u8 result;

        asm
        {
            li r0, 1;
            rlwnm r0, r0, playerNo, 0x18, 0x1F;
            xor result, flagByte, r0;
        exit:
        }

        return result;
    }

    void clearMechanicEnabledMasks()
    {
        u8* currMask = mechanicEnabledMasks - 1;
        for (u32 i = 0; i < totalMechanicsCount; i++)
        {
            *(++currMask) = 0;
        }
    }
    void updateMechanicEnabledMasks()
    {
        if (*mechanicsDisabledMask != 0) return;

        u8* currMask = mechanicEnabledMasks - 1;
        for (u32 i = 0; i < amid__COUNT; i++)
        {
            currMask++;
            u8 maskValue = *currMask;
            u8 currMaskBit = 0b1;
            for (u32 u = 0; u < 4; u++)
            {
                u32 activeMechanic = ((codeMenu::cmSelectionLine*)indexBuffer[lid_ACTIVE_MECHANIC_P1 + u])->m_value;
                if (activeMechanic == i)
                {
                    maskValue |= currMaskBit;
                }
                else
                {
                    maskValue &= ~currMaskBit;
                }
                currMaskBit = currMaskBit << 1;
            }
            *currMask = maskValue;
        }
        for (u32 i = 0; i < pmid__COUNT; i++)
        {
            currMask++;
            u32 p1ToggleLineID = passiveMechanicP1ToggleLineIDs[i];
            u8 maskValue = *currMask;
            u8 currMaskBit = 0b1;
            for (u32 u = 0; u < 4; u++)
            {
                u32 mechanicEnabled = ((codeMenu::cmSelectionLine*)indexBuffer[p1ToggleLineID + u])->m_value;
                if (mechanicEnabled != 0)
                {
                    maskValue |= currMaskBit;
                }
                else
                {
                    maskValue &= ~currMaskBit;
                }
                currMaskBit = currMaskBit << 1;
            }
            *currMask = maskValue;
        }
    }

    bool populate()
    {
        bool result = codeMenu::loadCodeMenuAddonLOCsToBuffer(addonShortName, indexBuffer, lid__COUNT);
        if (result)
        {
            OSReport_N("%sSuccessfully Loaded Addon Index File to Buffer 0x%08X!\n", outputTag, indexBuffer);
        }
        else
        {
            *mechanicsDisabledMask = 0xFF;
            OSReport_N("%sFailed to Load Addon Index File to Buffer!\n", outputTag);
        }
        return result;
    }
    void registerHooks()
    {
        fighterHooks::ftCallbackMgr::registerMeleeOnGameSetCallback(clearMechanicEnabledMasks);
        fighterHooks::ftCallbackMgr::registerMeleeOnStartCallback(updateMechanicEnabledMasks);
    }

    bool getMechanicEnabled(u32 playerNo, u32 mechanicID, u8* maskArray)
    {
        return (*mechanicsDisabledMask == 0x00) ? getFlagForPlayer(maskArray[mechanicID], playerNo) : false;
    }
    bool getActiveMechanicEnabled(u32 playerNo, activeMechanicIDs mechanicID)
    {
        asm
        {
            lis r5, activeMechanicEnabledMasks@ha;
            lwz r5, activeMechanicEnabledMasks@l(r5);
            bl getMechanicEnabled;
        }
    }
    bool getPassiveMechanicEnabled(u32 playerNo, passiveMechanicIDs mechanicID)
    {
        asm
        {
            lis r5, passiveMechanicEnabledMasks@ha;
            lwz r5, passiveMechanicEnabledMasks@l(r5);
            bl getMechanicEnabled;
        }
    }

    int doMeterGain(Fighter* fighterIn, float meterIn, EfID meterGainGraphic, float graphicScale, u32 announcerClipCond)
    {
        int result = 0;

        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount)
        {
            soModuleEnumeration* moduleEnum = fighterIn->m_moduleAccesser->m_enumerationStart;
            fighterMeters::meterBundle* targetMeterBundle = fighterMeters::playerMeters + fighterPlayerNo;

            int initialStockCount = targetMeterBundle->getMeterStocks();
            targetMeterBundle->addMeter(meterIn);
            int finalStockCount = targetMeterBundle->getMeterStocks();
            result = finalStockCount - initialStockCount;

            bool doAnnouncerClip = 0;
            if (result != 0)
            {
                doAnnouncerClip = (result > 0) ? announcerClipCond & announcerOnStockGain : announcerClipCond & announcerOnStockLoss;
            }

            if (result > 0)
            {
                if (doAnnouncerClip && finalStockCount <= 10)
                {
                    mechHub::playSE(fighterIn, (SndID)((snd_se_narration_one + 1) - finalStockCount));
                }
                mechHub::reqCenteredGraphic(fighterIn, meterGainGraphic, graphicScale, 1);
            }
        }

        return result;
    }

    float getDistanceBetween(StageObject* obj1, StageObject* obj2, bool usePrevPos)
    {
        register float result;

        register soPostureModule* postureModule1 = obj1->m_moduleAccesser->getPostureModule();
        register soPostureModule* postureModule2 = obj2->m_moduleAccesser->getPostureModule();
        Vec3f pos1, pos2;

        register Vec3f* pos1Ptr = &pos1;
        register Vec3f* pos2Ptr = &pos2;
        register u32 posFuncOff = (usePrevPos) ? 0x20 : 0x18;
        asm
        {
            mr r3, pos1Ptr;
            mr r4, postureModule1;
            lwz r12, 0x00(r4);
            lwzx r12, r12, posFuncOff;
            mtctr r12;
            bctrl;
            mr r3, pos2Ptr;
            mr r4, postureModule2;
            lwz r12, 0x00(r4);
            lwzx r12, r12, posFuncOff;
            mtctr r12;
            bctrl;
        }
        
        return pos1.distance(&pos2);
    }
    u32 reqCenteredGraphic(StageObject* objectIn, EfID effectID, float scale, bool follow)
    {
        u32 result;

        soEffectModule* effectModule = objectIn->m_moduleAccesser->getEffectModule();
        if (!follow)
        {
            result = effectModule->req(effectID,gfxRootBoneID, &zeroVec, &zeroVec, scale, &zeroVec, &zeroVec, 0, 0);
        }
        else
        {
            result = effectModule->reqFollow(effectID, gfxRootBoneID, &zeroVec, &zeroVec, scale, 0, 0, 0, 0);
        }

        return result;
    }
    u32 playSE(StageObject* objectIn, SndID soundID)
    {
        return objectIn->m_moduleAccesser->getSoundModule()->playSE(soundID, 1, 1, 0);
    }
    bool isAttackingStatusKind(u32 statusKind)
    {
        return (statusKind >= Fighter::Status_Attack && statusKind <= Fighter::Status_Attack_Air) || (statusKind >= 0x112 && statusKind != 0x116);
    }
    bool isDamageStatusKind(u32 statusKind)
    {
        return (statusKind >= Fighter::Status_Damage) || (statusKind <= Fighter::Status_Damage_Fly_Roll);
    }
    void initDefaultHitboxData(soCollisionAttackData* attackDataIn)
    {
        attackDataIn->m_power = 0x0;
        attackDataIn->m_offsetPos.m_x = 0.0f;
        attackDataIn->m_offsetPos.m_y = 0.0f;
        attackDataIn->m_offsetPos.m_z = 0.0f;
        attackDataIn->m_size = 1.0f;
        attackDataIn->m_vector = 361;
        attackDataIn->m_reactionEffect = 0x00;
        attackDataIn->m_reactionFix = 0x00;
        attackDataIn->m_reactionAdd = 0x0;
        attackDataIn->m_slipChance = 0.0f;
        attackDataIn->m_hitStopFrame = 1.0f;
        attackDataIn->m_hitStopDelay = 1.0f;
        attackDataIn->m_nodeIndex = 0x00;
        attackDataIn->m_targetCategory = 0x3FF;
        attackDataIn->m_targetSituation = 0x3;
        attackDataIn->m_targetPart = 0xF;
        attackDataIn->m_attribute = soCollisionAttackData::Attribute_Normal;
        attackDataIn->m_soundLevel = soCollisionAttackData::Sound_Level_M;
        attackDataIn->m_soundAttribute = soCollisionAttackData::Sound_Attribute_Punch;
        attackDataIn->m_setOffKind = soCollisionAttackData::SetOff_Off;
        attackDataIn->m_noScale = 0;
        attackDataIn->m_isShieldable = 1;
        attackDataIn->m_isReflectable = 0;
        attackDataIn->m_isAbsorbable = 0;
        attackDataIn->m_subShield = 0;
        attackDataIn->field_0x34_9 = 0x00;
        attackDataIn->m_serialHitFrame = 0x00;
        attackDataIn->m_isDirect = 1;
        attackDataIn->m_isInvalidInvincible = 0;
        attackDataIn->m_isInvalidXlu = 0;
        attackDataIn->m_lrCheck = soCollisionAttackData::Lr_Check_Pos;
        attackDataIn->m_isCatch = 0;
        attackDataIn->m_noTeam = 0;
        attackDataIn->m_noHitStop = 0;
        attackDataIn->m_noEffect = 0;
        attackDataIn->m_noTransaction = 0;
        attackDataIn->m_region = soCollisionAttackData::Region_None;
        attackDataIn->m_shapeType = soCollision::Shape_Sphere;
        attackDataIn->m_isDeath100 = 0;
        attackDataIn->field_0x3c_2 = 0x0;
    }
}
