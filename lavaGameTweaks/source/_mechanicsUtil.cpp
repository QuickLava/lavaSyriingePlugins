#include "_mechanicsUtil.h"

namespace mechUtil
{
    Vec3f gfxFaceScreenRotVec = Vec3f(0.0f, -90.0f * radianConvConstant, 0.0f);
    Vec3f zeroVec = Vec3f(0.0f, 0.0f, 0.0f );
    Vec3f gfxFlattenSclVec = Vec3f(2.0f, 2.0f, 0.1f );

    int doMeterGain(Fighter* fighterIn, float meterIn, EfID meterGainGraphic, float graphicScale, meterGainAnnouncerCond announcerClipCond)
    {
        int result = 0;

        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount)
        {
            soModuleEnumeration* moduleEnum = fighterIn->m_moduleAccesser->m_enumerationStart;
            fighterMeters::meterBundle* targetMeterBundle = fighterMeters::playerMeters + fighterPlayerNo;

            result = targetMeterBundle->addMeter(meterIn);

            bool doAnnouncerClip = 0;
            if (result != 0)
            {
                u32 finalStockCount = targetMeterBundle->getMeterStocks();
                doAnnouncerClip = (result > 0) ? announcerClipCond & mgac_ON_STOCK_GAIN : announcerClipCond & mgac_ON_STOCK_LOSS;
                if (doAnnouncerClip && finalStockCount <= 10)
                {
                    mechUtil::playSE(fighterIn, (SndID)((snd_se_narration_one + 1) - finalStockCount));
                }
                mechUtil::reqCenteredGraphic(fighterIn, meterGainGraphic, graphicScale, 1);
            }
        }

        return result;
    }

    float getDistanceBetween(StageObject* obj1, StageObject* obj2, bool usePrevPos)
    {
        register float result;

        register soPostureModule* postureModule1 = obj1->m_moduleAccesser->m_enumerationStart->m_postureModule;
        register soPostureModule* postureModule2 = obj2->m_moduleAccesser->m_enumerationStart->m_postureModule;
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

        soEffectModule* effectModule = objectIn->m_moduleAccesser->m_enumerationStart->m_effectModule;
        if (!follow)
        {
            result = effectModule->req(effectID, gfxRootBoneID, &zeroVec, &zeroVec, scale, &zeroVec, &zeroVec, 0, 0);
        }
        else
        {
            result = effectModule->reqFollow(effectID, gfxRootBoneID, &zeroVec, &zeroVec, scale, 0, 0, 0, 0);
        }

        return result;
    }
    u32 playSE(StageObject* objectIn, SndID soundID)
    {
        return objectIn->m_moduleAccesser->getSoundModule().playSE(soundID, 1, 1, 0);
    }
    bool isAttackingStatusKind(u32 statusKind)
    {
        return (statusKind >= Fighter::Status_Attack && statusKind <= Fighter::Status_Attack_Air) || (statusKind >= 0x112 && statusKind != 0x116);
    }
    bool isDamageStatusKind(u32 statusKind)
    {
        return (statusKind >= Fighter::Status_Damage && statusKind <= Fighter::Status_Damage_Fly_Roll);
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
    float currAnimProgress(StageObject* objectIn)
    {
        float result = 0.0f;

        soMotionModule* motionModule = objectIn->m_moduleAccesser->m_enumerationStart->m_motionModule;
        if (motionModule != NULL && motionModule->getKind() > 0)
        {
            result = motionModule->getFrame() / motionModule->getEndFrame();
        }

        return result;
    }
}
