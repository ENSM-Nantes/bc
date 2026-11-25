#ifndef CANBUS_HPP
#define CANBUS_HPP

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

class CanBus
{
public:

  CanBus();
  ~CanBus();

  bool Open(const std::string& aInterface);
  void Close(void);
  bool IsOpen(void) const;

  bool SetFilters(const std::vector<std::pair<uint32_t, uint32_t>>& aIdMasks);
  bool Send(uint32_t aId, const std::vector<uint8_t>& aData, bool aExtended = false);
  bool Receive(uint32_t& aId, std::vector<uint8_t>& aData, bool& aExtended);

private:

  int mSocket;
  std::string mInterface;
};

#endif
