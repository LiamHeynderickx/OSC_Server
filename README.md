# OSC_Server
This repository holds a TCP server process that accepts incoming network connections from multiple client devices that upload sensor data with room temperatures. 
The sensor data is then processed and mapped to sensor locations, then stored in a .csv file. 
All programming is done in C and makefile targets are used to build the project.
This server process is the final assignment for the course: Operating Systems at Group T, KU Leuven University.
See TCP_SensorMonitoringSystem/project for the final server implimentation.
The other file paths contain intermediary steps and other related code.

To run any of this code make use of the makefiles and .sh code provided. The final project implimentation can also be run using the linux termainal.
