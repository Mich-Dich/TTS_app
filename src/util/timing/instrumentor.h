#pragma once

#include "util/pch.h"

#include "util/core_config.h"
#include "util/data_structures/data_types.h"
#include "util/io/logger.h"
#include "util/macros.h"
#include "util/timing/stopwatch.h"


namespace AT {

// #if defined(DEBUG)

	namespace Profiling {

#define ISOLATED_PROFILER_SCOPED(num_of_tests, message, profile_duration_precision)     static Profiling::simple_profiler loc_simple_profiler(num_of_tests, message, profile_duration_precision);      \
                                                                                            stopwatch loc_stopwatch_for_profiling(&loc_simple_profiler.single_duration, profile_duration_precision);       \
                                                                                            if (loc_simple_profiler.add_value())                                                                                \
                                                                                                loc_stopwatch_for_profiling.restart()


#define DURATION_microseconds       " microseconds"
#define DURATION_miliseconds        " milliseconds"
#define DURATION_seconds            " seconds"

#define ISOLATED_PROFILER_LOOP(num_of_iterations, message, profile_duration_precision, func)                                                                                                      \
            {                                                                                                                                                                                         \
                f32 duration = -1.f;                                                                                                                                                                  \
                {   stopwatch loc_stpowatch(&duration);                                                                                                                                          \
                    for (size_t x = 0; x < num_of_iterations; x++) { func }                                                                                                                           \
                } LOG(Info, message << " => sample count: " << num_of_iterations << " average duration: " << (duration / (f64)num_of_iterations) << DURATION_##profile_duration_precision);  \
            }

		class simple_profiler {
		public:
			
			// Constructs a simple profiler that measures execution time across multiple tests.
			// @param num_of_tests The total number of tests to run before reporting results.
			// @param message The log message prefix displayed when results are reported.
			// @param precision The precision used for measuring durations (microseconds, milliseconds, or seconds).
			simple_profiler(u64 num_of_tests, std::string message, duration_precision precision = duration_precision::milliseconds)
				: m_num_of_tests(num_of_tests), m_message(message), m_precision(precision) {}

			// Adds the current duration measurement to the profiler and calculates averages when complete.
			// @return True if the profiler accepted the value, false if the test limit has been reached.
			bool add_value() {

				if (single_duration == -1.f)
					return true;

				if (m_number_of_values >= m_num_of_tests)
					return false;

				m_durations += single_duration;
				m_number_of_values++;
				if (m_number_of_values >= m_num_of_tests) {

					std::string precision_string;
					switch (m_precision) {
						case duration_precision::microseconds:     precision_string = " microseconds"; break;
						case duration_precision::seconds:          precision_string = " seconds"; break;
						default:
						case duration_precision::milliseconds:     precision_string = " milliseconds"; break;
					}
					LOG(Info, m_message << " => sample count: " << m_number_of_values << " average duration: " << (m_durations / (f64)m_number_of_values) << precision_string);
				}
				return true;
			}

			f32  						single_duration = -1.f;		// Stores the duration of a single measured run.

		private:

			u64                 m_num_of_tests{};   	// Total number of tests to run.
			std::string         m_message{};        	// Message prefix for logging.
			duration_precision  m_precision{};      	// Precision of duration measurements.
			f64                 m_durations{};      	// Accumulated duration sum.
			u32                 m_number_of_values{}; 	// Number of completed tests.
		};

	}
// #else
// 	#define ISOLATED_PROFILER_SCOPED(...)
// 	#define ISOLATED_PROFILER_LOOP(...)
// #endif

	using float_microseconds = std::chrono::duration<double, std::micro>;

	// Stores the result of a single profiling event.
	struct profile_result {
		std::string 				name;    		// Name of the profiled function or block.
		float_microseconds 			start;          // Timestamp when profiling began.
		std::chrono::microseconds 	elapsed_time;   // Duration of the profiled section.
		std::thread::id 			thread_ID;      // Thread on which the profiling occurred.
	};


	// Represents an active profiling session.
	struct instrumentation_session {
		std::string 				name; 			// The session's display name.
	};


