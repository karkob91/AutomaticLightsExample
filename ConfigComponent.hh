#pragma once

#include "ConfigSkel.hh"

// Inherit generated skell class (component without behavior)
struct ConfigComponent : skel::ConfigComponent
{
    public:
        ConfigComponent(dzn::locator const& locator);

    private:
        // API methods
        ::Result api_GetLightDelay(int& ms) override;
};
