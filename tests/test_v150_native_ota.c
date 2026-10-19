/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Host test of firmware/src/ota.c: a fake M-UPGRADE serves a real .fwsc over
 * the SysEx protocol (pack7, cmd 0x30 reads, 0xE0000000 -> "success") and a
 * simulated NOR plays the flash.
 *   cc -o ota_test tests/ota_test.c && ./ota_test FILE.fwsc
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FELUCCA_ID "FM-1_1500"
#define FELUCCA_OTA_DRYRUN 0

static uint8_t nor[0x100000];
static uint8_t installed_head[0x4000];
static int test_cross_install;
static uint8_t *logical;
static size_t logical_len;
static uint8_t rx[1024];                 /* one pending host frame (7-bit bytes) */
static uint32_t rx_len, rx_full;
static uint32_t now_ms, requests, ident_replies, mute_after = 0xFFFFFFFFu, bad_erase;
static int committed;
static uint8_t committed_parm[112];
static int32_t last_code;
static uint32_t io_ops,fail_at,partial,fail_forever,cancel_after,cancel_at_io;
static int fault(void){++io_ops;return fail_at&&(fail_forever?io_ops>=fail_at:io_ops==fail_at);}
static uint8_t *corrupt_at;              /* flip a served byte (loader body) */

static uint32_t pack7(const uint8_t *in, uint32_t n, uint8_t *out)
{
    uint32_t acc = 0, nb = 0, o = 0;
    while (n--) {
        acc |= (uint32_t)*in++ << nb;
        nb += 8;
        while (nb >= 7) { out[o++] = acc & 0x7F; acc >>= 7; nb -= 7; }
    }
    if (nb) out[o++] = acc & 0x7F;
    return o;
}

static uint32_t unpack7h(const uint8_t *in, uint32_t n, uint8_t *out)
{
    uint32_t acc = 0, nb = 0, o = 0;
    while (n--) {
        acc |= (uint32_t)*in++ << nb;
        nb += 7;
        if (nb >= 8) { out[o++] = (uint8_t)acc; acc >>= 8; nb -= 8; }
    }
    return o;
}

static void host_reply(const uint8_t *msg, uint32_t n)       /* queue msg as a device-bound frame */
{
    rx_len = pack7(msg, n, rx);
    rx_full = 1;
}

