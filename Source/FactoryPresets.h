#pragma once

#include <utility>
#include <vector>

/** Values are in real parameter units (ms, Hz, semitones, choice index, 0/1 for switches). */
struct FactoryPreset
{
    const char* name;
    const char* category;
    const char* builtinSource; // used when no user sample is loaded
    std::vector<std::pair<const char*, float>> values;
};

const std::vector<FactoryPreset>& getFactoryPresets();
