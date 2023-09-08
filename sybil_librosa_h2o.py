import librosa
import tensorflow as tf
import numpy as np

# Load audio file
audio_file = "be sweet_rough.mp3"
y, sr = librosa.load(audio_file)

# Compute pitches
pitches, magnitudes = librosa.core.piptrack(y, sr=sr, fmin=librosa.note_to_hz('C2'), fmax=librosa.note_to_hz('C7'))

# Remove unvoiced pitches
pitches[pitches == 0] = np.nan

# Remove NaN values and transpose to get pitches by time
pitches = pitches[~np.isnan(pitches)].reshape(-1, 1)

# Define window size and hop length for feature extraction
window_size = 1024
hop_length = 512

# Extract features from pitches
mfccs = librosa.feature.mfcc(y=y, sr=sr, n_mfcc=13, hop_length=hop_length, n_fft=window_size)

# Prepare data for TensorFlow model
X = mfccs.T[:-1]
y = mfccs.T[1:]

# Define TensorFlow model
model = tf.keras.models.Sequential([
    tf.keras.layers.Dense(128, input_shape=(X.shape[1],)),
    tf.keras.layers.Dense(128, activation='relu'),
    tf.keras.layers.Dense(y.shape[1])
])

# Compile model
model.compile(optimizer='adam', loss='mse')

# Train model
model.fit(X, y, epochs=100)

# Predict next note
input_features = np.expand_dims(mfccs.T[-1], axis=0)
predicted_features = model.predict(input_features)
predicted_pitch = librosa.feature.inverse.mfccs(predicted_features.T, n_mfcc=13, hop_length=hop_length, n_fft=window_size)[0]
predicted_note = librosa.hz_to_note(librosa.core.pitch.tonality_estimation(predicted_pitch))
print(f"Predicted next note: {predicted_note}")