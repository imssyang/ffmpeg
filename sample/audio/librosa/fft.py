import librosa
import matplotlib.pyplot as plt
import numpy as np


class AudioFeature:
    def __init__(self, path, offset, duration):
        self.path = path
        self.offset = offset
        self.duration = duration
        self.data, self.sample_rate = self._load()

    def _load(self):
        data, sample_rate = librosa.load(
            self.path,
            sr=None,
            offset=self.offset,
            duration=self.duration,
        )
        return data, sample_rate

    def waveshow(self):
        fig = plt.figure(1)
        ax = fig.add_subplot(1,1,1)
        librosa.display.waveshow(self.data, sr=self.sample_rate, ax=ax, marker='')
        plt.show()


if __name__ == "__main__":
    path = "/opt/ffmpeg/sample/audio/librosa/winamp-intro.aac"
    AudioFeature(path, 2, 0.05).waveshow()
