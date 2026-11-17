#include <vector>
#include <cmath>
#include <cstdio>
#include <complex>
#include "../../Source/Synth.h"
using namespace zephyr;
static std::vector<float> render(SynthParams p,int note,float vel,int blocks=120){ Synth s; s.prepare(48000.f); s.setParams(p); s.noteOn(note,vel); std::vector<float> L(512),R(512),o; for(int b=0;b<blocks;++b){ s.process(L.data(),R.data(),512); o.insert(o.end(),L.begin(),L.end()); } return o; }
// pic du spectre par DFT sur une fenetre (zone stable)
static void spec(const std::vector<float>& x,int n0,int N,double fs,double& fpk,double& cent,double& peak){ std::vector<double> mag(N/2); for(int k=1;k<N/2;++k){ std::complex<double> a=0; for(int i=0;i<N;++i){ double w=0.5-0.5*std::cos(2*M_PI*i/N); a+=w*x[n0+i]*std::exp(std::complex<double>(0,-2*M_PI*k*i/N)); } mag[k]=std::abs(a);} double m=0,sm=0,sf=0; int kp=1; for(int k=1;k<N/2;++k){ if(mag[k]>m){m=mag[k];kp=k;} sm+=mag[k]; sf+=mag[k]*k;} fpk=kp*fs/N; cent=sf/sm*fs/N; peak=0; for(float v:x) peak=std::max(peak,(double)std::fabs(v)); }
static SynthParams base(){ SynthParams p; p.ftype=1; p.wave1=3; p.wave2=3; p.lvl1=.8f; p.lvl2=0; p.noise=0; p.lpf=.9f; p.aA=0; p.aD=.9f; p.aS=1.f; p.aR=.3f; p.fmRatio=3; p.fmWave=1; p.fmDir=0; p.quality=0; return p; }
static double meanF(const std::vector<float>& x,int n0,int n1){ int z=0; for(int i=n0+1;i<n1;++i) if(x[i-1]<0&&x[i]>=0) ++z; return z/((n1-n0)/48000.0); }
int main(){ int bad=0;
 for(int ty=0;ty<2;++ty) for(float fm : {0.2f,0.4f}){ auto p=base(); p.fmType=ty; p.fm=fm; p.fmRatio=5; auto x=render(p,69,.8f); std::printf("%s fm=%.1f ratio 2:1 : frequence moyenne %.1f Hz (note = 440)\n",ty?"LINEAIRE":"EXP     ",fm,meanF(x,20000,60000)); }
 for(int ty=0;ty<2;++ty){ auto p=base(); p.fmType=ty; p.fm=0.45f; auto x=render(p,69,.8f); double f,c,pk; spec(x,20000,4096,48000,f,c,pk); std::printf("type %s : pic %.1f Hz  centroide %.0f Hz  crete %.2f\n",ty?"LINEAIRE":"EXP",f,c,pk); }
 { auto p=base(); p.fmType=1; p.fm=0.5f; for(int ks=0;ks<2;++ks){ p.fmKey=ks; auto x=render(p,96,.8f); double f,c,pk; spec(x,20000,4096,48000,f,c,pk); std::printf("note 96 key=%d : centroide %.0f Hz\n",ks,c);} }
 // stabilite : toutes notes, FM max + feedback max, types, directions, qualites
 int n=0; double maxpk=0; for(int ty=0;ty<2;++ty) for(int dir=0;dir<3;++dir) for(int q=0;q<3;q+=2) for(int note=24;note<=108;note+=7){ auto p=base(); p.fmType=ty; p.fmDir=dir; p.fm=1.f; p.fmFb=1.f; p.fmEnvAmt=1.f; p.lvl2=.5f; p.quality=q; p.fmRatio=(note%18); p.wave1=note%4; p.wave2=(note+1)%4; auto x=render(p,note,1.f,40); for(float v:x) if(!std::isfinite(v)||std::fabs(v)>8.f){ ++bad; std::printf("PB ty%d dir%d note%d : %f\n",ty,dir,note,v); break;} for(float v:x) maxpk=std::max(maxpk,(double)std::fabs(v)); ++n; }
 std::printf("%d cas de stabilite, crete max %.2f, %d anomalie(s)\n",n,maxpk,bad); return bad; }
