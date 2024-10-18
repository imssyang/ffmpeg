import librosa
import numpy as np
import matplotlib.pyplot as plt
import soundfile as sf


class SpecsubDenoise:
    def __init__(self, path, n_fft, win_length, hop_length, dtype):
        self.path = path
        self.n_fft = n_fft
        self.win_length = win_length
        self.hop_length = hop_length
        self.y, self.sample_rate = librosa.load(path, sr=None)
        self.S, self.power, self.phase = self.stft(self.y, n_fft, win_length, hop_length)
        if dtype == 1:
            self.amplitude_enhenc = self.specsub_1(self.S, self.power, by_power=True)
        elif dtype == 2:
            self.amplitude_enhenc = self.specsub_2(self.S, self.power, smooth=True)
        else:
            self.amplitude_enhenc = self.specsub_1(self.S, self.power, by_power=False)
        self.y_enhenc = self.istft(self.amplitude_enhenc, self.phase, win_length, hop_length)

    def save(self, y, path):
        sf.write(path, y, self.sample_rate)

    def stft(self, y, n_fft, win_length, hop_length):
        S = librosa.stft(y, n_fft=n_fft, win_length=win_length, hop_length=hop_length)
        phase = np.angle(S)
        amplitude= np.abs(S)
        power = amplitude**2
        print(f'STFT {S.shape=} {amplitude.shape=} {phase.shape=} {n_fft=} {win_length=} {hop_length=}')
        return S, power, phase

    def noise(self, S, power):
        # 估计噪声信号的能量，由于噪声信号未知, 这里假设信号的前30帧为噪声
        D, T = np.shape(S)
        amplitude_noise = np.mean(np.abs(S[:,:30]), axis=1, keepdims=True)
        power_noise = np.tile(amplitude_noise**2, [1, T])
        return power_noise

    def specsub_1(self, S, power, by_power):
        power_noise = self.noise(S, power)
        if by_power:
            power_enhen = power - power_noise # 能量减
            power_enhen[power_enhen < 0] = 0  # 保证能量大于0
            amplitude_enhenc = np.sqrt(power_enhen)
        else:
            amplitude_enhenc = np.sqrt(power) - np.sqrt(power_noise) # 幅度减
            amplitude_enhenc[amplitude_enhenc < 0] = 0  # 保证能量大于0
        return amplitude_enhenc

    def specsub_2(self, S, power, alpha=4, gamma=1, beta=0.0001, smooth=False):
        D, T = np.shape(S)
        power_noise = self.noise(S, power)
        if smooth:
            amplitude = np.sqrt(power)
            amplitude_new = np.copy(amplitude)
            k=1
            for t in range(k, T-k):
                amplitude_new[:, t] = np.mean(amplitude[:,t-k:t+k+1], axis=1)
            power = amplitude_new**2

        power_enhen = np.power(power, gamma) - alpha*np.power(power_noise, gamma)
        power_enhen = np.power(power_enhen, 1/gamma)
        mask = (power_enhen >= beta*power_noise) - 0
        power_enhen = mask*power_enhen + beta*(1-mask)*power_noise # 对于过小的值用 beta* power_noise 替代
        amplitude_enhenc = np.sqrt(power_enhen)

        if smooth:
            amplitude_noise = np.sqrt(power_noise[:, :31])
            maxnr = np.max(np.abs(S[:,:31])-amplitude_noise, axis=1) # 计算最大噪声残差
            amplitude_enhenc_new  = np.copy(amplitude_enhenc)
            k = 1
            for t in range(k, T-k):
                index = np.where(amplitude_enhenc[:,t] < maxnr)[0]
                temp = np.min(amplitude_enhenc[:,t-k:t+k+1], axis=1)
                amplitude_enhenc_new[index,t] = temp[index]
                amplitude_enhenc = amplitude_enhenc_new
        return amplitude_enhenc

    def istft(self, amplitude, phase, win_length, hop_length):
        S = amplitude * np.exp(1j*phase)
        signal = librosa.istft(S, win_length=win_length, hop_length=hop_length)
        return signal


if __name__ == '__main__':
    y, sample_rate = librosa.load(
        path='/opt/ffmpeg/sample/audio/denoise/origin.wav',
        sr=None,
    )
    SD = SpecsubDenoise(
        path='/opt/ffmpeg/sample/audio/denoise/origin_noise_L.wav',
        n_fft=256, win_length=256, hop_length=128, dtype=2
    )
    SD.save(SD.y_enhenc, path='/opt/ffmpeg/sample/audio/denoise/origin_specsub_L.wav')

    plt.subplot(3,1,1)
    plt.specgram(y, NFFT=SD.n_fft, Fs=SD.sample_rate)
    plt.xlabel("Origin specgram")
    plt.subplot(3,1,2)
    plt.specgram(SD.y, NFFT=SD.n_fft, Fs=SD.sample_rate)
    plt.xlabel("AddNoisy specgram")
    plt.subplot(3,1,3)
    plt.specgram(SD.y_enhenc, NFFT=SD.n_fft, Fs=SD.sample_rate)
    plt.xlabel("Denoise specgram")
    plt.show()
    #plt.imshow(librosa.amplitude_to_db(SD.amplitude_enhenc, ref=np.max), origin='lower')
    #plt.show()
