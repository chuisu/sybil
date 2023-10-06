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
rhythm_extractor = es.RhythmExtractor2013()
w = es.Windowing(type='hann')
spectrum = es.Spectrum()
spectral_peaks = es.SpectralPeaks(orderBy='magnitude')
hpcp = es.HPCP()

# model = tf.keras.models.load_model('sybil.h5', custom_objects={'custom_loss': custom_loss})
model = tf.saved_model.load('sybil_model')
infer = model.signatures["serving_default"]
previous_prediction = np.array([[1]])

def audio_callback(indata, frames, time, status):
    np.set_printoptions(threshold=sys.maxsize)
    audio_samples = indata[:, 0]
    windowed = w(audio_samples)
    # windowed = w(indata[:, 0])  # take the first channel
    bpm, ticks, confidence, estimates, bpmIntervals = rhythm_extractor(windowed)
    spec = spectrum(windowed)
    frequencies, magnitudes = spectral_peaks(spec)
    hpcps = hpcp(frequencies, magnitudes)
    hpcps = np.expand_dims(hpcps, axis=0)
    #prediction = model.predict(hpcps)
    input_tensor = tf.convert_to_tensor(hpcps, dtype=tf.float32)
    predictions = infer(input_tensor)
    print(predictions.keys())
    prediction = predictions['dense_6'].numpy()

    if prediction < 130:
        prediction = previous_prediction

    print(prediction)
    freq = prediction
    t = np.linspace(0, rate, int(44100 * rate), endpoint=False)
    sound = np.sin(2 * np.pi * freq * t).T
    sound = sound.ravel()
    sound *= 32767 / np.max(np.abs(sound))
    sound = sound.astype(np.int16)
    sd.play(sound)
    # time.sleep(rate)

# Start the streaming with sounddevice
with sd.InputStream(samplerate=44100, channels=1, blocksize=1024, callback=audio_callback):
    print("Press Ctrl+C to stop the stream")
    sd.sleep(1000000000)  # This keeps the streaming active. Adjust sleep duration as needed.

'''
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
'''