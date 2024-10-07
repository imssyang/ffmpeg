
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec


def signal_1(T, sample_rate, freq):
    x = np.linspace(0, T, int(sample_rate * T), endpoint=False)
    y = np.sin(2 * np.pi * freq * x + np.pi / 6)
    return x, y


def signal_2(T, sample_rate, freq):
    x = np.linspace(0, T, int(sample_rate * T), endpoint=False)
    y = np.cos(2 * np.pi * freq * x + np.pi / 3)
    return x, y


def signal_noise(y, loc, scale):
    y_noise = np.random.normal(loc, scale, y.shape)  # 正态分布的噪声
    return y_noise


def calc_snr(signal, noise):
    signal_amplitude = np.mean(signal**2) # 计算信号振幅RMS
    noise_amplitude = np.mean(noise**2)   # 计算噪声振幅RMS
    snr = 20 * np.log10(signal_amplitude / noise_amplitude) # 计算 SNR (以 dB 为单位)
    print(f"SNR: {snr:.2f}dB {signal_amplitude=} {noise_amplitude=}")
    return snr


def signal_sample(T, sample_rate, freq_1, freq_2, noise_loc, noise_scale):
    # 原始信号
    x = np.linspace(0, T, int(sample_rate * T), endpoint=False)
    _, sy_1 = signal_1(T, sample_rate, freq_1)
    _, sy_2 = signal_2(T, sample_rate, freq_2)
    y_signal = sy_1 + sy_2
    y_noise = signal_noise(y_signal, noise_loc, noise_scale)
    y = y_signal + y_noise
    snr = calc_snr(y_signal, y_noise)
    return x, y, y_noise, snr


def fft(y, sample_rate, N, T, by_db):
    X = np.fft.fftfreq(N, 1/sample_rate)
    Y = np.fft.fft(y)
    amplitudes = np.abs(Y)
    powers = amplitudes ** 2
    if by_db:
        # 转换为 dB (为避免log(0)，在功率上加一个很小的值)
        amplitudes = 20 * np.log10(amplitudes + 1e-6)
        powers = 10 * np.log10(powers + 1e-12)
    phases = np.angle(Y)
    return X, Y, amplitudes, powers, phases


