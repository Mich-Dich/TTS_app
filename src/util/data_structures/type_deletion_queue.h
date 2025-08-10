#pragma once

#include <typeindex>

namespace AT::util {

    // this is an extended version of th normal deletion queue
    // you can just push pointers (smaller, faster) and define how to handel them in the custom [flush_pointer() function]
	class type_deletion_queue {
	public:

		DEFAULT_CONSTRUCTORS(type_deletion_queue);

		template<typename T>
		void push_pointer(T* pointer) { m_pointers.push_back(std::pair<std::type_index, void*>{std::type_index(typeid(T*)), pointer}); }
		void push_func(std::function<void()>&& function);

		void flush();
		virtual void flush_pointer(std::pair<std::type_index, void*> pointer) = 0;

	protected:
		std::deque<std::function<void()>>				m_deletors{};
		std::vector<std::pair<std::type_index, void*>>	m_pointers{};
	};
}
