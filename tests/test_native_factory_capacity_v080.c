#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/fm1-native/native_factory.h"
#include "../firmware/fm1-native/native_project_codec.h"
static struct etch_wave_library l;
static struct etch_native_pattern p;
static struct etch_project_settings cfg;
int main(void){etch_factory_seed(&l);etch_pattern_clear(&p);memset(&cfg,0,sizeof cfg);cfg.source_version=11;cfg.seq_length=16;cfg.tempo=120;cfg.seq_gate=75;uint32_t size=0;int r=etch_project_measure(&l,&p,&cfg,&size);printf("r=%d size=%u max=%u\n",r,size,ETCH_PROJECT_MAX_BYTES);assert(r==0 && size<=ETCH_PROJECT_MAX_BYTES);}
