#pragma once

#include <typeindex>

namespace AT::util {

    // this is an extended version of th normal deletion queue
    // you can just push pointers (smaller, faster) and define how to handel them in the custom [flush_pointer() function]
	class type_deletion_queue {
	public:
		
		// Declares the default constructors and assignment operators for type_deletion_queue.
		// The exact constructors generated depend on the macro definition (e.g. default
		// constructor, copy/move constructors, copy/move assignment, destructor).
		// @return None.
		DEFAULT_CONSTRUCTORS(type_deletion_queue);

		// Stores a pointer in the deletion queue together with its runtime type information.
		// The stored type information is produced via std::type_index(typeid(T*)), and the
		// pointer itself is stored as void* to avoid template bloat.
		// @tparam T The pointer element type.
		// @param pointer The pointer to store for later handling during flush().
		// @return None.
		template<typename T>
		void push_pointer(T* pointer) { m_pointers.push_back(std::pair<std::type_index, void*>{std::type_index(typeid(T*)), pointer}); }
		
		// Stores a callable that will be invoked when flush() is called.
		// Callables are stored in insertion order but executed in reverse order during flush().
		// Use this to enqueue arbitrary cleanup code without allocating per-pointer wrappers.
		// @param function An rvalue reference to a callable (e.g., lambda) to store.
		// @return None.
		void push_func(std::function<void()>&& function);

		// Executes all queued cleanup operations and then clears the internal queues.
		// First, all stored callables in [m_deleter] are invoked in reverse insertion order.
		// Next, each stored pointer entry in [m_pointers] is forwarded to flush_pointer()
		// so derived classes can perform type-aware deletion/cleanup. Finally both
		// containers are cleared.
		// @return None.
		void flush();

		// Handle a single stored pointer entry. This is a pure virtual hook that must be
		// implemented by derived classes to perform the appropriate deletion or cleanup
		// for the pointer type encoded in the provided pair.
		// @param pointer A pair containing the runtime type index and the pointer (as void*).
		//                The type_index was created with typeid(T*) when the pointer was pushed.
		// @return None.
		virtual void flush_pointer(std::pair<std::type_index, void*> pointer) = 0;

	protected:

		// Deque of cleanup callables enqueued via push_func(). These are executed in reverse
		// order during flush() to allow stack-like destruction ordering for resources.
		// @see push_func(), flush()
		std::deque<std::function<void()>>				m_deleter{};

		// Vector of stored pointers paired with their runtime type information.
		// Each element is (type_index(typeid(T*)), void*). Derived classes should interpret
		// the type_index and cast the void* appropriately inside flush_pointer().
		// @see push_pointer(), flush_pointer()
		std::vector<std::pair<std::type_index, void*>>	m_pointers{};
	};
}
