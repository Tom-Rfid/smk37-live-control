/* Drives the SMK37 Open simulator: BT long press -> MAP page, assignments,
 * knob/fader behaviour, MIDI CC output, persistence. Dumps RGB565 frames. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "smk37_controls.h"
#include "smk37_map.h"
#include "instrument.h"
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
  knob(1,5); knob(2,74); knob(3,1);
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
  printf("fails=%d\n",fails); return fails!=0;
}
