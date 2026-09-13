#include "iflyctrl.h"

#include <cstdint>
#include <type_traits>
#include <utility>


static_assert(sizeof(std::int32_t) == 4, "the landing values are four-byte integers");
static_assert(std::is_same_v<decltype(std::declval<IFlyControl &>().Landing_Altitude()), std::int32_t>,
	"landing altitude is a fixed-width signed integer");
static_assert(std::is_same_v<decltype(std::declval<IFlyControl &>().Landing_Direction()), std::int32_t>,
	"landing direction is a fixed-width signed integer");
static_assert(std::is_same_v<decltype(std::declval<IFlyControl &>().Is_Loaded()), bool>,
	"loaded state is a C++ boolean");
static_assert(std::is_same_v<decltype(std::declval<IFlyControl &>().Is_Strafe()), bool>,
	"strafe state is a C++ boolean");
static_assert(std::is_same_v<decltype(std::declval<IFlyControl &>().Is_Locked()), bool>,
	"locked state is a C++ boolean");


class ContractProbe final : public IFlyControl
{
	public:
		std::int32_t Landing_Altitude(void) override { return(-123456789); }
		std::int32_t Landing_Direction(void) override { return(123456789); }
		bool Is_Loaded(void) override { return(true); }
		bool Is_Strafe(void) override { return(false); }
		bool Is_Locked(void) override { return(true); }
};


int main(void)
{
	ContractProbe probe;
	IFlyControl & contract = probe;
	return(contract.Landing_Altitude() != -123456789
		|| contract.Landing_Direction() != 123456789
		|| !contract.Is_Loaded()
		|| contract.Is_Strafe()
		|| !contract.Is_Locked()) ? 1 : 0;
}
