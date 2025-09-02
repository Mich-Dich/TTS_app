#pragma once

#include "serializer_data.h"

namespace AT::serializer {

	class yaml {
	public:

		yaml(const std::filesystem::path filename, const std::string& section_name, option option);
		~yaml();

		DELETE_COPY_MOVE_CONSTRUCTOR(yaml);
		DEFAULT_GETTER(option, option);

		// @brief This function adds or looks for a subsection with the specified section name in the YAML file.
		//          If the serializer option is set to [save_to_file], it adds the subsection to the YAML content
		//          and executes the provided function within that subsection. If the serializer option is set to
		//          [load_from_file], it looks for the subsection in the YAML content, deserializes its content,
		//          and executes the provided function within that subsection.
		// @param [section_name] The name of the subsection to be added or looked for.
		// @param [sub_section_function] The function to be executed within the subsection.
		//                               The function should accept a reference to a yaml object as its parameter.
		// @return A reference to the YAML object for chaining function calls.
		yaml& sub_section(const std::string& section_name, std::function<void(serializer::yaml&)> sub_section_function);

		
		// @brief This function is responsible for serializing or deserializing a single variable 
		//          to or from the YAML file based on the specified serialization option. If the 
		//          option is set to [save_to_file], it converts the variable to its string 
		//          representation and writes it to the YAML file. If the option is set to [load_from_file],
		//			it reads the variable's value from the YAML file and converts it back to 
		//          its original type. For vectors, it handles serialization and deserialization 
		//          of each element individually.
		// @param [key_name] The key name associated with the variable in the YAML file.
		// @param [value] Reference to the variable to be serialized or deserialized.
		// @return A reference to the YAML object for chaining function calls.
		template<typename T>
		yaml& entry(const std::string& key_name, T& value) {

			if (m_option == serializer::option::save_to_file) {

				std::string buffer{};
				if constexpr (is_vector<T>::value) {			// value is a vector

					m_file_content << util::add_spaces(m_level_of_indention) << m_prefix << key_name << ":\n";
					for (auto interation : value) {

						util::convert_to_string<T::value_type>(interation, buffer);
						m_file_content << util::add_spaces(m_level_of_indention + 1) << "- " << buffer << "\n";
					}

				} else {

					util::convert_to_string<T>(value, buffer);
					m_file_content << util::add_spaces(m_level_of_indention) << m_prefix << key_name << ": " << buffer << "\n";
				}

			} else {				// load from file

				if constexpr (is_vector<T>::value) {			// value is a vector

					// deserialize content of subsections				
					T buffer{};
					u32 section_indentation = 0;
					bool found_section = false;
					std::string line;
					while (std::getline(m_file_content, line)) {

						// skip empty lines or comments
						if (line.empty() || line.front() == '#')
							continue;

						// if line contains desired section enter inner-loop
						//   has correct indentaion                                 has correct section_name                      ends with double-point
						if ((util::measure_indentation(line) == 0) && (line.find(key_name) != std::string::npos) && (line.back() == ':')) {

							found_section = true;
							//LOG(Debug, "sub_section() found section => line: [" << line << "]");

							//     not end of content                     has correct indentaion                                 doesn't end in double-points              
							while (std::getline(m_file_content, line) && (util::measure_indentation(line) == 1) && (line.back() != ':')) {

								// remove indentation                       remove "- " (array element marker)
								line = line.substr(NUM_OF_INDENTING_SPACES + 2);
								util::convert_from_string(line, buffer);
								value.emplace_back(buffer);
							}

							//LOG(Debug, "END OF SUB-SECTION");
						}

						// skip rest of content if section found
						if (found_section)
							break;
					}

				} else {

					std::string buffer{};
					auto iterator = m_key_value_pares.find(key_name);
					if (iterator == m_key_value_pares.end())				// key is not in map
						return *this;

					buffer = iterator->second;
					util::convert_from_string(buffer, value);
				}
			}

			m_prefix = m_prefix_fallback;
			return *this;
		}


