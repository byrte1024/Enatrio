#pragma once

#include "../cts/Class.h"

#define TYPE Exploder

BEGIN_CLASS(0x22AB);

DECLARE_MID(ShimmiShimmiYea);

MESSAGE_HANDLER_BEGIN(ShimmiShimmiYea)

    MH_ExtractDeref(Strength, float);
    LOG_INFO("Strength: %f", Strength);

MESSAGE_HANDLER_END()

CAN_RECEIVE_BEGIN()
    CAN_RECEIVE_MID(ShimmiShimmiYea)
CAN_RECEIVE_END()

RECEIVE_MESSAGE_BEGIN()
    RECEIVE_MESSAGE_ROUTE(ShimmiShimmiYea)
RECEIVE_MESSAGE_END()

CLASSDEF()

#undef TYPE
