#ifndef VMICORE_MOCK_PAGEPROTECTION_H
#define VMICORE_MOCK_PAGEPROTECTION_H

#include <gmock/gmock.h>
#include <vmicore/os/IPageProtection.h>

class MockPageProtection : public IPageProtection
{
  public:
    MOCK_METHOD(ProtectionValues, get, (), (const override));

    MOCK_METHOD(uint64_t, getRaw, (), (const override));

    MOCK_METHOD(std::string, toString, (), (const override));
};

#endif // VMICORE_MOCK_PAGEPROTECTION_H
