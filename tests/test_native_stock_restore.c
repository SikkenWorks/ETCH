#define ETCH_TEST_RESTORE_ONLY 1
#define main generic_ota_main
#include "test_native_ota.c"
#undef main
static int load(const char *path){FILE *f=fopen(path,"rb");if(!f)return 0;uint8_t *raw=malloc(0x200000);size_t n=fread(raw,1,0x200000,f);fclose(f);if(n<1024)return 0;free(logical);logical=malloc(n);for(size_t i=0;i<20;i++)memcpy(logical+i*47,raw+i*48,47);memcpy(logical+940,raw+960,n-960);logical_len=n-20;free(raw);return 1;}
int main(int argc,char **argv){
 if(argc!=3||!load(argv[1]))return 2;
 reset_dev();int rc=ota_session(),bad=0;bad+=check("stock-only build accepts exact V15 restore image",rc==0&&committed);
 if(!load(argv[2]))return 2;reset_dev();rc=ota_session();bad+=check("stock-only build rejects custom TEST before erase",rc==-20&&!committed&&area_erased());
 free(logical);return !!bad;
}
