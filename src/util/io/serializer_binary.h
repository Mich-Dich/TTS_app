#pragma once

#include "serializer_data.h"

namespace AT::serializer {

	class binary {
	public:

		DELETE_COPY_MOVE_CONSTRUCTOR(binary);

		// Declares a default getter for the serialization option member.
		// The macro will expand to a function that returns the current option.
		// @return The current serialization option.
		DEFAULT_GETTER(option, option);


		// Constructs a binary serializer/deserializer for the given file and section.
		// When [option] is save_to_file the object opens the file for binary output;
		// otherwise it opens the file for binary input.
		// @param filename The path to the file to read from or write to.
		// @param section_name A human-readable name for the section being (de)serialized.
		// @param option Controls whether the instance is used to save to or load from file.
		// @return Constructs a binary object ready to perform (de)serialization.
		binary(const std::filesystem::path filename, const std::string& section_name, option option);


		// Destroys the binary (de)serializer and closes any open file streams.
		// If the object opened an output stream it will be closed; likewise for input streams.
		// @return None.
		~binary();


		// Serializes or deserializes a single value depending on the configured option.
		// If saving:
		//   - For std::filesystem::path: converts to a string and serializes that string.
		//   - For std::string: writes a length (size_t) followed by the raw characters.
		//   - For other types: writes raw bytes of sizeof(T).
		// If loading:
		//   - For std::filesystem::path: reads a string and constructs the path from it.
		//   - For std::string: reads a length then fills the string buffer from the stream.
		//   - For other types: reads raw bytes into the provided value.
		// @tparam T The type of the value to (de)serialize.
		// @param value Reference to the value to serialize (when saving) or to receive the value (when loading).
		// @return A reference to *this to allow chaining of entry(...) calls.
		template<typename T>
		binary& entry(T& value) {

			if (m_option == option::save_to_file) {

				if constexpr (std::is_same_v<T, std::filesystem::path>) {

					std::string path_str = value.generic_string();
					entry<std::string>(path_str);

				} else if constexpr (std::is_same_v<T, std::string>) {

					size_t length = value.size();
					m_ostream.write(reinterpret_cast<const char*>(&length), sizeof(length));
					m_ostream.write(reinterpret_cast<const char*>(value.data()), length);

				} else
					m_ostream.write(reinterpret_cast<const char*>(&value), sizeof(T));

			} else {

				if constexpr (std::is_same_v<T, std::filesystem::path>) {

					std::string buffer;
					entry<std::string>(buffer);
					value = std::filesystem::path(buffer);

				} else if constexpr (std::is_same_v<T, std::string>) {

					size_t length = 0;
					m_istream.read(reinterpret_cast<char*>(&length), sizeof(length));
					
					ASSERT(length < 65565, "", "Corrupted path length")

					value.resize(length);
					m_istream.read(value.data(), length);

				} else
					m_istream.read(reinterpret_cast<char*>(&value), sizeof(T));
			}

			return *this;
		}


		// Serializes or deserializes a contiguous std::vector<T>.
		// If saving: writes the vector's size (size_t) followed by the raw element bytes (sizeof(T) * size).
		// If loading: reads the size, resizes the vector, then reads raw element bytes into vector.data().
		// NOTE: This assumes T is trivially copyable / safely writable as raw bytes.
		// @tparam T The vector element type.
		// @param vector The vector to write (when saving) or to fill (when loading).
		// @return A reference to *this to allow chaining.
		template<typename T>
		binary& entry(std::vector<T>& vector) {

			if (m_option == option::save_to_file) {

				size_t size = vector.size();
				m_ostream.write(reinterpret_cast<const char*>(&size), sizeof(size_t));
				m_ostream.write(reinterpret_cast<const char*>(vector.data()), sizeof(T) * size);

			} else {

				size_t vector_size = 0;
				m_istream.read(reinterpret_cast<char*>(&vector_size), sizeof(size_t));
				vector.resize(vector_size);
				m_istream.read(reinterpret_cast<char*>(vector.data()), sizeof(T) * vector_size);
			}

			return *this;
		}


		// Serializes or deserializes a raw array region of known size.
		// If saving: writes the array data (sizeof(T) * array_size) from array_start to the output stream.
		// If loading: allocates a buffer with malloc(sizeof(T) * array_size), reads bytes into it,
		//             and assigns the pointer to array_start. Caller becomes the owner and is responsible for freeing it.
		// WARNING: On load ownership transfers to the caller; memory is allocated with malloc.
		// @tparam T The element type of the array.
		// @param array_start Pointer reference that points to the array data (or will be assigned the allocated buffer when loading).
		// @param array_size Number of elements in the array.
		// @return A reference to *this to allow chaining.
		template<typename T>
		binary& array(T*& array_start, size_t array_size) {

			const size_t total_bytes = sizeof(T) * array_size;
			if (m_option == option::save_to_file) {
				m_ostream.write(reinterpret_cast<const char*>(array_start), total_bytes);
			} else {

				array_start = (T*)malloc(total_bytes);
				LOG(Trace, "Deserializing [" << total_bytes << "] bytes into [" << (void*)array_start << "]")
				m_istream.read(reinterpret_cast<char*>(array_start), total_bytes);
			}

			return *this;
		}


		// Placeholder for serializing/deserializing a vector with a custom per-element callback.
		// Currently unimplemented — logs an error and returns immediately.
		// Intended behavior (when implemented): iterate elements and call vector_function for each element,
		// allowing custom (de)serialization logic for complex element types.
		// @tparam T The vector element type.
		// @param vector The vector to operate on.
		// @param vector_function A callback that performs (de)serialization per element.
		//                        Signature: void(AT::serializer::binary&, const u64 iteration)
		// @return A reference to *this (currently the function returns immediately after logging).
		template<typename T>
		binary& vector(std::vector<T>& vector, std::function<void(AT::serializer::binary&, const u64 iteration)> vector_function) {

			LOG(Error, "NOT IMPLEMENTED YET");
			return *this;
		}


	private:

		std::filesystem::path 		m_filename{};
		std::string 				m_name{};
		option 						m_option;
		std::ofstream 				m_ostream{};
		std::ifstream 				m_istream{};

	};

}
