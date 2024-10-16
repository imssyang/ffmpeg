
import numpy as np
import matplotlib.pyplot as plt
import scipy

# 原始信号 f(t)=cos(2π*3*t + π/3)，频率为每秒产生 3 个信号周期
freq = 3
x = np.linspace(0, 1, 1000, endpoint=False)
y = np.cos(2 * np.pi * freq * x + np.pi / 3)

# 以 16Hz 频率采样 1s
T = 1
sampling_rate = 16
t = np.linspace(0, T, sampling_rate, endpoint=False) # 时间序列
y_sample = np.cos(2 * np.pi * freq * t + np.pi / 3)
print('x_n', y_sample)

# 计算 DCT
Y = scipy.fftpack.dct(y_sample, type=2, norm='ortho')
print('DCT', len(Y))

# 计算 DFT
#Y = np.fft.fft(y_sample)
#print('X_k', Y)

# 计算频率轴
#fftfreqs = np.fft.fftfreq(sampling_rate, 1/sampling_rate)
N = 16
fftfreqs = np.arange(N) * sampling_rate / (2 * N)

# 计算振幅
amplitudes = np.abs(Y)

# 计算相位
phases = np.angle(Y)

fig, axs = plt.subplots(nrows=4, ncols=1, sharex=False, sharey=False)
axs[0].set(title='Original f(t)=cos(2π*3t + π/3)')
axs[0].set_xlim(0, 1)
axs[0].set_xlabel("Time (s)")
axs[0].set_ylabel("Amplitude")
axs[0].xaxis.set_label_coords(1.05, -0.05)
axs[0].plot(x, y)
for i, (px, py) in enumerate(zip(t, y_sample)):
    axs[0].scatter(px, py, color='red')
    axs[0].text(px, py, f'$x_{{{i}}}$ {py:.2f}', fontsize=10, ha='left')

axs[1].set(title="DFT Frequency")
axs[1].set_xlabel("Frequency (Hz)")
axs[1].set_ylabel("Complex-Value")
axs[1].xaxis.set_label_coords(1.0, -0.05)
axs[1].stem(fftfreqs, Y)
for i, (px, py) in enumerate(zip(fftfreqs, Y)):
    axs[1].scatter(px, py, color='red')
    axs[1].text(px, py, f'$X_{{{i}}}$ {py:.2f}', fontsize=10, ha='left', rotation=15)

axs[2].set(title='DFT Amplitudes')
axs[2].set_xlabel("Frequency (Hz)")
axs[2].set_ylabel("Amplitude")
axs[2].xaxis.set_label_coords(1.0, -0.05)
axs[2].stem(fftfreqs, amplitudes)
for i, (px, py) in enumerate(zip(fftfreqs, amplitudes)):
    axs[2].scatter(px, py, color='red')
    axs[2].text(px, py, f'$X_{{{i}}}$ {py:.2f}', fontsize=10, ha='left', rotation=15)

axs[3].set(title='DFT Phase')
axs[3].set_xlabel("Frequency (Hz)")
axs[3].set_ylabel("Phase (radian)")
axs[3].xaxis.set_label_coords(1.0, -0.05)
axs[3].stem(fftfreqs, phases)
for i, (px, py) in enumerate(zip(fftfreqs, phases)):
    axs[3].scatter(px, py, color='red')
    axs[3].text(px, py, f'$X_{{{i}}}$ {py:.2f}', fontsize=10, ha='left', rotation=15)
plt.tight_layout()
plt.show()
