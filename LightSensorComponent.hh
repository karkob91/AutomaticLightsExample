#pragma once

#include "LightSensor.hh"

struct LightSensorComponent : skel::LightSensorComponent
{
    public:
        LightSensorComponent(dzn::locator const& locator);
};

