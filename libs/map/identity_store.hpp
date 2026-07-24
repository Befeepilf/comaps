#pragma once

#include <string>
#include <string_view>

class IdentityStore
{
public:
  static std::string GetOrCreateDeviceId();

  static bool HasUsername();
  static std::string GetUsername();
  static bool SetUsername(std::string_view username);
  static void ClearUsername();

  static bool HasExploreConsent();
  static bool SetExploreConsent(bool consented);

  static bool IsValidUsername(std::string_view username);

private:
  static std::string GenerateDeviceId();
};