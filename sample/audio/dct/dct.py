import numpy as np
import matplotlib.pyplot as plt
import scipy

# 生成一个简单的时域信号（例如，正弦波 + 噪声）
N = 64  # 信号长度
fs = 100  # 采样率，假设为 100 Hz
x = np.linspace(0, 2 * np.pi, N)
signal = np.sin(5 * x) + 0.5 * np.random.randn(N)

# 使用 librosa 进行 DCT-II 变换
dct_coefficients = scipy.fftpack.dct(signal, type=2, norm='ortho')

# 计算 DCT 对应的频率
frequencies = np.arange(N) * fs / (2 * N)

# 绘制时域信号和 DCT 频域图像
plt.figure(figsize=(12, 6))

# 绘制时域信号
plt.subplot(1, 2, 1)
plt.plot(np.linspace(0, N/fs, N), signal, label='Time Domain Signal')
plt.title('Time Domain Signal')
plt.xlabel('Time (seconds)')
plt.ylabel('Amplitude')
plt.grid(True)
plt.legend()

# 绘制 DCT 频域图像
plt.subplot(1, 2, 2)
plt.stem(frequencies, dct_coefficients, basefmt=" ")
plt.title('DCT-II (Frequency Domain)')
plt.xlabel('Frequency (Hz)')
plt.ylabel('Amplitude')
plt.grid(True)

# 显示图像
plt.tight_layout()
plt.show()
