#include <iostream>
#include "CanBus.hpp"

#ifdef __linux__
#include <cerrno>
#include <cstring>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#endif

CanBus::CanBus()
{
  mSocket = -1;
}

CanBus::~CanBus()
{
  Close();
}

#ifdef __linux__

bool CanBus::Open(const std::string& aInterface)
{
  Close();

  if (aInterface.empty())
    return false;

  if (aInterface.size() >= IFNAMSIZ)
    {
      std::cout << "CanBus::Error : interface name too long : " << aInterface << std::endl;
      return false;
    }

  int sock = socket(PF_CAN, SOCK_RAW | SOCK_NONBLOCK, CAN_RAW);
  if (sock < 0)
    {
      std::cout << "CanBus::Error : socket : " << strerror(errno) << std::endl;
      return false;
    }

  struct ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));
  strncpy(ifr.ifr_name, aInterface.c_str(), IFNAMSIZ - 1);
  if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0)
    {
      std::cout << "CanBus::Error : interface " << aInterface << " : " << strerror(errno) << std::endl;
      close(sock);
      return false;
    }

  struct sockaddr_can addr;
  memset(&addr, 0, sizeof(addr));
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;
  if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
      std::cout << "CanBus::Error : bind " << aInterface << " : " << strerror(errno) << std::endl;
      close(sock);
      return false;
    }

  mSocket = sock;
  mInterface = aInterface;
  std::cout << "CanBus: " << mInterface << " opened." << std::endl;
  return true;
}

void CanBus::Close(void)
{
  if (mSocket >= 0)
    {
      close(mSocket);
      mSocket = -1;
      std::cout << "CanBus: " << mInterface << " closed." << std::endl;
    }
}

bool CanBus::IsOpen(void) const
{
  return mSocket >= 0;
}

bool CanBus::SetFilters(const std::vector<std::pair<uint32_t, uint32_t>>& aIdMasks)
{
  if (!IsOpen())
    return false;

  std::vector<struct can_filter> filters;
  for (const auto& idMask : aIdMasks)
    {
      struct can_filter filter;
      filter.can_id = idMask.first;
      filter.can_mask = idMask.second;
      filters.push_back(filter);
    }

  const void* filterData = filters.empty() ? nullptr : filters.data();
  socklen_t filterSize = filters.size() * sizeof(struct can_filter);
  if (setsockopt(mSocket, SOL_CAN_RAW, CAN_RAW_FILTER, filterData, filterSize) < 0)
    {
      std::cout << "CanBus::Error : filters : " << strerror(errno) << std::endl;
      return false;
    }

  return true;
}

bool CanBus::Send(uint32_t aId, const std::vector<uint8_t>& aData, bool aExtended)
{
  if (!IsOpen())
    return false;

  if (aData.size() > CAN_MAX_DLEN)
    {
      std::cout << "CanBus::Error : frame too long (" << aData.size() << " bytes)" << std::endl;
      return false;
    }

  struct can_frame frame;
  memset(&frame, 0, sizeof(frame));
  if (aExtended)
    frame.can_id = (aId & CAN_EFF_MASK) | CAN_EFF_FLAG;
  else
    frame.can_id = aId & CAN_SFF_MASK;
  frame.can_dlc = aData.size();
  memcpy(frame.data, aData.data(), aData.size());

  ssize_t nbytes = write(mSocket, &frame, sizeof(frame));
  if (nbytes != sizeof(frame))
    {
      if (errno != EAGAIN && errno != ENOBUFS)
	std::cout << "CanBus::Error : send : " << strerror(errno) << std::endl;
      return false;
    }

  return true;
}

bool CanBus::Receive(uint32_t& aId, std::vector<uint8_t>& aData, bool& aExtended)
{
  if (!IsOpen())
    return false;

  struct can_frame frame;
  while (true)
    {
      ssize_t nbytes = read(mSocket, &frame, sizeof(frame));
      if (nbytes < 0)
	{
	  if (errno != EAGAIN && errno != EWOULDBLOCK)
	    std::cout << "CanBus::Error : receive : " << strerror(errno) << std::endl;
	  return false;
	}

      if (nbytes != sizeof(frame))
	continue;

      if (frame.can_id & (CAN_ERR_FLAG | CAN_RTR_FLAG))
	continue;

      aExtended = (frame.can_id & CAN_EFF_FLAG) != 0;
      aId = aExtended ? (frame.can_id & CAN_EFF_MASK) : (frame.can_id & CAN_SFF_MASK);
      unsigned int length = frame.can_dlc > CAN_MAX_DLEN ? CAN_MAX_DLEN : frame.can_dlc;
      aData.assign(frame.data, frame.data + length);
      return true;
    }
}

#else

bool CanBus::Open(const std::string& aInterface)
{
  if (!aInterface.empty())
    std::cout << "CanBus::Error : CAN bus is only supported on Linux." << std::endl;
  return false;
}

void CanBus::Close(void)
{
}

bool CanBus::IsOpen(void) const
{
  return false;
}

bool CanBus::SetFilters(const std::vector<std::pair<uint32_t, uint32_t>>& aIdMasks)
{
  return false;
}

bool CanBus::Send(uint32_t aId, const std::vector<uint8_t>& aData, bool aExtended)
{
  return false;
}

bool CanBus::Receive(uint32_t& aId, std::vector<uint8_t>& aData, bool& aExtended)
{
  return false;
}

#endif
