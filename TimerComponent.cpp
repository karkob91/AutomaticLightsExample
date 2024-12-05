#include "TimerComponent.hh"
#include "Loop.hh"

TimerComponent::TimerComponent(const dzn::locator& locator) :
    skel::TimerComponent(locator),
    m_loop(locator.get<Loop>())
{
}

void TimerComponent::api_Create(int ms)
{
    if (!m_id)
    {
        m_id = m_loop.Register([this] { Update(); });
    }

    m_due = m_loop.Millis() + ms;
    m_loop.Enable(*m_id);
}

void TimerComponent::api_Cancel()
{
    if (m_id)
    {
        m_loop.Disable(*m_id);
    }
}

void TimerComponent::Update()
{
    if (m_due <= m_loop.Millis())
    {
        m_loop.Disable(*m_id);
        api_Timeout();
    }
}
