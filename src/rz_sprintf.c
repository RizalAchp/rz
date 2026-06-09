#include "rz_sprintf.h"

#ifdef RZ_SPRINTF_NOUNALIGNED // define this before inclusion to force rz_sprintf to always use aligned accesses
#    define RZ__UNALIGNED(code)
#else
#    define RZ__UNALIGNED(code) code
#endif

#ifndef RZ_SPRINTF_NOFLOAT
// internal float utility functions
static rz_i32 rz__real_to_str(char const **start, rz_u32 *len, char *out, rz_i32 *decimal_pos, double value, rz_u32 frac_digits);
static rz_i32 rz__real_to_parts(rz_i64 *bits, rz_i32 *expo, double value);
#    define RZ__SPECIAL 0x7000
#endif

static void   rz__lead_sign(rz_u32 fl, char *sign);
static rz_u32 rz__strlen_limited(char const *s, rz_u32 limit);
static char  *rz__clamp_callback(const char *buf, void *user, int len);
static char  *rz__count_clamp_callback(const char *buf, void *user, int len);
static void   rz__raise_to_power10(double *ohi, double *olo, double d, rz_i32 power); // power can be -323 to +350

static char rz__period = '.';
static char rz__comma  = ',';
static struct {
    short temp; // force next field to be 2-byte aligned
    char  pair[201];
} rz__digitpair = {0, "00010203040506070809101112131415161718192021222324"
                      "25262728293031323334353637383940414243444546474849"
                      "50515253545556575859606162636465666768697071727374"
                      "75767778798081828384858687888990919293949596979899"};

RZ_DEF void rz_set_separators(char pcomma, char pperiod) {
    rz__period = pperiod;
    rz__comma  = pcomma;
}

#define RZ__LEFTJUST       1
#define RZ__LEADINGPLUS    2
#define RZ__LEADINGSPACE   4
#define RZ__LEADING_0X     8
#define RZ__LEADINGZERO    16
#define RZ__INTMAX         32
#define RZ__TRIPLET_COMMA  64
#define RZ__NEGATIVE       128
#define RZ__METRIC_SUFFIX  256
#define RZ__HALFWIDTH      512
#define RZ__METRIC_NOSPACE 1024
#define RZ__METRIC_1024    2048
#define RZ__METRIC_JEDEC   4096

static void rz__lead_sign(rz_u32 fl, char *sign) {
    sign[0] = 0;
    if (fl & RZ__NEGATIVE) {
        sign[0] = 1;
        sign[1] = '-';
    } else if (fl & RZ__LEADINGSPACE) {
        sign[0] = 1;
        sign[1] = ' ';
    } else if (fl & RZ__LEADINGPLUS) {
        sign[0] = 1;
        sign[1] = '+';
    }
}

static rz_u32 rz__strlen_limited(char const *s, rz_u32 limit) {
    char const *sn = s;

    // get up to 4-byte alignment
    for (;;) {
        if (((rz_uptr)sn & 3) == 0) break;

        if (!limit || *sn == 0) return (rz_u32)(sn - s);

        ++sn;
        --limit;
    }

    // scan over 4 bytes at a time to find terminating 0
    // this will intentionally scan up to 3 bytes past the end of buffers,
    // but becase it works 4B aligned, it will never cross page boundaries
    // (hence the RZ_ASAN markup; the over-read here is intentional
    // and harmless)
    while (limit >= 4) {
        rz_u32 v = *(rz_u32 *)sn;
        // bit hack to find if there's a 0 byte in there
        if ((v - 0x01010101) & (~v) & 0x80808080UL) break;

        sn += 4;
        limit -= 4;
    }

    // handle the last few characters to find actual size
    while (limit && *sn) {
        ++sn;
        --limit;
    }

    return (rz_u32)(sn - s);
}

