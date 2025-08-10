
#include "util/pch.h"

#include "deletion_queue.h"

namespace AT::util {


    void deletion_queue::flush() {

        for (auto it = m_functions.rbegin(); it != m_functions.rend(); it++)
            (*it)();
        m_functions.clear();
    }

}