	// ==================================================================== instrumentor ====================================================================


    class instrumentor {
    public:
        DELETE_COPY_CONSTRUCTOR(instrumentor);

        
		// Begins a new profiling session, opening a JSON output file to record events.
		// @param name The name of the profiling session.
		// @param directory The directory where the profiling result file will be saved.
		// @param filename The name of the output file (defaults to "result.json").
		void begin_session(const std::string& name, const std::filesystem::path& directory, const std::string& filename = "result.json") {
            std::unique_lock lock(m_mutex);
            
            if (m_current_session) {
                LOG(Error, "Instrumentor::BeginSession(" << name << ") when session [" << m_current_session->name << "] already open");
                internal_end_session();
            }

            if (!std::filesystem::exists(directory)) {
                if (!std::filesystem::create_directory(directory)) {
                    LOG(Error, "Failed to create folder");
                    return;
                }
            }

            std::filesystem::path total_filepath = directory / filename;
            m_output_stream.open(total_filepath);
            
            if (m_output_stream.is_open()) {
                m_current_session = new instrumentation_session{name};
                write_header();
                m_session_active.store(true, std::memory_order_release);
            } else {
                LOG(Error, "Instrumentor could not open file: " << filename);
            }
        }

		// Ends the currently active profiling session, if any, and closes the output file.
		void end_session() {
			std::unique_lock lock(m_mutex);
			internal_end_session();
		}

		// Writes a single profile result into the active profiling session file.
		// @param result The profiling data to be recorded.
        void write_profile(const profile_result& result) {
            // Quick check without locking first
            if (!m_session_active.load(std::memory_order_acquire)) return;
            
            std::stringstream json;
            json << std::setprecision(3) << std::fixed;
            json << ",{";
            json << "\"cat\":\"function\",";
            json << "\"dur\":" << (result.elapsed_time.count()) << ',';
            json << "\"name\":\"" << result.name << "\",";
            json << "\"ph\":\"X\",";
            json << "\"pid\":0,";
            json << "\"tid\":" << result.thread_ID << ",";
            json << "\"ts\":" << result.start.count();
            json << "}";

            std::shared_lock lock(m_mutex);
            if (m_current_session && m_output_stream.is_open()) {
                m_output_stream << json.str();
                m_output_stream.flush();
            }
        }

		// Returns the singleton instance of the instrumentor.
		// @return A reference to the global instrumentor instance.
        static instrumentor& get() {
            static instrumentor instance;
            return instance;
        }

    private:

		// Default constructor. Initializes the instrumentor instance.
		instrumentor() {}

		// Destructor. Ensures any active profiling session is properly ended.
		~instrumentor() {
			end_session();
		}

		// Writes the JSON header for the profiling session output file.
		void write_header() {
            if (m_output_stream.is_open()) {
                m_output_stream << "{\"otherData\": {},\"traceEvents\":[{}";
                m_output_stream.flush();
            }
        }
        
		// Writes the JSON footer to close the profiling session output file.
		void write_footer() {
            if (m_output_stream.is_open()) {
                m_output_stream << "]}";
                m_output_stream.flush();
            }
        }
        
		// Internally handles ending a profiling session and releasing associated resources.
		void internal_end_session() {
            if (m_current_session) {
                write_footer();
                m_output_stream.close();
                delete m_current_session;
                m_current_session = nullptr;
                m_session_active.store(false, std::memory_order_release);
            }
        }

    private:
	
		mutable std::shared_mutex 	m_mutex;            			// Synchronization mutex for thread safety.
		instrumentation_session*  	m_current_session = nullptr; 	// Active profiling session.
		std::ofstream            	m_output_stream;     			// Output stream for writing profiling data.
		std::atomic<bool>        	m_session_active = false; 		// Indicates if a session is active.
	};

	// ==================================================================== instrumentor_timer ====================================================================

	class instrumentor_timer {
	public:

		// Constructs an instrumentor_timer, starting timing immediately.
		// @param name The name of the timed scope or function.
		instrumentor_timer(const char* name)
			: m_name(name), m_stopped(false) {

			m_start_timepoint = std::chrono::steady_clock::now();
		}
		
