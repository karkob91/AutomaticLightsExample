#include "UpdateLoopComponent.hh"
#include "Loop.hh"

UpdateLoopComponent::UpdateLoopComponent(dzn::locator const& locator) :
    skel::UpdateLoopComponent(locator),
    m_loop(locator.get<Loop>())
{
}

void UpdateLoopComponent::api_Enable()
{
    if (!m_id)
    {
        m_id = m_loop.Register([this] { api_Update(); });
    }

    m_loop.Enable(*m_id);
}

void UpdateLoopComponent::api_Disable()
{
    if (m_id)
    {
        m_loop.Disable(*m_id);
    }
}