RZ_DEF int rz_vsprintfcb(RZ_SprintfCallback *callback, void *user, char *buf, char const *fmt, va_list va) {
    static char hex[]  = "0123456789abcdefxp";
    static char hexu[] = "0123456789ABCDEFXP";
    char       *bf;
    char const *f;
    int         tlen = 0;

    bf               = buf;
    f                = fmt;
    for (;;) {
        rz_i32 fw, pr, tz;
        rz_u32 fl;

// macros for the callback buffer stuff
#define rz__chk_cb_bufL(bytes)                                         \
    {                                                                  \
        int len = (int)(bf - buf);                                     \
        if ((len + (bytes)) >= RZ_SPRINTF_MIN) {                       \
            tlen += len;                                               \
            if (0 == (bf = buf = callback(buf, user, len))) goto done; \
        }                                                              \
    }
#define rz__chk_cb_buf(bytes)       \
    {                               \
        if (callback) {             \
            rz__chk_cb_bufL(bytes); \
        }                           \
    }
#define rz__flush_cb()                                 \
    {                                                  \
        rz__chk_cb_bufL(RZ_SPRINTF_MIN - 1);           \
    } // flush if there is even one byte in the buffer
#define rz__cb_buf_clamp(cl, v)                    \
    cl = v;                                        \
    if (callback) {                                \
        int lg = RZ_SPRINTF_MIN - (int)(bf - buf); \
        if (cl > lg) cl = lg;                      \
    }

        // fast copy everything up to the next % (or end of string)
        for (;;) {
            while (((rz_uptr)f) & 3) {
            schk1:
                if (f[0] == '%') goto scandd;
            schk2:
                if (f[0] == 0) goto endfmt;
                rz__chk_cb_buf(1);
                *bf++ = f[0];
                ++f;
            }
            for (;;) {
                // Check if the next 4 bytes contain %(0x25) or end of string.
                // Using the 'hasless' trick:
                // https://graphics.stanford.edu/~seander/bithacks.html#HasLessInWord
                rz_u32 v, c;
                v = *(rz_u32 *)f;
                c = (~v) & 0x80808080;
                if (((v ^ 0x25252525) - 0x01010101) & c) goto schk1;
                if ((v - 0x01010101) & c) goto schk2;
                if (callback)
                    if ((RZ_SPRINTF_MIN - (int)(bf - buf)) < 4) goto schk1;
#ifdef RZ_SPRINTF_NOUNALIGNED
                if (((rz_uptr)bf) & 3) {
                    bf[0] = f[0];
                    bf[1] = f[1];
                    bf[2] = f[2];
                    bf[3] = f[3];
                } else
#endif
                {
                    *(rz_u32 *)bf = v;
                }
                bf += 4;
                f += 4;
            }
        }
    scandd:

        ++f;

        // ok, we have a percent, read the modifiers first
        fw = 0;
        pr = -1;
        fl = 0;
        tz = 0;

        // flags
        for (;;) {
            switch (f[0]) {
            // if we have left justify
            case '-':
                fl |= RZ__LEFTJUST;
                ++f;
                continue;
            // if we have leading plus
            case '+':
                fl |= RZ__LEADINGPLUS;
                ++f;
                continue;
            // if we have leading space
            case ' ':
                fl |= RZ__LEADINGSPACE;
                ++f;
                continue;
            // if we have leading 0x
            case '#':
                fl |= RZ__LEADING_0X;
                ++f;
                continue;
            // if we have thousand commas
            case '\'':
                fl |= RZ__TRIPLET_COMMA;
                ++f;
                continue;
            // if we have kilo marker (none->kilo->kibi->jedec)
            case '$':
                if (fl & RZ__METRIC_SUFFIX) {
                    if (fl & RZ__METRIC_1024) {
                        fl |= RZ__METRIC_JEDEC;
                    } else {
                        fl |= RZ__METRIC_1024;
                    }
                } else {
                    fl |= RZ__METRIC_SUFFIX;
                }
                ++f;
                continue;
            // if we don't want space between metric suffix and number
            case '_':
                fl |= RZ__METRIC_NOSPACE;
                ++f;
                continue;
            // if we have leading zero
            case '0':
                fl |= RZ__LEADINGZERO;
                ++f;
                goto flags_done;
            default:
                goto flags_done;
            }
        }
    flags_done:

        // get the field width
        if (f[0] == '*') {
            fw = va_arg(va, rz_u32);
            ++f;
        } else {
            while ((f[0] >= '0') && (f[0] <= '9')) {
                fw = fw * 10 + f[0] - '0';
                f++;
            }
        }
        // get the precision
        if (f[0] == '.') {
            ++f;
            if (f[0] == '*') {
                pr = va_arg(va, rz_u32);
                ++f;
            } else {
                pr = 0;
                while ((f[0] >= '0') && (f[0] <= '9')) {
                    pr = pr * 10 + f[0] - '0';
                    f++;
                }
            }
        }

        // handle integer size overrides
        switch (f[0]) {
        // are we halfwidth?
        case 'h':
            fl |= RZ__HALFWIDTH;
            ++f;
            if (f[0] == 'h') ++f; // QUARTERWIDTH
            break;
        // are we 64-bit (unix style)
        case 'l':
            fl |= ((sizeof(long) == 8) ? RZ__INTMAX : 0);
            ++f;
            if (f[0] == 'l') {
                fl |= RZ__INTMAX;
                ++f;
            }
            break;
        // are we 64-bit on intmax? (c99)
        case 'j':
            fl |= (sizeof(size_t) == 8) ? RZ__INTMAX : 0;
            ++f;
            break;
        // are we 64-bit on size_t or ptrdiff_t? (c99)
        case 'z':
            fl |= (sizeof(ptrdiff_t) == 8) ? RZ__INTMAX : 0;
            ++f;
            break;
        case 't':
            fl |= (sizeof(ptrdiff_t) == 8) ? RZ__INTMAX : 0;
            ++f;
            break;
        // are we 64-bit (msft style)
        case 'I':
            if ((f[1] == '6') && (f[2] == '4')) {
                fl |= RZ__INTMAX;
                f += 3;
            } else if ((f[1] == '3') && (f[2] == '2')) {
                f += 3;
            } else {
                fl |= ((sizeof(void *) == 8) ? RZ__INTMAX : 0);
                ++f;
            }
            break;
        default:
            break;
        }

        // handle each replacement
        switch (f[0]) {
#define RZ__NUMSZ 512 // big enough for e308 (with commas) or e-307
            char        num[RZ__NUMSZ];
            char        lead[8];
            char        tail[8];
            char       *s;
            char const *h;
            rz_u32      l, n, cs;
            rz_u64      n64;
#ifndef RZ_SPRINTF_NOFLOAT
            double fv;
#endif
            rz_i32      dp;
            char const *sn;

        case 's':
            // get the string
            s = va_arg(va, char *);
            if (s == 0) s = (char *)"null";
            // get the length, limited to desired precision
            // always limit to ~0u chars since our counts are 32b
            l       = rz__strlen_limited(s, (pr >= 0) ? pr : ~0u);
            lead[0] = 0;
            tail[0] = 0;
            pr      = 0;
            dp      = 0;
            cs      = 0;
            // copy the string in
            goto scopy;

        case 'c': // char
            // get the character
            s       = num + RZ__NUMSZ - 1;
            *s      = (char)va_arg(va, int);
            l       = 1;
            lead[0] = 0;
            tail[0] = 0;
            pr      = 0;
            dp      = 0;
            cs      = 0;
            goto scopy;

        case 'n': // weird write-bytes specifier
        {
            int *d = va_arg(va, int *);
            *d     = tlen + (int)(bf - buf);
        } break;

#ifdef RZ_SPRINTF_NOFLOAT
        case 'A':               // float
        case 'a':               // hex float
        case 'G':               // float
        case 'g':               // float
        case 'E':               // float
        case 'e':               // float
        case 'f':               // float
            va_arg(va, double); // eat it
            s       = (char *)"No float";
            l       = 8;
            lead[0] = 0;
            tail[0] = 0;
            pr      = 0;
            cs      = 0;
            RZ__NOTUSED(dp);
            goto scopy;
#else
        case 'A':                 // hex float
        case 'a':                 // hex float
            h  = (f[0] == 'A') ? hexu : hex;
            fv = va_arg(va, double);
            if (pr == -1) pr = 6; // default is 6
            // read the double into a string
            if (rz__real_to_parts((rz_i64 *)&n64, &dp, fv)) fl |= RZ__NEGATIVE;

            s = num + 64;

            rz__lead_sign(fl, lead);

            if (dp == -1023) dp = (n64) ? -1022 : 0;
            else n64 |= (((rz_u64)1) << 52);
            n64 <<= (64 - 56);
            if (pr < 15) n64 += ((((rz_u64)8) << 56) >> (pr * 4));
            // add leading chars

#    ifdef RZ_SPRINTF_MSVC_MODE
            *s++ = '0';
            *s++ = 'x';
#    else
            lead[1 + lead[0]] = '0';
            lead[2 + lead[0]] = 'x';
            lead[0] += 2;
#    endif
            *s++ = h[(n64 >> 60) & 15];
            n64 <<= 4;
            if (pr) *s++ = rz__period;
            sn = s;

            // print the bits
            n = pr;
            if (n > 13) n = 13;
            if (pr > (rz_i32)n) tz = pr - n;
            pr = 0;
            while (n--) {
                *s++ = h[(n64 >> 60) & 15];
                n64 <<= 4;
            }

            // print the expo
            tail[1] = h[17];
            if (dp < 0) {
                tail[2] = '-';
                dp      = -dp;
            } else tail[2] = '+';
            n       = (dp >= 1000) ? 6 : ((dp >= 100) ? 5 : ((dp >= 10) ? 4 : 3));
            tail[0] = (char)n;
            for (;;) {
                tail[n] = '0' + dp % 10;
                if (n <= 3) break;
                --n;
                dp /= 10;
            }

            dp = (int)(s - sn);
            l  = (int)(s - (num + 64));
            s  = num + 64;
            cs = 1 + (3 << 24);
            goto scopy;

        case 'G': // float
        case 'g': // float
            h  = (f[0] == 'G') ? hexu : hex;
            fv = va_arg(va, double);
            if (pr == -1) pr = 6;
            else if (pr == 0) pr = 1; // default is 6
            // read the double into a string
            if (rz__real_to_str(&sn, &l, num, &dp, fv, (pr - 1) | 0x80000000)) fl |= RZ__NEGATIVE;

            // clamp the precision and delete extra zeros after clamp
            n = pr;
            if (l > (rz_u32)pr) l = pr;
            while ((l > 1) && (pr) && (sn[l - 1] == '0')) {
                --pr;
                --l;
            }

            // should we use %e
            if ((dp <= -4) || (dp > (rz_i32)n)) {
                if (pr > (rz_i32)l) pr = l - 1;
                else if (pr) --pr; // when using %e, there is one digit before the decimal
                goto doexpfromg;
            }
            // this is the insane action to get the pr to match %g semantics for %f
            if (dp > 0) {
                pr = (dp < (rz_i32)l) ? l - dp : 0;
            } else {
                pr = -dp + ((pr > (rz_i32)l) ? (rz_i32)l : pr);
            }
            goto dofloatfromg;

        case 'E':                 // float
        case 'e':                 // float
            h  = (f[0] == 'E') ? hexu : hex;
            fv = va_arg(va, double);
            if (pr == -1) pr = 6; // default is 6
            // read the double into a string
            if (rz__real_to_str(&sn, &l, num, &dp, fv, pr | 0x80000000)) fl |= RZ__NEGATIVE;
        doexpfromg:
            tail[0] = 0;
            rz__lead_sign(fl, lead);
            if (dp == RZ__SPECIAL) {
                s  = (char *)sn;
                cs = 0;
                pr = 0;
                goto scopy;
            }
            s = num + 64;
            // handle leading chars
            *s++ = sn[0];

            if (pr) *s++ = rz__period;

            // handle after decimal
            if ((l - 1) > (rz_u32)pr) l = pr + 1;
            for (n = 1; n < l; n++) *s++ = sn[n];
            // trailing zeros
            tz = pr - (l - 1);
            pr = 0;
            // dump expo
            tail[1] = h[0xe];
            dp -= 1;
            if (dp < 0) {
                tail[2] = '-';
                dp      = -dp;
            } else tail[2] = '+';
#    ifdef RZ_SPRINTF_MSVC_MODE
            n = 5;
#    else
            n = (dp >= 100) ? 5 : 4;
#    endif
            tail[0] = (char)n;
            for (;;) {
                tail[n] = '0' + dp % 10;
                if (n <= 3) break;
                --n;
                dp /= 10;
            }
            cs = 1 + (3 << 24); // how many tens
            goto flt_lead;

        case 'f':               // float
            fv = va_arg(va, double);
        doafloat:
            // do kilos
            if (fl & RZ__METRIC_SUFFIX) {
                double divisor;
                divisor = 1000.0f;
                if (fl & RZ__METRIC_1024) divisor = 1024.0;
                while (fl < 0x4000000) {
                    if ((fv < divisor) && (fv > -divisor)) break;
                    fv /= divisor;
                    fl += 0x1000000;
                }
            }
            if (pr == -1) pr = 6; // default is 6
            // read the double into a string
            if (rz__real_to_str(&sn, &l, num, &dp, fv, pr)) fl |= RZ__NEGATIVE;
        dofloatfromg:
            tail[0] = 0;
            rz__lead_sign(fl, lead);
            if (dp == RZ__SPECIAL) {
                s  = (char *)sn;
                cs = 0;
                pr = 0;
                goto scopy;
            }
            s = num + 64;

            // handle the three decimal varieties
            if (dp <= 0) {
                rz_i32 i;
                // handle 0.000*000xxxx
                *s++ = '0';
                if (pr) *s++ = rz__period;
                n = -dp;
                if ((rz_i32)n > pr) n = pr;
                i = n;
                while (i) {
                    if ((((rz_uptr)s) & 3) == 0) break;
                    *s++ = '0';
                    --i;
                }
                while (i >= 4) {
                    *(rz_u32 *)s = 0x30303030;
                    s += 4;
                    i -= 4;
                }
                while (i) {
                    *s++ = '0';
                    --i;
                }
                if ((rz_i32)(l + n) > pr) l = pr - n;
                i = l;
                while (i) {
                    *s++ = *sn++;
                    --i;
                }
                tz = pr - (n + l);
                cs = 1 + (3 << 24); // how many tens did we write (for commas below)
            } else {
                cs = (fl & RZ__TRIPLET_COMMA) ? ((600 - (rz_u32)dp) % 3) : 0;
                if ((rz_u32)dp >= l) {
                    // handle xxxx000*000.0
                    n = 0;
                    for (;;) {
                        if ((fl & RZ__TRIPLET_COMMA) && (++cs == 4)) {
                            cs   = 0;
                            *s++ = rz__comma;
                        } else {
                            *s++ = sn[n];
                            ++n;
                            if (n >= l) break;
                        }
                    }
                    if (n < (rz_u32)dp) {
                        n = dp - n;
                        if ((fl & RZ__TRIPLET_COMMA) == 0) {
                            while (n) {
                                if ((((rz_uptr)s) & 3) == 0) break;
                                *s++ = '0';
                                --n;
                            }
                            while (n >= 4) {
                                *(rz_u32 *)s = 0x30303030;
                                s += 4;
                                n -= 4;
                            }
                        }
                        while (n) {
                            if ((fl & RZ__TRIPLET_COMMA) && (++cs == 4)) {
                                cs   = 0;
                                *s++ = rz__comma;
                            } else {
                                *s++ = '0';
                                --n;
                            }
                        }
                    }
                    cs = (int)(s - (num + 64)) + (3 << 24); // cs is how many tens
                    if (pr) {
                        *s++ = rz__period;
                        tz   = pr;
                    }
                } else {
                    // handle xxxxx.xxxx000*000
                    n = 0;
                    for (;;) {
                        if ((fl & RZ__TRIPLET_COMMA) && (++cs == 4)) {
                            cs   = 0;
                            *s++ = rz__comma;
                        } else {
                            *s++ = sn[n];
                            ++n;
                            if (n >= (rz_u32)dp) break;
                        }
                    }
                    cs = (int)(s - (num + 64)) + (3 << 24); // cs is how many tens
                    if (pr) *s++ = rz__period;
                    if ((l - dp) > (rz_u32)pr) l = pr + dp;
                    while (n < l) {
                        *s++ = sn[n];
                        ++n;
                    }
                    tz = pr - (l - dp);
                }
            }
            pr = 0;

            // handle k,m,g,t
            if (fl & RZ__METRIC_SUFFIX) {
                char idx;
                idx = 1;
                if (fl & RZ__METRIC_NOSPACE) idx = 0;
                tail[0] = idx;
                tail[1] = ' ';
                {
                    if (fl >> 24) { // SI kilo is 'k', JEDEC and SI kibits are 'K'.
                        if (fl & RZ__METRIC_1024) tail[idx + 1] = "_KMGT"[fl >> 24];
                        else tail[idx + 1] = "_kMGT"[fl >> 24];
                        idx++;
                        // If printing kibits and not in jedec, add the 'i'.
                        if (fl & RZ__METRIC_1024 && !(fl & RZ__METRIC_JEDEC)) {
                            tail[idx + 1] = 'i';
                            idx++;
                        }
                        tail[0] = idx;
                    }
                }
            };

        flt_lead:
            // get the length that we copied
            l = (rz_u32)(s - (num + 64));
            s = num + 64;
            goto scopy;
#endif

        case 'B': // upper binary
        case 'b': // lower binary
            h       = (f[0] == 'B') ? hexu : hex;
            lead[0] = 0;
            if (fl & RZ__LEADING_0X) {
                lead[0] = 2;
                lead[1] = '0';
                lead[2] = h[0xb];
            }
            l = (8 << 4) | (1 << 8);
            goto radixnum;

        case 'o': // octal
            h       = hexu;
            lead[0] = 0;
            if (fl & RZ__LEADING_0X) {
                lead[0] = 1;
                lead[1] = '0';
            }
            l = (3 << 4) | (3 << 8);
            goto radixnum;

        case 'p':                   // pointer
            fl |= (sizeof(void *) == 8) ? RZ__INTMAX : 0;
            pr = sizeof(void *) * 2;
            fl &= ~RZ__LEADINGZERO; // 'p' only prints the pointer with zeros
                                    // fall through - to X

        case 'X':                   // upper hex
        case 'x':                   // lower hex
            h       = (f[0] == 'X') ? hexu : hex;
            l       = (4 << 4) | (4 << 8);
            lead[0] = 0;
            if (fl & RZ__LEADING_0X) {
                lead[0] = 2;
                lead[1] = '0';
                lead[2] = h[16];
            }
        radixnum:
            // get the number
            if (fl & RZ__INTMAX) n64 = va_arg(va, rz_u64);
            else n64 = va_arg(va, rz_u32);

            s  = num + RZ__NUMSZ;
            dp = 0;
            // clear tail, and clear leading if value is zero
            tail[0] = 0;
            if (n64 == 0) {
                lead[0] = 0;
                if (pr == 0) {
                    l  = 0;
                    cs = 0;
                    goto scopy;
                }
            }
            // convert to string
            for (;;) {
                *--s = h[n64 & ((1 << (l >> 8)) - 1)];
                n64 >>= (l >> 8);
                if (!((n64) || ((rz_i32)((num + RZ__NUMSZ) - s) < pr))) break;
                if (fl & RZ__TRIPLET_COMMA) {
                    ++l;
                    if ((l & 15) == ((l >> 4) & 15)) {
                        l &= ~15;
                        *--s = rz__comma;
                    }
                }
            };
            // get the tens and the comma pos
            cs = (rz_u32)((num + RZ__NUMSZ) - s) + ((((l >> 4) & 15)) << 24);
            // get the length that we copied
            l = (rz_u32)((num + RZ__NUMSZ) - s);
            // copy it
            goto scopy;

        case 'u': // unsigned
        case 'i':
        case 'd': // integer
            // get the integer and abs it
            if (fl & RZ__INTMAX) {
                rz_i64 i64 = va_arg(va, rz_i64);
                n64        = (rz_u64)i64;
                if ((f[0] != 'u') && (i64 < 0)) {
                    n64 = (rz_u64)-i64;
                    fl |= RZ__NEGATIVE;
                }
            } else {
                rz_i32 i = va_arg(va, rz_i32);
                n64      = (rz_u32)i;
                if ((f[0] != 'u') && (i < 0)) {
                    n64 = (rz_u32)-i;
                    fl |= RZ__NEGATIVE;
                }
            }

#ifndef RZ_SPRINTF_NOFLOAT
            if (fl & RZ__METRIC_SUFFIX) {
                if (n64 < 1024) pr = 0;
                else if (pr == -1) pr = 1;
                fv = (double)(rz_i64)n64;
                goto doafloat;
            }
#endif

            // convert to string
            s = num + RZ__NUMSZ;
            l = 0;

            for (;;) {
                // do in 32-bit chunks (avoid lots of 64-bit divides even with constant denominators)
                char *o = s - 8;
                if (n64 >= 100000000) {
                    n = (rz_u32)(n64 % 100000000);
                    n64 /= 100000000;
                } else {
                    n   = (rz_u32)n64;
                    n64 = 0;
                }
                if ((fl & RZ__TRIPLET_COMMA) == 0) {
                    do {
                        s -= 2;
                        *(rz_u16 *)s = *(rz_u16 *)&rz__digitpair.pair[(n % 100) * 2];
                        n /= 100;
                    } while (n);
                }
                while (n) {
                    if ((fl & RZ__TRIPLET_COMMA) && (l++ == 3)) {
                        l    = 0;
                        *--s = rz__comma;
                        --o;
                    } else {
                        *--s = (char)(n % 10) + '0';
                        n /= 10;
                    }
                }
                if (n64 == 0) {
                    if ((s[0] == '0') && (s != (num + RZ__NUMSZ))) ++s;
                    break;
                }
                while (s != o)
                    if ((fl & RZ__TRIPLET_COMMA) && (l++ == 3)) {
                        l    = 0;
                        *--s = rz__comma;
                        --o;
                    } else {
                        *--s = '0';
                    }
            }

            tail[0] = 0;
            rz__lead_sign(fl, lead);

            // get the length that we copied
            l = (rz_u32)((num + RZ__NUMSZ) - s);
            if (l == 0) {
                *--s = '0';
                l    = 1;
            }
            cs = l + (3 << 24);
            if (pr < 0) pr = 0;

        scopy:
            // get fw=leading/trailing space, pr=leading zeros
            if (pr < (rz_i32)l) pr = l;
            n = pr + lead[0] + tail[0] + tz;
            if (fw < (rz_i32)n) fw = n;
            fw -= n;
            pr -= l;

            // handle right justify and leading zeros
            if ((fl & RZ__LEFTJUST) == 0) {
                if (fl & RZ__LEADINGZERO) // if leading zeros, everything is in pr
                {
                    pr = (fw > pr) ? fw : pr;
                    fw = 0;
                } else {
                    fl &= ~RZ__TRIPLET_COMMA; // if no leading zeros, then no commas
                }
            }

            // copy the spaces and/or zeros
            if (fw + pr) {
                rz_i32 i;
                rz_u32 c;

                // copy leading spaces (or when doing %8.4d stuff)
                if ((fl & RZ__LEFTJUST) == 0)
                    while (fw > 0) {
                        rz__cb_buf_clamp(i, fw);
                        fw -= i;
                        while (i) {
                            if ((((rz_uptr)bf) & 3) == 0) break;
                            *bf++ = ' ';
                            --i;
                        }
                        while (i >= 4) {
                            *(rz_u32 *)bf = 0x20202020;
                            bf += 4;
                            i -= 4;
                        }
                        while (i) {
                            *bf++ = ' ';
                            --i;
                        }
                        rz__chk_cb_buf(1);
                    }

                // copy leader
                sn = lead + 1;
                while (lead[0]) {
                    rz__cb_buf_clamp(i, lead[0]);
                    lead[0] -= (char)i;
                    while (i) {
                        *bf++ = *sn++;
                        --i;
                    }
                    rz__chk_cb_buf(1);
                }

                // copy leading zeros
                c = cs >> 24;
                cs &= 0xffffff;
                cs = (fl & RZ__TRIPLET_COMMA) ? ((rz_u32)(c - ((pr + cs) % (c + 1)))) : 0;
                while (pr > 0) {
                    rz__cb_buf_clamp(i, pr);
                    pr -= i;
                    if ((fl & RZ__TRIPLET_COMMA) == 0) {
                        while (i) {
                            if ((((rz_uptr)bf) & 3) == 0) break;
                            *bf++ = '0';
                            --i;
                        }
                        while (i >= 4) {
                            *(rz_u32 *)bf = 0x30303030;
                            bf += 4;
                            i -= 4;
                        }
                    }
                    while (i) {
                        if ((fl & RZ__TRIPLET_COMMA) && (cs++ == c)) {
                            cs    = 0;
                            *bf++ = rz__comma;
                        } else *bf++ = '0';
                        --i;
                    }
                    rz__chk_cb_buf(1);
                }
            }

            // copy leader if there is still one
            sn = lead + 1;
            while (lead[0]) {
                rz_i32 i;
                rz__cb_buf_clamp(i, lead[0]);
                lead[0] -= (char)i;
                while (i) {
                    *bf++ = *sn++;
                    --i;
                }
                rz__chk_cb_buf(1);
            }

            // copy the string
            n = l;
            while (n) {
                rz_i32 i;
                rz__cb_buf_clamp(i, n);
                n -= i;
                RZ__UNALIGNED(while (i >= 4) {
                    *(rz_u32 volatile *)bf = *(rz_u32 volatile *)s;
                    bf += 4;
                    s += 4;
                    i -= 4;
                })
                while (i) {
                    *bf++ = *s++;
                    --i;
                }
                rz__chk_cb_buf(1);
            }

            // copy trailing zeros
            while (tz) {
                rz_i32 i;
                rz__cb_buf_clamp(i, tz);
                tz -= i;
                while (i) {
                    if ((((rz_uptr)bf) & 3) == 0) break;
                    *bf++ = '0';
                    --i;
                }
                while (i >= 4) {
                    *(rz_u32 *)bf = 0x30303030;
                    bf += 4;
                    i -= 4;
                }
                while (i) {
                    *bf++ = '0';
                    --i;
                }
                rz__chk_cb_buf(1);
            }

            // copy tail if there is one
            sn = tail + 1;
            while (tail[0]) {
                rz_i32 i;
                rz__cb_buf_clamp(i, tail[0]);
                tail[0] -= (char)i;
                while (i) {
                    *bf++ = *sn++;
                    --i;
                }
                rz__chk_cb_buf(1);
            }

            // handle the left justify
            if (fl & RZ__LEFTJUST)
                if (fw > 0) {
                    while (fw) {
                        rz_i32 i;
                        rz__cb_buf_clamp(i, fw);
                        fw -= i;
                        while (i) {
                            if ((((rz_uptr)bf) & 3) == 0) break;
                            *bf++ = ' ';
                            --i;
                        }
                        while (i >= 4) {
                            *(rz_u32 *)bf = 0x20202020;
                            bf += 4;
                            i -= 4;
                        }
                        while (i--) *bf++ = ' ';
                        rz__chk_cb_buf(1);
                    }
                }
            break;

        default: // unknown, just copy code
            s  = num + RZ__NUMSZ - 1;
            *s = f[0];
            l  = 1;
            fw = fl = 0;
            lead[0] = 0;
            tail[0] = 0;
            pr      = 0;
            dp      = 0;
            cs      = 0;
            goto scopy;
        }
        ++f;
    }
endfmt:

    if (!callback) *bf = 0;
    else rz__flush_cb();

done:
    return tlen + (int)(bf - buf);
}

