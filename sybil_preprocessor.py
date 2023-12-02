import sys
import essentia
import essentia.standard as es
import numpy
from essentia.streaming import *
from essentia import Pool, array
import tensorflow as tf
from keras.layers import Dense
from spleeter.separator import Separator
import os
from pathlib import Path
from tensorflow.keras.layers import Activation
from tensorflow.keras import backend as K
from tensorflow.keras.utils import get_custom_objects
import h5py

def custom_loss(y_true, y_pred):
    in_range_mask = tf.logical_and(y_pred >= 130, y_pred <= 1046.5)
    out_of_range_mask = tf.logical_not(in_range_mask)

    zero_mask = tf.equal(y_true, 0.0)
    non_zero_mask = tf.logical_not(zero_mask)

    zero_pred_mask = tf.equal(y_pred, 0.0)
    non_zero_pred_mask = tf.logical_not(zero_pred_mask)

    in_range_error = tf.square(y_true - y_pred) * tf.cast(in_range_mask, dtype=tf.float32)
    out_of_range_error = tf.square(y_true - y_pred) * tf.cast(out_of_range_mask, dtype=tf.float32)

    zero_error = tf.square(y_true - y_pred) * tf.cast(tf.logical_and(zero_mask, non_zero_pred_mask), dtype=tf.float32)
    non_zero_error = tf.square(y_true - y_pred) * tf.cast(tf.logical_and(non_zero_mask, zero_pred_mask), dtype=tf.float32)

    out_of_range_penalty = 1  # increase this value to apply a larger penalty for out-of-range predictions
    zero_penalty = 0.001  # increase this value to apply a larger penalty for non-zero predictions when true value is zero
    non_zero_penalty = 0.1  # increase this value to apply a larger penalty for zero predictions when true value is non-zero

    out_of_range_error *= out_of_range_penalty
    zero_error *= zero_penalty
    non_zero_error *= non_zero_penalty

    return tf.reduce_mean(in_range_error + out_of_range_error + zero_error + non_zero_error)