		// @brief This function is responsible for serializing or deserializing a vector variable to or from
		//          the YAML file based on the specified serialization option. If the option is set to save to file,
		//          it serializes each element of the vector individually and writes them to the YAML file. If the option
		//          is set to load from file, it reads each element of the vector from the YAML file and deserializes them
		//          back into the vector. Additionally, it executes a provided function for each element of the vector during
		//          serialization or deserialization.
		// @param [vector_name] The name of the vector variable in the YAML file.
		// @param [vector] Reference to the vector variable to be serialized or deserialized.
		// @param [vector_function] The function to be executed for each element of the vector during serialization
		//                           or deserialization. The function should accept a reference to a yaml object and
		//                           the current iteration index as parameters.
		// @return A reference to the YAML object for chaining function calls.
		template<typename T>
		yaml& vector(const std::string& vector_name, std::vector<T>& vector, std::function<void(serializer::yaml&, const u64 iteration)> vector_function) {

			vector_func_index++;

			if (vector_func_index != 1)
				m_level_of_indention++;

			if (m_option == serializer::option::save_to_file) {			// save to file

				const u32 indent_buffer = vector_func_index != 1 ? m_level_of_indention - 1 : m_level_of_indention;
				m_file_content << util::add_spaces(indent_buffer) << m_prefix << vector_name << ":\n";
				for (u64 x = 0; x < vector.size(); x++) {

					// start of array element
					m_prefix = "- ";
					m_prefix_fallback = "  ";
					vector_function(*this, x);
				}

				m_prefix = "";
				m_prefix_fallback = "";

			} else {		// load from file

				// buffer [m_key_value_pares] for duration of function
				std::unordered_map<std::string, std::string> key_value_pares_buffer = m_key_value_pares;
				std::vector<std::unordered_map<std::string, std::string>> vector_of_key_value_pares{};
				m_key_value_pares = {};

				// buffer [m_file_content] for duration of function
				std::stringstream file_content_buffer;
				std::vector<std::stringstream> vector_of_file_content{};		// for array element in file
				file_content_buffer << m_file_content.str();
				m_file_content = {};

				// deserialize content of subsections				
				int64 index = -1;
				std::string line;
				while (std::getline(file_content_buffer, line)) {

					// skip empty lines or comments
					if (line.empty() || line.front() == '#')
						continue;

					// if line contains desired section enter inner-loop
					//   has correct indentaion              has correct vector_name                          ends with double-point
					if ((util::measure_indentation(line, NUM_OF_INDENTING_SPACES) == 0) && (line.find(vector_name) != std::string::npos) && (line.back() == ':')) {

						//     not end of content
						while (std::getline(file_content_buffer, line)) {

							if (line.front() == '-') {

								vector_of_key_value_pares.push_back({});
								vector_of_file_content.push_back({});
								index++;
							}

							// remove array-prefix "- " or "  "
							line = line.substr(NUM_OF_INDENTING_SPACES);

							//  more indented                                        beginning of new sub-section
							if (util::measure_indentation(line, NUM_OF_INDENTING_SPACES) != 0 || line.back() == ':' || line.front() == '-') {

								//m_file_content << line << "\n";
								vector_of_file_content[index] << line << "\n";
								continue;
							}

							std::string key, value;
							extract_key_value(key, value, line);
							vector_of_key_value_pares[index][key] = value;
						}
					}
				}

				ASSERT(vector_of_key_value_pares.size() == vector_of_file_content.size(), "", "two buffers are of diffrent size");

				if (vector_of_key_value_pares.size() > 0) {

					vector.resize(vector_of_key_value_pares.size());
					for (u64 x = 0; x < vector.size(); x++) {

						m_key_value_pares = vector_of_key_value_pares[x];
						m_file_content = {};
						auto temp_buffer = vector_of_file_content[x].str();
						m_file_content << temp_buffer;
						vector_function(*this, x);
					}
				}

				// restore
				m_key_value_pares = key_value_pares_buffer;
				auto temp_buffer = file_content_buffer.str();
				m_file_content << temp_buffer;

			}

			if (vector_func_index != 1)
				m_level_of_indention--;

			vector_func_index--;
			return *this;
		}


