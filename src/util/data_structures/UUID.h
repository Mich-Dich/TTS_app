#pragma once


class UUID {
public:

	// Declares the default copy constructor for UUID.
	// @param other The UUID instance to copy from.
	// @return None.
	DEFAULT_COPY_CONSTRUCTOR(UUID);
	
	// Constructs a new UUID by generating a random 64-bit value using the
	// library's global RNG and distribution.
	// @return None. (A new UUID object is constructed with a generated value.)
	UUID();

	// Constructs a UUID from an existing 64-bit value.
	// @param uuid The explicit 64-bit value to initialize this UUID with.
	// @return None. (A new UUID object is constructed with [uuid].)
	UUID(u64 uuid);

	// Destroys this UUID. If experimental collision avoidance is enabled,
	// the destructor will remove this UUID's value from the set of generated UUIDs.
	// @return None.
	~UUID();

	// Conversion operator that returns the underlying 64-bit UUID value.
	// @return The 64-bit integer representation of this UUID.
	operator u64() const { return m_UUID; }

private:

	u64 m_UUID;
};


namespace std {

	template <typename T> struct hash;

	// Specialization of std::hash for UUID so UUIDs can be used as keys in
	// unordered containers (e.g. std::unordered_map, std::unordered_set).
	template<>
	struct hash<UUID> {

		// Produces a hash value for a UUID by returning its underlying 64-bit value
		// converted to std::size_t.
		// @param uuid The UUID to hash.
		// @return A std::size_t hash derived from the UUID's 64-bit value.
		std::size_t operator()(const UUID& uuid) const { return (u64)uuid; }
	};

}
