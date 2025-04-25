#include "AutomaticLights.hh"
#include "Loop.hh"
#include <memory>
#include <iostream>
#include <chrono> // For time measurement
#include "ProviderExample/src/Provider/SensorHandler.h"
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

    bool bSecureProviderInterface = false; //Enables HTTPS interface on the application service (with token enabled)
    bool bSecureArrowheadInterface = false;

    SensorHandler oSensorHandler;

//SenML format
//todo:
//generate own measured value into "measuredValue"
//"value" should be periodically updated
//"sLinuxEpoch" should be periodically updated

     std::string measuredValue; //JSON - SENML format
     time_t linuxEpochTime = std::time(0);
     std::string sLinuxEpoch = std::to_string(static_cast<uint64_t>(linuxEpochTime));

     double value = 26.0;
//convert double to string
     std::ostringstream streamObj;
     streamObj << std::fixed;
     streamObj << std::setprecision(1);
     streamObj << value;
     std::string sValue = streamObj.str();

     measuredValue =
          "{"
               "\"e\":[{"
                    "\"n\": \"this_is_the_sensor_id\","
                    "\"v\":" + sValue +","
                    "\"t\": \"" + sLinuxEpoch + "\""
                    "}],"
               "\"bn\": \"this_is_the_sensor_id\","
               "\"bu\": \"Celsius\""
          "}";

//do not modify below this

     oSensorHandler.processProvider(measuredValue, bSecureProviderInterface, bSecureArrowheadInterface);

    // autoLightsSystem->lightSensor.out.LowLight();


    while (true) {
        // Update register loop component to for example - check if timer has elapsed
        updateLoop.Update();

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}
