#include <iostream>
#include "message.h"

Message::Message()
{
}

Message::~Message()
{    
}


int Message::Process(std::string& aMsg)
{ 
  //std::cout << "----------> " << aMsg << std::endl;

  if(aMsg.substr(0,2) == "BC" || aMsg.substr(0,2) == "SD")
                                return (E_MSG_TO_SLAVE | E_MSG_TO_MC);
  if(aMsg.substr(0,3) == "SCN") return (E_MSG_TO_SLAVE | E_MSG_TO_MASTER_MP | E_MSG_TO_MC);
  if(aMsg.substr(0,3) == "MPF") return E_MSG_TO_MH;
  if(aMsg.substr(0,2) == "MC")  return E_MSG_TO_MASTER;
  if(aMsg.substr(0,2) == "MH")  return E_MSG_TO_MASTER_MP;
  if(aMsg.substr(0,2) == "OS")  return E_MSG_TO_WI;
  if(aMsg.substr(0,2) == "WI")  return E_MSG_TO_MASTER | E_MSG_TO_MC;

  return E_MSG_TO_UNKNOW_HOST;
}
