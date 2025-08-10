import os
import subprocess
import scripts.utils as utils



def initialize_submodules():
    """Initialize and update all submodules with SSH/HTTPS fallback"""
    
    # Check if we're in CI environment
    is_ci = os.getenv("CI") == "true"
    
    if is_ci:
        # Use HTTPS for submodules in CI
        utils.print_c("CI environment detected, using HTTPS for submodules", "blue")
        try:
            with open('.gitmodules', 'r') as f:
                content = f.read()
            new_content = content.replace('git@github.com:', 'https://github.com/')
            new_content = new_content.replace('ssh://git@github.com/', 'https://github.com/')
            with open('.gitmodules', 'w') as f:
                f.write(new_content)
        except Exception as e:
            utils.print_c(f"Failed to convert URLs: {str(e)}", "red")
            return False

    # Initialize submodules
    try:
        subprocess.run(["git", "submodule", "update", "--init", "--recursive"], check=True)
        utils.print_c("Submodules initialized successfully.", "green")
        return True
    except subprocess.CalledProcessError as e:
        utils.print_c(f"Failed to initialize submodules: {e}", "red")
        return False


def update_submodule(submodule_path, branch="main"):
    print(f"\nUpdating submodule: {os.path.basename(submodule_path)}")
    try:
        # Fetch the latest changes
        subprocess.run(["git", "fetch", "origin"], cwd=submodule_path, check=True)
        
        # Check if branch exists
        branch_check = subprocess.run(["git", "show-ref", "--verify", "--quiet", f"refs/remotes/origin/{branch}"], cwd=submodule_path)
        
        # Use default branch if specified branch doesn't exist
        if branch_check.returncode != 0:
            utils.print_c(f"Branch '{branch}' not found, trying default branch", "yellow")
            result = subprocess.run(["git", "symbolic-ref", "refs/remotes/origin/HEAD"], cwd=submodule_path, stdout=subprocess.PIPE, text=True, stderr=subprocess.DEVNULL )
            default_branch = result.stdout.strip().split('/')[-1] if result.returncode == 0 else "main"
            branch = default_branch
            utils.print_c(f"Using branch: {branch}", "yellow")
        
        # Checkout and update the branch
        subprocess.run(["git", "checkout", branch], cwd=submodule_path, check=True)
        subprocess.run(["git", "pull", "origin", branch], cwd=submodule_path, check=True)
        utils.print_c(f"Successfully updated {submodule_path} to branch '{branch}'.", "green")
        return True

    except subprocess.CalledProcessError as e:
        utils.print_c(f"Failed to update {submodule_path}: {e}", "red")
        return False