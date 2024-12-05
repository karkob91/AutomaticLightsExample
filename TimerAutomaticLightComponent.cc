#include "TimerAutomaticLightComponent.hh"


TimerAutomaticLightComponent::TimerAutomaticLightComponent(const dzn::locator& locator) :
    skel::TimerAutomaticLightComponent(locator)

{
}

void TimerAutomaticLightComponent::api_Create()
{
    printf("Create Timer\n");
    // if (!m_id)
    // {
    //     m_id = m_loop.Register([this] { Update(); });
    // }

    // m_due = m_loop.Millis() + ms;
    // m_loop.Enable(*m_id);
}

void TimerAutomaticLightComponent::api_Cancel()
{
    printf("Cancel Timer/n");
//     if (m_id)
//     {
//         m_loop.Disable(*m_id);
//     }
}

