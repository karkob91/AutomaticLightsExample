#pragma once

#include "FrontCameraSkel.hh"

struct FrontCameraComponent : skel::FrontCameraComponent
{
    public:
        FrontCameraComponent(dzn::locator const& locator);
};