// cleanup
#undef RZ__LEFTJUST
#undef RZ__LEADINGPLUS
#undef RZ__LEADINGSPACE
#undef RZ__LEADING_0X
#undef RZ__LEADINGZERO
#undef RZ__INTMAX
#undef RZ__TRIPLET_COMMA
#undef RZ__NEGATIVE
#undef RZ__METRIC_SUFFIX
#undef RZ__NUMSZ
#undef rz__chk_cb_bufL
#undef rz__chk_cb_buf
#undef rz__flush_cb
#undef rz__cb_buf_clamp

// ============================================================================
//   wrapper functions

RZ_DEF int rz_sprintf(char *buf, char const *fmt, ...) {
    int     result;
    va_list va;
    va_start(va, fmt);
    result = rz_vsprintfcb(0, 0, buf, fmt, va);
    va_end(va);
    return result;
}

typedef struct rz__context {
    char *buf;
    int   count;
    int   length;
    char  tmp[RZ_SPRINTF_MIN];
} rz__context;

static char *rz__clamp_callback(const char *buf, void *user, int len) {
    rz__context *c = (rz__context *)user;
    c->length += len;

    if (len > c->count) len = c->count;

    if (len) {
        if (buf != c->buf) {
            const char *s, *se;
            char       *d;
            d  = c->buf;
            s  = buf;
            se = buf + len;
            do {
                *d++ = *s++;
            } while (s < se);
        }
        c->buf += len;
        c->count -= len;
    }

    if (c->count <= 0) return c->tmp;
    return (c->count >= RZ_SPRINTF_MIN) ? c->buf : c->tmp; // go direct into buffer if you can
}