		// Serializes or deserializes an unordered_map to/from YAML stored in the class's file stream.
		// When saving, writes a YAML mapping named [map_name] and writes each key/value pair as "key: value"
		// using util::to_string<T>/util::to_string<K>. When loading, finds the section named [map_name]
		// at the current indentation level, reads key/value lines until the section ends, converts each
		// string key and value back into types T and K via util::convert_from_string, and inserts them
		// into [map].
		// @tparam T The unordered_map key type. Must be convertible to/from std::string by util::to_string<T>
		//            and util::convert_from_string.
		// @tparam K The unordered_map mapped value type. Must be convertible to/from std::string by
		//            util::to_string<K> and util::convert_from_string.
		// @param map_name The YAML key/name under which the map is serialized/deserialized.
		// @param map The unordered_map to write to the YAML stream (when saving) or to populate
		//            with parsed values (when loading).
		// @return A reference to this yaml serializer/deserializer to allow chaining.
		template<typename T, typename K>
		yaml& unordered_map(const std::string& map_name, std::unordered_map<T, K>& map) {

			if (m_option == serializer::option::save_to_file) {											// Serialize the map
				m_file_content << util::add_spaces(m_level_of_indention) << map_name << ":\n";
				for (const auto& [key, value] : map)
					m_file_content << util::add_spaces(m_level_of_indention + 1) << util::to_string<T>(key) << ": " << util::to_string<K>(value) << "\n";
				
			} else {																					// Deserialize the map
				
				// Deserialize map from YAML
				std::unordered_map<std::string, std::string> temp_map;
				std::string line;
				
				// Read until we find the map section
				while (std::getline(m_file_content, line)) {
					if (line.find(map_name + ":") != std::string::npos && 
						util::measure_indentation(line) == m_level_of_indention) {
						break;
					}
				}

				while (std::getline(m_file_content, line)) {							// Read key-value pairs
					if (util::measure_indentation(line) <= m_level_of_indention)		// End of map section
						break;
					
					std::string key, value;
					extract_key_value(key, value, line);
					temp_map.emplace(std::move(key), std::move(value));
				}

				// Convert strings to actual types
				for (const auto& [key_str, value_str] : temp_map) {
					T key;
					K value;
					util::convert_from_string(key_str, key);
					util::convert_from_string(value_str, value);
					map.emplace(std::move(key), std::move(value));
				}
			}
			return *this;
		}

		
		// Serializes or deserializes an unordered_set to/from YAML stored in the class's file stream.
		// When saving, writes a YAML sequence named [set_name] and writes each element as "- element".
		// When loading, finds the sequence named [set_name] at the current indentation level, reads each
		// sequence entry (lines beginning with "- "), converts the string representation to type T using
		// util::convert_from_string, and inserts the values into [set].
		// @tparam T The element type stored in the unordered_set. Must be convertible to/from std::string
		//            via util::convert_from_string and util::convert_to_string<T>.
		// @param set_name The YAML key/name under which the set sequence is serialized/deserialized.
		// @param set The unordered_set to write to the YAML stream (when saving) or to populate with parsed
		//            elements (when loading).
		// @return A reference to this yaml serializer/deserializer to allow chaining.
		template<typename T>
		yaml& unordered_set(const std::string& set_name, std::unordered_set<T>& set) {

			if (m_option == option::save_to_file) {
				// Serialize the set as a YAML sequence
				m_file_content << util::add_spaces(m_level_of_indention) << set_name << ":\n";
				for (const auto& element : set) {
					std::string buffer;
					util::convert_to_string<T>(element, buffer);
					m_file_content << util::add_spaces(m_level_of_indention + 1) << "- " << buffer << "\n";
				}
			} else {																	// Deserialize the set from YAML

				std::unordered_set<T> temp_set;
				std::string line;
				while (std::getline(m_file_content, line)) {							// Read until we find the set section
					if (line.find(set_name + ":") != std::string::npos && 
						util::measure_indentation(line) == m_level_of_indention) {
						break;
					}
				}

				while (std::getline(m_file_content, line)) {							// Read sequence elements
					if (util::measure_indentation(line) <= m_level_of_indention) 		// End of set section
						break;
					
					if (line.find("- ") != std::string::npos) {							// Extract element value

						size_t dash_pos = line.find("- ");
						std::string element_str = line.substr(dash_pos + 2);
						T element;
						util::convert_from_string(element_str, element);
						temp_set.insert(element);
					}
				}
				set = std::move(temp_set);
			}
			return *this;
		}

		
	private:

		void serialize();
		yaml& deserialize();
		void extract_key_value(std::string& key, std::string& value, std::string& line);

		static const u32 NUM_OF_INDENTING_SPACES = 2;		// should not change

		u32 m_level_of_indention = 0;
		u64 vector_func_index = 0;
		std::string m_prefix{};				// can maybe be a [char*]
		std::string m_prefix_fallback{};	// can maybe be a [char*]

		// file data
		std::filesystem::path m_filename{};
		std::ofstream m_ostream{};
		std::ifstream m_istream{};
		
		// content data
		bool m_is_correct_struct = false;
		std::string m_name{};
		option m_option;
		std::stringstream m_file_content{};
		std::unordered_map<std::string, std::string> m_key_value_pares{};

	};

}
