#ifndef COM_H
#define COM_H

#include <iostream>
#include <vector>
#include "enet/enet.h"
#include "miscstatus.h"
#include "comstatus.h"

#define MAX_RETRY_COUNTER (100)
#define MAX_CLIENT_CONNEXION (8)

typedef enum{
  MASTER=0x0A,
  SLAVE,
  MASTER_MP,
  MULTIHUB,
  MAP_CTRL,
  WIND_INJ,
  UNKNOWN
}eTarget;

class Com
{
 public:

  Com();
  Com(std::string aAddr, unsigned short aPort);
  ~Com();

  int InitCom(void);
  int WaitEvent(unsigned short aTimeout);
  
  eServState GetState(void);
  void SetState(eServState aState);
  
 private:

  int ClientConnect(ENetPeer** aPeer, unsigned int aData);
  int ClientDisconnect(ENetPeer** aPeer);
  int ClientMsg(const char *aData, size_t aDataSize);  
  void SendMsg(eTarget aTarget);
  
  /*Server*/
  ENetAddress mAddrServ;
  ENetHost* mServer;
  ENetEvent mEvent;
  ComStatus mStatus;

  /*Client*/
  ENetPacket* mPacket;
  ENetPeer* mPeerClient[MAX_CLIENT_CONNEXION];
  unsigned int mTypeClient[MAX_CLIENT_CONNEXION];
  unsigned char mClientCounter;

  
};

#endif
