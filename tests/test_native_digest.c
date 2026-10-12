#include "../firmware/fm1-native/native_digest.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void check(const char *input,const char *expected){struct etch_sha256 s;uint8_t out[32];char hex[65];etch_sha256_init(&s);for(unsigned i=0;i<strlen(input);i++)etch_sha256_add(&s,(const uint8_t*)input+i,1);etch_sha256_end(&s,out);for(int i=0;i<32;i++)sprintf(hex+i*2,"%02x",out[i]);assert(!strcmp(hex,expected));}
int main(void){
 check("","e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
 check("abc","ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
 check("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq","248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
 struct etch_sha256 s;uint8_t out[32];char hex[65];uint8_t a=97;etch_sha256_init(&s);for(int i=0;i<1000000;i++)etch_sha256_add(&s,&a,1);etch_sha256_end(&s,out);for(int i=0;i<32;i++)sprintf(hex+i*2,"%02x",out[i]);assert(!strcmp(hex,"cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"));
 puts("SHA-256: empty, abc, two-block boundary and million-byte standard vectors passed");
}