static char *rz__count_clamp_callback(const char *buf, void *user, int len) {
    rz__context *c = (rz__context *)user;
    (void)sizeof(buf);

    c->length += len;
    return c->tmp; // go direct into buffer if you can
}

RZ_DEF int rz_vsnprintf(char *buf, int count, char const *fmt, va_list va) {
    rz__context c;

    if ((count == 0) && !buf) {
        c.length = 0;

        rz_vsprintfcb(rz__count_clamp_callback, &c, c.tmp, fmt, va);
    } else {
        int l;

        c.buf    = buf;
        c.count  = count;
        c.length = 0;

        rz_vsprintfcb(rz__clamp_callback, &c, rz__clamp_callback(0, &c, 0), fmt, va);

        // zero-terminate
        l = (int)(c.buf - buf);
        if (l >= count) // should never be greater, only equal (or less) than count
            l = count - 1;
        buf[l] = 0;
    }

    return c.length;
}

RZ_DEF int rz_snprintf(char *buf, int count, char const *fmt, ...) {
    int     result;
    va_list va;
    va_start(va, fmt);

    result = rz_vsnprintf(buf, count, fmt, va);
    va_end(va);

    return result;
}

RZ_DEF int rz_vsprintf(char *buf, char const *fmt, va_list va) {
    return rz_vsprintfcb(0, 0, buf, fmt, va);
}

