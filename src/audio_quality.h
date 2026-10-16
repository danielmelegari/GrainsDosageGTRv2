#pragma once
#include <algorithm>
namespace qg {
// Audio always runs at the host rate; these are CPU budgets, not resampling modes.
enum class AudioQuality { Draft=0, Normal=1, High=2 };
inline AudioQuality audioQuality(int value){return AudioQuality(std::clamp(value,0,2));}
inline int qualityVoices(AudioQuality q){return q==AudioQuality::Draft?16:q==AudioQuality::Normal?64:128;}
inline int qualityCoefficientStride(AudioQuality q){return q==AudioQuality::Draft?32:q==AudioQuality::Normal?8:1;}
}
