#include <vector>
#include <cmath>
#include <cstdio>
#include "../../Source/Synth.h"
using namespace zephyr;
static int gQ=1; static float FS=48000.f; static int flagged=0, total=0;
// rend une note tenue (saw) et mesure la part d'energie hors harmoniques (parasites) + crete
static void test(int ft,std::vector<int> notes,float lpf,float res,float drive,float lvl,float fenv=0.f){ int note=notes[0];
  Synth s; s.prepare(FS); SynthParams p; p.ftype=ft; p.quality=gQ; p.wave1=0; p.lvl1=lvl; p.lvl2=0; p.noise=0; p.lpf=lpf; p.lpfRes=res; p.keytrack=0.5f; p.drive=drive; p.fltEnv=fenv; p.fA=0.3f; p.fS=1.f; p.hpf=0; p.aA=0.01f; p.aS=1.f; s.setParams(p); for(int nn:notes) s.noteOn(nn,1.f);
  std::vector<float> L(512),R(512),o; for(int b=0;b<300;++b){ s.process(L.data(),R.data(),512); if(b>=150) o.insert(o.end(),L.begin(),L.end()); }
  const int N=1<<16; std::vector<double> re(N/2+1),im(N/2+1);
  double f0=440.0*std::pow(2.0,(note-69)/12.0);
  // DFT naive sur une grille grossiere : energie totale et energie pres des harmoniques
  double tot=0,harm=0,pk=0; for(int i=0;i<N;++i){ pk=std::max(pk,(double)std::fabs(o[i])); }
  static std::vector<double> w; if(w.empty()){ w.resize(N); for(int i=0;i<N;++i) w[i]=0.5-0.5*std::cos(2*M_PI*i/N);} 
  // FFT radix-2
  std::vector<double> ar(N),ai(N,0.0); for(int i=0;i<N;++i) ar[i]=o[i]*w[i];
  for(int i=1,j=0;i<N;++i){ int bit=N>>1; for(;j&bit;bit>>=1) j^=bit; j^=bit; if(i<j){ std::swap(ar[i],ar[j]); std::swap(ai[i],ai[j]); } }
  for(int len=2;len<=N;len<<=1){ double ang=-2*M_PI/len; double wr=std::cos(ang),wi=std::sin(ang); for(int i=0;i<N;i+=len){ double cr=1,ci=0; for(int k=0;k<len/2;++k){ double ur=ar[i+k],ui=ai[i+k]; double vr=ar[i+k+len/2]*cr-ai[i+k+len/2]*ci, vi=ar[i+k+len/2]*ci+ai[i+k+len/2]*cr; ar[i+k]=ur+vr; ai[i+k]=ui+vi; ar[i+k+len/2]=ur-vr; ai[i+k+len/2]=ui-vi; double nr=cr*wr-ci*wi; ci=cr*wi+ci*wr; cr=nr; } } }
  double maxInh=0, fInh=0;
  for(int k=4;k<N/2;++k){ double e=ar[k]*ar[k]+ai[k]*ai[k]; double f=k*FS/N; double d=1e9; for(int nn:notes){ double fn=440.0*std::pow(2.0,(nn-69)/12.0); double h=f/fn; d=std::min(d,std::fabs(h-std::round(h))*fn); } tot+=e; if(d<6.0) harm+=e; else if(e>maxInh){maxInh=e;fInh=f;} }
  double inh=10*std::log10((tot-harm)/tot+1e-12); ++total;
  if(pk>0.05 && (pk>1.3 || inh>-20)){ ++flagged; std::printf("! fs %.0f ft %d notes %d(x%d) lpf %.2f res %.2f drv %.1f fenv %.1f : peak %.3f  hors-harm %6.1f dB a %.0f Hz\n",FS,ft,note,(int)notes.size(),lpf,res,drive,fenv,pk,inh,fInh); }
}
int main(int argc,char**argv){ gQ=argc>1?atoi(argv[1]):1; for(float fs:{48000.f}){ FS=fs;
  for(int ft:{1,2,3}) for(int note:{48,60,72,84,96}) for(float res:{0.3f,0.6f,0.8f,0.9f}) for(float lpf:{0.7f,0.85f,0.95f,1.0f}) for(float drv:{0.f,0.2f,0.6f}) for(float fe:{0.f,0.6f}) test(ft,{note},lpf,res,drv,0.8f,fe);
  for(int ft:{1,2,3}) for(float res:{0.3f,0.6f,0.8f,0.9f}) for(float lpf:{0.5f,0.7f,0.85f,1.0f}) for(float fe:{0.f,0.6f}) { test(ft,{48,55,60,64,67},lpf,res,0.2f,0.6f,fe); test(ft,{60,67,72,76,79},lpf,res,0.2f,0.6f,fe); } }
  std::printf("%d cas signales sur %d\n",flagged,total); }
