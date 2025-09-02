#pragma once

struct GLFWwindow;
struct VkExtent2D;
struct VkInstance_T;
struct VkSurfaceKHR_T;

class event;
class application;



namespace AT {

	using EventCallbackFn = std::function<void(event&)>;
	
	// Represents the current size state of a window.
	enum class window_size_state {
		windowed,             // Default windowed mode.
		minimized,            // Window minimized to taskbar/dock.
		fullscreen,           // True fullscreen mode.
		fullscreen_windowed,  // Borderless fullscreen (same as monitor size).
	};
	
	// Holds attributes and metadata for window creation and management.
	struct window_attrib {

		std::string title;                 // Title of the window.
		u32 pos_x = 100;                   // Initial X position of the window.
		u32 pos_y = 100;                   // Initial Y position of the window.
		f64 cursor_pos_x{};                // Last known X position of the cursor.
		f64 cursor_pos_y{};                // Last known Y position of the cursor.
		u32 width{};                       // Width of the window in pixels.
		u32 height{};                      // Height of the window in pixels.
		bool vsync = false;                // Whether VSync is enabled.
		application* app_ref{};            // Reference to the owning application.
		window_size_state size_state = window_size_state::windowed; // Current window size state.

		EventCallbackFn event_callback;    // Callback function to handle events.

		// Constructs a window_attrib instance with default or user-defined values.
		// @param title Window title.
		// @param width Window width in pixels.
		// @param height Window height in pixels.
		// @param vsync Whether to enable VSync.
		// @param callback Event callback function for handling events.
		window_attrib(const std::string title = "Application Template", const u32 width = 1600, const  u32 height = 900, const  bool vsync = false, const EventCallbackFn& callback = nullptr)
			: title(title), width(width), height(height), vsync(vsync), event_callback(callback){}
	};
	
	// ==============================================================  window ==============================================================
	
	class window {
	public:
	
		using EventCallbackFn = std::function<void(event&)>;

		// Constructs a new window with the specified attributes.
		// Initializes GLFW if needed, creates the window, sets up icons, monitors,
		// and event callbacks.
		// @param attributes Initial window attributes (title, size, vsync, etc.).
		window(window_attrib attributes = window_attrib());

		// Destroys the window, saves its last position, and shuts down GLFW if needed.
		~window();
	
		// Delete copy constructor to avoid accidental window duplication.
		DELETE_COPY_CONSTRUCTOR(window);

		// Enables or disables VSync.
		// @param vsync True to enable VSync, false to disable.
		GETTER_SETTER_C(bool, vsync, m_data.vsync)

		// Returns the current width of the window.
		// @return Window width in pixels.
		GETTER_C(u32, width, m_data.width)

		// Returns the current height of the window.
		// @return Window height in pixels.
		GETTER_C(u32, height, m_data.height)
		
		// Returns the current window attributes.
		// @return A copy of the window_attrib struct.
		FORCEINLINE window_attrib get_attributes() const { return m_data; }

		// Returns the raw GLFW window pointer.
		// @return A pointer to the GLFWwindow object.
		FORCEINLINE GLFWwindow* get_window() const { return m_Window; }

		// Sets the callback function used to handle window events.
		// @param callback Function to call when events occur.
		FORCEINLINE void set_event_callback(const EventCallbackFn& callback) { m_data.event_callback = callback; }

		// Returns the last known cursor X position relative to the window.
		// @return Cursor X coordinate.
		FORCEINLINE f64 get_cursor_pos_x() const { return m_data.cursor_pos_x; }

		// Returns the last known cursor Y position relative to the window.
		// @return Cursor Y coordinate.
		FORCEINLINE f64 get_cursor_pos_y() const { return m_data.cursor_pos_y; }

		// Returns the current window size state (windowed, fullscreen, etc.).
		// @return The current window_size_state.
		FORCEINLINE window_size_state get_window_size_state() const { return m_data.size_state; }

		// Retrieves the size of the window’s framebuffer in pixels.
		// @param width Output reference for framebuffer width.
		// @param height Output reference for framebuffer height.
		void get_framebuffer_size(int& width, int& height);

		// Shows or hides the window.
		// @param show True to show, false to hide.
		void show_window(bool show);

		// Checks whether the window is currently maximized.
		// @return True if the window is maximized, false otherwise.
		bool is_maximized();

		// Minimizes the window to the taskbar/dock.
		// @return None.
		void minimize_window();

		// Restores the window from minimized or maximized state.
		// @return None.
		void restore_window();

		// Maximizes the window to fill the screen.
		// @return None.
		void maximize_window();

		// Shows or hides the window’s title bar.
		// @param show True to show, false to hide.
		void show_titlebar(bool show);

		// Retrieves the size of the monitor the window is displayed on.
		// @param width Output pointer for monitor width.
		// @param height Output pointer for monitor height.
		void get_monitor_size(int* width, int* height);

		// Retrieves the current mouse position relative to the client area.
		// @param pos Output reference storing mouse position as a glm::vec2.
		void get_mouse_position(glm::vec2& pos);

	#if defined(RENDER_API_VULKAN)
		// Creates a Vulkan surface for rendering to this window.
		// @param instance Vulkan instance.
		// @param get_surface Output pointer to the created Vulkan surface.
		void create_vulkan_surface(VkInstance_T* instance, VkSurfaceKHR_T** get_surface);
	#endif
	
		// Returns the size of the window as a glm::ivec2.
		// @return Window size in pixels.
		glm::ivec2 get_extend();

		// Checks if the window should close.
		// @return True if the user requested the window to close, false otherwise.
		bool should_close();

		// Processes window events by polling GLFW and executing queued custom events.
		// @return None.
		void poll_events();

		// Captures the mouse cursor, locking it to the window.
		// @return None.
		void capture_cursor();

		// Releases the mouse cursor, making it free to move outside the window.
		// @return None.
		void release_cursor();

		// Queues a custom event or function to be executed during event polling.
		// @tparam Func Callable object type.
		// @param func Function to queue for execution.
		template<typename Func>
		void queue_event(Func&& func) 	{ m_event_queue.push(func); }
	
	private:
	
		// Binds all GLFW event callbacks to the window.
		// This function sets up handling for resize, focus, close, mouse, and key events.
		// @return None.
		void bind_event_callbacks();
	
		std::mutex 							m_event_queue_mutex;	// Mutex protecting the event queue.
		std::queue<std::function<void()>> 	m_event_queue;         	// Custom event execution queue.
		std::filesystem::path 				m_icon_path;            // Path to the window icon.
		window_attrib 						m_data{};     			// Stores all attributes of the window.
		GLFWwindow* 						m_Window{};    			// Pointer to the GLFW window object.
	};

}	