		// Destructor. Automatically stops timing if it hasn't been stopped already.
		~instrumentor_timer() {

			if (!m_stopped)
				stop();
		}

		// Stops the timer, calculates elapsed time, and records profiling data.
		void stop() {

			auto end_timepoint = std::chrono::steady_clock::now();
			auto high_res_start = float_microseconds{ m_start_timepoint.time_since_epoch() };
			auto elapsed_time = std::chrono::time_point_cast<std::chrono::microseconds>(end_timepoint).time_since_epoch() - std::chrono::time_point_cast<std::chrono::microseconds>(m_start_timepoint).time_since_epoch();

			instrumentor::get().write_profile({ m_name, high_res_start, elapsed_time, std::this_thread::get_id() });
			m_stopped = true;
		}

	private:

		const char* 											m_name;   			// Name of the timed scope or function.
		bool 													m_stopped;       	// Indicates whether the timer has been stopped.
		std::chrono::time_point<std::chrono::steady_clock> 		m_start_timepoint; 	// Start time.
	};



// ==================================== profiler ENABLED ====================================
#if PROFILE

    // Resolves to the compiler-specific macro or built-in variable that represents the current
    #if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__)
	    // function signature. Used by PROFILE_FUNCTION() to automatically name profiled scopes.
        #define FUNC_SIG __PRETTY_FUNCTION__
    #elif defined(__DMC__) && (__DMC__ >= 0x810)
	    // function signature. Used by PROFILE_FUNCTION() to automatically name profiled scopes.
        #define FUNC_SIG __PRETTY_FUNCTION__
    #elif (defined(__FUNCSIG__) || (_MSC_VER))
	    // function signature. Used by PROFILE_FUNCTION() to automatically name profiled scopes.
        #define FUNC_SIG __FUNCSIG__
    #elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || (defined(__IBMCPP__) && (__IBMCPP__ >= 500))
	    // function signature. Used by PROFILE_FUNCTION() to automatically name profiled scopes.
        #define FUNC_SIG __FUNCTION__
    #elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)
	    // function signature. Used by PROFILE_FUNCTION() to automatically name profiled scopes.
        #define FUNC_SIG __FUNC__
    #elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)
	    // function signature. Used by PROFILE_FUNCTION() to automatically name profiled scopes.
        #define FUNC_SIG __func__
    #elif defined(__cplusplus) && (__cplusplus >= 201103)
	    // function signature. Used by PROFILE_FUNCTION() to automatically name profiled scopes.
        #define FUNC_SIG __func__
    #else
	    // If no known compiler signature is found, defaults to "FUNC_SIG unknown!".
        #define FUNC_SIG "FUNC_SIG unknown!"
    #endif

    // Creates a scoped profiling timer for a code block.
    //
    // @param name  The name of the profiling scope.
    // @param line  A unique line identifier used to differentiate timers within the same function.
    //
    // Usage example:
    //     PROFILE_SCOPE_LINE("PhysicsUpdate", __LINE__);
    //
    // Implementation details:
    //   - Defines a constexpr name for the profiling scope.
    //   - Instantiates an AT::instrumentor_timer object, which automatically starts timing.
    //   - When the timer goes out of scope, it stops and records the result.
   	#define PROFILE_SCOPE_LINE(name, line)						constexpr auto fixed_name##line = name;    	AT::instrumentor_timer benchmark_timer##line(fixed_name##line)

    // Begins a new profiling session and writes results to a JSON file.
    //
    // @param name       The name of the profiling session.
    // @param directory  The directory where the profiling result file will be saved.
    // @param filename   (Optional) The JSON filename. Defaults to "result.json".
    //
    // Usage example:
    //     PROFILER_SESSION_BEGIN("GameStartup", "profiling", "startup.json");
   	#define PROFILER_SESSION_BEGIN(name, directory, filename) 	AT::instrumentor::get().begin_session(name, directory, filename)
    
	// Ends the currently active profiling session and closes the profiling result file.
    //
    // Usage example:
    //     PROFILER_SESSION_END();
	#define PROFILER_SESSION_END()                            	AT::instrumentor::get().end_session()
    
