#pragma once

#include <util/pch.h>

namespace AT::crash_handler {
    

    // Installs crash and termination signal/exception handlers.
    // 
    // On Linux:
    //   - Registers custom handlers for various POSIX signals (e.g., SIGINT, SIGTERM, SIGSEGV).
    //   - Saves the previous signal actions so they can be restored later.
    //   - Ensures duplicate signals are filtered out before registration.
    // 
    // On Windows:
    //   - Installs an unhandled exception filter via SetUnhandledExceptionFilter().
    //   - Adds a vectored exception handler to intercept breakpoint and crash events.
    //
    // @throws std::system_error If a signal handler fails to install on Linux.
    // @note Call detach() to restore the previous state when the crash handler is no longer needed.
    void attach();


    // Removes previously installed crash and termination signal/exception handlers.
    //
    // On Linux:
    //   - Restores the old signal handlers saved during attach().
    //   - Clears the internal list of tracked signals.
    //
    // On Windows:
    //   - Restores the previous unhandled exception filter.
    //   - Removes the vectored exception handler if one was installed.
    //
    // @throws std::system_error If restoring a signal handler fails on Linux.
    void detach();


    // @brief register a function that should be executed when the crash handler is invoked
    // @param function A pointer to the function that should be executed when crash occurs
    // @return A u64 index to the registered function, can be used to unsubscribe the function. The index is never reused.
	u64 subscribe(std::function<void()> function);

    
    // @brief Remove a registered function so it will not be executed when the crash handler is invoked
    // @param index The u64 index that was returned by [subscribe()]
    void unsubscribe(u64 index);

}
