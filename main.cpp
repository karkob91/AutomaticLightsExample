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
    // locator and runtime for operating Dezyne component
    dzn::runtime runtime;
    dzn::locator locator;
    // loop component to register frequently updated components like Timer
    Loop updateLoop;

    // Setup runtime and updateLoop in locator so those elements are available
    // for hand-written code in the foreign components (like TimerComponent.hh)
    locator.set(runtime);
    locator.set(updateLoop);

    // Create the AutoLights component instance
    autoLightsSystem = std::make_unique<AutomaticLights>(locator);
    std::cout << "Dezyne component successfully created." << std::endl;

    bool HighBeamsOn = false;

    // Capture `HighBeamsOn` by reference in the lambda
    autoLightsSystem->relay.in.TurnOn = [&HighBeamsOn]() {
        HighBeamsOn = true;
        std::cout << "HighBeamsOn set to true." << std::endl;
    };

    // Capture `HighBeamsOn` by reference in the lambda
    autoLightsSystem->relay.in.TurnOff = [&HighBeamsOn]() {
        HighBeamsOn = false;
        std::cout << "HighBeamsOn set to false." << std::endl;
    };

    // Call initialize trigger to read config for light timer
    ::Result res = autoLightsSystem->module.in.Initialize();

    if (res == ::Result::Ok) {
        std::cout << "Automatic Lights System Initialized." << std::endl;
    }

    // DEZYNE 

    namespace http = boost::beast::http;

    std::cout << "==================================================================" << std::endl;
    std::cout << "Arrowhead Framework Manager with HTTP Server Example" << std::endl;
    std::cout << "==================================================================" << std::endl;
    
    // Define system information
    const std::string SYSTEM_NAME = "ExampleSystem";
    const std::string SYSTEM_ADDRESS = "127.0.0.1";  // Change to your actual IP address
    const int SYSTEM_PORT = 8080;
    const std::string MAC_ADDRESS = "00:11:22:33:44:55";  // Example MAC address
    
    // Define services provided by this system
    std::vector<std::pair<std::string, std::string>> providedServices = {
        {"api-armed", "/api_armed"},
        {"api-disarmed", "/api_disarmed"},
        {"api-arming", "/api_arming"},
        {"api-detected", "/api_detected"},
        {"console-services", "/console_services"}
    };
    
    // Define services consumed by this system
    std::vector<std::string> consumedServices = {
        "api-arm"
    };
    
    // Create ArrowheadManager instance
    ArrowheadManager manager(
        SYSTEM_NAME,
        SYSTEM_ADDRESS,
        SYSTEM_PORT,
        MAC_ADDRESS,
        providedServices,
        consumedServices
    );
    
    // Set Arrowhead server address (change to match your Arrowhead deployment)
    const std::string ARROWHEAD_ADDRESS = "172.25.164.36";
    const int SERVICE_REGISTRY_PORT = 8443;
    const int SYSTEM_REGISTRY_PORT = 8437;
    
    std::cout << "Setting up Arrowhead connection..." << std::endl;
    manager.setArrowheadServiceRegistry(ARROWHEAD_ADDRESS, SERVICE_REGISTRY_PORT);
    manager.setArrowheadSystemRegistry(ARROWHEAD_ADDRESS, SYSTEM_REGISTRY_PORT);
    
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
        
        std::cout << "Handled /api_armed request" << std::endl;
    });
    
    // Custom handler for the /api_disarmed endpoint
    manager.on("/api_disarmed", "GET", [](const http::request<http::string_body>& req, http::response<http::string_body>& res) {
        // Create JSON response
        std::string jsonResponse = R"({
            "status": "DISARMED",
            "timestamp": ")" + std::to_string(std::time(nullptr)) + R"(",
            "message": "System is disarmed"
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
    std::cout << "Starting HTTP server on port " << SYSTEM_PORT << "..." << std::endl;
    if (!manager.startServer()) {
        std::cerr << "Failed to start HTTP server. Exiting." << std::endl;
        return 1;
    }
    
    std::cout << "HTTP server started successfully." << std::endl;
    
    // Register with Arrowhead
    std::cout << "\nRegistering system and services with Arrowhead..." << std::endl;
    bool ahReady = false;
    
    // Main loop - similar to your ESP32 implementation
    for (int i = 0; i < 30; i++) { // Run for 30 iterations for this example
        // Try to register with Arrowhead
        if (manager.registerSystemAndServices()) {
            if (!ahReady) {
                std::cout << "Successfully registered with Arrowhead!" << std::endl;
                ahReady = true;
            }
            
            // If registered, we could handle button presses or other inputs here
            // For this example, we'll just simulate some service consumption
            if (i % 5 == 0) { // Every 5 seconds, consume a service
                std::cout << "\nConsuming 'api-arm' service with PIN 1234..." << std::endl;
                bool result = manager.consumeService("api-arm", "pin", 1234);
                std::cout << "Result: " << (result ? "Valid PIN" : "Invalid PIN") << std::endl;
            }
        } else if (ahReady) {
            ahReady = false;
            std::cout << "Lost connection to Arrowhead, attempting to reconnect..." << std::endl;
        }
        
        // Sleep for a second before next iteration
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Print a status message for the example
        std::cout << "Server running... (iteration " << i + 1 << " of 30)" << std::endl;
    }
    
    // Stop the server when done
    std::cout << "\nStopping HTTP server..." << std::endl;
    manager.stopServer();

    while (true) {
        // Update register loop component to for example - check if timer has elapsed
        updateLoop.Update();

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}
