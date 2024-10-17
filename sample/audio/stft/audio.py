import os
import librosa
import matplotlib.pyplot as plt
from matplotlib.ticker import FormatStrFormatter
import numpy as np
import soundfile


class BaseDomain:
    def __init__(self, path, offset, duration):
        self.path = path
        self.offset = offset
        self.duration = duration
        self.data, self.sample_rate = self.load()
        print("load:", path, len(self.data), self.sample_rate)

    def load(self):
        data, sample_rate = librosa.load(
            self.path,
            sr=None,
            offset=self.offset,
            duration=self.duration,
        )
        return data, sample_rate

    def save(self, data, extra_name, output_ext='.wav'):
        orig_base, _ = os.path.splitext(self.path)
        offset_hold = f'-{self.offset}' if self.offset else str()
        duration_hold = f'-{self.duration}' if self.duration else str()
        output_path = f'{orig_base}{extra_name}{offset_hold}{duration_hold}{output_ext}'
        soundfile.write(output_path, data, self.sample_rate)
        print('save:', output_path, len(data))


class TimeDomain(BaseDomain):
    def __init__(self, path, offset=0.0, duration=None):
        super().__init__(path, offset, duration)

    def show(self):
        self.save(self.data, '-orig')
        fig = plt.figure(1)
        ax = fig.add_subplot(1,1,1)
        librosa.display.waveshow(self.data, sr=self.sample_rate, ax=ax, marker='')
        plt.show()

    def trim(self, top_db):
        trim_data, mute_index = librosa.effects.trim(self.data, top_db=top_db)
        self.save(trim_data, '-trim')
        print("trim:", len(trim_data), mute_index)

        fig, axs = plt.subplots(nrows=2, ncols=1)
        librosa.display.waveshow(self.data, sr=self.sample_rate, ax=axs[0])
        axs[0].vlines(mute_index[0]/self.sample_rate, -0.5, 0.5, colors='r')
        axs[0].vlines(mute_index[1]/self.sample_rate, -0.5, 0.5, colors='r')
        librosa.display.waveshow(trim_data, sr=self.sample_rate, ax=axs[1])
        plt.show()

    def remix(self, top_db):
        non_silent_intervals = librosa.effects.split(self.data, top_db=top_db)
        remix_data = librosa.effects.remix(self.data, non_silent_intervals)
        self.save(remix_data, '-remix')
        print("remix:", non_silent_intervals, len(remix_data))

        fig, axs = plt.subplots(nrows=2, ncols=1, sharex=True, sharey=True)
        librosa.display.waveshow(self.data, sr=self.sample_rate, ax = axs[0])
        librosa.display.waveshow(remix_data, sr=self.sample_rate, ax = axs[1],
            offset=non_silent_intervals[0][0]/self.sample_rate)
        for non_silent_interval in non_silent_intervals:
            axs[0].vlines(non_silent_interval[0]/self.sample_rate,-0.5,0.5,colors='r')
            axs[0].vlines(non_silent_interval[1]/self.sample_rate,-0.5,0.5,colors='r')
        plt.show()