	// Creates a profiling timer for current scope that automatically times a section of code.
    //
    // parameter [name] The name to display in the profiling results.
    //
    // Usage example:
    //     PROFILE_SCOPE("Physics Step");
	#define PROFILE_SCOPE(name)                               	PROFILE_SCOPE_LINE(name, __LINE__)
    
	// Automatically profiles the current function using the resolved FUNC_SIG macro.
    //
    // Usage example:
    //     void update() {
    //         PROFILE_FUNCTION();
    //         // Function code...
    //     }
	#define PROFILE_FUNCTION()                                	PROFILE_SCOPE(FUNC_SIG)

    // ------------------------------------ subsystem: application ------------------------------------ 
    #if PROFILE_APPLICATION

        // Profiles a specific named scope in the application subsystem.
        // Usage example:
        //     PROFILE_APPLICATION_SCOPE("App::Init");
        #define PROFILE_APPLICATION_SCOPE(name)                  PROFILE_SCOPE_LINE(name, __LINE__)
		
        // Profiles the entire current function in the application subsystem.
        // Usage example:
        //     void loadAssets() {
        //         PROFILE_APPLICATION_FUNCTION();
        //         ...
        //     }
		#define PROFILE_APPLICATION_FUNCTION()                   PROFILE_SCOPE(FUNC_SIG)

	#else
		// DISABLED, to enable change [PROFILE_APPLICATION] in [util/core_config.h]
        #define PROFILE_APPLICATION_SCOPE(name)
		// DISABLED, to enable change [PROFILE_APPLICATION] in [util/core_config.h]
        #define PROFILE_APPLICATION_FUNCTION()
    #endif

    // ------------------------------------ subsystem: renderer ------------------------------------ 
    #if PROFILE_RENDERER
        
        // Profiles a specific named scope in the renderer subsystem.
        // Usage example:
        //     PROFILE_RENDER_SCOPE("ShadowPass");
		#define PROFILE_RENDER_SCOPE(name)                       PROFILE_SCOPE_LINE(name, __LINE__)
		
        // Profiles the entire current function in the renderer subsystem.
        // Usage example:
        //     void drawFrame() {
        //         PROFILE_RENDER_FUNCTION();
        //         ...
        //     }
		#define PROFILE_RENDER_FUNCTION()                        PROFILE_SCOPE(FUNC_SIG)

	#else
		// DISABLED, to enable change [PROFILE_RENDERER] in [util/core_config.h]
        #define PROFILE_RENDER_SCOPE(name)
		// DISABLED, to enable change [PROFILE_RENDERER] in [util/core_config.h]
        #define PROFILE_RENDER_FUNCTION()
    #endif

// ==================================== profiler DISABLED ====================================
#else
	
	// DISABLED, to enable change [PROFILE] in [util/core_config.h]
	#define PROFILER_SESSION_BEGIN(name, directory, filename)
	// DISABLED, to enable change [PROFILE] in [util/core_config.h]
	#define PROFILER_SESSION_END()
	// DISABLED, to enable change [PROFILE] in [util/core_config.h]
	#define PROFILE_SCOPE(name)
	// DISABLED, to enable change [PROFILE] in [util/core_config.h]
	#define PROFILE_FUNCTION()

	// ------------------------------------ subsystem ------------------------------------ 

	// DISABLED, to enable change [PROFILE] and [PROFILE_APPLICATION] in [util/core_config.h]
	#define PROFILE_APPLICATION_SCOPE(name)
	// DISABLED, to enable change [PROFILE] and [PROFILE_APPLICATION] in [util/core_config.h]
	#define PROFILE_APPLICATION_FUNCTION()
	// DISABLED, to enable change [PROFILE] and [PROFILE_RENDERER] in [util/core_config.h]
	#define PROFILE_RENDER_SCOPE(name)
	// DISABLED, to enable change [PROFILE] and [PROFILE_RENDERER] in [util/core_config.h]
	#define PROFILE_RENDER_FUNCTION()

#endif	// PROFILE

}
