
#include "util/pch.h"
#include "util/crash_handler.h"
#include "util/system.h"
#include "application.h"


#if defined(PLATFORM_LINUX)

    int main(int argc, char* argv[]) {

        AT::crash_handler::attach();
        AT::logger::init("[$B$T:$J$E] [$B$L$X $I - $P:$G$E] $C$Z", true, AT::util::get_executable_path() / "logs", "application.log", true);
        AT::logger::set_buffer_threshold(AT::logger::severity::Warn);
        AT::crash_handler::subscribe(AT::logger::shutdown);
        
        AT::application app = AT::application(argc, argv);
        app.run();
        return EXIT_SUCCESS;
    }

#elif defined(PLATFORM_WINDOWS)

    #include <Windows.h>

    int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
        
        AT::crash_handler::attach();
        AT::logger::init("[$B$T:$J$E] [$B$L$X $I - $P:$G$E] $C$Z", true, AT::util::get_executable_path() / "logs", "application.log", true);
        AT::logger::set_buffer_threshold(AT::logger::severity::Warn);
        AT::crash_handler::subscribe(AT::logger::shutdown);
        
        AT::application app = AT::application(__argc, __argv);
        app.run();
        return EXIT_SUCCESS;
    }

#endif
