#pragma once

#include "SampleData.h"

/** Synthesised source sounds so the instrument makes sound before any sample is dropped in. */
namespace BuiltinSources
{
    juce::StringArray names();

    /** name is one of names(); returns nullptr if unknown. */
    SamplePtr create (const juce::String& name);

    inline const juce::String prefix { "builtin:" };
}
