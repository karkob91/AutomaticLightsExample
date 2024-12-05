#pragma once

#include "UpdateLoopSkel.hh"
#include <optional> // Required for std::optional


class Loop;

class UpdateLoopComponent : public skel::UpdateLoopComponent
{
public:
    UpdateLoopComponent(dzn::locator const& locator);

private:
    void api_Enable() override;
    void api_Disable() override;

private:
    Loop& m_loop;
    std::optional<size_t> m_id;
};
