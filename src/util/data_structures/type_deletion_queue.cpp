
#include "util/pch.h"

#include "type_deletion_queue.h"

namespace AT::util {


    void type_deletion_queue::flush() {

		for (auto it = m_deleter.rbegin(); it != m_deleter.rend(); it++)
			(*it)();
            
        for (const auto& entry : m_pointers)
            flush_pointer(entry);                   // to be defined by derived class
        
        m_deleter.clear();
        m_pointers.clear();
    }

	void type_deletion_queue::push_func(std::function<void()>&& function) { m_deleter.push_back(function); }
    
}
