#pragma once
#include <memory>
#include "tgaimage.h"
#include "geometry.h"
#include "texture.h"

//preprocessing
void foreach_irradiance_map();
void foreach_prefilter_miplevel();
void BRDF_LUT();
