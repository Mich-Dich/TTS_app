#pragma once

#include <util/pch.h>

namespace AT::crash_handler {
    

    //
    void attach();


    //
    void detach();


    // @brief register a function that should be executed when the crash handler is invoked
    // @param function A pointer to the function that should be executed when crash occurs
    // @return A u64 index to the registered function, can be used to unsubscribe the function. The index is never reused.
	u64 subscribe(std::function<void()> function);

    
    // @brief Remove a registered function so it will not be executed when the crash handler is invoked
    // @param index The u64 index that was returned by [subscribe()]
    void unsubscribe(u64 index);

}
