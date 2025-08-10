
#include "util/pch.h"
#include "util/crash_handler.h"
#include "application.h"


#if defined(PLATFORM_LINUX)

    int main(int argc, char* argv[]) {

        AT::attach_crash_handler();
        
        AT::application app = AT::application(argc, argv);
        app.run();
        return EXIT_SUCCESS;
    }

#elif defined(PLATFORM_WINDOWS)

    #include <Windows.h>

    int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
        
        AT::attach_crash_handler();
        
        AT::application app = AT::application(__argc, __argv);
        app.run();
        return EXIT_SUCCESS;
    }

#endif
