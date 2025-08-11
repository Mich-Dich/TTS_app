import os
import numpy as np
import soundfile as sf
from kokoro_onnx import Kokoro
from misaki import en, espeak  # For phoneme conversion

def generate_tts(text: str, output_path: str) -> bool:
    try:
        # Ensure output directory exists
        os.makedirs(os.path.dirname(output_path), exist_ok=True)
        
        base_dir = os.path.dirname(os.path.abspath(__file__))
        model_path = os.path.join(base_dir, "models", "kokoro-v1.0.fp16-gpu.onnx")
        voices_path = os.path.join(base_dir, "voices", "voices-v1.0.bin")
        
        # Initialize Kokoro with model paths
        model = Kokoro(model_path, voices_path)
        
        # Set up G2P (Grapheme-to-Phoneme) with espeak fallback
        fallback = espeak.EspeakFallback(british=False)
        g2p = en.G2P(trf=False, british=False, fallback=fallback)
        
        # Convert text to phonemes
        phonemes, _ = g2p(text)
        
        # Generate TTS audio
        samples, sample_rate = model.create(phonemes, "af_heart", is_phonemes=True)
        
        # Save audio
        sf.write(output_path, samples, sample_rate)
        return True
    except Exception as e:
        print(f"Error during TTS generation: {e}")
        return False
