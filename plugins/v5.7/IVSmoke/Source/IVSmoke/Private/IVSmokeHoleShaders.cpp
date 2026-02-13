// Copyright (c) 2026, Team SDB. All rights reserved.

#include "IVSmokeHoleShaders.h"

IMPLEMENT_GLOBAL_SHADER(FIVSmokeHoleCarveCS, "/Plugin/IVSmoke/IVSmokeHoleCarveCS.usf", "MainCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FIVSmokeHoleBlurCS, "/Plugin/IVSmoke/IVSmokeHoleBlurCS.usf", "MainCS", SF_Compute);
