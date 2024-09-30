
import numpy as np
import matplotlib.pyplot as plt

# 原始信号 f(t)=sin(2π*40t + π/4)+0.5sin(2π*90t + π/3)
T = 0.2
freq_1 = 40
freq_2 = 90
x = np.linspace(0, T, 1000, endpoint=False)
y = np.sin(2 * np.pi * freq_1 * x + np.pi / 4) + 0.5 * np.sin(2 * np.pi * freq_2 * x + np.pi / 3)

# 在 T 时间内采样 N 次，采样率为 N*1/T
N = 50
t = np.linspace(0, T, N, endpoint=False) # 时间序列
y_sample = np.sin(2 * np.pi * freq_1 * t + np.pi / 4) + 0.5 * np.sin(2 * np.pi * freq_2 * t + np.pi / 3)
print('x_n', y_sample)

# 计算 DFT
Y = np.fft.fft(y_sample)
print('X_k', Y)

# 计算频率轴
fftfreqs = np.fft.fftfreq(N, 1/N)
mask = fftfreqs > 0 # 只绘制正频率部分

# 计算振幅
amplitudes = np.abs(Y)

# 计算相位
phases = np.angle(Y)

fig, axs = plt.subplots(nrows=4, ncols=1, sharex=False, sharey=False)
axs[0].set(title='Original f(t)=sin(2π*40t + π/4) + 0.5sin(2π*90t + π/3)')
axs[0].set_xlim(0, T)
axs[0].set_xlabel("Time (s)")
axs[0].set_ylabel("Amplitude")
axs[0].xaxis.set_label_coords(1.0, -0.13)
axs[0].plot(x, y)
for i, (px, py) in enumerate(zip(t, y_sample)):
    axs[0].scatter(px, py, color='red')
    axs[0].text(px, py, f'$x_{{{i}}}$ {py:.2f}', fontsize=10, ha='left')

axs[1].set(title="DFT Frequency")
axs[1].set_xlabel("Frequency (Hz)")
axs[1].set_ylabel("Complex-Value")
axs[1].xaxis.set_label_coords(1.0, -0.13)
axs[1].stem(fftfreqs[mask], Y[mask])
for i, (px, py) in enumerate(zip(fftfreqs[mask], Y[mask])):
    axs[1].scatter(px, py, color='red')
    axs[1].text(px, py, f'$X_{{{i}}}$ {py:.2f}', fontsize=10, ha='left', rotation=15)

axs[2].set(title='DFT Amplitudes')
axs[2].set_xlabel("Frequency (Hz)")
axs[2].set_ylabel("Amplitude")
axs[2].xaxis.set_label_coords(1.0, -0.13)
axs[2].stem(fftfreqs[mask], amplitudes[mask])
for i, (px, py) in enumerate(zip(fftfreqs[mask], amplitudes[mask])):
    axs[2].scatter(px, py, color='red')
    axs[2].text(px, py, f'$X_{{{i}}}$ {py:.2f}', fontsize=10, ha='left', rotation=15)

axs[3].set(title='DFT Phase')
axs[3].set_xlabel("Frequency (Hz)")
axs[3].set_ylabel("Phase (radian)")
axs[3].xaxis.set_label_coords(1.0, -0.13)
axs[3].stem(fftfreqs[mask], phases[mask])
for i, (px, py) in enumerate(zip(fftfreqs[mask], phases[mask])):
    axs[3].scatter(px, py, color='red')
    axs[3].text(px, py, f'$X_{{{i}}}$ {py:.2f}', fontsize=10, ha='left', rotation=15)
plt.tight_layout()
plt.show()
