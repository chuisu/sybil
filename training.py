import h5py
import numpy as np
import tensorflow as tf
from tensorflow.keras.layers import Input, Dense, LSTM, Concatenate, Reshape, Flatten
from tensorflow.keras.models import Model
from tensorflow.keras.layers import LSTM, TimeDistributed

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
print("Shape of vocal_energy_windows:", vocal_energy_windows.shape)

vocal_mfcc_windows = create_windows(vocal_mfcc, window_size)
print("Shape of vocal_mfcc_windows:", vocal_mfcc_windows.shape)

vocal_pitches_windows = create_windows(vocal_pitches, window_size)
print("Shape of vocal_pitches_windows:", vocal_pitches_windows.shape)

accompaniment_hpcps_windows = create_windows(accompaniment_hpcps, window_size)
print("Shape of accompaniment_hpcps_windows:", accompaniment_hpcps_windows.shape)

accompaniment_energy_windows = create_windows(accompaniment_energy, window_size)
print("Shape of accompaniment_energy_windows:", accompaniment_energy_windows.shape)

# Input layers - the shape is now considering windows
input_accompaniment_hpcp = Input(shape=(window_size, accompaniment_hpcps_windows.shape[2]))
input_accompaniment_energy = Input(shape=(window_size, 1))  # 1 feature after windowing

# LSTM layers for each input
lstm_accompaniment_hpcp = LSTM(64, return_sequences=True)(input_accompaniment_hpcp)
lstm_accompaniment_energy = LSTM(64, return_sequences=True)(input_accompaniment_energy)
combined = Concatenate()([lstm_accompaniment_hpcp, lstm_accompaniment_energy])
flattened_output = Flatten()(combined)
lstm_layer_for_mfcc = LSTM(64, return_sequences=True)(combined)

output_vocal_energy = Dense(12, activation='linear', name='vocal_energy')(flattened_output)
output_vocal_pitches = Dense(12, activation='linear', name='vocal_pitches')(flattened_output)
output_vocal_mfcc = TimeDistributed(Dense(13, activation='linear'), name='vocal_mfcc')(lstm_layer_for_mfcc)

# Create and compile the model
model = Model(inputs=[input_accompaniment_hpcp, input_accompaniment_energy],
              outputs=[output_vocal_energy, output_vocal_pitches, output_vocal_mfcc])

model.compile(optimizer='adam', loss={'vocal_energy': 'mean_squared_error',
                                      'vocal_pitches': 'mean_squared_error',
                                      'vocal_mfcc': 'mean_squared_error'})

model.summary()

# Train the model
model.fit([accompaniment_hpcps_windows, accompaniment_energy_windows],
          [vocal_energy_windows, vocal_pitches_windows, vocal_mfcc_windows],
          epochs=10, batch_size=32)

model.save('sybil_model')