def sybil_preprocessor(file_for_preprocessing, hdf5_filename):
    audio_file = file_for_preprocessing
    loader = es.MonoLoader(filename=audio_file)
    audio = loader()

    # Estimate beats per minute
    rhythm_descriptors = es.RhythmDescriptors()
    beats_position, confidence, bpm, bpm_estimates, bpm_intervals, first_peak_bpm, first_peak_spread, first_peak_weight, second_peak_bpm, second_peak_spread, second_peak_weight, histogram = rhythm_descriptors(
        audio)
    print('the bpm is: ' + str(bpm))
    sample_rate = 44100
    samples_per_beat = int(sample_rate * 60 / bpm)
    samples_per_sixteenth = samples_per_beat // 4
    # Ensure samples_per_sixteenth is even
    if samples_per_sixteenth % 2 != 0:
        samples_per_sixteenth += 1

    # Frame the audio at the sixteenth note rate
    frames = [frame for frame in
              es.FrameGenerator(audio, frameSize=samples_per_sixteenth, hopSize=samples_per_sixteenth)]

    # So, we need exactly as many members in the final input data arrays and vocal data arrays as there are frames
    # use spleeter to split the vocals and accompaniment
    def check_if_spleeted(track_name, output_dir):
        # Spleeter's standard output format
        vocals_path = os.path.join(output_dir, track_name, 'vocals.wav')
        accompaniment_path = os.path.join(output_dir, track_name, 'accompaniment.wav')

        # Check if both files exist
        if os.path.exists(vocals_path) and os.path.exists(accompaniment_path):
            return True  # Track has been processed
        else:
            return False  # Track has not been processed

    separator = Separator('spleeter:2stems')
    output_folder = '/home/b/Workspace/sybil/spleeted'

    track_name = Path(audio_file).stem

    # Check if the track has been spleeted already
    if not check_if_spleeted(track_name, output_folder):
        separator.separate_to_file(audio_file, output_folder)

    output_file_path_vocals = output_folder + '/' + track_name + '/vocals.wav'
    output_file_path_accompaniment = output_folder + '/' + track_name + '/accompaniment.wav'

    # load up the vocal file
    vocal_data_loader = es.MonoLoader(filename=output_file_path_vocals)
    vocals = vocal_data_loader()

    # Vocal data collection (training data)
    # Per beat, detect note onset and durations, with each node being one sixteenth note
    pitch_algo = es.PitchYinFFT(frameSize=samples_per_sixteenth)

    # Instantiate all the algos for both vocals and accompaniment
    energy_algo = es.Energy()
    mfcc_algo = es.MFCC(numberCoefficients=13)
    chords_detection = es.ChordsDetection()
    window = es.Windowing(type='blackmanharris62')
    spectrum = es.Spectrum()
    spectral_peaks = es.SpectralPeaks(orderBy='magnitude')
    hpcp = es.HPCP()

    # Declare the pitches per sixteenth variable
    vocal_pitches = []
    vocal_energy = []
    vocal_mfcc = []
    vocal_frames = [frame for frame in
                    es.FrameGenerator(vocals, frameSize=samples_per_sixteenth, hopSize=samples_per_sixteenth)]

    # Load up the accompaniment file
    accompaniment_data_loader = es.MonoLoader(filename=output_file_path_accompaniment)
    accompaniment = accompaniment_data_loader()
    accompaniment_frames = [frame for frame in
                            es.FrameGenerator(accompaniment, frameSize=samples_per_sixteenth,
                                              hopSize=samples_per_sixteenth)]
    # Check for frame alignment
    if len(vocal_frames) != len(accompaniment_frames):
        raise ValueError("Frame mismatch: The number of vocal and accompaniment frames does not match.")

    # vocal data collection
    for vocal_frame in vocal_frames:
        # Detect energy
        current_frame_vocal_energy = energy_algo(vocal_frame)
        vocal_energy.append(current_frame_vocal_energy)
        # print("added energy: " + str(current_frame_vocal_energy))
        pitches, _ = pitch_algo(vocal_frame)
        # Check if there is sound happening AND if the pitch is in Tammy's range (between C3 and C6, to be safe)
        if current_frame_vocal_energy > 10 and 130 < pitches < 1046.5:
            vocal_pitches.append(pitches)
            # print("adding pitch: " + str(pitches))
        else:
            vocal_pitches.append(0)
            # print("adding a zero")
        # Add vocal mfccs
        mfcc_bands, mfcc_coeffs = mfcc_algo(vocal_frame)
        vocal_mfcc.append(mfcc_coeffs)
        # print("adding MFCCs: " + str(mfcc_coeffs))
    # print(vocal_energy)
    # print(vocal_pitches)
    # print(vocal_mfcc)

    # Accompaniment data collection
    # (input data) in much the same way as each frame is a sixteenth, and each pitch is
    # assigned per sixteenth, we need to do the same with chords and other data
    # declare the variables that will be appended to
    accompaniment_chords = []
    accompaniment_groove = []
    accompaniment_volume = []
    accompaniment_energy = []
    accompaniment_hpcps = []

    # OKAY!! So here's the bit where we add more data.
    # This data must be sliced properly.
    # Review notes for what the shape of this data should look like
    for accompaniment_frame in accompaniment_frames:
        # Estimate chords and qualities
        accompaniment_frame_windowed = window(accompaniment_frame)
        accompaniment_frame_spectrum = spectrum(accompaniment_frame_windowed)
        frequencies, magnitudes = spectral_peaks(accompaniment_frame_spectrum)
        accompaniment_frame_hpcp = hpcp(frequencies, magnitudes)
        accompaniment_frame_energy = energy_algo(accompaniment_frame)

        accompaniment_hpcps.append(accompaniment_frame_hpcp)
        accompaniment_energy.append(accompaniment_frame_energy)

    numpy.set_printoptions(threshold=sys.maxsize)
    print("accompaniment hpcps: " + str(accompaniment_hpcps))

    # Here, we need to slice the data so that it makes sense going into tensorflow
    # As of 11/13/2023, we no longer need this format of data.
    # Instead, we are going to write our collected data to an hdf5 file.
    # There should be a separate part for training the model
    # (this is better for testing anyway)
    # vocal_data = numpy.array(vocal_pitches)
    # input_data = numpy.array(accompaniment_frame_hpcps)

    # we need to slice the vocal and input data such that they make sense

    # Return vocal data and input data
    # return vocal_data, input_data

# Define the neural network architecture
model = tf.keras.Sequential([
    tf.keras.layers.Dense(512, activation='relu', input_shape=(12,)),
    tf.keras.layers.Dense(256, activation='relu'),
    tf.keras.layers.Dense(128, activation='relu'),
    tf.keras.layers.Dense(64, activation='relu'),
    tf.keras.layers.Dense(32, activation='relu'),
    tf.keras.layers.Dense(16, activation='relu'),
    tf.keras.layers.Dense(1)
])

model.compile(tf.keras.optimizers.Adam(learning_rate=.0005), loss=custom_loss)

# Define learning rate scheduler that changes learning rate as epochs advance
'''def scheduler(epoch, lr):
    if epoch < 10:
        return lr
    else:
        return lr * tf.math.exp(-0.1)

callback = tf.keras.callbacks.LearningRateScheduler(scheduler)'''

# List audio directory
audio_directory = '/home/b/Workspace/sybil-git/sybil/raw_audio/'

# Iterate through audio directory
for idx, filename in enumerate(os.listdir(audio_directory)):
    file_path = os.path.join(audio_directory, filename)
    if os.path.isfile(file_path):
        print(file_path)
        vocal_data, input_data = sybil_preprocessor(file_path)


'''
        # right now, we're just trying to write the db, so we don't need to include this in the loop
        model.fit(input_data, vocal_data, epochs=256) #, callbacks=[callback])

        # we need to save the model as a savedmodel, not an h5
        model.save('sybil.h5')
        model = tf.keras.models.load_model('sybil.h5', custom_objects={'custom_loss': custom_loss})

    # train model with input data using the vocal data as the training set

model = tf.keras.models.load_model('sybil.h5', custom_objects={'custom_loss': custom_loss})

for input_hpcp in input_data:
    input_hpcp = np.expand_dims(input_hpcp, axis=0)
    prediction = model.predict(input_hpcp)
    print(prediction)

'''