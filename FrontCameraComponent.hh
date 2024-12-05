#pragma once

#include "FrontCamera.hh"

struct FrontCameraComponent : skel::FrontCameraComponent
{
    public:
        FrontCameraComponent(dzn::locator const& locator);
};
