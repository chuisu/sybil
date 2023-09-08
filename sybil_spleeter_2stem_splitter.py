#use spleeter to split the vocals and accompaniment
from spleeter.separator import Separator

separator = Separator('spleeter:2stems')
input_audio_file = audio_file
output_folder = '/home/b/Workspace/sybil'
separator.separate_to_file(input_audio_file, output_folder)