/* V1.48 OTA recipient checks. Uses the same simulated NOR/SysEx host as the
 * original ETCH/Felucca test. NEVER writes to physical hardware. */
#define main old_ota_main
#include "test_native_ota.c"
#undef main
static int load_pkg148(const char *name){
 FILE *f=fopen(name,"rb");if(!f){perror(name);return 0;}
 uint8_t *raw=malloc(0x200000);size_t n=fread(raw,1,0x200000,f);fclose(f);
 if(n<960){free(raw);return 0;}
 free(logical);logical=malloc(n);
 for(size_t j=0;j<20;j++)memcpy(logical+j*47,raw+j*48,47);
 memcpy(logical+940,raw+960,n-960);logical_len=n-20;
 free(raw);return 1;
}
static int test_on_head(const char *what,const char *package,const uint8_t head[0x4000],int want){
 if(!load_pkg148(package))return 1;
 memcpy(installed_head,head,0x4000);test_cross_install=1;
 reset_dev();int rc=ota_session();
 int ok=(rc==want && (rc==0)==!!committed && !memcmp(nor,head,0x4000) && !bad_erase);
 printf("%-55s %s (code=%d, requests=%u)\n",what,ok?"PASS":"FAIL",rc,requests);
 return !ok;
}
int main(int argc,char **argv){
 if(argc!=3 && argc!=4){fprintf(stderr,"usage: test_native_v148_cross ETCH.fwsc STOCK-HEAD-ETCH.fwsc [ORIGINAL-V15.fwsc]\n");return 2;}
 if(!load_pkg148(argv[1]))return 2;
 uint8_t etch_head[0x4000],stock_head[0x4000],garbage_head[0x4000];
 memcpy(etch_head,logical+0x400,0x4000);
 if(!load_pkg148(argv[2]))return 2;
 memcpy(stock_head,logical+0x400,0x4000);
 memcpy(garbage_head,etch_head,0x4000);garbage_head[0x111]^=0x23;
 int bad=0;
 bad+=test_on_head("Same ETCH boothead",argv[1],etch_head,0);
 bad+=test_on_head("ETCh app with stock boothead",argv[1],stock_head,0);
 bad+=test_on_head("Stockhead app on installed ETCH",argv[2],etch_head,0);
 bad+=test_on_head("Stockhead app on installed stock",argv[2],stock_head,0);
 bad+=test_on_head("Different installed boothead preserved",argv[2],garbage_head,0);
 if(argc==4)bad+=test_on_head("Pinned exact original V15 restore from ETCH",argv[3],etch_head,0);
 else puts("Original V15 restore test SKIPPED (supply pinned vendor package).");
 // Unknown staging data must not be cleared without a valid owned stage.
 if(!load_pkg148(argv[1]))return 2;
 memcpy(installed_head,etch_head,0x4000);reset_dev();nor[0xE1000]=0;
 int rc=ota_session();bad+=check("Foreign OTA staging refused before erase",rc==-15&&!committed&&nor[0xE1000]==0);
 free(logical);
 return !!bad;
}
