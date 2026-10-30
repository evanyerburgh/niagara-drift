/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Charles Vestal */
/* Niagara Drift: the flash core of fm1-base's restore (https://github.com/charlesvestal/fm1-base, device/backup.c,
 * commit d8ff8f4; its own from Polly's), taken as it is there, MPL-2.0 (LICENSES/MPL-2.0.txt): the staging of each
 * object without its header, the journal record, the commit and its finish at power-on. Around it, as there: the
 * object list, the buffer, the helpers they use; the hooks are Niagara Drift's (main_fm1.c), as is everything else
 * of the restore (the USB commands, the question, the installers). Only its pieces are here, as they are there:
 *
 *   2. stages each object as it comes into the copy storage.c would save it to next (the other of its A/B pair:
 *      erased, the payload programmed, read back and checked) WITHOUT its header: storage.c's header is the commit
 *      record of a copy, so a copy without one does not exist for st_load: the current set stays in charge;
 *   4. yes: commits. A journal record (RS_REC, in the update's staging area, unused but by an update) lists the
 *      headers to write; written and read back first, then the headers, then the record erased. A power loss after
 *      the record is valid is finished at the next boot (rs_boot: the headers written again, idempotent on NOR);
 *      before it, nothing happened. So the set changes whole or not at all.
 *
 * Needs, before it: storage.c (st_*), APP_BACKUP_OBJS, and the hooks rs_rec_erase / rs_rec_prog / rs_rec_read
 * (the journal record's sector). */
static const uint16_t bu_objs[] = APP_BACKUP_OBJS;
#define BU_NOBJ ((uint32_t)(sizeof bu_objs / sizeof bu_objs[0]))
#define RS_REC 0x000E1000u                            /* the journal record: the update's staging area's 2nd sector */
#define RS_REC_MAGIC 0x31534C50u                      /* "PLS1" */
enum { RS_E_SIZE = 1, RS_E_FETCH, RS_E_HEAD, RS_E_OBJ, RS_E_CRC, RS_E_TOTAL, RS_E_ERASE, RS_E_PROG, RS_E_VERIFY,
       RS_E_PLAYING, RS_E_NO, RS_E_COMMIT };

static int rs_rec_erase(void);
static int rs_rec_prog(uint32_t off, const void *src, uint32_t n);
static int rs_rec_read(uint32_t off, void *dst, uint32_t n);

static uint8_t bu_buf[ST_PAYLOAD_MAX] __attribute__((aligned(4)));   /* an object, backup or restore */

static uint32_t bu_rd16(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8; }
static uint32_t bu_rd32(const uint8_t *p) { return bu_rd16(p) | bu_rd16(p + 2) << 16; }
static uint32_t bu_obj(uint32_t i) { return bu_objs[i]; }

static uint32_t bu_crc(uint32_t c, const uint8_t *p, uint32_t n)   /* zlib's crc32(c, p, n): chains */
{
    static const uint32_t T[16] = {
        0x00000000u, 0x1DB71064u, 0x3B6E20C8u, 0x26D930ACu, 0x76DC4190u, 0x6B6B51F4u, 0x4DB26158u, 0x5005713Cu,
        0xEDB88320u, 0xF00F9344u, 0xD6D6A3E8u, 0xCB61B38Cu, 0x9B64C2B0u, 0x86D3D2D4u, 0xA00AE278u, 0xBDBDF21Cu};
    c = ~c;
    while (n--) {
        c ^= *p++;
        c = (c >> 4) ^ T[c & 15u];
        c = (c >> 4) ^ T[c & 15u];
    }
    return ~c;
}

/* ------------------------------------------------------------ restore --- */
typedef struct {
    uint16_t obj, copy;
    uint32_t seq, len, crc;
} rs_ent_t;
static struct {
    uint32_t magic, n;
    rs_ent_t e[BU_NOBJ];
    uint32_t crc;
} rs_rec __attribute__((aligned(4)));
_Static_assert(sizeof rs_rec <= 4096u, "the journal record fits its sector: fewer objects (APP_BACKUP_OBJS)");

static void rs_head(const rs_ent_t *e, st_hdr_t *h)   /* the header storage.c would write: the copy's commit */
{
    h->magic = ST_MAGIC;
    h->type = e->obj;
    h->slot = e->copy;
    h->seq = e->seq;
    h->len = e->len;
    h->crc = e->crc;
    h->rsv[0] = h->rsv[1] = 0xFFFFFFFFu;
    h->hcrc = st_crc32(h, sizeof *h - 4u);
}

/* object e->obj's payload (bu_buf, e->len bytes) into the copy its next save would take, without its header (so
 * not there yet for st_load); the seq that will make it current */
