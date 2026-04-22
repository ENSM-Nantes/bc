#ifndef SHIP_PARAMS_H
#define SHIP_PARAMS_H

#include <iostream>
#include <vector>

struct sGeoParams{
  double lPP; //length  
  double b; //breadth
  double d; //draught
  double volume; //subwater volume
  double zG; //Altitudinal coordinate of center of gravity of ship
  double xG; //Longitudinal coordinate of center of gravity of ship
  double cB; //Coefficient Block
  double kM; //Distance between kheel to metacentric 
  double gM; //Distance between gravity center to metacentric
  double propSpacing; //length between 2 propeller
  double rudSpacing; //length between 2 rudder  
};

struct sAddedMassParams{
  double mpX; //Added masses of x axis direction and y axis direction, respectively
  double mpY;
  double jpZ; //Added moment of inertia
};

#endif
