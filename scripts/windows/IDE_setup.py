
import os
import sys
import subprocess

def detect_visual_studio_versions():
    vs_version_map = {
        "17": "2022",  # VS 2022 (major version 17)
        "16": "2019",  # VS 2019 (major version 16)
        "15": "2017"   # VS 2017 (major version 15)
    }
    try:
        vswhere_path = os.path.join(os.environ["ProgramFiles(x86)"], "Microsoft Visual Studio", "Installer", "vswhere.exe")
        result = subprocess.run([vswhere_path, "-format", "json", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64"], capture_output=True, text=True)
        if result.returncode == 0:
            import json
            installations = json.loads(result.stdout)
            versions = []
            for install in installations:
                major_version = install["installationVersion"].split('.')[0]
                premake_id = vs_version_map.get(major_version, None)
                if premake_id:
                    versions.append(premake_id)
            return list(set(versions))  # Remove duplicates
    except Exception as e:
        print(f"Error detecting Visual Studio versions: {e}")
    return []


def detect_rider():
    # Check common Rider installation paths
    rider_paths = [
        os.path.join(os.environ["ProgramFiles"], "JetBrains", "JetBrains Rider *"),
        os.path.join(os.environ["LocalAppData"], "JetBrains", "Toolbox", "apps", "Rider", "*")
    ]
    for path in rider_paths:
        if os.path.exists(path):
            return True
    return False


def detect_vscode():
    try:
        subprocess.run(["where", "code"], check=True, capture_output=True)
        return True
    except subprocess.CalledProcessError:
        pass

    # Fallback: check common installation paths
    paths = [
        os.path.join(os.environ.get("ProgramFiles", ""), "Microsoft VS Code", "Code.exe"),
        os.path.join(os.environ.get("LocalAppData", ""), "Programs", "Microsoft VS Code", "Code.exe")
    ]
    return any(os.path.exists(p) for p in paths)


def detect_IDEs():
    IDEs = []
    vs_versions = detect_visual_studio_versions()
    if vs_versions:
        IDEs.extend([f"Visual Studio {version}" for version in vs_versions])

    if detect_vscode():
        IDEs.append("VSCode")

    if detect_rider():
        IDEs.append("JetBrains Rider")

    if not IDEs:
        print("No supported IDEs detected.")
        sys.exit(1)
    
    return IDEs

def prompt_ide_selection():
    
    IDEs = detect_IDEs()
    print("Detected IDEs:")
    for i, ide in enumerate(IDEs):
        print(f"{i}. {ide}")

    if len(IDEs) == 1:
        print("only one IDE detected")
        return IDEs[0]

    choice = input("Select an IDE to use (enter the number): ")
    try:
        choice_index = int(choice)
        if 0 <= choice_index < len(IDEs):
            return IDEs[choice_index]
        else:
            print("Invalid selection.")
            sys.exit(1)
            
    except ValueError:
        print("Invalid input.")
        sys.exit(1)



def setup_vscode_configs(project_root, build_config, application_name):
    vscode_dir = os.path.join(project_root, ".vscode")
    os.makedirs(vscode_dir, exist_ok=True)
    arch = "x86_64"
    system = "windows"
    output_dir = f"{build_config}-{system}-{arch}"
    bin_dir = os.path.join(project_root, "bin", output_dir)
    exe_path = os.path.join(bin_dir, "{application_name}", "{application_name}.exe")

    # Create build.bat script
    build_script_path = os.path.join(vscode_dir, "build.bat")
    build_script_content = f"""@echo off
setlocal

set build_config={build_config}
set timestamp=%date:~-4%-%date:~7,2%-%date:~4,2%-%time:~0,2%-%time:~3,2%-%time:~6,2%
set stage_name={application_name}_%build_config%_%timestamp%
set STAGE_DIR={bin_dir}\\%stage_name%

echo ------ Clearing previous artifacts (trash at: %STAGE_DIR%) ------
mkdir "%STAGE_DIR%" 2>nul

move "{bin_dir}\\{application_name}\\{application_name}.exe" "%STAGE_DIR%\\" 2>nul
move "{bin_dir}\\{application_name}\\{application_name}.pdb" "%STAGE_DIR%\\" 2>nul

rd /s /q "{bin_dir}\\{application_name}" 2>nul
cd "{project_root}"
del /f /q Makefile 2>nul
del /f /q *.make 2>nul

echo ------ Regenerating Makefiles and rebuilding ------
vendor\\premake\\premake5 gmake2

set make_config=%build_config%
set make_config=%make_config: =%
set make_config=%make_config:-=%
set make_config=%make_config:RelWithDebInfo=release_with_debug_info%
set make_config=%make_config:Debug=debug%
set make_config=%make_config:Release=release%

mingw32-make config=%make_config%_x64 -j -k

echo ------ Done ------
endlocal
"""
    with open(build_script_path, "w") as f:
        f.write(build_script_content)

    # Create tasks.json
    tasks_json_path = os.path.join(vscode_dir, "tasks.json")
    tasks_data = {
        "version": "2.0.0",
        "tasks": [
            {
                "label": "Clean & Build",
                "type": "shell",
                "command": "${workspaceFolder}\\.vscode\\build.bat",
                "problemMatcher": [],
                "group": { "kind": "build", "isDefault": True },
                "presentation": {
                    "reveal": "always",
                    "panel": "shared"
                }
            }
        ]
    }
    with open(tasks_json_path, "w") as f:
        json.dump(tasks_data, f, indent=4)

    # Create launch.json
    launch_json_path = os.path.join(vscode_dir, "launch.json")
    cwd_path = os.path.join("${workspaceFolder}", "bin", output_dir, "{application_name}")

    launch_data = {
        "version": "0.2.0",
        "configurations": [
            {
                "name": f"Launch {application_name} ({build_config})",
                "type": "cppvsdbg",
                "request": "launch",
                "program": exe_path,
                "args": [],
                "cwd": cwd_path,
                "preLaunchTask": "Clean & Build",
                "stopAtEntry": False,
                "externalConsole": False,
                "environment": []
            }
        ]
    }
    with open(launch_json_path, "w") as f:
        json.dump(launch_data, f, indent=4)

    # Create c_cpp_properties.json
    config_define = {
        "debug": "DEBUG",
        "release_with_debug_info": "RELEASE_WITH_DEBUG_INFO",
        "release": "RELEASE"
    }.get(build_config.lower(), None)
    defines = ["PLATFORM_WINDOWS"]
    if config_define:
        defines.append(config_define)

    # Try to find MSVC compiler path
    compiler_path = "cl.exe"
    vs_versions = detect_visual_studio_versions()
    if vs_versions:
        vs_version = vs_versions[0]  # Use the first detected version
        compiler_path = f"C:/Program Files/Microsoft Visual Studio/{vs_version}/Community/VC/Tools/MSVC/*/bin/Hostx64/x64/cl.exe"
        compiler_path = glob.glob(compiler_path)[0] if glob.glob(compiler_path) else "cl.exe"

    cpp_props = {
        "version": 4,
        "configurations": [
            {
                "name": f"{build_config} ({system}-{arch})",
                "includePath": [
                    "${workspaceFolder}/**",
                    "${workspaceFolder}/vendor/**"
                ],
                "defines": defines,
                "compilerPath": compiler_path,
                "cStandard": "c11",
                "cppStandard": "c++20",
                "intelliSenseMode": "windows-msvc-x64"
            }
        ]
    }
    cpp_props_path = os.path.join(vscode_dir, "c_cpp_properties.json")
    with open(cpp_props_path, "w") as f:
        json.dump(cpp_props, f, indent=4)

    print("VSCode integration files generated in .vscode/")
    utils.print_c("\nVSCode usage", "blue")
    print("  Build the project:                 Ctrl+Shift+B (runs 'Clean & Build')")
    print("  Launch the application:            F5 (runs the debugger with selected config)")