static int rs_stage(rs_ent_t *e)
{
    st_hdr_t h;
    int cur = st_current(e->obj, &h);
    uint32_t base, off;
    e->copy = (uint16_t)(cur == 0 ? 1u : 0u);
    e->seq = cur < 0 ? 1u : h.seq + 1u;
    base = st_sector(e->obj, e->copy);
    if (st_erase(base))
        return RS_E_ERASE;
    for (off = 0; off < e->len; off += 256u) {
        uint32_t n = e->len - off > 256u ? 256u : e->len - off;
        if (st_prog(base + ST_PAYLOAD_OFF + off, bu_buf + off, n))
            return RS_E_PROG;
    }
    if (st_read(base, &h, sizeof h) || st_read(base + ST_PAYLOAD_OFF, st_buf, e->len) ||
        bu_crc(0, st_buf, e->len) != e->crc)
        return RS_E_VERIFY;
    for (off = 0; off < sizeof h; off++)              /* its header still erased: the copy not there */
        if (((const uint8_t *)&h)[off] != 0xFFu)
            return RS_E_VERIFY;
    return 0;
}

static int rs_rec_valid(void)
{
    return rs_rec.magic == RS_REC_MAGIC && rs_rec.n == BU_NOBJ &&
           rs_rec.crc == bu_crc(0, (const uint8_t *)&rs_rec, sizeof rs_rec - 4u);
}

/* the staged copies' headers (the record's), each written unless already there; 0 = all there */
static int rs_heads(void)
{
    int rc = 0;
    for (uint32_t i = 0; i < BU_NOBJ; i++) {
        st_hdr_t h, f;
        const rs_ent_t *e = &rs_rec.e[i];
        uint32_t base = st_sector(e->obj, e->copy);
        rs_head(e, &h);
        if (st_read(base, &f, sizeof f))
            rc = -1;
        else if (memcmp(&f, &h, sizeof h) && st_prog(base, &h, sizeof h))
            rc = -1;
    }
    for (uint32_t i = 0; i < BU_NOBJ; i++) {          /* each the current copy now */
        st_hdr_t h;
        const rs_ent_t *e = &rs_rec.e[i];
        if (st_current(e->obj, &h) != (int)e->copy || h.seq != e->seq || h.crc != e->crc || h.len != e->len)
            rc = -1;
    }
    return rc;
}

/* boot: a commit cut short (a power loss after its record was written) finished: 1; 0 none; -1 failed */
static int rs_boot(void)
{
    uint8_t b[4];
    uint32_t i;
    int rc;
    if (rs_rec_read(0, &rs_rec, sizeof rs_rec))
        return -1;
    if (!rs_rec_valid()) {
        for (i = 0; i < 4u; i++)                     /* a record half written or half erased: nothing to do; gone */
            b[i] = ((const uint8_t *)&rs_rec)[i];
        if (bu_rd32(b) != 0xFFFFFFFFu)
            rs_rec_erase();
        return 0;
    }
    for (i = 0; i < BU_NOBJ; i++)                     /* (a record of objects storage.c does not have: ignored) */
        if (rs_rec.e[i].obj != bu_obj(i) || rs_rec.e[i].copy > 1u || rs_rec.e[i].len > ST_PAYLOAD_MAX) {
            rs_rec_erase();
            return 0;
        }
    rc = rs_heads();
    if (rc == 0)
        rs_rec_erase();
    return rc ? -1 : 1;
}

/* every object staged: the record, the headers, the record gone. 0 = the new set is current */
static int rs_commit(void)
{
    uint32_t off;
    rs_rec.magic = RS_REC_MAGIC;
    rs_rec.n = BU_NOBJ;
    rs_rec.crc = bu_crc(0, (const uint8_t *)&rs_rec, sizeof rs_rec - 4u);
    if (rs_rec_erase())
        return RS_E_ERASE;
    for (off = 0; off < sizeof rs_rec; off += 256u) {
        uint32_t n = sizeof rs_rec - off > 256u ? 256u : sizeof rs_rec - off;
        if (rs_rec_prog(off, (const uint8_t *)&rs_rec + off, n))
            return RS_E_PROG;
    }
    {   /* read back: a record that is not whole commits nothing (the old set stays) */
        uint32_t want = rs_rec.crc;
        if (rs_rec_read(0, &rs_rec, sizeof rs_rec) || !rs_rec_valid() || rs_rec.crc != want) {
            rs_rec_erase();
            return RS_E_VERIFY;
        }
    }
    if (rs_heads())                                   /* (the record stays: the next boot tries again) */
        return RS_E_COMMIT;
    rs_rec_erase();
    return 0;
}
