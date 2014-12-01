How to run/compile: import project into visual studio and compile

List of files modified/added:

Implementation for Shield Battery Strategy
BatteryStratManager.h
BatteryStratManager.cpp

Fixed behavior to lifted terran buildings
CombatCommander.cpp

Added support to choose which managers to use based on strategies
GameCommander.cpp
GameCommander.h

Added rough approximation to estimating enemy force with bunkers for sparcraft to analyze
InformationManager.cpp

Added logic to ignore getting enemy units from an area if they are a lifted building
MapGrid.cpp

Minor edits, options for rush size with probe rush strategy
Options.cpp

Implementation for Probe Rush Strategy
ScoutRushManager.cpp
ScoutRushManager.h

Added logic so if using air rush strategy do not regroup
Squad.cpp
Squad.h

Implementation of our 3 new strategies(Probe Rush, Shield Battery, Air Rush)
StrategyManager.cpp
StrategyManager.h

Implementation of Air Rush Strategy
AirManager.cpp
AirManager.h

Fix logic to enemy workers in our area to have combat units attack them
MicroManager.cpp

Building management implementation for Air Rush Strategy
BuildingManager.cpp
BuildingManager.h
