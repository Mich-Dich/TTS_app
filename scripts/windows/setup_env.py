import os
import sys
import shutil
import platform
import scripts.windows.win_utils as win_util
from .. import utils

class env_configuration:

    @classmethod
    def validate(cls):
        if platform.system() != "Windows":              # Skip on Linux
            return True
        
        missing_packages = cls.check_packages()
        if missing_packages:
            cls.install_packages(missing_packages)
        return True  # Return True after installing or if nothing was missing

    @classmethod
    def check_packages(cls):
        list_of_packages = []
        # Check for GLEW installation
        target_dir = os.path.abspath("vendor/glew")
        required_files = [
            os.path.join(target_dir, "include", "GL", "glew.h"),
            os.path.join(target_dir, "lib", "Release", "x64", "glew32s.lib")
        ]
        
        # Verify if all required files exist
        missing_files = [f for f in required_files if not os.path.exists(f)]
        if missing_files:
            utils.print_c("GLEW files missing:", "orange")
            for f in missing_files:
                utils.print_c(f"  - {os.path.basename(f)}", "orange")
            list_of_packages.append("glew")
        
        return list_of_packages

    @classmethod
    def install_packages(cls, list_of_packages):
        if "glew" in list_of_packages:
            if not cls.install_glew():
                utils.print_c("GLEW installation failed - setup aborted", "red")
                sys.exit(1)

    @classmethod
    def install_glew(cls, target_dir="vendor/glew"):
        # Convert to absolute path
        target_dir = os.path.abspath(target_dir)
        parent_dir = os.path.dirname(target_dir)
        os.makedirs(parent_dir, exist_ok=True)

        # CORRECTED DOWNLOAD URL (using win32.zip which contains both 32/64 bit)
        url = "https://github.com/nigels-com/glew/releases/download/glew-2.2.0/glew-2.2.0-win32.zip"
        zip_path = os.path.join(parent_dir, "glew-2.2.0-win32.zip")

        # Verify if we have a valid zip file already
        if os.path.exists(zip_path) and not cls.is_valid_zip(zip_path):
            utils.print_c("Deleting invalid zip file", "orange")
            os.remove(zip_path)

        # Download GLEW if needed
        if not os.path.exists(zip_path):
            try:
                utils.print_c("Downloading GLEW...", "blue")
                utils.download_file(url, zip_path)
            except Exception as e:
                utils.print_c(f"Download failed: {e}", "red")
                return False

        # Verify downloaded file
        if not cls.is_valid_zip(zip_path):
            utils.print_c("Downloaded file is not a valid ZIP archive", "red")
            return False

        # Extract files
        try:
            utils.print_c("Extracting GLEW...", "blue")
            win_util.unzip_file(zip_path, deleteZipFile=True)
        except Exception as e:
            utils.print_c(f"Extraction failed: {e}", "red")
            return False

        # Move and rename extracted directory
        extracted_dir = os.path.join(parent_dir, "glew-2.2.0")
        if os.path.exists(target_dir):
            shutil.rmtree(target_dir)
        os.rename(extracted_dir, target_dir)

        # Final verification
        required_files = [
            os.path.join(target_dir, "include", "GL", "glew.h"),
            os.path.join(target_dir, "lib", "Release", "x64", "glew32s.lib")
        ]
        if all(os.path.exists(f) for f in required_files):
            utils.print_c("GLEW installed successfully", "green")
            return True
        
        utils.print_c("GLEW verification failed after installation", "red")
        return False

    @classmethod
    def is_valid_zip(cls, file_path):
        """Check if file is a valid ZIP archive"""
        if not os.path.exists(file_path):
            return False
            
        # Check file size
        if os.path.getsize(file_path) < 4:
            return False
            
        # Check ZIP signature (first 4 bytes: PK\x03\x04)
        try:
            with open(file_path, 'rb') as f:
                return f.read(4) == b'PK\x03\x04'
        except:
            return False