class FrequencyDomain(BaseDomain):
    def __init__(self, path, offset=0.0, duration=None):
        super().__init__(path, offset, duration)

    def stft(self):
        win_time = 25 # 25ms
        hop_time = 10 # 10ms
        win_length = int(win_time * self.sample_rate / 1000)
        hop_length = int(hop_time * self.sample_rate / 1000)
        n_fft = int(2**np.ceil(np.log2(win_length)))
        D_matrix = librosa.stft(self.data, n_fft=n_fft, hop_length=hop_length, win_length=win_length)
        S_amplitude = np.abs(D_matrix)
        S_db = librosa.amplitude_to_db(S_amplitude, ref=np.max)
        D, N = S_db.shape
        print('STFT-param', len(self.data), n_fft, win_length, hop_length)
        print('STFT-D', D, len(D_matrix), n_fft/2 + 1)   # frequency feature
        print('STFT-N', N, len(self.data) / hop_length)  # number of frame
        print('STFT-amplitude', S_amplitude.shape)
        print('STFT-db', S_db.shape)

        fig, axs = plt.subplots(nrows=1, ncols=3, sharex=False, sharey=False)
        axs[0].imshow(S_amplitude, origin='lower', cmap='hot')
        range_D = np.arange(0, D, 100)
        range_N = np.arange(0, N, 100)
        range_time = range_N * (hop_length / self.sample_rate)
        range_freq = range_D * (self.sample_rate / n_fft / 1000)
        axs[1].set_xticks(range_N, range_time, rotation=0)
        axs[1].set_yticks(range_D, range_freq)
        axs[1].xaxis.set_major_formatter(FormatStrFormatter('%.0f'))
        axs[1].yaxis.set_major_formatter(FormatStrFormatter('%.0f'))
        axs[1].imshow(S_db, origin='lower', cmap='hot')
        img = librosa.display.specshow(S_db, y_axis='linear', x_axis='time', hop_length=hop_length, sr=self.sample_rate, ax=axs[2])
        plt.colorbar(img, ax=axs[2], format="%+0.f dB")
        plt.show()

    def preemphasis(self):
        data_emphasis = librosa.effects.preemphasis(self.data)
        self.save(data_emphasis, '-emphasis')

        win_time = 25 # 25ms
        hop_time = 10 # 10ms
        win_length = int(win_time * self.sample_rate / 1000)
        hop_length = int(hop_time * self.sample_rate / 1000)
        n_fft = int(2**np.ceil(np.log2(win_length)))

        D_matrix = librosa.stft(self.data, n_fft=n_fft, hop_length=hop_length, win_length=win_length)
        S_db = librosa.amplitude_to_db(np.abs(D_matrix))

        D_emphasis = librosa.stft(data_emphasis, n_fft=n_fft, hop_length=hop_length, win_length=win_length)
        S_emphasis = librosa.amplitude_to_db(np.abs(D_emphasis))

        fig, axs = plt.subplots(2, 1, sharex=True, sharey=True)
        axs[0].set(title='Original signal')
        librosa.display.specshow(S_db, sr=self.sample_rate, hop_length=hop_length, y_axis='linear', x_axis='time', ax=axs[0])
        axs[1].set(title='Pre-emphasis signal')
        img = librosa.display.specshow(S_emphasis, sr=self.sample_rate, hop_length=hop_length, y_axis='linear', x_axis='time', ax=axs[1])
        fig.colorbar(img, ax=axs, format="%+2.f dB")
        plt.show()

    def melspectrogram(self):
        win_time = 25 # 25ms
        hop_time = 10 # 10ms
        win_length = int(win_time * self.sample_rate / 1000)
        hop_length = int(hop_time * self.sample_rate / 1000)
        n_fft = int(2**np.ceil(np.log2(win_length)))
        D_matrix = librosa.stft(self.data, n_fft=n_fft, hop_length=hop_length, win_length=win_length)
        S_db = librosa.amplitude_to_db(np.abs(D_matrix))

        n_mels= 40
        Mel_matrix = librosa.filters.mel(sr=self.sample_rate, n_fft=n_fft, n_mels=n_mels, htk=True)
        Mel_fbank = librosa.feature.melspectrogram(
            y=self.data,
            sr=self.sample_rate,
            n_fft=n_fft,
            win_length=win_length,
            hop_length=hop_length,
            n_mels=n_mels,
            #S=np.abs(D_matrix),
        )
        Mel_db = librosa.power_to_db(Mel_fbank, ref=np.max)
        print(Mel_matrix.shape)
        print(Mel_fbank.shape)

        fig, axs = plt.subplots(nrows=1, ncols=3, sharex=False, sharey=False)
        axs[0].set(title='STFT spectrogram')
        librosa.display.specshow(S_db, sr=self.sample_rate, hop_length=hop_length, y_axis='linear', x_axis='time', ax=axs[0])
        axs[1].set(title='Mel-filters')
        x = np.arange(Mel_matrix.shape[1]) * self.sample_rate / n_fft
        axs[1].plot(x, Mel_matrix.T)
        img = librosa.display.specshow(Mel_db, x_axis='time', y_axis='mel', sr=self.sample_rate, fmax=self.sample_rate/2, ax=axs[2])
        fig.colorbar(img, ax=axs[2], format='%+2.0f dB')
        axs[2].set(title='Mel-frequency spectrogram')
        plt.show()

    def mfcc(self):
        win_time = 25 # 25ms
        hop_time = 10 # 10ms
        win_length = int(win_time * self.sample_rate / 1000)
        hop_length = int(hop_time * self.sample_rate / 1000)
        n_fft = int(2**np.ceil(np.log2(win_length)))
        n_mels = 128
        n_mfcc = 20
        Mfcc_1 = librosa.feature.mfcc(
            y=self.data,
            sr=self.sample_rate,
            n_mfcc=n_mfcc,
            n_mels=n_mels,
            n_fft=n_fft,
            win_length=win_length,
            hop_length=hop_length,
        )
        print(Mfcc_1.shape)

        Mel_fbank = librosa.feature.melspectrogram(
            y=self.data,
            sr=self.sample_rate,
            n_fft=n_fft,
            win_length=win_length,
            hop_length=hop_length,
            n_mels=n_mels,
        )
        Mel_db = librosa.power_to_db(Mel_fbank, ref=np.max)
        Mfcc_2 = librosa.feature.mfcc(
            S=Mel_db,
            sr=self.sample_rate,
            n_mfcc=n_mfcc,
        )
        print(Mel_fbank.shape)
        print(Mfcc_2.shape)

        fig, axs = plt.subplots(1, 2, sharex=True, sharey=True)
        axs[0].set(title='Mel-frequency cepstral coefficients (MFCC-1)')
        librosa.display.specshow(Mfcc_1, sr=self.sample_rate, hop_length=hop_length, y_axis='linear', x_axis='time', ax=axs[0])
        axs[1].set(title='Mel-frequency cepstral coefficients (MFCC-2)')
        img = librosa.display.specshow(Mfcc_2, sr=self.sample_rate, hop_length=hop_length, y_axis='linear', x_axis='time', ax=axs[1])
        fig.colorbar(img, ax=axs, format="%+2.f dB")
        plt.show()

    def mfcc_dct(self):
        win_length = 512
        hop_length = 160
        n_fft = 512
        n_mels = 128
        n_mfcc = 20
        Mfcc_1 = librosa.feature.mfcc(
            y=self.data,
            sr=self.sample_rate,
            n_mfcc=n_mfcc,
            win_length=win_length,
            hop_length=hop_length,
            n_fft=n_fft,
            n_mels=n_mels,
            dct_type=1,
        )
        Mfcc_2 = librosa.feature.mfcc(
            y=self.data,
            sr=self.sample_rate,
            n_mfcc=n_mfcc,
            win_length=win_length,
            hop_length=hop_length,
            n_fft=n_fft,
            n_mels=n_mels,
            dct_type=2,
        )
        Mfcc_3 = librosa.feature.mfcc(
            y=self.data,
            sr=self.sample_rate,
            n_mfcc=n_mfcc,
            win_length=win_length,
            hop_length=hop_length,
            n_fft=n_fft,
            n_mels=n_mels,
            dct_type=3,
        )
        fig, axs = plt.subplots(nrows=3, sharex=True, sharey=True)
        img1 = librosa.display.specshow(Mfcc_1, x_axis='time',ax = axs[0])
        axs[0].set_title("DCT type 1")
        fig.colorbar(img1,ax=axs[0])
        img2 = librosa.display.specshow(Mfcc_2, x_axis='time',ax=axs[1])
        axs[1].set_title("DCT type 2")
        fig.colorbar(img2,ax=axs[1])
        img3 = librosa.display.specshow(Mfcc_3, x_axis='time',ax = axs[2])
        axs[2].set_title("DCT type 3")
        fig.colorbar(img3,ax=axs[2])
        plt.show()

    def mfcc_delta(self):
        win_length =512
        hop_length = 160
        n_fft = 512
        n_mels = 128
        n_mfcc = 20
        Mfcc = librosa.feature.mfcc(
            y=self.data,
            sr=self.sample_rate,
            n_mfcc=n_mfcc,
            win_length=win_length,
            hop_length=hop_length,
            n_fft=n_fft,
            n_mels=n_mels,
            dct_type=1,
        )
        mfcc_deta =  librosa.feature.delta(Mfcc)         # 一阶差分
        mfcc_deta2 = librosa.feature.delta(Mfcc,order=2) # 二阶差分
        mfcc_d1_d2 = np.concatenate([Mfcc, mfcc_deta, mfcc_deta2], axis=0) # 特征拼接
        fig = plt.figure()
        img = librosa.display.specshow(mfcc_d1_d2, x_axis='time',hop_length=hop_length, sr=self.sample_rate)
        fig.colorbar(img)
        plt.show()


if __name__ == "__main__":
    path = "/opt/ffmpeg/sample/audio/stft/winamp-intro-orig.wav"
    #TimeDomain(path, 2, 0.05).show()
    #TimeDomain(path).show()
    #TimeDomain(path).trim(top_db=30)
    #TimeDomain(path).remix(top_db=30)
    #FrequencyDomain(path).stft()
    #FrequencyDomain(path).preemphasis()
    #FrequencyDomain(path).melspectrogram()
    FrequencyDomain(path).mfcc()
    #FrequencyDomain(path).mfcc_dct()
    #FrequencyDomain(path).mfcc_delta()
