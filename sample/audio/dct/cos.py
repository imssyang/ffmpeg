import librosa
import numpy as np
import matplotlib.pyplot as plt
import scipy


def signal_sample(T, sample_rate, freq, freq_scale=1, x_offset=0, y_offset=0, y_scale=1):
    x = np.linspace(0, T, int(sample_rate * T), endpoint=False) + x_offset
    y = np.cos(2 * np.pi * freq * freq_scale * x) * y_scale + y_offset
    return x, y


def dct2(y, sample_rate, N):
    X = np.arange(N) * sample_rate / (2 * N)
    Y = scipy.fftpack.dct(y, type=2, norm='ortho')
    return X, Y


def main():
    # 原始信号
    T = np.pi # 信号总时长，秒
    freq = 1 / (2 * np.pi)
    freq_scale = 3
    y_offset = 0
    y_scale = 1
    x, y = signal_sample(T, 1000, freq, freq_scale, 0, y_offset, y_scale)

    # 在 T 时间内采样 N 次，采样率为 N/T
    sample_rate = 8 / T
    N = sample_rate * T
    t, y_sample = signal_sample(T, sample_rate, freq, freq_scale, T/N/2, y_offset, y_scale)
    print(f'SIGNAL {len(t)=} {len(y_sample)=} {freq=} {sample_rate=} {N=} {T=}')

    X, Y = dct2(y_sample, sample_rate, N)
    print(f'DCT2 {len(X)=} {len(Y)=} {X=} {Y=}')

    fig, axs = plt.subplots(nrows=2, ncols=1, sharex=False, sharey=False)
    freq_scale_v = '' if freq_scale == 1 else f'{freq_scale}'
    y_scale_v = '' if y_scale == 1 else f'{y_scale}*'
    y_offset_v = '' if y_offset == 0 else f'+{y_offset}'
    axs[0].set(title=f'Original f(t)={y_scale_v}cos({freq_scale_v}t){y_offset_v}, t=[0,π], freq={freq*freq_scale:.2f}')
    axs[0].set_xlim(0, T)
    axs[0].set_xlabel("Time (s)")
    axs[0].set_ylabel("Amplitude")
    axs[0].xaxis.set_label_coords(1.0, -0.13)
    axs[0].plot(x, y)
    for i, (px, py) in enumerate(zip(t, y_sample)):
        axs[0].scatter(px, py, color='red')
        axs[0].text(px, py, f'$x_{{{i}}}$ ({2*i+1}π/16, {py:.2f})', fontsize=10, ha='left')

    axs[1].set(title="DCT Frequency")
    axs[1].set_xlabel("Frequency (Hz)")
    axs[1].set_ylabel("Coefficient (Real)")
    axs[1].xaxis.set_label_coords(1.0, -0.13)
    axs[1].plot(X, Y)
    for i, (px, py) in enumerate(zip(X, Y)):
        axs[1].scatter(px, py, color='red')
        axs[1].text(px, py, f'$X_{{{i}}}$ ({px:.2f}, {py:.2f})', fontsize=10, ha='left', rotation=15)
    plt.tight_layout()
    plt.show()


if __name__ == '__main__':
    main()
