#include <iostream>
#include "ThrusterSerial.hpp"

ThrusterSerial::ThrusterSerial()
{

}

ThrusterSerial::~ThrusterSerial()
{
  if (mSerialPort.isOpen())
    {
      try
	{
	  mSerialPort.close();
	}
      catch (std::exception const& e)
	{
	}
    }
}

void ThrusterSerial::Init(std::string aComPort, unsigned int aBaudrate)
{
  if (aComPort.empty() || 0 == aBaudrate)
    return;

  try
    {
      serial::Timeout timeout = serial::Timeout::simpleTimeout(50);

      mSerialPort.setPort(aComPort);
      mSerialPort.setBaudrate(aBaudrate);
      mSerialPort.setTimeout(timeout);

      mSerialPort.open();
      std::cout << "ThrusterSerial: Serial port opened." << std::endl;
    }
  catch (std::exception const& e)
    {
      std::cout << "ThrusterSerial::Error : " << e.what() << std::endl;
    }
}

void ThrusterSerial::Send(bool aHasBowThruster, bool aHasSternThruster)
{
  if (!mSerialPort.isOpen())
    return;

  std::string message = "THRUSTER,BOW=" + std::string(aHasBowThruster ? "1" : "0") +
    ",STERN=" + std::string(aHasSternThruster ? "1" : "0") + "\r\n";

  try
    {
      mSerialPort.write(message);
    }
  catch (std::exception const& e)
    {
      std::cout << "ThrusterSerial::Error : " << e.what() << std::endl;
    }
}
