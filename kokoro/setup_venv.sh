#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Create virtual environment with system site packages (for better compatibility)
if [ ! -d "$SCRIPT_DIR/venv" ]; then
    python3 -m venv "$SCRIPT_DIR/venv" --system-site-packages
fi

# Activate virtual environment
source "$SCRIPT_DIR/venv/bin/activate"

# Ensure pip is installed and upgraded
python -m ensurepip
python -m pip install --upgrade pip

# Install dependencies
pip install kokoro-onnx tokenizers onnxruntime numpy soundfile

# Install misaki from GitHub using the venv's pip
pip install "git+https://github.com/thewh1teagle/misaki.git#egg=misaki[en]"

# Create model directories
MODEL_DIR="$SCRIPT_DIR/models"
VOICES_DIR="$SCRIPT_DIR/voices"
mkdir -p "$MODEL_DIR"
mkdir -p "$VOICES_DIR"

# Download model files if they don't exist
if [ ! -f "$MODEL_DIR/kokoro-v1.0.fp16-gpu.onnx" ]; then
    echo "Downloading model..."
    wget https://github.com/thewh1teagle/kokoro-onnx/releases/download/model-files-v1.0/kokoro-v1.0.fp16-gpu.onnx -O "$MODEL_DIR/kokoro-v1.0.fp16-gpu.onnx"
fi

if [ ! -f "$VOICES_DIR/voices-v1.0.bin" ]; then
    echo "Downloading voices..."
    wget https://github.com/thewh1teagle/kokoro-onnx/releases/download/model-files-v1.0/voices-v1.0.bin -O "$VOICES_DIR/voices-v1.0.bin"
fi

echo "Setup complete!"