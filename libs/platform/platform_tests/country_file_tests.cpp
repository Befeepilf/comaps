#include "testing/testing.hpp"

#include "defines.hpp"

#include "platform/country_file.hpp"
#include "platform/mwm_version.hpp"

#include <string>

namespace platform
{
UNIT_TEST(CountryFile_Smoke)
{
  {
    CountryFile cf("One");
    TEST_EQUAL("One", cf.GetName(), ());
    auto const mapFileName = cf.GetFileName(MapFileType::Map);

    TEST_EQUAL("One" DATA_FILE_EXTENSION, mapFileName, ());
    TEST_EQUAL(0, cf.GetRemoteSize(), ());
    TEST_EQUAL(0, cf.GetRemoteSpaSize(), ());
    TEST(cf.GetSpaSha1().empty(), ());
    TEST(!cf.HasRemoteSpa(), ());
  }

  {
    CountryFile cf("Three", 666, "xxxSHAxxx");
    TEST_EQUAL("Three", cf.GetName(), ());
    auto const mapFileName = cf.GetFileName(MapFileType::Map);

    TEST_EQUAL("Three" DATA_FILE_EXTENSION, mapFileName, ());
    TEST_EQUAL(666, cf.GetRemoteSize(), ());
    TEST_EQUAL("xxxSHAxxx", cf.GetSha1(), ());
    TEST_EQUAL(0, cf.GetRemoteSpaSize(), ());
    TEST(cf.GetSpaSha1().empty(), ());
    TEST(!cf.HasRemoteSpa(), ());
  }

  {
    CountryFile cf("SpaLeaf", 1000, "mwmSha", 42, "spaSha");
    TEST_EQUAL("SpaLeaf", cf.GetName(), ());
    TEST_EQUAL(1000, cf.GetRemoteSize(), ());
    TEST_EQUAL("mwmSha", cf.GetSha1(), ());
    TEST_EQUAL(42, cf.GetRemoteSpaSize(), ());
    TEST_EQUAL("spaSha", cf.GetSpaSha1(), ());
    TEST(cf.HasRemoteSpa(), ());
  }

  {
    // Size alone or hash alone is not an advertisement.
    CountryFile sizeOnly("SizeOnly", 1, "m", 99, "");
    TEST(!sizeOnly.HasRemoteSpa(), ());
    TEST_EQUAL(99, sizeOnly.GetRemoteSpaSize(), ());

    CountryFile hashOnly("HashOnly", 1, "m", 0, "spaSha");
    TEST(!hashOnly.HasRemoteSpa(), ());
    TEST_EQUAL("spaSha", hashOnly.GetSpaSha1(), ());
  }
}
}  // namespace platform
