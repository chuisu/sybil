import h5py
import numpy as np
import tensorflow as tf
from tensorflow.keras.layers import Input, Dense, LSTM, Concatenate
from tensorflow.keras.models import Model


with h5py.File('sybil_features.h5', 'r') as hdf5_file:
    vocal_energy = np.array(hdf5_file['vocal_energy'])
    vocal_mfcc = np.array(hdf5_file['vocal_mfcc'])
    vocal_pitches = np.array(hdf5_file['vocal_pitches'])
    accompaniment_hpcps = np.array(hdf5_file['accompaniment_hpcps'])
    accompaniment_energy = np.array(hdf5_file['accompaniment_energy'])

# Input layers
input_accompaniment_hpcp = Input(shape=(accompaniment_hpcps.shape[1],))
# Ensure accompaniment_energy is a 2D array, e.g., reshape if necessary
if accompaniment_energy.ndim == 1:
    accompaniment_energy = accompaniment_energy.reshape(-1, 1)
input_accompaniment_energy = Input(shape=(accompaniment_energy.shape[1],))

# Shared layers
shared = Concatenate()([input_accompaniment_hpcp, input_accompaniment_energy])
shared = Dense(128, activation='relu')(shared)

# Separate output layers for each target feature
# Ensure accompaniment_energy is a 2D array, e.g., reshape if necessary
if vocal_energy.ndim == 1:
    vocal_energy = vocal_energy.reshape(-1, 1)
output_vocal_energy = Dense(vocal_energy.shape[1], activation='linear', name='vocal_energy')(shared)
if vocal_pitches.ndim == 1:
    vocal_pitches = vocal_pitches.reshape(-1, 1)
output_vocal_pitches = Dense(vocal_pitches.shape[1], activation='linear', name='vocal_pitches')(shared)
output_vocal_mfcc = Dense(vocal_mfcc.shape[1], activation='linear', name='vocal_mfcc')(shared)

# Create and compile the model
model = Model(inputs=[input_accompaniment_hpcp, input_accompaniment_energy],
              outputs=[output_vocal_energy, output_vocal_pitches, output_vocal_mfcc])

model.compile(optimizer='adam', loss={'vocal_energy': 'mean_squared_error',
                                      'vocal_pitches': 'mean_squared_error',
                                      'vocal_mfcc': 'mean_squared_error'})

# Train the model
model.fit([accompaniment_hpcps, accompaniment_energy],
          {'vocal_energy': vocal_energy, 'vocal_pitches': vocal_pitches, 'vocal_mfcc': vocal_mfcc},
          epochs=10, batch_size=32)