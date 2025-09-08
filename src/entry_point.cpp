
#include "util/pch.h"
#include "util/timing/instrumentor.h"
#include "util/crash_handler.h"
#include "application.h"

#if defined(PLATFORM_LINUX)
    #define ARGC        argc
    #define ARGV        argv
#elif defined(PLATFORM_WINDOWS)
    #define ARGC        __argc
    #define ARGV        __argv
    #include <Windows.h>
#endif


#if defined(PLATFORM_LINUX)
    int main(int argc, char* argv[])
#elif defined(PLATFORM_WINDOWS)
    int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#endif
    {
        PROFILER_SESSION_BEGIN("application", AT::util::get_executable_path() / "profiler", "application.json");
        {

            PROFILE_SCOPE("sub-systems startup");
            AT::crash_handler::attach();
            AT::logger::init("[$B$T:$J$E] [$B$L$X $Q - $I:$P:$G$E] $C$Z", true, AT::util::get_executable_path() / "logs", "application.log", true);
            AT::logger::set_buffer_threshold(AT::logger::severity::Warn);
            AT::logger::register_label_for_thread("main");
            AT::crash_handler::subscribe(AT::logger::shutdown);
        }

        {   // put application in scope to guarantee termination at specific point
            AT::application app = AT::application(ARGC, ARGV);
            app.run();
        }

        {
            PROFILE_SCOPE("sub-systems shutdown");
            
            AT::logger::shutdown();
            AT::crash_handler::detach();
        }

        PROFILER_SESSION_END();
        return EXIT_SUCCESS;
    }
