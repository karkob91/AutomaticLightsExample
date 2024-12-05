#include "AutomaticLights.hh"
#include <memory>
#include <iostream>

// Declare the unique pointer for HelloWorldSystem
std::unique_ptr<ModeSelector> modeSelector;

int main()
{
    try
    {
        dzn::runtime runtime;
        dzn::locator locator; // Create the locator instance

        // Set the runtime for the locator
        locator.set(runtime);

        // Create the HelloWorldSystem instance
        modeSelector = std::make_unique<ModeSelector>(locator);
        if (!modeSelector) {
            std::cerr << "Failed to create HelloWorldSystem." << std::endl;
            return 1;
        }

        // Here, you can add more code to interact with helloWorldSystem if needed

        std::cout << "HelloWorldSystem successfully created." << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
        return 1;
    }

    // helloWorldSystem->hello.in.World();


    return 0;
}
