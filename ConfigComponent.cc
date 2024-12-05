#include "ConfigComponent.hh"

ConfigComponent::ConfigComponent(dzn::locator const& locator) :
    skel::ConfigComponent(locator)
{
}

::Result ConfigComponent::api_GetLightDelay(int& ms) {
    ms = 3000;
    return Result::Ok;
}