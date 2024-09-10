#pragma once

#ifdef __APPLE__
#include "TargetConditionals.h"
#endif

#include "halley/text/enum_names.h"

namespace Halley {
	enum class GamePlatform {
		Unknown,
		Windows,
		MacOS,
		Linux,
		Switch,
		XboxOne,
		PS4,
		UWP,
		Android,
		iOS,
        Emscripten,
		FreeBSD,
        FuturePlatform1,
        FuturePlatform2,
        FuturePlatform3,
        FuturePlatform4,
        FuturePlatform5,
	};

	template <>
	struct EnumNames<GamePlatform> {
		constexpr std::array<const char*, 17> operator()() const {
			return {{
				"unknown",
				"windows",
				"macos",
				"linux",
				"switch",
				"xboxone",
				"ps4",
				"uwp",
				"android",
				"ios",
                "emscripten",
				"freebsd",
                "futurePlatform1",
                "futurePlatform2",
                "futurePlatform3",
                "futurePlatform4",
                "futurePlatform5"
			}};
		}
	};

    constexpr inline GamePlatform getPlatform()
    {
	#if defined(__FUTURE_PLATFORM_1__)
        return GamePlatform::FuturePlatform1;
	#elif defined(__FUTURE_PLATFORM_2__)
        return GamePlatform::FuturePlatform2;
	#elif defined(__FUTURE_PLATFORM_3__)
        return GamePlatform::FuturePlatform3;
	#elif defined(__FUTURE_PLATFORM_4__)
        return GamePlatform::FuturePlatform4;
	#elif defined(__FUTURE_PLATFORM_5__)
        return GamePlatform::FuturePlatform5;
	#elif defined(__NX_TOOLCHAIN_MAJOR__)
        return GamePlatform::Switch;
    #elif defined(__ORBIS__)
        return GamePlatform::PS4;
    #elif defined(_XBOX_ONE)
        return GamePlatform::XboxOne;
    #elif defined(WINDOWS_STORE)
        return GamePlatform::UWP;
    #elif defined(_WIN32)
        return GamePlatform::Windows;
    #elif defined(__ANDROID__)
        return GamePlatform::Android;
    #elif defined(__EMSCRIPTEN__)
        return GamePlatform::Emscripten;
    #elif defined(__APPLE__)
        #if TARGET_OS_IPHONE
            return GamePlatform::iOS;
        #else
            return GamePlatform::MacOS;
        #endif
    #elif defined(__linux)
        return GamePlatform::Linux;
    #elif defined(__FreeBSD__)
        return GamePlatform::FreeBSD;
    #else
        return GamePlatform::Unknown;
    #endif
    }

    constexpr inline bool isPCPlatform()
    {
        return getPlatform() == GamePlatform::Windows || getPlatform() == GamePlatform::Linux || getPlatform() == GamePlatform::MacOS || getPlatform() == GamePlatform::FreeBSD || getPlatform() == GamePlatform::Emscripten;
    }

    constexpr inline bool isMobilePlatform()
    {
        return getPlatform() == GamePlatform::Android || getPlatform() == GamePlatform::iOS;
    }

    constexpr inline bool isConsolePlatform()
    {
        return getPlatform() == GamePlatform::Switch || getPlatform() == GamePlatform::PS4 || getPlatform() == GamePlatform::XboxOne || getPlatform() == GamePlatform::UWP;
    }
}
