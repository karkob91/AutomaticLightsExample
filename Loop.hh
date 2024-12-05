#pragma once
#include <vector>
#include <functional>
#include <chrono>

class Loop
{
public:
    using Fn = std::function<void()>;

    Loop();
    explicit Loop(size_t capacity);

    size_t Register(Fn update);
    void Enable(size_t id);
    void Disable(size_t id);

    unsigned long Millis() const;

    void Update();

private:
    struct UpdateFunction {
        bool enabled;
        Fn update;
    };

    unsigned long m_millis;
    std::vector<UpdateFunction> m_updateFunctions;

    // Start time for simulating millis()
    std::chrono::steady_clock::time_point m_startTime;
};
