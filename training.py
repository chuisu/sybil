import h5py
import numpy as np
import tensorflow as tf
from tensorflow.keras.layers import Input, Dense, LSTM, Concatenate
from tensorflow.keras.models import Model
from tensorflow.keras.layers import LSTM

with h5py.File('sybil_features.h5', 'r') as hdf5_file:
    vocal_energy = np.array(hdf5_file['vocal_energy'])
    vocal_mfcc = np.array(hdf5_file['vocal_mfcc'])
    vocal_pitches = np.array(hdf5_file['vocal_pitches'])
    accompaniment_hpcps = np.array(hdf5_file['accompaniment_hpcps'])
    accompaniment_energy = np.array(hdf5_file['accompaniment_energy'])

def create_windows(data, window_size):
    windows = []
    for i in range(len(data) - window_size + 1):
        windows.append(data[i:i + window_size])
    return np.array(windows)

window_size = 12
vocal_energy_windows = create_windows(vocal_energy, window_size)
vocal_mfcc_windows = create_windows(vocal_mfcc, window_size)
vocal_pitches_windows = create_windows(vocal_pitches, window_size)
accompaniment_hpcps_windows = create_windows(accompaniment_hpcps, window_size)
accompaniment_energy_windows = create_windows(accompaniment_energy, window_size)

# Example LSTM implementation
input_accompaniment_hpcp = Input(shape=(window_size, accompaniment_hpcps.shape[-1]))  # Adjust 'window_size' and feature dimensions
input_accompaniment_energy = Input(shape=(window_size, 1))  # Reshaped to 2D

# Process each input with an LSTM layer
lstm_accompaniment_hpcp = LSTM(64)(input_accompaniment_hpcp)
lstm_accompaniment_energy = LSTM(64)(input_accompaniment_energy)

# Combine the outputs from LSTM layers
combined = Concatenate()([lstm_accompaniment_hpcp, lstm_accompaniment_energy])

# Separate output layers for each target feature
# Output layers
output_vocal_energy = Dense(1, activation='linear', name='vocal_energy')(combined)
output_vocal_pitches = Dense(1, activation='linear', name='vocal_pitches')(combined)
output_vocal_mfcc = Dense(13, activation='linear', name='vocal_mfcc')(combined)  # 13 corresponds to the shape of vocal_mfcc

# Create and compile the model
model = Model(inputs=[input_accompaniment_hpcp, input_accompaniment_energy],
              outputs=[output_vocal_energy, output_vocal_pitches, output_vocal_mfcc])

model.compile(optimizer='adam', loss={'vocal_energy': 'mean_squared_error',
                                      'vocal_pitches': 'mean_squared_error',
                                      'vocal_mfcc': 'mean_squared_error'})

# Train the model
model.fit([accompaniment_hpcps_windows, accompaniment_energy_windows],
          {'vocal_energy': vocal_energy_windows, 'vocal_pitches': vocal_pitches_windows, 'vocal_mfcc': vocal_mfcc_windows},
          epochs=10, batch_size=32)
