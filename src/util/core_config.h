#pragma once

// -------------------------------------------------------------------------------------------------------------------------------------------
// Defines that have influence on system behavior
// -------------------------------------------------------------------------------------------------------------------------------------------

#define RENDER_API_OPENGL
// #define RENDER_API_VULKAN


// collect timing-data from every magor function?
#define PROFILE								    0	// general

// log assert and validation behaviour?
// NOTE - expr in assert/validation will still be executed
#define ENABLE_LOGGING_FOR_ASSERTS              1
#define ENABLE_LOGGING_FOR_VALIDATION           1
