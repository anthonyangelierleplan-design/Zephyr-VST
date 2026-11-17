#include <juce_audio_utils/juce_audio_utils.h>
#define private public
#include "PluginProcessor.h"
#undef private
int main(){ juce::ScopedJuceInitialiser_GUI init; int bad=0;
 for(int i=0;i<=1000;++i){ float r=i/1000.f; float e=std::fabs(zephyr::sy::proResLaw(zephyr::sy::proResInv(r))-r); if(e>1e-4f){++bad; printf("roundtrip %f %f\n",r,e);} }
 ZephyrAudioProcessor p; p.prepareToPlay(48000,512);
 int nPro=0;
 for(int i=1;i<p.getNumFactoryPresets();++i){ p.loadFactoryPreset(i); if(juce::roundToInt(p.apvts.getRawParameterValue("s_ftype")->load())==3){ ++nPro; printf("  preset Prophet %-22s RES potard %.3f -> resonance effective %.3f\n", p.getFactoryPresetName(i).toRawUTF8(), p.apvts.getRawParameterValue("s_lpfRes")->load(), zephyr::sy::proResLaw(p.apvts.getRawParameterValue("s_lpfRes")->load())); } }
 // ancien projet : ftype 3, res 0.5, sans drapeau proP6
 p.setParamReal("s_ftype",3); p.setParamReal("s_lpfRes",0.5f);
 auto st=p.apvts.copyState(); juce::MemoryBlock mb; { auto x=st.createXml(); p.copyXmlToBinary(*x,mb);} 
 ZephyrAudioProcessor q; q.prepareToPlay(48000,512); q.setStateInformation(mb.getData(),(int)mb.getSize());
 float r=q.apvts.getRawParameterValue("s_lpfRes")->load(); printf("ancien projet RES 0.5 -> potard %.3f, resonance effective %.3f\n",r,zephyr::sy::proResLaw(r)); if(std::fabs(zephyr::sy::proResLaw(r)-0.5f)>0.01f) ++bad;
 // nouveau projet : sauvegarde / relecture inchangee
 juce::MemoryBlock m2; q.getStateInformation(m2); ZephyrAudioProcessor w; w.prepareToPlay(48000,512); w.setStateInformation(m2.getData(),(int)m2.getSize());
 float r2=w.apvts.getRawParameterValue("s_lpfRes")->load(); printf("nouveau projet relu : %.3f (attendu %.3f)\n",r2,r); if(std::fabs(r2-r)>1e-3f) ++bad;
 printf("%d presets Prophet, %s\n",nPro,bad?"ECHEC":"TOUT OK"); return bad; }
