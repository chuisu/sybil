import essentia.standard as estd
import sys
import numpy as np
import tensorflow as tf
import math

np.set_printoptions(threshold=sys.maxsize)

def hertz_to_midi(pitch_hz):
    # Converts a pitch value in Hertz to a MIDI note number.
    return 12 * math.log2(pitch_hz / 440) + 69

# Load audio file
loader = estd.MonoLoader(filename='be sweet_rough.mp3')
audio = loader()

# Extract pitch values for the vocals
pitch_extractor = estd.PredominantPitchMelodia()
pitch_values, pitch_confidences = pitch_extractor(audio)

# Build TensorFlow model
model = tf.keras.models.Sequential([
    tf.keras.layers.Dense(64, activation='relu', input_shape=(pitch_values[:-1].shape)),
    tf.keras.layers.Dense(64, activation='relu'),
    tf.keras.layers.Dense(1, activation='sigmoid')
])
model.compile(optimizer='adam', loss='binary_crossentropy', metrics=['accuracy'])

"""
# Train TensorFlow model on pitch values
if len(pitch_values) > 0:
    X = pitch_values[:-1]
    y = pitch_values[1:]
    history = model.fit(X, y, epochs=10)
else:
    print("No pitch values found in the audio file.")

# Make predictions about next note
next_note = model.predict(np.array([pitch_values[-1]]))

print(f"Next note predicted by TensorFlow: {next_note[0][0]:.2f} Hz")
"""