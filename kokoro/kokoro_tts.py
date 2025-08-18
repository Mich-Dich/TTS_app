import os
import time
import traceback
import soundfile as sf
from kokoro_onnx import Kokoro

def generate_tts(text: str, output_path: str, voice: str, speed: float) -> bool:
    try:
        os.makedirs(os.path.dirname(output_path), exist_ok=True)
        base_dir = os.path.dirname(os.path.abspath(__file__))
        model_path = os.path.join(base_dir, "models", "kokoro-v1.0.fp16-gpu.onnx")
        voices_path = os.path.join(base_dir, "voices", "voices-v1.0.bin")
        voice_speed = 1.2

        # Create Kokoro instance
        tts_engine = Kokoro(model_path, voices_path)  # Instantiate here
        print(f"generate audio with voice: \"{voice}\", speed: {speed}")  # Debug
        
        print(f"generate audio with model: {model_path}, voice: \"am_onyx\", speed: {voice_speed}")
        start_time = time.time()
        # Call create() on the INSTANCE
        samples, sample_rate = tts_engine.create(text, voice=voice, speed=speed, lang="en-us")
        sf.write(output_path, samples, sample_rate)
        generation_time = time.time() - start_time

        print(f"Audio generated successfully as {output_path} [Time: {generation_time:.2f}s]")
        return True
    
    except Exception as e:
        print(f"Error during TTS generation: {e}")
        traceback.print_exc()
        return False