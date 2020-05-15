## Created by Manxi Lin
## 1/20/2020
from matplotlib import pyplot as plt
import numpy as np

def readData(file_name):
    with open(file_name, 'r') as f:
        text = f.read()
    data = text.split(',')
    data = [eval(each) for each in data if each]
    return data

def drawFigure1(x, title):
    plt.figure()
    plt.plot(x)
    plt.ylim([min(x)-10, max(x)+10])
    plt.title(title)
    plt.show()

def drawFigure2(x, y, color='b'):
    plt.plot(x, y, color)
    plt.ylim([min(y)-10, max(y)+10])

def meanFilt(data, len_window=50, step=50, mid=False):
    n = 2
    data = [each for each in data if each>np.mean(data)-n*np.std(data) and each<np.mean(data)+n*np.std(data)]
    meas, num = [], int(np.floor((len(data)-len_window)/step))
    for i in range(num):
        sample = data[i*step:(len_window+i*step)]
        if mid:
            sample = [each for each in sample if each<max(sample) and each>min(sample)]
            meas.append(np.mean(sample))
        else:
            meas.append(np.mean(sample))
    return meas

def mid_filt(data, len_window=50, step=50):
    mids, num = [], int(np.floor((len(data)-len_window)/step))
    for i in range(num):
        sample = data[i*step:(len_window+i*step)]
        sample.sort()
        mids.append(sample[int(np.floor(len_window/2))])
    return mids

def res_am(data, len_window=50, step=50, n=2):
    res, num = [], int(np.floor((len(data)-len_window)/step))
    for i in range(num):
        sample = data[i*step:(len_window+i*step)]
        for each in sample:
            if each < np.mean(sample) - n * np.std(sample):
                each = np.mean(sample) - n * np.std(sample)
            if each > np.mean(sample) + n * np.std(sample):
                each = np.mean(sample) + n * np.std(sample)
            res.append(each)
    return res


if __name__ == '__main__':
    data = readData('datalog_ave_pin17.txt') # 取到35
    d = []
    Time = 60
    t = np.linspace(0, Time, num=len(data))
    drawFigure2(t, data, 'b')
    n, end = 2, Time
    res = res_am(data, 50, 10, n=n)
    t = np.linspace(0, Time, num=len(res))
    drawFigure2(t, res, 'k')
    meas = meanFilt(data, 50, 10)
    t = np.linspace(0, Time, num=len(meas))
    drawFigure2(t, meas, 'r')
    mids = mid_filt(data, 50, 10)
    t = np.linspace(0, Time, num=len(mids))
    drawFigure2(t, mids, 'g')
    meas = meanFilt(data, 50, 10, mid=True)
    t = np.linspace(0, Time, num=len(meas))
    drawFigure2(t, meas, 'y')
    plt.legend(['raw data', '2 sigma', 'ave', 'mid', 'mid-ave'])
    plt.show()
