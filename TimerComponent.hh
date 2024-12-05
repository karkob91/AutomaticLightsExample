#pragma once

#include "TimerSkel.hh"
#include <optional> // For std::optional

class Loop;

class TimerComponent : public skel::TimerComponent
{
public:
    explicit TimerComponent(const dzn::locator& locator);

private:
    // API methods
    void api_Create(int ms) override;
    void api_Cancel() override;

    // Internal update function
    void Update();

private:
    Loop& m_loop;                    // Reference to the Loop instance
    std::optional<size_t> m_id;      // Optional ID for the timer task
    unsigned long m_due;             // Due time in milliseconds
};
