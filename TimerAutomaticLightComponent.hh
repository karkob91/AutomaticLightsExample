#pragma once

#include "TimerAutomaticLight.hh"

class Loop;

class TimerAutomaticLightComponent : public skel::TimerAutomaticLightComponent
{
public:
    TimerAutomaticLightComponent(const dzn::locator& locator);

private:
    void api_Create() override;
    void api_Cancel() override;
};

