#!/usr/bin/env python3
"""Generate synthesized sound effects for the Star Wars presentation.

Outputs WAV files into resources/. Pure stdlib — no external dependencies.
"""

import math
import os
import random
import struct
import wave

SAMPLE_RATE = 44100
RESOURCES_DIR = os.path.join(os.path.dirname(__file__), '..', 'resources')


def write_wav(filename, samples, sample_rate=SAMPLE_RATE):
    """Write mono 16-bit WAV from a list of floats in [-1, 1]."""
    path = os.path.join(RESOURCES_DIR, filename)
    with wave.open(path, 'w') as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(sample_rate)
        data = b''
        for s in samples:
            clamped = max(-1.0, min(1.0, s))
            data += struct.pack('<h', int(clamped * 32767))
        wf.writeframes(data)
    print(f'  {filename} ({len(samples) / sample_rate:.1f}s, {os.path.getsize(path) // 1024} KB)')


def lowpass(samples, cutoff_hz, sample_rate=SAMPLE_RATE):
    """Simple one-pole low-pass filter."""
    rc = 1.0 / (2.0 * math.pi * cutoff_hz)
    dt = 1.0 / sample_rate
    alpha = dt / (rc + dt)
    out = [0.0] * len(samples)
    out[0] = samples[0] * alpha
    for i in range(1, len(samples)):
        out[i] = out[i - 1] + alpha * (samples[i] - out[i - 1])
    return out


def brown_noise(n, seed=42):
    """Generate brown noise (integrated white noise), normalized."""
    random.seed(seed)
    out = [0.0] * n
    val = 0.0
    for i in range(n):
        val += random.gauss(0, 1)
        out[i] = val
    # Normalize to [-1, 1]
    peak = max(abs(min(out)), abs(max(out)))
    if peak > 0:
        out = [s / peak for s in out]
    return out


def white_noise(n, seed=99):
    """Generate white noise."""
    random.seed(seed)
    return [random.gauss(0, 1) for _ in range(n)]


def envelope(t, attack, sustain_end, release_end):
    """Simple ASR envelope. t in [0, release_end] -> amplitude in [0, 1]."""
    if t < attack:
        return t / attack
    if t < sustain_end:
        return 1.0
    if t < release_end:
        return 1.0 - (t - sustain_end) / (release_end - sustain_end)
    return 0.0


def generate_engine():
    """Spaceflight engine noise — ~8s, loopable.

    Brown noise filtered to a deep rumble + mid-frequency turbulence layer
    with amplitude modulation for a throbbing engine feel.
    """
    duration = 8.0
    n = int(duration * SAMPLE_RATE)

    # Deep rumble layer: brown noise, low-pass ~120 Hz
    rumble_raw = brown_noise(n, seed=42)
    rumble = lowpass(rumble_raw, 120.0)

    # Mid-frequency turbulence: brown noise, band ~200-600 Hz
    # (low-pass at 600, then subtract low-pass at 200)
    turb_raw = brown_noise(n, seed=77)
    turb_hi = lowpass(turb_raw, 600.0)
    turb_lo = lowpass(turb_raw, 200.0)
    turbulence = [turb_hi[i] - turb_lo[i] for i in range(n)]

    # High sizzle: white noise, low-pass ~2000 Hz for texture
    sizzle_raw = white_noise(n, seed=55)
    sizzle = lowpass(sizzle_raw, 2000.0)

    samples = []
    for i in range(n):
        t = i / SAMPLE_RATE

        # Dual LFO for organic pulsing
        lfo1 = 0.65 + 0.35 * math.sin(2.0 * math.pi * 0.25 * t)
        lfo2 = 0.80 + 0.20 * math.sin(2.0 * math.pi * 0.7 * t + 1.2)
        lfo = lfo1 * lfo2

        sample = (rumble[i] * 0.55
                  + turbulence[i] * 0.30
                  + sizzle[i] * 0.08) * lfo

        # Fade tails for seamless looping (0.5s)
        fade_len = 0.5
        if t < fade_len:
            sample *= t / fade_len
        elif t > duration - fade_len:
            sample *= (duration - t) / fade_len

        samples.append(sample * 0.75)

    write_wav('sfx_engine.wav', samples)


