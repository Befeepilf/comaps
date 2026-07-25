#include <healpix_base.h>
#include <healpix_tables.h>

#include <cstdint>

namespace hp
{
T_Healpix_Base<std::int64_t> const & GetHealpixBase()
{
  static T_Healpix_Base<std::int64_t> base(1048576, Healpix_Ordering_Scheme::NEST, SET_NSIDE);
  return base;
}
}  // namespace hp
