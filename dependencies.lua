
------------ path ------------
vendor_path = {}
vendor_path["glew"]				= "%{wks.location}/vendor/glew"
vendor_path["glfw"]          	= "%{wks.location}/vendor/glfw"
vendor_path["glm"]           	= "%{wks.location}/vendor/glm"
vendor_path["ImGui"]         	= "%{wks.location}/vendor/imgui"
vendor_path["implot"]         	= "%{wks.location}/vendor/implot"
vendor_path["stb_image"]     	= "%{wks.location}/vendor/stb_image"

------------ include ------------ 
IncludeDir = {}
IncludeDir["glew"]              = "%{vendor_path.glew}/include"
IncludeDir["glfw"]              = "%{vendor_path.glfw}"
IncludeDir["glm"]               = "%{vendor_path.glm}"
IncludeDir["ImGui"]             = "%{vendor_path.ImGui}"
IncludeDir["implot"]            = "%{vendor_path.implot}"
IncludeDir["stb_image"]         = "%{vendor_path.stb_image}"