def main():
    # 原始信号
    T = 0.25 # 信号总时长，秒
    freq_1 = 50
    freq_2 = 200
    sr_signal = 10240
    noise_loc = 0
    noise_scale_1 = 0.005
    noise_scale_2 = 0.5
    x_sub1, y_sub1 = signal_1(T, sr_signal, freq_1)
    x_sub2, y_sub2 = signal_2(T, sr_signal, freq_2)
    x_s1, y_s1, y_noise_s1, snr_s1 = signal_sample(T, sr_signal, freq_1, freq_2, noise_loc, noise_scale_1)
    x_s2, y_s2, y_noise_s2, snr_s2 = signal_sample(T, sr_signal, freq_1, freq_2, noise_loc, noise_scale_2)

    # 在 T 时间内采样 N 次，采样率为 N/T
    sample_rate = 1024
    N = int(sample_rate * T)
    x1, y1, y1_noise, snr_1 = signal_sample(T, sample_rate, freq_1, freq_2, noise_loc, noise_scale_1)
    x2, y2, y2_noise, snr_2 = signal_sample(T, sample_rate, freq_1, freq_2, noise_loc, noise_scale_2)
    print(f'SIGNAL-1 {len(x1)=} {len(y1)=} {len(y1_noise)=} {sample_rate=} {N=} {T=}')
    print(f'SIGNAL-2 {len(x2)=} {len(y2)=} {len(y1_noise)=} {sample_rate=} {N=} {T=}')

    X1, Y1, amplitudes1, powers1, phases1 = fft(y1, sample_rate, N, T, by_db=False)
    X2, Y2, amplitudes2, powers2, phases2 = fft(y2, sample_rate, N, T, by_db=False)
    freq_mask1 = X1 > 0 # 只绘制正频率部分
    freq_mask2 = X2 > 0 # 只绘制正频率部分
    print(f'FFT-1 {len(X1)=} {len(Y1)=} {len(amplitudes1)=} {len(powers1)=} {len(phases1)=} {len(freq_mask1)=}')
    print(f'FFT-2 {len(X2)=} {len(Y2)=} {len(amplitudes2)=} {len(powers2)=} {len(phases2)=} {len(freq_mask2)=}')

    fig = plt.figure(figsize=(10, 10))
    gs = gridspec.GridSpec(nrows=5, ncols=2)
    ax0 = fig.add_subplot(gs[0, :])
    ax0.set(title='Original f(t)=sin(2π*50t + π/6)')
    ax0.set_xlim(0, T)
    ax0.set_xlabel("Time (s)")
    ax0.set_ylabel("Amplitude")
    ax0.xaxis.set_label_coords(1.0, -0.13)
    ax0.plot(x_sub1, y_sub1)

    ax1 = fig.add_subplot(gs[1, :])
    ax1.set(title='Original f(t)=cos(2π*200t + π/3)')
    ax1.set_xlim(0, T)
    ax1.set_xlabel("Time (s)")
    ax1.set_ylabel("Amplitude")
    ax1.xaxis.set_label_coords(1.0, -0.13)
    ax1.plot(x_sub2, y_sub2)

    ax2 = fig.add_subplot(gs[2, 0])
    ax2.set(title=f'White Noise, scale={noise_scale_1}')
    ax2.set_xlim(0, T)
    ax2.set_xlabel("Time (s)")
    ax2.set_ylabel("Amplitude")
    ax2.xaxis.set_label_coords(1.0, -0.13)
    ax2.plot(x1, y1_noise)

    ax3 = fig.add_subplot(gs[3, 0])
    ax3.set(title=f'Mixed Signal, sample_rate={sample_rate}, SNR={snr_1:.0f}')
    ax3.set_xlim(0, T)
    ax3.set_xlabel("Time (s)")
    ax3.set_ylabel("Amplitude")
    ax3.xaxis.set_label_coords(1.0, -0.13)
    ax3.plot(x1, y1)

    ax4 = fig.add_subplot(gs[4, 0])
    ax4.set(title='FFT Amplitudes')
    ax4.set_xlabel("Frequency (Hz)")
    ax4.set_ylabel("Amplitude")
    ax4.xaxis.set_label_coords(1.0, -0.13)
    ax4.plot(X1[freq_mask1], amplitudes1[freq_mask1])

    ax5 = fig.add_subplot(gs[2, 1])
    ax5.set(title=f'White Noise, scale={noise_scale_2}')
    ax5.set_xlim(0, T)
    ax5.set_xlabel("Time (s)")
    ax5.set_ylabel("Amplitude")
    ax5.xaxis.set_label_coords(1.0, -0.13)
    ax5.plot(x2, y2_noise)

    ax6 = fig.add_subplot(gs[3, 1])
    ax6.set(title=f'Mixed Signal, sample_rate={sample_rate}, SNR={snr_2:.0f}')
    ax6.set_xlim(0, T)
    ax6.set_xlabel("Time (s)")
    ax6.set_ylabel("Amplitude")
    ax6.xaxis.set_label_coords(1.0, -0.13)
    ax6.plot(x2, y2)

    ax7 = fig.add_subplot(gs[4, 1])
    ax7.set(title='FFT Amplitudes')
    ax7.set_xlabel("Frequency (Hz)")
    ax7.set_ylabel("Amplitude")
    ax7.xaxis.set_label_coords(1.0, -0.13)
    ax7.plot(X2[freq_mask2], amplitudes2[freq_mask2])
    plt.tight_layout()
    plt.show()


if __name__ == '__main__':
    main()