// =======================================================================
//   low level float utility functions

#ifndef RZ_SPRINTF_NOFLOAT

// copies d to bits w/ strict aliasing (this compiles to nothing on /Ox)
#    define RZ__COPYFP(dest, src)                                                \
        {                                                                        \
            int cn;                                                              \
            for (cn = 0; cn < 8; cn++) ((char *)&dest)[cn] = ((char *)&src)[cn]; \
        }

// get float info
static rz_i32 rz__real_to_parts(rz_i64 *bits, rz_i32 *expo, double value) {
    double d;
    rz_i64 b = 0;

    // load value and round at the frac_digits
    d = value;

    RZ__COPYFP(b, d);

    *bits = b & ((((rz_u64)1) << 52) - 1);
    *expo = (rz_i32)(((b >> 52) & 2047) - 1023);

    return (rz_i32)((rz_u64)b >> 63);
}

static double const rz__bot[23]       = {1e+000, 1e+001, 1e+002, 1e+003, 1e+004, 1e+005, 1e+006, 1e+007, 1e+008, 1e+009, 1e+010, 1e+011,
                                         1e+012, 1e+013, 1e+014, 1e+015, 1e+016, 1e+017, 1e+018, 1e+019, 1e+020, 1e+021, 1e+022};
static double const rz__negbot[22]    = {1e-001, 1e-002, 1e-003, 1e-004, 1e-005, 1e-006, 1e-007, 1e-008, 1e-009, 1e-010, 1e-011,
                                         1e-012, 1e-013, 1e-014, 1e-015, 1e-016, 1e-017, 1e-018, 1e-019, 1e-020, 1e-021, 1e-022};
