/* Drives the SMK37 Open simulator: BT long press -> MAP page, assignments,
 * knob/fader behaviour, MIDI CC output, persistence. Dumps RGB565 frames. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "smk37_controls.h"
#include "smk37_map.h"
#include "instrument.h"
int groove_fm_copy(unsigned track, uint8_t patch[156], struct groove_fm_controls *controls);
unsigned groove_tempo_current(void);
void sim_boot(unsigned); const float *sim_render(void); int sim_smk37(unsigned,unsigned,int);
const uint16_t *sim_pixels(void); void sim_midi_capture(unsigned); unsigned sim_midi_output(void); uint8_t *sim_file(void);
uint32_t fm1_millis(void);
static int fails;
#define CHECK(c,msg) do{ if(c) printf("PASS %s\n",msg); else {printf("FAIL %s\n",msg); fails++;} }while(0)
static void wait(unsigned ms){ uint32_t end=fm1_millis()+ms; while(fm1_millis()<end) sim_render(); }
static void shot(const char *name){ wait(60); char p[128]; snprintf(p,sizeof p,"build/host-tests/%s.raw",name);
  FILE *f=fopen(p,"wb"); fwrite(sim_pixels(),2,240*240,f); fclose(f); }
static void button(unsigned id,unsigned ms){ sim_smk37(SMK37_BUTTON,id,1); wait(ms); sim_smk37(SMK37_BUTTON,id,0); wait(50); }
static void knob(unsigned id,int d){ for(int i=0;i<(d<0?-d:d);++i){ sim_smk37(SMK37_ENCODER,id,d<0?-1:1); wait(20);} wait(50); }
static void fader(unsigned id,unsigned v){ sim_smk37(SMK37_FADER,id,(int)v); wait(30); }
int main(void){
  sim_boot(1); wait(1500);
  for(unsigned i=0;i<4;++i) fader(i,0);           /* establish physical positions */
  shot("01_home");
  /* BT press opens the control map directly. */
  button(11,120);
  CHECK(smk37_screen_map_mode()==2,"BT press opens CONTROL MAP");
  button(2,80); wait(100);                        /* HOME */
  CHECK(smk37_screen_map_mode()==1,"HOME returns to a performance page");
  button(11,120);
  CHECK(smk37_screen_map_mode()==2,"BT press reopens CONTROL MAP");
  shot("03_map_empty");
  /* K5 -> REVERB on track 3. */
  knob(0,4); knob(1,2); knob(2,3);
  const struct smk37_map_entry *e;
  e=smk37_map_get(4);
  CHECK(e->target==SMK37_MAP_REVERB && e->track==3,"K5 = REVERB T3");
  /* Fader learn: moving F2 selects its row; then MIDI CC 74 ch 2. */
  fader(1,40); fader(1,60);
  knob(1,14); knob(2,74); knob(3,1);
  e=smk37_map_get(9);
  CHECK(e->target==SMK37_MAP_CC && e->cc==74 && e->channel==1,"F2 learned and set to MIDI CC74 CH2");
  /* F1 -> LEVEL on track 5 (via K1 row navigation). */
  knob(0,-1); knob(1,1); knob(2,5);
  e=smk37_map_get(8);
  CHECK(e->target==SMK37_MAP_LEVEL && e->track==5,"F1 = LEVEL T5");
  shot("04_map_set");
  /* Exit and use the assignments. */
  button(2,80); wait(100);
  uint8_t s[2]; groove_sends_get(2,s); unsigned before=s[0];
  knob(4,3); groove_sends_get(2,s);
  CHECK(s[0]==before+3,"K5 turns T3 reverb send on HOME");
  shot("05_home_notice");
  /* SHIFT bypass. */
  sim_smk37(SMK37_BUTTON,0,1); wait(50); knob(4,2); sim_smk37(SMK37_BUTTON,0,0); wait(50);
  groove_sends_get(2,s); CHECK(s[0]==before+3,"SHIFT bypasses the knob assignment");
  /* Editor pages keep their own knobs: FX page (physical 14 = FX). */
  button(14,80); wait(100);
  CHECK(smk37_screen_map_mode()==0,"FX editor page bypasses knob assignments");
  knob(4,2); groove_sends_get(2,s); CHECK(s[0]==before+3,"K5 on FX page does not drive the T3 reverb assignment");
  button(2,80); button(2,80); wait(100);
  /* F2 sends MIDI CC on external ports. */
  sim_midi_capture(1); fader(1,100); wait(50);
  unsigned found=0,o; while((o=sim_midi_output())){ uint8_t *m=sim_file(); if((o&255)==3 && m[0]==0xB1 && m[1]==74 && m[2]==100) found=1; }
  CHECK(found,"F2 sends B1 4A 64 (CC74=100 on ch2)");
  /* F1 LEVEL T5 with pickup: volume starts at 100, fader at 0. */
  struct groove_view v; groove_get_view(4,0,&v); unsigned vol0=v.volume;
  fader(0,50); groove_get_view(4,0,&v); CHECK(v.volume==vol0,"F1 below level does not jump (pickup)");
  fader(0,101); fader(0,90); groove_get_view(4,0,&v); CHECK(v.volume==90,"F1 picks up then sets T5 level to 90");
  /* Default faders still work: F3 (unassigned) drives track 3 level. */
  groove_get_view(2,0,&v); fader(2,110); fader(2,v.volume); fader(2,70); groove_get_view(2,0,&v);
  CHECK(v.volume==70,"unassigned F3 keeps default track-3 level");
  /* Persistence across a reboot (no storage clear). */
  wait(4000);
  sim_boot(0); wait(2500);
  e=smk37_map_get(4); CHECK(e->target==SMK37_MAP_REVERB && e->track==3,"K5 assignment survives reboot");
  e=smk37_map_get(9); CHECK(e->target==SMK37_MAP_CC && e->cc==74 && e->channel==1,"F2 assignment survives reboot");
  /* SAVE clears the selected row. */
  button(11,120); CHECK(smk37_screen_map_mode()==2,"BT opens MAP after reboot");
  /* ---- Live targets, pages and LIVE SETUP ---- */
  button(2,80); wait(100);
  button(11,120); CHECK(smk37_screen_map_mode()==2,"BT opens page 1");
  button(11,120); CHECK(smk37_screen_map_mode()==2,"BT again stays in the menu (page 2)");
  shot("07_setup");
  knob(1,3);                                   /* velocity: FIXED 100 */
  knob(0,1); knob(1,1);                        /* faders: JUMP */
  CHECK(smk37_setup[0]==3 && smk37_setup[1]==1,"LIVE SETUP velocity FIXED, faders JUMP");
  button(4,80);                                /* OCT-: back to page 1 */
  smk37_screen_map_select(6); wait(50);        /* K7 row */
  knob(1,7); knob(2,1);                        /* LFO WAVE, T1 (DX7 piano) */
  smk37_screen_map_select(7); wait(50);        /* K8 row */
  knob(1,13);                                  /* TEMPO */
  smk37_screen_map_select(1); wait(50);
  knob(1,9);                                   /* K2 = REV TIME (global) */
  shot("08_live_targets");
  CHECK(smk37_map_get(6)->target==SMK37_MAP_LFO_WAVE && smk37_map_get(7)->target==SMK37_MAP_TEMPO,"K7 LFO WAVE, K8 TEMPO");
  button(2,80); wait(100);
  uint8_t patch[156]; groove_fm_copy(0,patch,NULL); unsigned w0=patch[142];
  knob(6,-2); groove_fm_copy(0,patch,NULL);
  CHECK(patch[142]==w0-2,"K7 changes the DX7 LFO wave of T1 (SINE -> SAW UP)");
  uint8_t fx0[4],fx1[4]; groove_effects_get(fx0); knob(1,3); groove_effects_get(fx1);
  CHECK(fx1[0]!=fx0[0],"K2 changes the shared reverb time");
  struct groove_view tv; groove_get_view(0,0,&tv); unsigned bpm=tv.tempo; knob(7,5); groove_get_view(0,0,&tv);
  CHECK(tv.tempo!=bpm,"K8 changes the tempo");
  /* Fader JUMP: F4 (unassigned, T4 level) follows at once, no pickup. */
  fader(3,10); fader(3,20); groove_get_view(3,0,&tv); CHECK(tv.volume==20,"JUMP: F4 sets T4 level 20 directly");
  /* Velocity FIXED 100: key note-on on MIDI out carries 100. */
  sim_midi_capture(1); sim_smk37(SMK37_KEY,12,30); wait(40); sim_smk37(SMK37_KEY,12,0); wait(40);
  unsigned vel=0,o2; while((o2=sim_midi_output())){ uint8_t *m=sim_file(); if((o2&255)==3 && (m[0]&0xf0)==0x90 && m[2]) vel=m[2]; }
  CHECK(vel==100,"FIXED velocity sends 100 for a soft key");
  wait(4000); sim_boot(0); wait(2500);
  CHECK(smk37_setup[0]==3 && smk37_setup[1]==1 && smk37_map_get(7)->target==SMK37_MAP_TEMPO,"LIVE SETUP and new targets survive reboot");
  printf("fails=%d\n",fails); return fails!=0;
}
