#include "Loop.hh"
#include <iostream>
#include <chrono>

// Constructor
Loop::Loop() : m_millis(0), m_startTime(std::chrono::steady_clock::now())
{
}

// Constructor with capacity
Loop::Loop(size_t capacity) : m_startTime(std::chrono::steady_clock::now())
{
    m_updateFunctions.reserve(capacity);
}

// Register a new function
size_t Loop::Register(Fn update)
{
    auto id = m_updateFunctions.size();
    m_updateFunctions.push_back({false, std::move(update)});
    return id;
}

// Enable a function by ID
void Loop::Enable(size_t id)
{
    if (id < m_updateFunctions.size()) {
        m_updateFunctions[id].enabled = true;
    }
}

// Disable a function by ID
void Loop::Disable(size_t id)
{
    if (id < m_updateFunctions.size()) {
        m_updateFunctions[id].enabled = false;
    }
}

// Simulate Arduino's millis() using steady clock
unsigned long Loop::Millis() const
{
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - m_startTime).count();
}

// Update loop
void Loop::Update()
{
    m_millis = Millis();
    for (size_t i = 0; i < m_updateFunctions.size(); ++i)
    {
        const auto& updateFunction = m_updateFunctions[i];
        if (updateFunction.enabled) {
            updateFunction.update();
        }
    }
}