/* ---- hooks ---- */
static int ota_wire_send(const uint8_t *p, uint32_t n)
{
    uint8_t u[64], m[600];
    uint32_t d, addr, len, i, s = 0;
    if (p[0] != 0xF0 || p[n - 1] != 0xF7) { printf("bad wire framing\n"); exit(1); }
    d = unpack7h(p + 1, n - 2, u);
    if (u[2] == 0x11) { ident_replies++; return 0; }
    if (d != 15 || u[2] != 0x30) { printf("unexpected device msg cmd %02x len %u\n", u[2], d); exit(1); }
    addr = u[7] | u[8] << 8 | u[9] << 16 | (uint32_t)u[10] << 24;
    len = u[11] | u[12] << 8 | u[13] << 16;
    if (++requests > mute_after)
        return 0;                                         /* host gone */
    memcpy(m, "\x00\x59\x30", 3);
    m[3] = (uint8_t)(len + 8); m[4] = (uint8_t)((len + 8) >> 8); m[5] = 0;
    m[6] = 0;
    memcpy(m + 7, &addr, 4);
    m[11] = (uint8_t)len; m[12] = (uint8_t)(len >> 8); m[13] = 0;
    if (addr == 0xE0000000u) {
        memset(m + 14, 0, len);
        memcpy(m + 14, "success", 8);
    } else {
        if (addr + len > logical_len) { printf("read past the package %#x\n", addr); exit(1); }
        memcpy(m + 14, logical + addr, len);
        if (corrupt_at && corrupt_at >= logical + addr && corrupt_at < logical + addr + len)
            m[14 + (corrupt_at - (logical + addr))] ^= 0x01;
    }
    for (i = 6; i < 14 + len; i++) s += m[i];
    m[14 + len] = (uint8_t)~s;
    host_reply(m, 15 + len);
    return 0;
}
static int ota_frame_get(const uint8_t **p, uint32_t *n)
{
    if (!rx_full) return 0;
    *p = rx; *n = rx_len;
    return 1;
}
static void ota_frame_done(void) { rx_full = 0; }
static uint32_t ota_now_ms(void) { return now_ms; }
static void ota_idle(void) { now_ms += 1; }
static int ota_aborted(void){return (cancel_after&&requests>=cancel_after)||(cancel_at_io&&io_ops>=cancel_at_io);}
static int ota_erase(uint32_t off)
{
    if (off < 0xE0000u || off >= 0xE5000u || (off & 0xFFFu)) { bad_erase++; return -8; }
    int cut=fault();memset(nor + off, 0xFF, cut?(partial?2048:0):4096);
    if(cut)return -1;
    return 0;
}
static int ota_prog(uint32_t off, const void *src, uint32_t n)
{
    const uint8_t *s = src;
    uint32_t i;
    if (off < 0xE0000u || off + n > 0xE5000u) { bad_erase++; return -8; }
    while(n){uint32_t k=256u-(off&255u);if(k>n)k=n;int cut=fault();uint32_t take=cut?(partial?k/2:0):k;
        for(i=0;i<take;i++)nor[off+i]&=s[i];if(cut)return -1;off+=k;s+=k;n-=k;}
    return 0;
}
static int ota_fread(uint32_t off, void *dst, uint32_t n) {
    if(!((off<=0x4000u&&n<=0x4000u-off)||(off>=0xE0000u&&off<=0xE5000u&&n<=0xE5000u-off))){bad_erase++;return -1;}
    if(fault())return -1;memcpy(dst,nor+off,n);return 0;
}
static void ota_show(uint32_t step, int32_t code) { if (step == 9) last_code = code; }
static void ota_commit(const uint8_t *parm) { committed = 1; memcpy(committed_parm, parm, 112); }

#include "../firmware/fm1-native/ota.c"

static uint32_t ota_body_off(void)                  /* ota.bin data + 0x20 in the logical image */
{
    uint8_t h[0x400];
    uint32_t i, n;
    memcpy(h, logical, sizeof h);
    ota_jl_enc(h, 0x40);
    n = ota_rd16(h + 8);
    for (i = 0; i < n; i++) {
        uint8_t *e = h + 0x40 + i * 0x50u;
        ota_jl_enc(e, 0x50);
        if (ota_rd16(e) == 100) return ota_rd32(e + 8) + 0x20u;
    }
    return 0;
}

static void edit_entry(uint32_t index,uint32_t field,uint32_t value,uint8_t word){
 uint8_t *e=logical+0x40u+index*0x50u;ota_jl_enc(e,0x50);if(word)ota_wr16(e+field,value);else ota_wr32(e+field,value);ota_jl_enc(e,0x50);
 ota_jl_enc(logical,0x40);ota_wr16(logical+2,ota_crc16(logical+0x40,ota_rd16(logical+8)*0x50,0));ota_wr16(logical,ota_crc16(logical+2,62,0));ota_jl_enc(logical,0x40);
}
static int check(const char *what, int ok)
{
    printf("%-52s %s\n", what, ok ? "ok" : "FAIL");
    return ok ? 0 : 1;
}

static int area_erased(void)
{
    uint32_t i;
    for (i = 0xE0000; i < 0xE5000; i++) if (nor[i] != 0xFF) return 0;
    return 1;
}

static void reset_dev(void)
{
    memset(nor, 0xFF, sizeof nor);
    memset(nor+0x97000,0x4d,0xA000);
    io_ops=fail_at=partial=fail_forever=cancel_after=cancel_at_io=0;
    memcpy(nor, test_cross_install ? installed_head : logical + 0x400, 0x4000); /* cross-firmware boot head or package head */
    rx_full = 0; requests = 0; ident_replies = 0; committed = 0; last_code = 0;
    mute_after = 0xFFFFFFFFu; corrupt_at = NULL; bad_erase = 0;
}

