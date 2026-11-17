import numpy as np
from scipy.signal.windows import blackmanharris
fs=48000
def f0of(x):
    zc=np.where((x[:-1]<=0)&(x[1:]>0))[0]; t=zc - x[zc]/(x[zc+1]-x[zc]); return fs*(len(t)-1)/(t[-1]-t[0])
def meas(x,f1,top=16000):
    x=x[2000:]; N=len(x); w=blackmanharris(N); X=np.abs(np.fft.rfft(x*w))**2; f=np.fft.rfftfreq(N,1/fs)
    mask=f<20; k=1
    while k*f1<fs/2: mask|=np.abs(f-k*f1)<6*fs/N; k+=1
    sel=(~mask)&(f<top)
    return 10*np.log10(X[sel].max()/X[mask].max())