def generate_hyperspace_accel():
    """Hyperspace acceleration — ~2.0s.

    Charge-up phase: deep rumble building with rising resonant tones.
    Jump peak: explosive noise burst with high-frequency content.
    Matches ticks 0-75 (charge 0-30, jump 30-75) at ~16-20ms/tick.
    """
    duration = 2.0
    n = int(duration * SAMPLE_RATE)

    # Precompute noise layers
    noise_raw = white_noise(n, seed=200)
    noise_lp = lowpass(noise_raw, 3000.0)
    rumble_raw = brown_noise(n, seed=210)
    rumble = lowpass(rumble_raw, 150.0)

    samples = []
    phase1 = 0.0  # Main sweep
    phase2 = 0.0  # Sub-harmonic
    phase3 = 0.0  # High overtone

    for i in range(n):
        t = i / SAMPLE_RATE
        progress = t / duration

        # Exponential amplitude build: quiet start, explosive peak
        amp = 0.08 + 0.92 * (progress ** 2.5)

        # Frequency sweeps (multiple voices for richness)
        # Main: 80 Hz -> 800 Hz
        freq1 = 80.0 * math.pow(10.0, progress)
        phase1 += 2.0 * math.pi * freq1 / SAMPLE_RATE
        # Sub-harmonic: 40 Hz -> 400 Hz (octave below)
        freq2 = 40.0 * math.pow(10.0, progress)
        phase2 += 2.0 * math.pi * freq2 / SAMPLE_RATE
        # High overtone: 200 Hz -> 2000 Hz
        freq3 = 200.0 * math.pow(10.0, progress)
        phase3 += 2.0 * math.pi * freq3 / SAMPLE_RATE

        tone = (0.30 * math.sin(phase1)
                + 0.20 * math.sin(phase2)
                + 0.15 * math.sin(phase3)
                + 0.10 * math.sin(phase1 * 2.0))  # harmonic

        # Rumble layer (strong at start, fades as sweep takes over)
        rumble_mix = rumble[i] * 0.25 * (1.0 - progress * 0.6)

        # Noise layer (builds exponentially, dominates at peak)
        noise_mix = noise_lp[i] * 0.20 * (progress ** 3.0)

        # Vibrato/instability that increases
        vibrato = 1.0 + 0.02 * progress * math.sin(2.0 * math.pi * 12.0 * t)

        sample = (tone * vibrato + rumble_mix + noise_mix) * amp

        # Quick fade-in (30ms)
        if t < 0.03:
            sample *= t / 0.03

        samples.append(sample * 0.80)

    write_wav('sfx_hyperspace_accel.wav', samples)


def generate_hyperspace_decel():
    """Hyperspace deceleration — ~2.2s.

    Starts at high energy (matching where accel left off), rapidly
    decays with falling pitch and a settling low thump.
    Matches ticks 75-165 (deceleration + settle) at ~16-20ms/tick.
    """
    duration = 2.2
    n = int(duration * SAMPLE_RATE)

    noise_raw = white_noise(n, seed=300)
    noise_lp = lowpass(noise_raw, 2500.0)
    rumble_raw = brown_noise(n, seed=310)
    rumble = lowpass(rumble_raw, 200.0)

    samples = []
    phase1 = 0.0
    phase2 = 0.0

    for i in range(n):
        t = i / SAMPLE_RATE
        progress = t / duration

        # Amplitude: starts loud, rapid exponential decay
        amp = (1.0 - progress) ** 2.0

        # Frequency sweeps down
        # Main: 800 Hz -> 60 Hz
        freq1 = 800.0 * math.pow(60.0 / 800.0, progress)
        phase1 += 2.0 * math.pi * freq1 / SAMPLE_RATE
        # Sub: 400 Hz -> 30 Hz
        freq2 = 400.0 * math.pow(30.0 / 400.0, progress)
        phase2 += 2.0 * math.pi * freq2 / SAMPLE_RATE

        tone = (0.35 * math.sin(phase1)
                + 0.25 * math.sin(phase2)
                + 0.10 * math.sin(phase1 * 2.0))

        # Noise: strong at start, dies quickly
        noise_mix = noise_lp[i] * 0.25 * ((1.0 - progress) ** 3.0)

        # Settling rumble: emerges in the second half
        rumble_amt = 0.0
        if progress > 0.3:
            rumble_amt = rumble[i] * 0.3 * min(1.0, (progress - 0.3) / 0.3) * (1.0 - progress)

        # Low settling thump around 60%
        thump = 0.0
        if 0.4 < progress < 0.7:
            thump_t = (progress - 0.4) / 0.3
            thump = 0.3 * math.sin(2.0 * math.pi * 45.0 * t) * math.sin(math.pi * thump_t)

        sample = (tone + noise_mix + rumble_amt + thump) * amp

        samples.append(sample * 0.75)

    write_wav('sfx_hyperspace_decel.wav', samples)


def generate_outro():
    """Warm ambient pad chord (~3.5s with natural fade-out)."""
    duration = 3.5
    n = int(duration * SAMPLE_RATE)
    samples = []

    # C major chord: C4, E4, G4 with octave below
    freqs = [
        (130.81, 0.20),  # C3
        (261.63, 0.30),  # C4
        (329.63, 0.25),  # E4
        (392.00, 0.20),  # G4
        (523.25, 0.10),  # C5 (soft octave)
    ]

    for i in range(n):
        t = i / SAMPLE_RATE

        # ASR envelope: 0.8s attack, sustain until 1.8s, 1.7s release
        env = envelope(t, 0.8, 1.8, 3.5)

        # Gentle vibrato
        vibrato = 1.0 + 0.003 * math.sin(2.0 * math.pi * 4.5 * t)

        sample = 0.0
        for freq, amp in freqs:
            f = freq * vibrato
            sample += amp * math.sin(2.0 * math.pi * f * t)
            sample += amp * 0.15 * math.sin(2.0 * math.pi * f * 2.0 * t)

        sample *= env
        samples.append(sample * 0.6)

    write_wav('sfx_outro.wav', samples)


if __name__ == '__main__':
    print('Generating sound effects...')
    generate_engine()
    generate_hyperspace_accel()
    generate_hyperspace_decel()
    generate_outro()
    print('Done.')