static double const rz__negboterr[22] = {-5.551115123125783e-018, -2.0816681711721684e-019, -2.0816681711721686e-020, -4.7921736023859299e-021, -8.1803053914031305e-022,
                                         4.5251888174113741e-023, 4.5251888174113739e-024,  -2.0922560830128471e-025, -6.2281591457779853e-026, -3.6432197315497743e-027,
                                         6.0503030718060191e-028, 2.0113352370744385e-029,  -3.0373745563400371e-030, 1.1806906454401013e-032,  -7.7705399876661076e-032,
                                         2.0902213275965398e-033, -7.1542424054621921e-034, -7.1542424054621926e-035, 2.4754073164739869e-036,  5.4846728545790429e-037,
                                         9.2462547772103625e-038, -4.8596774326570872e-039};
static double const rz__top[13]       = {1e+023, 1e+046, 1e+069, 1e+092, 1e+115, 1e+138, 1e+161, 1e+184, 1e+207, 1e+230, 1e+253, 1e+276, 1e+299};
static double const rz__negtop[13]    = {1e-023, 1e-046, 1e-069, 1e-092, 1e-115, 1e-138, 1e-161, 1e-184, 1e-207, 1e-230, 1e-253, 1e-276, 1e-299};
static double const rz__toperr[13]    = {8388608,
                                         6.8601809640529717e+028,
                                         -7.253143638152921e+052,
                                         -4.3377296974619174e+075,
                                         -1.5559416129466825e+098,
                                         -3.2841562489204913e+121,
                                         -3.7745893248228135e+144,
                                         -1.7356668416969134e+167,
                                         -3.8893577551088374e+190,
                                         -9.9566444326005119e+213,
                                         6.3641293062232429e+236,
                                         -5.2069140800249813e+259,
                                         -5.2504760255204387e+282};
