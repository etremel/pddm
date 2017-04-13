# Private, Distributed Data Mining (PDDM)

This is the code repository for the Private, Distributed Data Mining project. It is a tool for querying sensitive data that resides in a large network of client devices, without collecting the data into a central location or revealing individual contributions. It can also be run as a simulation, in which a single computer simulates the entire network of devices running the protocol, in order to test variations of the protocol under different scenarios (including crash failures, which can be simulated).

## Configuring

To switch the code between "simulation" and "real life" mode, edit Configuration.h and ConfigurationIncludes.h to change the compile-time mapping of the types `NetworkClient_t`, `Meter_t`, `TimerManager_t`, and `CrytoLibrary_t` from simulated to non-simulated implementations. (Configuration.h changes the type mapping, and ConfigurationIncludes.h changes which implementation files are included). Changing the type mapping in these configuration files is also the way to change the protocol the system uses, between basic crash-tolerant, Byzantine-fault-tolrant, and high-failure-rate tolerant. 

## Building

This project requires a compiler with C++14 support, such as Clang++ 3.8 or GCC 5.4. The provided Makefile assumes a Linux system (with tools such as find) and depends on libpthread and librt being installed. There are several build targets that choose different sets of headers to include, depending on whether the executable being compiled expects the code to be configured in "simulation" mode or "real life" mode. (Thus, building "AllSimulated" will not attempt to build or include the TCP networking files). 
