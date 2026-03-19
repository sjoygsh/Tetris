"""
setup_assets.py — Generate sound effects and download a font for the Tetris game.

Run this script once before building the game:
    python setup_assets.py

It will:
  1. Generate simple WAV sound effects (move, rotate, drop, clear, gameover)
  2. Download a free font (DejaVu Sans Mono) and save it as assets/font.ttf

No external packages required — uses only Python standard library.
"""

import struct
import math
import os
import urllib.request

ASSETS_DIR = "assets"

def generate_wav(filename, frequency, duration_ms, volume=0.5, wave_type="sine"):
    """
    Generate a simple WAV sound effect file.

    Parameters:
        filename    — output file path (e.g., "assets/move.wav")
        frequency   — tone frequency in Hz (higher = higher pitch)
        duration_ms — duration in milliseconds
        volume      — loudness from 0.0 (silent) to 1.0 (max)
        wave_type   — "sine" for smooth tone, "square" for retro/8-bit tone
    """
    sample_rate = 44100  # Samples per second (CD quality)
    num_samples = int(sample_rate * duration_ms / 1000)
    max_amplitude = int(32767 * volume)  # 16-bit audio max = 32767

    samples = []
    for i in range(num_samples):
        t = i / sample_rate  # Current time in seconds

        # Generate the waveform
        if wave_type == "sine":
            value = math.sin(2 * math.pi * frequency * t)
        elif wave_type == "square":
            value = 1.0 if math.sin(2 * math.pi * frequency * t) >= 0 else -1.0
        else:
            value = 0

        # Apply a fade-out envelope so the sound doesn't click at the end
        fade = 1.0 - (i / num_samples)
        sample = int(value * max_amplitude * fade)
        samples.append(struct.pack('<h', sample))  # '<h' = little-endian 16-bit

    # Build the WAV file
    data = b''.join(samples)
    data_size = len(data)
    bits_per_sample = 16
    num_channels = 1  # Mono
    byte_rate = sample_rate * num_channels * bits_per_sample // 8
    block_align = num_channels * bits_per_sample // 8

    with open(filename, 'wb') as f:
        # WAV header (44 bytes)
        f.write(b'RIFF')
        f.write(struct.pack('<I', 36 + data_size))  # File size - 8
        f.write(b'WAVE')
        f.write(b'fmt ')
        f.write(struct.pack('<I', 16))               # Format chunk size
        f.write(struct.pack('<H', 1))                 # PCM format
        f.write(struct.pack('<H', num_channels))
        f.write(struct.pack('<I', sample_rate))
        f.write(struct.pack('<I', byte_rate))
        f.write(struct.pack('<H', block_align))
        f.write(struct.pack('<H', bits_per_sample))
        f.write(b'data')
        f.write(struct.pack('<I', data_size))
        f.write(data)

    print(f"  Created {filename}")

def download_font():
    """Download DejaVu Sans Mono (free, open-source font) as assets/font.ttf"""
    font_path = os.path.join(ASSETS_DIR, "font.ttf")
    if os.path.exists(font_path):
        print(f"  Font already exists: {font_path}")
        return

    url = "https://github.com/dejavu-fonts/dejavu-fonts/raw/master/ttf/DejaVuSansMono.ttf"
    print(f"  Downloading font from {url}...")
    try:
        urllib.request.urlretrieve(url, font_path)
        print(f"  Saved font to {font_path}")
    except Exception as e:
        print(f"  WARNING: Could not download font: {e}")
        print(f"  Please manually place a .ttf font file at {font_path}")

def main():
    os.makedirs(ASSETS_DIR, exist_ok=True)

    print("Generating sound effects...")
    generate_wav(os.path.join(ASSETS_DIR, "move.wav"),     400, 50,  0.3, "square")
    generate_wav(os.path.join(ASSETS_DIR, "rotate.wav"),   600, 80,  0.3, "sine")
    generate_wav(os.path.join(ASSETS_DIR, "drop.wav"),     150, 150, 0.5, "square")
    generate_wav(os.path.join(ASSETS_DIR, "clear.wav"),    800, 300, 0.4, "sine")
    generate_wav(os.path.join(ASSETS_DIR, "gameover.wav"), 200, 800, 0.5, "sine")

    print("Setting up font...")
    download_font()

    print("\nDone! Assets are ready in the 'assets' folder.")
    print("You can now build and run the game.")

if __name__ == "__main__":
    main()