static double const rz__negtoperr[13] = {3.9565301985100693e-040,  -2.299904345391321e-063, 3.6506201437945798e-086,  1.1875228833981544e-109, -5.0644902316928607e-132,
                                         -6.7156837247865426e-155, -2.812077463003139e-178, -5.7778912386589953e-201, 7.4997100559334532e-224, -4.6439668915134491e-247,
                                         -6.3691100762962136e-270, -9.436808465446358e-293, 8.0970921678014997e-317};

#    if defined(_MSC_VER) && (_MSC_VER <= 1200)
static rz_u64 const rz__powten[20] = {
    1,           10,           100,           1000,           10000,           100000,           1000000,           10000000,           100000000,           1000000000,
    10000000000, 100000000000, 1000000000000, 10000000000000, 100000000000000, 1000000000000000, 10000000000000000, 100000000000000000, 1000000000000000000, 10000000000000000000U};
#        define rz__tento19th ((rz_u64)1000000000000000000)
#    else
static rz_u64 const rz__powten[20] = {1,
                                      10,
                                      100,
                                      1000,
                                      10000,
                                      100000,
                                      1000000,
                                      10000000,
                                      100000000,
                                      1000000000,
                                      10000000000ULL,
                                      100000000000ULL,
                                      1000000000000ULL,
                                      10000000000000ULL,
                                      100000000000000ULL,
                                      1000000000000000ULL,
                                      10000000000000000ULL,
                                      100000000000000000ULL,
                                      1000000000000000000ULL,
                                      10000000000000000000ULL};
#        define rz__tento19th (1000000000000000000ULL)
#    endif

#    define rz__ddmulthi(oh, ol, xh, yh)                                  \
        {                                                                 \
            double ahi = 0, alo, bhi = 0, blo;                            \
            rz_i64 bt;                                                    \
            oh = xh * yh;                                                 \
            RZ__COPYFP(bt, xh);                                           \
            bt &= ((~(rz_u64)0) << 27);                                   \
            RZ__COPYFP(ahi, bt);                                          \
            alo = xh - ahi;                                               \
            RZ__COPYFP(bt, yh);                                           \
            bt &= ((~(rz_u64)0) << 27);                                   \
            RZ__COPYFP(bhi, bt);                                          \
            blo = yh - bhi;                                               \
            ol  = ((ahi * bhi - oh) + ahi * blo + alo * bhi) + alo * blo; \
        }

#    define rz__ddtoS64(ob, xh, xl)                   \
        {                                             \
            double ahi = 0, alo, vh, t;               \
            ob         = (rz_i64)xh;                  \
            vh         = (double)ob;                  \
            ahi        = (xh - vh);                   \
            t          = (ahi - xh);                  \
            alo        = (xh - (ahi - t)) - (vh + t); \
            ob += (rz_i64)(ahi + alo + xl);           \
        }

#    define rz__ddrenorm(oh, ol) \
        {                        \
            double s;            \
            s  = oh + ol;        \
            ol = ol - (s - oh);  \
            oh = s;              \
        }

#    define rz__ddmultlo(oh, ol, xh, xl, yh, yl) ol = ol + (xh * yl + xl * yh);

#    define rz__ddmultlos(oh, ol, xh, yl)        ol = ol + (xh * yl);

static void rz__raise_to_power10(double *ohi, double *olo, double d, rz_i32 power) // power can be -323 to +350
{
    double ph, pl;
    if ((power >= 0) && (power <= 22)) {
        rz__ddmulthi(ph, pl, d, rz__bot[power]);
    } else {
        rz_i32 e, et, eb;
        double p2h, p2l;

        e = power;
        if (power < 0) e = -e;
        et = (e * 0x2c9) >> 14; /* %23 */
        if (et > 13) et = 13;
        eb = e - (et * 23);

        ph = d;
        pl = 0.0;
        if (power < 0) {
            if (eb) {
                --eb;
                rz__ddmulthi(ph, pl, d, rz__negbot[eb]);
                rz__ddmultlos(ph, pl, d, rz__negboterr[eb]);
            }
            if (et) {
                rz__ddrenorm(ph, pl);
                --et;
                rz__ddmulthi(p2h, p2l, ph, rz__negtop[et]);
                rz__ddmultlo(p2h, p2l, ph, pl, rz__negtop[et], rz__negtoperr[et]);
                ph = p2h;
                pl = p2l;
            }
        } else {
            if (eb) {
                e = eb;
                if (eb > 22) eb = 22;
                e -= eb;
                rz__ddmulthi(ph, pl, d, rz__bot[eb]);
                if (e) {
                    rz__ddrenorm(ph, pl);
                    rz__ddmulthi(p2h, p2l, ph, rz__bot[e]);
                    rz__ddmultlos(p2h, p2l, rz__bot[e], pl);
                    ph = p2h;
                    pl = p2l;
                }
            }
            if (et) {
                rz__ddrenorm(ph, pl);
                --et;
                rz__ddmulthi(p2h, p2l, ph, rz__top[et]);
                rz__ddmultlo(p2h, p2l, ph, pl, rz__top[et], rz__toperr[et]);
                ph = p2h;
                pl = p2l;
            }
        }
    }
    rz__ddrenorm(ph, pl);
    *ohi = ph;
    *olo = pl;
}

