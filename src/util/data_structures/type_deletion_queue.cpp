
#include "util/pch.h"

#include "type_deletion_queue.h"

namespace AT::util {


    void type_deletion_queue::flush() {

		for (auto it = m_deletors.rbegin(); it != m_deletors.rend(); it++)
			(*it)();
            
        for (const auto& entry : m_pointers)
            flush_pointer(entry);                   // to be defined by derived class
        
        m_deletors.clear();
        m_pointers.clear();
    }

	void type_deletion_queue::push_func(std::function<void()>&& function) { m_deletors.push_back(function); }
    
}
