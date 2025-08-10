import os
import subprocess
import platform
from pathlib import Path
import scripts.linux.lin_utils as lin_utils
from .. import utils

class env_configuration:
    # Distro-specific package names
    package_map = {
        "debian": {
            "base_packages": [
                "build-essential", "cmake", "libgl1-mesa-dev", "libglfw3-dev",
                "libglew-dev", "libxinerama-dev", "libxcursor-dev",
                "libxi-dev", "xorg-dev", "pkg-config", "qtbase5-dev", "git"
            ]
        },
        "fedora": {
            "base_packages": [
                "gcc", "gcc-c++", "make", "cmake", "mesa-libGL-devel", "glfw-devel",
                "glew-devel", "libXinerama-devel", "libXcursor-devel",
                "libXi-devel", "xorg-x11-server-devel", "pkgconf", "qt5-qtbase-devel", "git"
            ]
        },
        "arch": {
            "base_packages": [
                "base-devel", "cmake", "mesa", "glfw-x11", "glew", 
                "libxinerama", "libxcursor", "libxi", "xorg-server-devel",
                "pkgconf", "qt5-base", "git"
            ]
        }
    }

    @classmethod
    def validate(cls):
        """Check for required development packages on Linux systems"""
        
        # Skip on Windows
        if platform.system() != "Linux":
            return True
            
        # Detect distro
        distro_id = cls.detect_distro()
        utils.print_c(f"Detected distro ID: {distro_id}", "blue")
        
        packages = cls.get_required_packages(distro_id)
        if not packages:
            utils.print_c(f"Unsupported distro: {distro_id}", "yellow")
            utils.print_c("Please install dependencies manually", "yellow")
            return True
        utils.print_c(f"Required packages: {', '.join(packages)}", "blue")
        
        # Check and install packages
        if not cls.check_packages(packages, distro_id):
            if not cls.install_packages(packages, distro_id):
                return False

        utils.print_c("All required dependencies are installed", "green")
        return True
        
    @classmethod
    def detect_distro(cls):
        """Detect Linux distribution"""
        try:
            if os.path.exists("/etc/os-release"):
                with open("/etc/os-release", "r") as f:
                    os_release = f.read()
                    
                    # First try to get ID_LIKE for family detection
                    id_like = None
                    for line in os_release.splitlines():
                        if line.startswith("ID_LIKE="):
                            id_like = line.strip().split("=")[1].strip('"')
                            break
                    
                    # Then get ID
                    for line in os_release.splitlines():
                        if line.startswith("ID="):
                            distro_id = line.strip().split("=")[1].strip('"')
                            # Prefer ID_LIKE if available (e.g., ubuntu -> debian)
                            if id_like:
                                return id_like.split()[0]  # Take first ID_LIKE
                            return distro_id
                            
        except Exception as e:
            print(f"Error detecting distro: {e}")
        
        # Fallback checks
        if os.path.exists("/etc/arch-release"):
            return "arch"
        if os.path.exists("/etc/centos-release"):
            return "centos"
        if os.path.exists("/etc/fedora-release"):
            return "fedora"
            
        return "unknown"

    @classmethod
    def get_required_packages(cls, distro_id):
        """Get distro-specific packages"""
        # Normalize distro IDs
        distro_id = distro_id.lower()
        
        # Special case for Ubuntu
        if distro_id == "ubuntu":
            return cls.package_map["debian"]["base_packages"]
        
        # Check for exact match first
        if distro_id in cls.package_map:
            return cls.package_map[distro_id].get("base_packages", [])
        
        # Check for family match (e.g., "debian" for "ubuntu")
        for family in ["debian", "fedora", "rhel", "arch"]:
            if family in distro_id:
                return cls.package_map.get(family, {}).get("base_packages", [])
        
        return None

    @classmethod
    def check_packages(cls, packages, distro_id):
        """Check if packages are installed"""
        missing = []
        for pkg in packages:
            if not cls.is_package_installed(pkg, distro_id):
                missing.append(pkg)
                
        if missing:
            utils.print_c(f"Missing packages: {', '.join(missing)}", "yellow")
            return False
        return True

    @classmethod
    def is_package_installed(cls, package, distro_id):
        """Check if a package is installed"""
        if distro_id in ["debian", "ubuntu"]:
            return subprocess.run(
                ["dpkg", "-s", package],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL
            ).returncode == 0
        elif distro_id in ["fedora", "centos", "rhel"]:
            return subprocess.run(
                ["rpm", "-q", package],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL
            ).returncode == 0
        elif distro_id in ["arch", "manjaro"]:
            return subprocess.run(
                ["pacman", "-Q", package],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL
            ).returncode == 0
        return False

    @classmethod
    def install_packages(cls, packages, distro_id):
        """Install missing packages"""
        # Auto-install in CI environments
        if os.getenv("CI") == "true":
            utils.print_c("CI environment detected - auto-installing packages", "blue")
            return cls.install_packages_auto(packages, distro_id)
            
        # Prompt user in interactive mode
        utils.print_c("Packages required for engine development:", "blue")
        for pkg in packages:
            print(f"  - {pkg}")
            
        reply = input("\nInstall missing packages? [Y/n]: ").strip().lower()
        if reply and reply[0] != "y":
            utils.print_c("Package installation aborted by user", "red")
            return False
            
        return cls.install_packages_auto(packages, distro_id)

    @classmethod
    def install_packages_auto(cls, packages, distro_id):
        """Automatically install packages with distro-specific commands"""
        utils.print_c(f"Installing packages: {', '.join(packages)}", "blue")
        
        try:
            if distro_id in ["debian", "ubuntu"]:
                subprocess.run(["sudo", "apt-get", "update"], check=True)
                subprocess.run(["sudo", "apt-get", "install", "-y"] + packages, check=True)
            elif distro_id in ["fedora", "centos", "rhel"]:
                subprocess.run(["sudo", "dnf", "install", "-y"] + packages, check=True)
            elif distro_id in ["arch", "manjaro"]:
                subprocess.run(["sudo", "pacman", "-Syu", "--noconfirm"] + packages, check=True)
            else:
                utils.print_c(f"Unsupported distro for auto-install: {distro_id}", "red")
                return False
                
            utils.print_c("Package installation completed", "green")
            return True
        except subprocess.CalledProcessError as e:
            utils.print_c(f"Package installation failed: {e}", "red")
            return False