// given a float value, returns the significant bits in bits, and the position of the
//   decimal point in decimal_pos.  +/-INF and NAN are specified by special values
//   returned in the decimal_pos parameter.
// frac_digits is absolute normally, but if you want from first significant digits (got %g and %e), or in 0x80000000
static rz_i32 rz__real_to_str(char const **start, rz_u32 *len, char *out, rz_i32 *decimal_pos, double value, rz_u32 frac_digits) {
    double d;
    rz_i64 bits = 0;
    rz_i32 expo, e, ng, tens;

    d = value;
    RZ__COPYFP(bits, d);
    expo = (rz_i32)((bits >> 52) & 2047);
    ng   = (rz_i32)((rz_u64)bits >> 63);
    if (ng) d = -d;

    if (expo == 2047) // is nan or inf?
    {
        *start       = (bits & ((((rz_u64)1) << 52) - 1)) ? "NaN" : "Inf";
        *decimal_pos = RZ__SPECIAL;
        *len         = 3;
        return ng;
    }

    if (expo == 0)                    // is zero or denormal
    {
        if (((rz_u64)bits << 1) == 0) // do zero
        {
            *decimal_pos = 1;
            *start       = out;
            out[0]       = '0';
            *len         = 1;
            return ng;
        }
        // find the right expo for denormals
        {
            rz_i64 v = ((rz_u64)1) << 51;
            while ((bits & v) == 0) {
                --expo;
                v >>= 1;
            }
        }
    }

    // find the decimal exponent as well as the decimal bits of the value
    {
        double ph, pl;

        // log10 estimate - very specifically tweaked to hit or undershoot by no more than 1 of log10 of all expos 1..2046
        tens = expo - 1023;
        tens = (tens < 0) ? ((tens * 617) / 2048) : (((tens * 1233) / 4096) + 1);

        // move the significant bits into position and stick them into an int
        rz__raise_to_power10(&ph, &pl, d, 18 - tens);

        // get full as much precision from double-double as possible
        rz__ddtoS64(bits, ph, pl);

        // check if we undershot
        if (((rz_u64)bits) >= rz__tento19th) ++tens;
    }

    // now do the rounding in integer land
    frac_digits = (frac_digits & 0x80000000) ? ((frac_digits & 0x7ffffff) + 1) : (tens + frac_digits);
    if ((frac_digits < 24)) {
        rz_u32 dg = 1;
        if ((rz_u64)bits >= rz__powten[9]) dg = 10;
        while ((rz_u64)bits >= rz__powten[dg]) {
            ++dg;
            if (dg == 20) goto noround;
        }
        if (frac_digits < dg) {
            rz_u64 r;
            // add 0.5 at the right position and round
            e = dg - frac_digits;
            if ((rz_u32)e >= 24) goto noround;
            r    = rz__powten[e];
            bits = bits + (r / 2);
            if ((rz_u64)bits >= rz__powten[dg]) ++tens;
            bits /= r;
        }
    noround:;
    }

    // kill long trailing runs of zeros
    if (bits) {
        rz_u32 n;
        for (;;) {
            if (bits <= 0xffffffff) break;
            if (bits % 1000) goto donez;
            bits /= 1000;
        }
        n = (rz_u32)bits;
        while ((n % 1000) == 0) n /= 1000;
        bits = n;
    donez:;
    }

    // convert to string
    out += 64;
    e = 0;
    for (;;) {
        rz_u32 n;
        char  *o = out - 8;
        // do the conversion in chunks of U32s (avoid most 64-bit divides, worth it, constant denomiators be damned)
        if (bits >= 100000000) {
            n = (rz_u32)(bits % 100000000);
            bits /= 100000000;
        } else {
            n    = (rz_u32)bits;
            bits = 0;
        }
        while (n) {
            out -= 2;
            *(rz_u16 *)out = *(rz_u16 *)&rz__digitpair.pair[(n % 100) * 2];
            n /= 100;
            e += 2;
        }
        if (bits == 0) {
            if ((e) && (out[0] == '0')) {
                ++out;
                --e;
            }
            break;
        }
        while (out != o) {
            *--out = '0';
            ++e;
        }
    }

    *decimal_pos = tens;
    *start       = out;
    *len         = e;
    return ng;
}

#    undef rz__ddmulthi
#    undef rz__ddrenorm
#    undef rz__ddmultlo
#    undef rz__ddmultlos
#    undef RZ__SPECIAL
#    undef RZ__COPYFP

#endif // RZ_SPRINTF_NOFLOAT

// clean up
#undef RZ__UNALIGNED

/// #if defined(RZ_ALLOC_IMPL)
RZ_DEF rz_char *rz_avsprintf(RZ_Allocator a, const rz_char *fmt, va_list arg) {
    va_list arg_cp;

    rz__context c = {.length = 0};
    {
        va_copy(arg_cp, arg);
        rz_vsprintfcb(rz__count_clamp_callback, &c, c.tmp, fmt, arg_cp);
        va_end(arg_cp);
        c.length += 1;
    }

    char *buf = rz_alloc(a, buf, c.length);
    if (buf == NULL) return NULL;
    rz_vsnprintf(buf, c.length, fmt, arg);

    return buf;
}

RZ_DEF rz_char *rz_asprintf(RZ_Allocator a, const rz_char *fmt, ...) {
    va_list arg;
    va_start(arg, fmt);
    rz_char *result = rz_avsprintf(a, fmt, arg);
    va_end(arg);
    return result;
}
/// #endif if defined(RZ_ALLOC_IMPL)
