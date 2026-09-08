#ifndef THRUSTERSERIAL_HPP
#define THRUSTERSERIAL_HPP

#include <string>
#include "../lib/serial/serial.h"

class ThrusterSerial
{
public:

  ThrusterSerial();
  ~ThrusterSerial();

  void Init(std::string aComPort, unsigned int aBaudrate);
  void Send(bool aHasBowThruster, bool aHasSternThruster);

private:

  serial::Serial mSerialPort;
};

#endif
