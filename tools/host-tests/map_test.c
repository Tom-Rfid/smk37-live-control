/* Drives the SMK37 Open simulator: BT LIVE page, editing (HOME / BT 2 s),
 * assignments acting only on LIVE, live targets, LIVE SETUP, MIDI CC,
 * persistence. Dumps RGB565 frames to build/host-tests/. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "smk37_controls.h"
#include "smk37_map.h"
#include "instrument.h"
#include "bass303.h"
int groove_fm_copy(unsigned track, uint8_t patch[156], struct groove_fm_controls *controls);
void sim_boot(unsigned); const float *sim_render(void); int sim_smk37(unsigned,unsigned,int);
const uint16_t *sim_pixels(void); void sim_midi_capture(unsigned); unsigned sim_midi_output(void); uint8_t *sim_file(void);
uint32_t fm1_millis(void);
enum { PATCH = 12, BT = 11, HOME = 2, OCTDN = 4, FX = 14 };
static int fails;
#define CHECK(c,msg) do{ if(c) printf("PASS %s\n",msg); else {printf("FAIL %s\n",msg); fails++;} }while(0)
static void wait(unsigned ms){ uint32_t end=fm1_millis()+ms; while(fm1_millis()<end) sim_render(); }
static void shot(const char *name){ wait(60); char p[128]; snprintf(p,sizeof p,"build/host-tests/%s.raw",name);
  FILE *f=fopen(p,"wb"); if(f){fwrite(sim_pixels(),2,240*240,f); fclose(f);} }
static void button(unsigned id,unsigned ms){ sim_smk37(SMK37_BUTTON,id,1); wait(ms); sim_smk37(SMK37_BUTTON,id,0); wait(50); }
static void knob(unsigned id,int d){ for(int i=0;i<(d<0?-d:d);++i){ sim_smk37(SMK37_ENCODER,id,d<0?-1:1); wait(20);} wait(50); }
static void fader(unsigned id,unsigned v){ sim_smk37(SMK37_FADER,id,(int)v); wait(30); }
static void row(unsigned r){ smk37_screen_map_select(r); wait(30); }
unsigned groove_screen_web_context(void);
int smk37_performance_octave(unsigned pad);
static unsigned mode(void){ return smk37_screen_map_mode(); }
int main(void){
  sim_boot(1); wait(300);
  for(unsigned i=0;i<4;++i) fader(i,0);
  /* ---- Navigation ---- */
  button(BT,100);  CHECK(mode()==1,"BT click opens LIVE");
  shot("01_live_empty");
  button(HOME,80); CHECK(mode()==0,"< on LIVE goes HOME (leaves LIVE)");
  button(BT,100);  CHECK(mode()==1,"LIVE again");
  sim_smk37(SMK37_BUTTON,BT,1); wait(500);
  CHECK(mode()==1,"BT held 0.5 s: still LIVE (bar running)");
  shot("02_live_hold_bar");
  wait(600); CHECK(mode()==2,"BT held 1 s opens editing");
  sim_smk37(SMK37_BUTTON,BT,0); wait(80);
  CHECK(mode()==2,"releasing the long BT press stays in editing");
  button(BT,100);  CHECK(mode()==1,"BT click in editing returns to LIVE");
  button(BT,100);  CHECK(mode()==0,"BT click on LIVE exits");
  button(BT,100); button(PATCH,80); CHECK(mode()==0,"PATCH on LIVE goes HOME (leaves LIVE)");
  button(BT,100); button(BT,1200); button(PATCH,80); CHECK(mode()==0,"PATCH in editing goes HOME");
  /* ---- Assign in editing ---- */
  button(BT,100); button(BT,1200);
  row(4); knob(1,2); knob(2,3);                       /* K5 REVERB T3 */
  row(6); knob(1,7); knob(2,1);                       /* K7 LFO WAVE T1 (DX7) */
  fader(1,40); fader(1,60);                           /* learn F2 */
  CHECK(mode()==2,"still editing after fader learn");
  knob(1,14); knob(2,74); knob(3,1);                  /* F2 MIDI CC 74 ch2 */
  row(8); knob(1,1); knob(2,5);                       /* F1 LEVEL T5 */
  unsigned bass=8; for(unsigned t=0;t<8;++t) if(bass==8 && groove_source_type(t)==GROOVE_303) bass=t;
  row(1); knob(1,4); knob(2,(int)bass+1);             /* K2 CUTOFF on the 303 track */
  row(5); knob(1,3); knob(2,3); knob(4,1);            /* K6 DELAY T3 ALL */
  row(2); knob(1,15); knob(2,1);                      /* K3 SOUND T1 */
  row(3); knob(1,16);                                 /* K4 KEYS TRK */
  CHECK(smk37_map_get(2)->target==SMK37_MAP_SOUND && smk37_map_get(3)->target==SMK37_MAP_KEYS,"K3 = SOUND T1, K4 = KEYS TRK");
  CHECK(smk37_map_get(5)->target==SMK37_MAP_DELAY && smk37_map_get(5)->all,"K6 = DELAY T3 ALL (K5 sets ALL)");
  CHECK(smk37_map_get(4)->target==SMK37_MAP_REVERB && smk37_map_get(4)->track==3,"K5 = REVERB T3");
  CHECK(smk37_map_get(9)->target==SMK37_MAP_CC && smk37_map_get(9)->cc==74 && smk37_map_get(9)->channel==1,"F2 = MIDI CC 74 CH2 (learned)");
  CHECK(smk37_map_get(8)->target==SMK37_MAP_LEVEL && smk37_map_get(8)->track==5,"F1 = LEVEL T5");
  shot("03_edit_map");
  /* ---- Outside the BT page: normal behaviour ---- */
  button(BT,100); button(BT,100); CHECK(mode()==0,"back on the normal pages");
  uint8_t s[2]; groove_sends_get(2,s); unsigned rev0=s[0];
  knob(4,3); groove_sends_get(2,s); CHECK(s[0]==rev0,"outside LIVE, K5 does not touch T3 reverb");
  unsigned dly0=s[1]; knob(5,2); groove_sends_get(2,s); CHECK(s[1]==dly0+2,"outside LIVE, K6 (ALL) turns T3 delay");
  struct groove_view v; groove_get_view(4,0,&v); unsigned t5=v.volume;
  fader(0,30); fader(0,50); groove_get_view(4,0,&v); CHECK(v.volume==t5,"outside LIVE, F1 does not touch T5 level");
  /* ---- LIVE: assignments play ---- */
  button(BT,100); CHECK(mode()==1,"LIVE again");
  knob(4,3); groove_sends_get(2,s); CHECK(s[0]==rev0+3,"LIVE: K5 turns T3 reverb");
  shot("04_live_reverb");
  CHECK(smk37_map_last==4,"LIVE readout follows K5");
  uint8_t patch[156]; groove_fm_copy(0,patch,NULL); unsigned w0=patch[142];
  knob(6,-2); groove_fm_copy(0,patch,NULL); CHECK(patch[142]==w0-2,"LIVE: K7 changes the DX7 LFO wave of T1");
  shot("05_live_wave");
  groove_fm_edit(0,137,40); groove_fm_edit(0,139,90); knob(6,1); shot("05b_live_wave_fast");
  knob(6,1); shot("05c_live_wave_sine"); knob(6,-2);
  sim_midi_capture(1); fader(1,100); wait(50);
  unsigned found=0,o; while((o=sim_midi_output())){ uint8_t *m=sim_file(); if((o&255)==3 && m[0]==0xB1 && m[1]==74 && m[2]==100) found=1; }
  CHECK(found,"LIVE: F2 sends CC74=100 on ch2");
  shot("06_live_cc");
  groove_get_view(4,0,&v); unsigned vol0=v.volume;
  fader(0,60); groove_get_view(4,0,&v); CHECK(v.volume==vol0,"LIVE: F1 below T5 level waits (pickup)");
  fader(0,vol0+1); fader(0,90); groove_get_view(4,0,&v); CHECK(v.volume==90,"LIVE: F1 picks up and sets T5 to 90");
  CHECK(smk37_map_last==8,"LIVE readout follows F1");
  uint8_t before[156]; groove_fm_copy(0,before,NULL);
  knob(0,3); groove_fm_copy(0,patch,NULL); CHECK(!memcmp(before,patch,156),"LIVE: unassigned K1 does nothing");
  CHECK(smk37_map_last==0,"LIVE: unassigned K1 shows -");
  shot("08_live_unassigned");
  { struct groove_view w; groove_get_view(0,0,&w); unsigned p0=w.program;
    knob(2,1); groove_get_view(0,0,&w); CHECK(w.program!=p0,"LIVE: K3 changes the T1 sound"); shot("12_live_sound");
    unsigned o0=smk37_performance_octave(0); button(5,80); CHECK(smk37_performance_octave(0)==(int)o0+1 && mode()==1,"LIVE: OCT+ raises the keys octave"); button(4,80);
    unsigned k0=smk37_screen_track(); knob(3,1); CHECK(smk37_screen_track()==k0+1 && mode()==1,"LIVE: K4 moves the keys to the next track, stays on LIVE");
    shot("13_live_keys"); knob(3,-1); }
  groove_get_view(2,0,&v); unsigned t3=v.volume;
  fader(2,30); fader(2,70); groove_get_view(2,0,&v);
  CHECK(v.volume==t3 && smk37_map_last==10,"LIVE: unassigned F3 does nothing and shows -");
  sim_smk37(SMK37_KEY,13,90); wait(60); shot("09_live_note"); sim_smk37(SMK37_KEY,13,0); wait(40);
  if(bass<8){ uint8_t tone[GROOVE_TONE_PARAMS]; groove_tone_copy(bass,0,tone); unsigned c0=tone[BASS303_CUTOFF];
    knob(1,-5); groove_tone_copy(bass,0,tone); CHECK(tone[BASS303_CUTOFF]==c0-5,"LIVE: K2 lowers the 303 cutoff");
    shot("10_live_cutoff"); }
  else printf("SKIP no 303 track\n");
  /* ---- LIVE SETUP ---- */
  button(BT,1200); button(OCTDN,80);
  knob(1,3); knob(0,1); knob(1,1);
  CHECK(smk37_setup[0]==3 && smk37_setup[1]==1,"LIVE SETUP velocity FIXED, faders JUMP");
  CHECK(groove_settings_radio_flags()&GROOVE_RADIO_ON,"Bluetooth on by default");
  knob(0,5); knob(1,-1);
  CHECK(!(groove_settings_radio_flags()&GROOVE_RADIO_ON),"LIVE SETUP: BLUETOOTH OFF");
  shot("07_setup");
  button(BT,100); button(BT,100); CHECK(mode()==0,"exit");
  sim_midi_capture(1); sim_smk37(SMK37_KEY,12,30); wait(40); sim_smk37(SMK37_KEY,12,0); wait(40);
  unsigned vel=0; while((o=sim_midi_output())){ uint8_t *m=sim_file(); if((o&255)==3 && (m[0]&0xf0)==0x90 && m[2]) vel=m[2]; }
  CHECK(vel==100,"FIXED velocity sends 100 for a soft key");
  fader(3,10); fader(3,20); groove_get_view(3,0,&v); CHECK(v.volume==20,"JUMP: F4 sets T4 level 20 at once");
  /* ---- Persistence ---- */
  wait(4000); sim_boot(0); wait(1500);
  CHECK(smk37_map_get(4)->target==SMK37_MAP_REVERB && smk37_map_get(9)->cc==74 && smk37_setup[0]==3,"assignments and LIVE SETUP survive reboot");
  CHECK(smk37_map_get(5)->all && !smk37_map_get(4)->all,"LIVE/ALL survives reboot");
  CHECK(!(groove_settings_radio_flags()&GROOVE_RADIO_ON),"Bluetooth OFF survives reboot");
  button(BT,100); CHECK(mode()==1,"BT opens LIVE after reboot");
  button(BT,100);
  sim_smk37(SMK37_BUTTON,0,1); button(PATCH,80); sim_smk37(SMK37_BUTTON,0,0); wait(60);
  unsigned src=groove_screen_web_context(); knob(1,1); wait(100);
  CHECK(groove_screen_web_context()==src,"SOURCE: K2 changes the track mode, page stays briefly");
  shot("11_source_k2"); wait(300);
  CHECK(groove_screen_web_context()!=src,"SOURCE: back to the previous page 300 ms after K2");
  button(PATCH,80); button(FX,80); wait(200); knob(0,2); shot("14_knob_hot"); wait(1000); shot("14b_knob_cold");
  printf("fails=%d\n",fails); return fails!=0;
}
