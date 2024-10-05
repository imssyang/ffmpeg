import librosa
import numpy as np
import matplotlib.pyplot as plt


def signal_sample(T, sample_rate, freq_1, freq_2):
    # 4段子信号，首尾是静音信号，中间包括20Hz和60Hz正弦信号）
    T_segment = float(T / 4)

    x = np.linspace(0, T, int(sample_rate * T), endpoint=False)

    y_silent_head = np.zeros(int(T_segment * sample_rate))
    y_silent_tail = np.zeros(int(T_segment * sample_rate))

    x_freq_1 = np.linspace(T_segment, T_segment*2, int(T_segment * sample_rate), endpoint=False)
    y_freq_1 = np.sin(2 * np.pi * freq_1 * x_freq_1)

    x_freq_2 = np.linspace(T_segment*2, T_segment*3, int(T_segment * sample_rate), endpoint=False)
    y_freq_2 = np.sin(2 * np.pi * freq_2 * x_freq_2)

    y = np.concatenate([y_silent_head, y_freq_1, y_freq_2, y_silent_tail])
    return x, y


def fft(y, N, T):
    X = np.fft.fftfreq(N, 1/N)
    Y = np.fft.fft(y)
    amplitudes = np.abs(Y)
    phases = np.angle(Y)
    return X, Y, amplitudes, phases


def stft(y, win_time, hop_time, sample_rate):
    win_length = int(win_time * sample_rate / 1000)
    hop_length = int(hop_time * sample_rate / 1000)
    n_fft = int(2**np.ceil(np.log2(win_length)))
    D = librosa.stft(y, n_fft=n_fft, hop_length=hop_length, win_length=win_length)
    S_amplitude = np.abs(D)
    S_db = librosa.amplitude_to_db(np.abs(D), ref=np.max) # convert amplitude to decibel scale
    return S_db, S_amplitude, n_fft, win_length, hop_length


def main():
    # 原始信号
    T = 2 # 信号总时长，秒
    freq_1 = 20
    freq_2 = 60
    x, y = signal_sample(T, 1000, freq_1, freq_2)

    # 在 T 时间内采样 N 次，采样率为 N/T
    sample_rate = 512
    N = sample_rate * T
    t, y_sample = signal_sample(T, sample_rate, freq_1, freq_2)
    print(f'SIGNAL {len(t)=} {len(y_sample)=} {sample_rate=} {N=} {T=}')

    X, Y, amplitudes, phases = fft(y_sample, N, T)
    X = X * sample_rate / N
    freq_mask = X > 0 # 只绘制正频率部分
    print(f'FFT {len(X)=} {len(Y)=} {len(amplitudes)=} {len(phases)=} {len(freq_mask)=}')

    win_time = 80 # ms
    hop_time = 40 # ms
    S_db, S_amplitude, n_fft, win_length, hop_length = stft(y_sample, win_time, hop_time, sample_rate)
    print(f'STFT {S_db.shape=} {S_amplitude.shape=} {n_fft=} {win_length=} {hop_length=}')

    fig, axs = plt.subplots(nrows=4, ncols=1, sharex=False, sharey=False)
    axs[0].set(title='Original f(t)=[sin(2π*20t) & sin(2π*60t)]')
    axs[0].set_xlim(0, T)
    axs[0].set_xlabel("Time (s)")
    axs[0].set_ylabel("Amplitude")
    axs[0].xaxis.set_label_coords(1.0, -0.13)
    axs[0].plot(x, y)

    axs[1].set(title="DFT Amplitudes")
    axs[1].set_xlabel("Frequency (Hz)")
    axs[1].set_ylabel("Amplitude")
    axs[1].xaxis.set_label_coords(1.0, -0.13)
    markerline_1, stemlines_1, baseline_1 = axs[1].stem(X[freq_mask], amplitudes[freq_mask])
    plt.setp(markerline_1, markersize=3)

    axs[2].set(title="STFT Spectrogram (in amplitude)")
    axs[2].xaxis.set_label_coords(1.0, -0.13)
    img_1 = librosa.display.specshow(S_amplitude, sr=sample_rate, hop_length=hop_length, x_axis='s', y_axis='fft', ax=axs[2])
    plt.colorbar(img_1, ax=axs[2], format="%+2.0f dB")

    axs[3].set(title='STFT Spectrogram (in dB)')
    axs[3].xaxis.set_label_coords(1.0, -0.13)
    img_1 = librosa.display.specshow(S_db, sr=sample_rate, hop_length=hop_length, x_axis='s', y_axis='log', ax=axs[3])
    plt.colorbar(img_1, ax=axs[3], format="%+2.0f dB")
    plt.tight_layout()
    plt.show()


if __name__ == '__main__':
    main()