int main(int argc, char **argv)
{
    FILE *f;
    uint8_t *raw;
    size_t n, i;
    int bad = 0, rc;
    if (argc < 2) { printf("usage: ota_test FILE.fwsc\n"); return 2; }
    f = fopen(argv[1], "rb");
    if (!f) { perror(argv[1]); return 2; }
    raw = malloc(0x200000);
    n = fread(raw, 1, 0x200000, f);
    fclose(f);
    if (n < 20 * 48 + 0x4400) { printf("%s: too short for a package\n", argv[1]); return 2; }
    logical = malloc(n);
    for (i = 0; i < 20; i++) memcpy(logical + i * 47, raw + i * 48, 47);   /* drop the marker bytes */
    memcpy(logical + 20 * 47, raw + 20 * 48, n - 20 * 48);
    logical_len = n - 20;
    /* V0.94 regression: real currently-installed V0.89 boot head vs new FWSC.
     * Pass a 16384-byte raw reference head as second CLI argument. Nothing
     * touches hardware. Emits CHECK phase result and refuses silent success. */
    if (argc >= 3) {
        f = fopen(argv[2], "rb");
        if (!f) { perror(argv[2]); return 2; }
        n = fread(installed_head, 1, sizeof installed_head, f);
        if (n != sizeof installed_head || fgetc(f) != EOF) {
            fprintf(stderr, "Reference head must be exactly 16384 bytes\n");
            fclose(f); return 2;
        }
        fclose(f);
        test_cross_install = 1;
        reset_dev();
        rc = ota_session();
        /* Native CHECK/commit for the ACTUAL file, with the known original
         * V15 head installed. No host OTA executable is ever staged for an
         * app-only update: staged bytes MUST be our audited loader. */
        int body_safe=0;
        uint32_t body_off=ota_body_off();
        uint32_t body_len=ota_rd32(logical+body_off-32u+8u);
        if(body_len==ETCH_OTA_BODY_LEN){
          body_safe=!memcmp(nor+OTA_AREA+32u,etch_trusted_app_loader,ETCH_OTA_BODY_LEN);
        }else if(body_len==ETCH_STOCK_OTA_BODY_LEN){
          body_safe=!memcmp(nor+OTA_AREA+32u,logical+body_off,body_len);
        }
        printf("CROSS_OTA_RESULT=%d REQUESTS=%u COMMITTED=%d BOOT_HEAD_UNCHANGED=%d TRUSTED_STAGED_LOADER=%d OTA_ID=%.*s\n",
               rc, requests, committed, !memcmp(nor, installed_head, sizeof installed_head),
               body_safe,12,committed_parm+8);
        return (rc == 0 && committed && body_safe && !memcmp(nor, installed_head, sizeof installed_head)) ? 0 : 1;
    }


    /* handshake outside a session */
    reset_dev();
    {
        uint8_t m[7] = {0, 0x59, 0x11, 0, 0, 0, 0xFF};
        host_reply(m, 7);
        ota_service();
        bad += check("handshake answered", ident_replies == 1);
        bad += check("identity frame is " FELUCCA_ID,
                     ota_msg[2] == 0x11 && !memcmp(ota_msg + 6, FELUCCA_ID, sizeof FELUCCA_ID - 1));
    }

    /* full session */
    reset_dev();
    rc = ota_session();
    printf("  %u requests\n", requests);
    bad += check("session commits", rc == 0 && committed);
    bad += check("header, trusted OTA source, chip key, confirmation", (requests >= 20 && requests <= 70));
    {
        uint8_t *h = nor + 0xE0000, *p = committed_parm;
        bad += check("staged body == the package's loader", !memcmp(nor + 0xE0020, logical + ota_body_off(), 64));
        bad += check("LOADER.BIN head CRC",
                     (h[0] | h[1] << 8) == ota_crc16(h + 2, 30, 0) && !memcmp(h + 16, "LOADER.BIN", 10));
        bad += check("record at 0xE4F00 == RAM record", !memcmp(nor + 0xE4F00, p, 112));
        bad += check("record CRC / type / magic / ota_addr",
                     (p[0] | p[1] << 8) == ota_crc16(p + 2, 78, 0) && (p[2] | p[3] << 8) == 0x5A0D &&
                     (p[6] | p[7] << 8) == 0x5441 && ota_rd32(p + 72) == 0xE0000);
        bad += check("nothing written outside 0xE0000..0xE4FFF", bad_erase == 0);
    }
    /* An intact stage can be explicitly reused, without unattended boot writes. */
    committed=0;requests=0;rc=ota_session();
    bad+=check("explicit retry of owned staging commits",rc==0&&committed);
    /* Real community loader clears commit sector on success, keeping body/head. */
    memset(nor+0xE4000,255,4096);committed=0;requests=0;rc=ota_session();
    bad+=check("loader after last-sector erase is handled safely",
               (ota_body_off()>=0xA0000u) ? (rc<0&&!committed) : (rc==0&&committed));

    /* host disappears in the middle */
    reset_dev();
    mute_after = 4;
    rc = ota_session();
    bad += check("host gone -> no commit, area clean", rc < 0 && !committed && area_erased());

    /* host never confirms */
    reset_dev();
    mute_after = 0xFFFFFFFFu;
    {
        uint32_t total;
        rc = ota_session();
        total = requests;                                  /* requests of a full run incl. confirm */
        reset_dev();
        mute_after = total - 1;                            /* answer everything but the confirmation */
        rc = ota_session();
        bad += check("no 'success' -> -13, no commit, area clean", rc == -13 && !committed && area_erased());
    }

    reset_dev();nor[0xE1000]=0;rc=ota_session();
    bad+=check("unknown staging refused without erase",rc==-15&&!committed&&nor[0xE1000]==0);
    reset_dev();corrupt_at=logical+ota_body_off()+80;rc=ota_session();
    bad+=check("altered loader refused before erase",rc==-16&&!committed&&area_erased());
    reset_dev();nor[0x100]^=1;rc=ota_session();
    bad+=check("device boot bytes preserved by app-only update",rc==0&&committed&&(nor[0x100] == (uint8_t)(logical[0x400+0x100]^1)));
    uint8_t original_header[1024];memcpy(original_header,logical,1024);
    edit_entry(0,8,0xfffffff0u,0);reset_dev();rc=ota_session();
    bad+=check("overflowing package address refused before erase",rc<0&&!committed&&area_erased());memcpy(logical,original_header,1024);
    edit_entry(0,0,100,1);edit_entry(0,8,0,0);reset_dev();rc=ota_session();
    bad+=check("duplicate zero-offset OTA entry refused",rc==-3&&!committed&&area_erased());memcpy(logical,original_header,1024);
    logical[2]^=1;reset_dev();rc=ota_session();bad+=check("corrupt UFW header refused before erase",rc==-2&&!committed&&area_erased());memcpy(logical,original_header,1024);
    reset_dev();rc=ota_session();uint32_t baseline=io_ops;int safe=rc==0;
    for(uint32_t cut=1;cut<=baseline;cut++)for(uint32_t half=0;half<2;half++){
        reset_dev();fail_at=cut;partial=half;rc=ota_session();
        safe&=rc<0&&!committed&&!bad_erase&&!memcmp(nor,logical+0x400,0x4000);
        for(uint32_t i=0x97000;i<0xA1000;i++)safe&=nor[i]==0x4d;
    }
    printf("  %u success-path I/O points, zero/half-write API faults\n",baseline);
    bad+=check("every read/erase/page/record fault refuses commit",safe);
    reset_dev();fail_at=105;fail_forever=1;rc=ota_session();
    bad+=check("persistent flash fault reports cleanup failure",rc==-17&&!committed&&!bad_erase);
    reset_dev();fail_at=baseline;fail_forever=1;rc=ota_session();
    uint8_t *pending=nor+OTA_RES;
    bad+=check("late cleanup fault can retain confirmed record",rc==-17&&!committed&&ota_rd16(pending)==ota_crc16(pending+2,78,0));
    reset_dev();cancel_at_io=80;rc=ota_session();
    bad+=check("cancel during staging cleans without commit",rc<0&&!committed&&area_erased());
    /* Same shape as the GitHub report: host's executable OTA payload differs.
     * ETCH must stage its OWN pinned app-only loader, not the host's. */
    {
        uint32_t off=ota_body_off();
        uint8_t save=logical[off+100];
        logical[off+100]^=0x17;
        uint32_t h=off-32u;
        ota_wr16(logical+h+2,ota_crc16(logical+off,ETCH_OTA_BODY_LEN,0));
        ota_wr16(logical+h,ota_crc16(logical+h+2,30,0));
        reset_dev();rc=ota_session();
        bad+=check("foreign OTA binary is NOT executed; trusted loader staged",
            rc==0 && committed && !memcmp(nor+OTA_AREA+32,etch_trusted_app_loader,ETCH_OTA_BODY_LEN) &&
            !memcmp(committed_parm+8,"ota-FM-1_015",12));
        logical[off+100]=save;
        /* Reconstruct the original pinned package's OTA entry. */
        ota_wr16(logical+h+2,ota_crc16(logical+off,ETCH_OTA_BODY_LEN,0));
        ota_wr16(logical+h,ota_crc16(logical+h+2,30,0));
    }
    /* A structurally valid package whose boot header bytes are different
     * remains eligible: the trusted app-only loader cannot overwrite them. */
    {
        uint32_t pos=0x400u+0x100u;
        uint8_t save=logical[pos];logical[pos]^=0x3C;
        reset_dev();rc=ota_session();
        bad+=check("foreign protected header accepted and device boot retained",
             rc==0 && committed && !memcmp(nor,logical+0x400,0x4000));
        logical[pos]=save;
    }
    /* A structurally correct ISD config with the WRONG chip key must be
     * rejected even after rebuilding both JieLi entry CRCs. */
    {
        uint8_t head[0xA0],saved_entry[32];
        memcpy(head,logical+0x400u,sizeof head);
        for(uint32_t k=0;k<sizeof head;k+=32u)ota_jl_enc(head+k,32);
        uint8_t *entry=head+0x40u;
        uint32_t isd_off=ota_rd32(entry+4u),isd_len=ota_rd32(entry+8u);
        if(isd_len>=32u && isd_len<0x4000u && isd_off<0x4000u-isd_len){
           memcpy(saved_entry,logical+0x400u+0x40u,32u);
           uint8_t saved=logical[0x400u+isd_off+16u];
           logical[0x400u+isd_off+16u]^=0x40u;
           ota_wr16(entry+2u,ota_crc16(logical+0x400u+isd_off,isd_len,0));
           ota_wr16(entry,ota_crc16(entry+2,30,0));
           ota_jl_enc(entry,32);
           memcpy(logical+0x400u+0x40u,entry,32u);
           reset_dev();rc=ota_session();
           bad+=check("wrong FM1 chip key rejected before erase",rc==-21&&!committed&&area_erased());
           logical[0x400u+isd_off+16u]=saved;
           memcpy(logical+0x400u+0x40u,saved_entry,32u);
        }else bad+=check("ISD config fixture is valid",0);
    }
    /* Image mismatch in app metadata is a hard failure before any erase. */
    {
        uint32_t pos=0x400u+0x4000u;
        uint8_t save=logical[pos];logical[pos]^=0x77;
        reset_dev();rc=ota_session();
        bad+=check("incompatible/corrupted app directory refused safely",
             rc==-21 && !committed && area_erased());
        logical[pos]=save;
    }
    reset_dev();now_ms=0xfffffff0u;mute_after=0;rc=ota_session();
    bad+=check("timeout across millisecond rollover refuses",rc<0&&!committed&&area_erased());
    printf("%s\n", bad ? "OTA TEST FAILED" : "ota test passed");
    return bad != 0;
}
