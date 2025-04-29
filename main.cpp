#include "AutomaticLights.hh"
#include "Loop.hh"
#include <memory>
#include <iostream>
#include <chrono> // For time measurement
#include "ArrowheadManager.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <boost/beast/http.hpp>
// #include <windows.h>

// Declare the unique pointer for HelloWorldSystem
std::unique_ptr<AutomaticLights> autoLightsSystem;

int main()
{
    std::cout << "==================================================================" << std::endl;
    std::cout << "Arrowhead Framework Manager with Automatic Lights System" << std::endl;
    std::cout << "==================================================================" << std::endl;
    
    // Initialize the Dezyne components
    dzn::runtime runtime;
    dzn::locator locator;
    Loop updateLoop;
    
    // Setup runtime and updateLoop in locator
    locator.set(runtime);
    locator.set(updateLoop);
    
    // Create the AutoLights component instance
    autoLightsSystem = std::make_unique<AutomaticLights>(locator);
    std::cout << "Dezyne component successfully created." << std::endl;
    
    bool HighBeamsOn = false;
    
    // Setup relay callbacks
    autoLightsSystem->relay.in.TurnOn = [&HighBeamsOn]() {
        HighBeamsOn = true;
        std::cout << "HighBeamsOn set to true." << std::endl;
    };
    
    autoLightsSystem->relay.in.TurnOff = [&HighBeamsOn]() {
        HighBeamsOn = false;
        std::cout << "HighBeamsOn set to false." << std::endl;
    };
    
    // Initialize the automatic lights system
    ::Result res = autoLightsSystem->module.in.Initialize();
    
    if (res == ::Result::Ok) {
        std::cout << "Automatic Lights System Initialized." << std::endl;
    }
    
    // Load Arrowhead configuration from JSON file
    arrowhead::ArrowheadConfig config;
    if (!config.loadFromFile("arrowhead_config.json")) {
        std::cout << "Failed to load configuration file. Using default configuration." << std::endl;
    }
    
    // Create ArrowheadManager instance with the configuration
    arrowhead::ArrowheadManager manager(config);
    
    // Custom handler for the /api_armed endpoint
    manager.on("/api_armed", "GET", [](const http::request<http::string_body>& req, http::response<http::string_body>& res) {
        // Create JSON response
        std::string jsonResponse = R"({
            "status": "ARMED",
            "timestamp": ")" + std::to_string(std::time(nullptr)) + R"(",
            "message": "System is armed and ready"
        })";
        
        res.result(http::status::ok);
        res.set(http::field::content_type, "application/json");
        res.body() = jsonResponse;
        res.prepare_payload();
        
        std::cout << "Handled /api_disarmed request" << std::endl;
    });
    
    // Custom handler for the /console_services endpoint
    manager.on("/console_services", "GET", [](const http::request<http::string_body>& req, http::response<http::string_body>& res) {
        // Create JSON response
        std::string jsonResponse = R"({
            "action": "SHOW_SERVICES",
            "timestamp": ")" + std::to_string(std::time(nullptr)) + R"(",
            "duration": 10
        })";
        
        res.result(http::status::ok);
        res.set(http::field::content_type, "application/json");
        res.body() = jsonResponse;
        res.prepare_payload();
        
        std::cout << "Handled /console_services request" << std::endl;
    });
    
    // Start the HTTP server
    std::cout << "Starting HTTP server on port " << config.getSystem().port << "..." << std::endl;
    if (!manager.startServer()) {
        std::cerr << "Failed to start HTTP server. Exiting." << std::endl;
        return 1;
    }
    bool ahReady = false;

    
    

    while (true) {
        // Update the Dezyne component loop
        updateLoop.Update();
        
        // Try to register with Arrowhead
        if (manager.registerSystemAndServices()) {
            if (!ahReady) {
                std::cout << "Successfully registered with Arrowhead!" << std::endl;
                ahReady = true;
            }

        } else if (ahReady) {
            ahReady = false;
            std::cout << "Lost connection to Arrowhead, attempting to reconnect..." << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    // Stop the server when done
    std::cout << "\nStopping HTTP server..." << std::endl;
    manager.stopServer();

    return 0;
}
