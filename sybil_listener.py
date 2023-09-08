import essentia
import essentia.standard as es
import pyaudio
import tensorflow as tf
import numpy as np
import sys
import sounddevice as sd
import time
import threading
from main import custom_loss

# global rate in seconds (controls duration of note, time.sleep(), and fade time
rate = 0.3

# setup PyAudio
p = pyaudio.PyAudio()
stream = p.open(format=pyaudio.paFloat32,
                channels=1,
                rate=48000,
                input=True,
                frames_per_buffer=1024,
                input_device_index=13)

rhythm_extractor = es.RhythmExtractor2013()
w = es.Windowing(type='hann')
spectrum = es.Spectrum()
spectral_peaks = es.SpectralPeaks(orderBy='magnitude')
hpcp = es.HPCP()

model = tf.keras.models.load_model('sybil.h5', custom_objects={'custom_loss': custom_loss})
previous_prediction = np.array([[1]])

def apply_crossfade(previous_prediction, crossfade_ratio=0.1):
    # Convert the input data to float
    previous_prediction = previous_prediction.astype(np.float64)

    # Calculate the length of the crossfade
    crossfade_length = int(len(previous_prediction) * crossfade_ratio)

    # Create a linear fade in/out
    fade_in = np.linspace(0, 1, crossfade_length)
    fade_out = np.linspace(1, 0, crossfade_length)

    # Apply the crossfade
    previous_prediction[:crossfade_length] *= fade_in
    previous_prediction[-crossfade_length:] *= fade_out

    return previous_prediction

def apply_fade(sound, sample_rate, start_volume=0.5, end_volume=0.5):
    fade_duration = rate/2  # in seconds
    fade_samples = int(fade_duration * sample_rate)

    fade_in = np.linspace(start_volume, 1, fade_samples)
    fade_out = np.linspace(1, end_volume, fade_samples)

    sound[:fade_samples] *= fade_in
    sound[-fade_samples:] *= fade_out
    return sound

while True:
    # read audio samples from the microphone
    buffer = stream.read(1024, exception_on_overflow=False)
    audio_samples = np.frombuffer(buffer, dtype=np.float32)
    np.set_printoptions(threshold=sys.maxsize)
    windowed = w(audio_samples)
    bpm, ticks, confidence, estimates, bpmIntervals = rhythm_extractor(windowed)
    spec = spectrum(windowed)
    frequencies, magnitudes = spectral_peaks(spec)
    hpcps = hpcp(frequencies, magnitudes)
    hpcps = np.expand_dims(hpcps, axis=0)
    prediction = model.predict(hpcps)

    if prediction < 130:
        prediction = previous_prediction

    print(prediction)
    freq = prediction
    duration = rate
    sample_rate = 44100
    t = np.arange(int(duration * sample_rate)) / sample_rate
    sound = np.concatenate([np.sin(2 * np.pi * freq * t) for freq in prediction])
    sound = sound.astype('float64')
    sound *= 32767 / np.max(np.abs(sound))
    sound = sound.astype(np.int16)
    crossfaded_note = apply_crossfade(previous_prediction)
    crossfaded_note = crossfaded_note.astype(np.int16)
    sd.play(crossfaded_note, blocking=True)
    previous_prediction = prediction
    time.sleep(rate)
