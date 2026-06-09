#include "rz_time.h"

static inline bool   rz_time_is_leap_year(rz_i32 year);
static inline rz_u16 rz_time_days_in_year(rz_i32 year);
static inline rz_u8  rz_time_weeks_in_year(rz_i32 year);
static inline rz_u8  rz_time_days_in_month(rz_u8 month, rz_i32 year);
static inline rz_u8  rz_time_days_in_month_leap(rz_u8 month, bool is_leap_year);

static inline bool rz__add_u64(rz_u64 lhs, rz_u64 rhs, rz_u64 *result) {
#if RZ_TARGET_COMPILER_MSVC
    return SUCCEEDED(ULongLongAdd(lhs, rhs, result));
#elif RZ_HAS_BUILTIN(__builtin_add_overflow)
    return __builtin_add_overflow(lhs, rhs, result);
#else
    if ((lhs + rhs) >= lhs) {
        *result = (lhs + rhs);
        return true;
    }
    return false;
#endif
}

static inline bool rz__sub_u64(rz_u64 lhs, rz_u64 rhs, rz_u64 *result) {
#if RZ_TARGET_COMPILER_MSVC
    return SUCCEEDED(ULongLongSub(lhs, rhs, result));
#elif RZ_HAS_BUILTIN(__builtin_sub_overflow)
    return __builtin_sub_overflow(lhs, rhs, result);
#else
    if (lhs >= rhs) {
        *result = (lhs - rhs);
        return true;
    }
    return false;
#endif
}

#define rz__assert_nanos(nanos) RZ_ASSERT(((nanos) >= RZ_TIME_NANOS_RANGE_MIN) && ((nanos) <= RZ_TIME_NANOS_RANGE_MAX))

RZ_DEF RZ_Duration rz_duration_secs_f32(rz_f32 secs) {

    return rz_duration_secs_f64((rz_f64)secs);
}

RZ_DEF RZ_Duration rz_duration_secs_f64(rz_f64 secs) {
    RZ_Duration dur = {0};

    /* use signed temp for integer seconds */
    rz_i64 sec  = (rz_i64)secs;       /* truncates toward zero */
    rz_f64 frac = secs - (rz_f64)sec; /* fractional part */

    if (frac < 0.0) {
        frac += 1.0;
        sec--;
    }

    /* compute nanoseconds with rounding */
    rz_i64 nsec = (rz_i64)(frac * ((rz_f64)RZ_TIME_NANOS_PER_SEC) + 0.5);
    if (nsec >= RZ_TIME_NANOS_PER_SEC) {
        nsec -= RZ_TIME_NANOS_PER_SEC;
        sec++;
    }

    /* assign back to unsigned storage safely */
    dur.secs  = (sec < 0u) ? 0u : (rz_u64)sec;
    dur.nanos = (rz_u32)nsec;

    rz__assert_nanos(dur.nanos);
    return dur;
}

RZ_DEF RZ_Duration rz_duration_millis(rz_u64 millis) {
    RZ_Duration dur = {0};
    dur.secs        = millis / RZ_TIME_MILLIS_PER_SEC;
    dur.nanos       = ((rz_u32)(millis % RZ_TIME_MILLIS_PER_SEC)) * RZ_TIME_NANOS_PER_MILLI;
    rz__assert_nanos(dur.nanos);
    return dur;
}

RZ_DEF RZ_Duration rz_duration_micros(rz_u64 micros) {
    RZ_Duration dur = {0};
    dur.secs        = micros / RZ_TIME_MICROS_PER_SEC;
    dur.nanos       = ((rz_u32)(micros % RZ_TIME_MICROS_PER_SEC)) * RZ_TIME_NANOS_PER_MICRO;
    rz__assert_nanos(dur.nanos);
    return dur;
}

RZ_DEF RZ_Duration rz_duration_nanos(rz_u64 nanos) {
    RZ_Duration dur = {0};
    dur.secs        = nanos / RZ_TIME_NANOS_PER_SEC;
    dur.nanos       = (rz_u32)(nanos % RZ_TIME_NANOS_PER_SEC);
    rz__assert_nanos(dur.nanos);
    return dur;
}

RZ_DEF RZ_Duration rz_duration_weeks(rz_u64 weeks) {
    RZ_ASSERT(weeks <= (RZ_U64_MAX / (RZ_TIME_SECS_PER_MINUTE * RZ_TIME_MINS_PER_HOUR * RZ_TIME_HOURS_PER_DAY * RZ_TIME_DAYS_PER_WEEK)), "overflow in `rz_duration_weeks()`");
    return rz_duration_secs(weeks * RZ_TIME_MINS_PER_HOUR * RZ_TIME_SECS_PER_MINUTE * RZ_TIME_HOURS_PER_DAY * RZ_TIME_DAYS_PER_WEEK);
}

RZ_DEF RZ_Duration rz_duration_days(rz_u64 days) {
    RZ_ASSERT(days <= (RZ_U64_MAX / (RZ_TIME_SECS_PER_MINUTE * RZ_TIME_MINS_PER_HOUR * RZ_TIME_HOURS_PER_DAY)), "overflow in `rz_duration_days()`");
    return rz_duration_secs(days * RZ_TIME_MINS_PER_HOUR * RZ_TIME_SECS_PER_MINUTE * RZ_TIME_HOURS_PER_DAY);
}

RZ_DEF RZ_Duration rz_duration_hours(rz_u64 hours) {
    RZ_ASSERT(hours <= (RZ_U64_MAX / (RZ_TIME_SECS_PER_MINUTE * RZ_TIME_MINS_PER_HOUR)), "overflow in `rz_duration_hours()`");
    return rz_duration_secs(hours * RZ_TIME_MINS_PER_HOUR * RZ_TIME_SECS_PER_MINUTE);
}

RZ_DEF RZ_Duration rz_duration_mins(rz_u64 mins) {
    RZ_ASSERT(mins <= (RZ_U64_MAX / RZ_TIME_SECS_PER_MINUTE), "overflow in `rz_duration_mins()`");
    return rz_duration_secs(mins * RZ_TIME_SECS_PER_MINUTE);
}

RZ_DEF RZ_Duration rz_duration_add(RZ_Duration lhs, RZ_Duration rhs) {
    RZ_Duration result = {0};
    if (rz_duration_checked_add(lhs, rhs, &result)) return result;
    RZ_PANIC("rz_duration_add overflow");
}

RZ_DEF bool rz_duration_checked_add(RZ_Duration lhs, RZ_Duration rhs, RZ_Duration *result) {
    RZ_ASSERT_NOT_NULL(result);
    if (!rz__add_u64(lhs.secs, rhs.secs, &result->secs)) return false;

    result->nanos = lhs.nanos + rhs.nanos;
    if (result->nanos >= RZ_TIME_NANOS_PER_SEC) {
        result->nanos -= RZ_TIME_NANOS_PER_SEC;
        if (!rz__add_u64(result->secs, 1, &result->secs)) return false;
    }
    rz__assert_nanos(result->nanos);
    return true;
}

RZ_DEF RZ_Duration rz_duration_saturating_add(RZ_Duration lhs, RZ_Duration rhs) {
    RZ_Duration result = {0};
    if (!rz_duration_checked_add(lhs, rhs, &result)) {
        result.secs  = RZ_U64_MAX;
        result.nanos = RZ_TIME_NANOS_RANGE_MAX;
    }
    return result;
}

RZ_DEF RZ_Duration rz_duration_sub(RZ_Duration lhs, RZ_Duration rhs) {
    RZ_Duration result = {0};
    if (rz_duration_checked_sub(lhs, rhs, &result)) return result;
    RZ_PANIC("rz_duration_sub overflow");
}

RZ_DEF bool rz_duration_checked_sub(RZ_Duration lhs, RZ_Duration rhs, RZ_Duration *result) {
    if (!rz__sub_u64(lhs.secs, rhs.secs, &result->secs)) return false;
    if (lhs.nanos >= rhs.nanos) {
        result->nanos = lhs.nanos - rhs.nanos;
    } else if (rz__sub_u64(result->secs, 1, &result->secs)) {
        result->nanos = lhs.nanos + RZ_TIME_NANOS_PER_SEC - rhs.nanos;
    } else {
        return false;
    };
    rz__assert_nanos(result->nanos);
    return true;
}

RZ_DEF RZ_Duration rz_duration_saturating_sub(RZ_Duration lhs, RZ_Duration rhs) {
    RZ_Duration result = {0};
    if (rz_duration_checked_sub(lhs, rhs, &result)) return result;
    return (RZ_Duration){0};
}

RZ_DEF RZ_Duration rz_duration_abs_diff(RZ_Duration lhs, RZ_Duration rhs) {
    RZ_Duration result = {0};
    if (!rz_duration_checked_sub(lhs, rhs, &result)) {
        bool is_overflow = rz_duration_checked_sub(rhs, lhs, &result);
        RZ_ASSERT(is_overflow);
    }
    return result;
}

RZ_DEF rz_ptrdiff rz_duration_cmp(RZ_Duration lhs, RZ_Duration rhs) {
    // if (lhs.secs < rhs.secs) return -1;
    // else if (lhs.secs > rhs.secs) return 1;
    // else if (lhs.nanos < rhs.nanos) return -1;
    // else if (lhs.nanos > rhs.nanos) return 1;
    // else return 0;

    if (lhs.secs > rhs.secs) return 1;
    else if (lhs.secs < rhs.secs) return -1;
    else return (lhs.nanos - rhs.nanos);
}

#if RZ_TARGET_OS_WINDOWS
static rz_i64 rz__perfcounter_now(void) {
    rz_i64 qpc_value = {0};
    if (!QueryPerformanceCounter((LARGE_INTEGER *)&qpc_value)) RZ_PANIC("QueryPerformanceCounter() failed.");
    return qpc_value;
}

static rz_i64 rz__perfcounter_frequency(void) {
    // Either the cached result of `QueryPerformanceFrequency` or `0` for
    // uninitialized. Storing this as a single `AtomicU64` allows us to use
    // `Relaxed` operations, as we are only interested in the effects on a
    // single memory location.
    static rz_u64 FREQUENCY = 0;
    rz_u64        cached    = FREQUENCY;
    // If a previous thread has filled in this global state, use that.
    if (cached != 0) return (rz_i64)cached;
    // ... otherwise learn for ourselves ...
    rz_i64 frequency = 0;
    if (!QueryPerformanceFrequency((LARGE_INTEGER *)&frequency)) RZ_PANIC("QueryPerformanceFrequency() failed.");
    FREQUENCY = frequency;
    return frequency;
}

// Per microsoft docs, the margin of error for cross-thread time comparisons
// using QueryPerformanceCounter is 1 "tick" -- defined as 1/frequency().
// Reference: https://docs.microsoft.com/en-us/windows/desktop/SysInfo
//                   /acquiring-high-resolution-time-stamps
static RZ_Duration rz__perfcounter_epsilon(void) {
    rz_u64 epsilon = RZ_TIME_NANOS_PER_SEC / ((rz_u64)rz__perfcounter_frequency());
    return rz_duration_nanos(epsilon);
}
#elif RZ_TARGET_FAMILY_UNIX
#    if RZ_TARGET_FAMILY_APPLE
#        define RZ__TIME_CLOCK_ID CLOCK_UPTIME_RAW
#    else
#        define RZ__TIME_CLOCK_ID CLOCK_MONOTONIC
#    endif
#    define rz__timespec2duration(ts)  ((RZ_Duration){.secs = (ts).tv_sec, .nanos = (ts).tv_nsec})
#    define rz__duration2timespec(dur) ((struct timespec){.tv_sec = (dur).secs, .tv_nsec = (dur).nanos})
static bool rz__timespec(rz_i64 tv_sec, rz_i64 tv_nsec, RZ_OsSystemTime *result) {
#    if RZ_TARGET_FAMILY_APPLE
    if (((tv_sec <= 0) && (tv_sec > RZ_I64_MIN)) && ((tv_nsec < 0) && (tv_nsec > -RZ_TIME_NANOS_PER_SEC))) {
        tv_sec--;
        tv_nsec += RZ_TIME_NANOS_PER_SEC;
    }
#    endif
    if (tv_nsec >= 0 && tv_nsec < (rz_i64)RZ_TIME_NANOS_PER_SEC) {
        result->tv_sec  = tv_sec;
        result->tv_nsec = tv_nsec;
        return true;
    } else {
        return false;
    }
}
#endif
#define rz__mul_div(value, numerator, denom) ((value / denom) * numerator + (value % denom) * numerator / denom)

RZ_DEF RZ_InstantTime rz_instant_now(void) {
    RZ_InstantTime result;
#if RZ_TARGET_OS_WINDOWS
    rz_u64 freq         = (rz_u64)rz__perfcounter_frequency();
    rz_i64 now          = rz__perfcounter_now();
    rz_u64 instant_nsec = rz__mul_div((rz_u64)now, RZ_TIME_NANOS_PER_SEC, freq);
    result.t            = rz_duration_nanos(instant_nsec);
#elif RZ_TARGET_FAMILY_UNIX
    RZ_OsInstantTime t = {0};
    if (clock_gettime(RZ__TIME_CLOCK_ID, &t) != 0) RZ_PANIC("system function `clock_gettime()` failed");
    struct timespec spec = {0};
    if (!rz__timespec(t.tv_sec, t.tv_nsec, &spec)) RZ_PANIC("invalid timestamp");
    result.t = rz_duration(spec.tv_sec, spec.tv_nsec);
#else
#    error "unimplemented for this os"
#endif
    return result;
}

RZ_DEF RZ_Duration rz_instant_duration_since(RZ_InstantTime self, RZ_InstantTime earlier) {
    RZ_Duration result = {0};
    if (rz_instant_checked_duration_since(self, earlier, &result)) return result;
    result.secs  = 0;
    result.nanos = 0;
    return result;
}

RZ_DEF bool rz_instant_checked_duration_since(RZ_InstantTime self, RZ_InstantTime earlier, RZ_Duration *result) {
#if RZ_TARGET_OS_WINDOWS
    RZ_Duration epsilon = rz__perfcounter_epsilon();
    if ((rz_duration_cmp(earlier.t, self.t) > 0) && (rz_duration_cmp(rz_duration_sub(earlier.t, self.t), epsilon) <= 0)) {
        *result = (RZ_Duration){0};
        return true;
    } else {
        return rz_duration_checked_sub(self.t, earlier.t, result);
    }
#elif RZ_TARGET_FAMILY_UNIX
    return rz_duration_checked_sub(self.t, earlier.t, result);
#else
#    error "unimplemented for this os"
#endif
}

RZ_DEF RZ_Duration rz_instant_elapsed(RZ_InstantTime self) {
    return rz_instant_duration_since(rz_instant_now(), self);
}

RZ_DEF bool rz_instant_checked_add(RZ_InstantTime self, RZ_Duration duration, RZ_InstantTime *result) {
#if RZ_TARGET_OS_WINDOWS
    return rz_duration_checked_add(self.t, duration, &result->t);
#elif RZ_TARGET_FAMILY_UNIX
    RZ_Duration res = {0};
    if (!rz_duration_checked_add(self.t, duration, &res)) return false;
    result->t = res;
    return true;
#else
#    error "unimplemented for this os"
#endif
}

RZ_DEF bool rz_instant_checked_sub(RZ_InstantTime self, RZ_Duration duration, RZ_InstantTime *result) {
#if RZ_TARGET_OS_WINDOWS
    return rz_duration_checked_sub(self.t, duration, &result->t);
#elif RZ_TARGET_FAMILY_UNIX
    RZ_Duration res = {0};
    if (!rz_duration_checked_sub(self.t, duration, &res)) return false;
    result->t = res;
    return true;
#else
#    error "unimplemented for this os"
#endif
}

RZ_DEF RZ_InstantTime rz_instant_add(RZ_InstantTime self, RZ_Duration duration) {
    RZ_InstantTime result;
    if (rz_instant_checked_add(self, duration, &result)) return result;
    RZ_PANIC("overflow when adding RZ_Duratioin on RZ_InstantTime");
}

RZ_DEF RZ_InstantTime rz_instant_sub(RZ_InstantTime self, RZ_Duration duration) {
    RZ_InstantTime result;
    if (rz_instant_checked_sub(self, duration, &result)) return result;
    RZ_PANIC("overflow when subtracting RZ_Duratioin on RZ_InstantTime");
}

#if RZ_TARGET_OS_WINDOWS
#    define RZ_TIME_INTERVALS_PER_SEC (RZ_TIME_NANOS_PER_SEC / 100)
#    define rz__systemtime_from_interval(intervals)       \
        rz_systemtime_from_os((RZ_OsSystemTime){          \
            .dwLowDateTime  = (DWORD)intervals,           \
            .dwHighDateTime = (DWORD)((intervals) >> 32), \
        })
#    define rz__systemtime_intervals(st) ((rz_i64)(st).t.dwLowDateTime) | (((rz_i64)(st).t.dwHighDateTime) << 32)
const RZ_SystemTime RZ_TIME_UNIX_EPOCH = rz__systemtime_from_interval(11644473600 * RZ_TIME_INTERVALS_PER_SEC);
#else
const RZ_SystemTime RZ_TIME_UNIX_EPOCH = {0};
#endif

RZ_DEF RZ_SystemTime rz_systemtime_from_os(RZ_OsSystemTime ostime) {
    RZ_UNUSED(ostime);
    RZ_TODO("rz_systemtime_from_os");
}

RZ_DEF RZ_SystemTime rz_systemtime_now(void) {
    RZ_OsSystemTime f = {0};
#if RZ_TARGET_OS_WINDOWS
    GetSystemTimePreciseAsFileTime(&f);
#elif RZ_TARGET_FAMILY_UNIX
    if (clock_gettime(CLOCK_REALTIME, &f) != 0) RZ_PANIC("system function `clock_gettime()` failed");
#endif
    return rz_systemtime_from_os(f);
}

RZ_DEF RZ_Duration rz_systemtime_duration_since(RZ_SystemTime self, RZ_SystemTime earlier) {
    RZ_Duration result = {0};
    if (rz_systemtime_checked_duration_since(self, earlier, &result)) return result;
    result.secs  = 0;
    result.nanos = 0;
    return result;
}
RZ_DEF bool rz_systemtime_checked_duration_since(RZ_SystemTime self, RZ_SystemTime earlier, RZ_Duration *result) {
    RZ_UNUSED(self), RZ_UNUSED(earlier), RZ_UNUSED(result);
    RZ_TODO("rz_systemtime_checked_duration_since");
}

RZ_DEF RZ_Duration rz_systemtime_elapsed(RZ_SystemTime self) {
    return rz_systemtime_duration_since(rz_systemtime_now(), self);
}

RZ_DEF bool rz_systemtime_checked_add(RZ_SystemTime self, RZ_Duration duration, RZ_SystemTime *result) {
    RZ_UNUSED(self);
    RZ_UNUSED(duration);
    RZ_UNUSED(result);
    RZ_TODO("rz_systemtime_checked_add");
}

RZ_DEF bool rz_systemtime_checked_sub(RZ_SystemTime self, RZ_Duration duration, RZ_SystemTime *result) {

    RZ_UNUSED(self);
    RZ_UNUSED(duration);
    RZ_UNUSED(result);
    RZ_TODO("rz_systemtime_checked_sub");
}

RZ_DEF RZ_SystemTime rz_systemtime_add(RZ_SystemTime self, RZ_Duration duration) {
    RZ_SystemTime result;
    if (rz_systemtime_checked_add(self, duration, &result)) return result;
    RZ_PANIC("overflow when adding RZ_Duratioin on RZ_SystemTime");
}

RZ_DEF RZ_SystemTime rz_systemtime_sub(RZ_SystemTime self, RZ_Duration duration) {
    RZ_SystemTime result;
    if (rz_systemtime_checked_sub(self, duration, &result)) return result;
    RZ_PANIC("overflow when subtracting RZ_Duratioin on RZ_SystemTime");
}

bool rz_time_is_leap_year(rz_i32 year) {
    RZ_ASSERT(year >= -9999), RZ_ASSERT(year <= 9999);
    rz_i64 y = (rz_i64)year;
    return ((((rz_u64)((y < 0) ? -y : y)) * 0x4000000028F5C28F) & 0xC000000F8000000F) <= 0xF80000000;
}

rz_u16 rz_time_days_in_year(rz_i32 year) {
    return (rz_time_is_leap_year(year)) ? 366 : 365;
}

rz_u8 rz_time_weeks_in_year(rz_i32 year) {
    // clang-format off
    const static rz_i32 __MATCHES[]= {
        -396 , -391 , -385 , -380 , -374 , -368 , -363 , -357 , -352 , -346 , -340 , -335
        , -329 , -324 , -318 , -312 , -307 , -301 , -295 , -289 , -284 , -278 , -272 , -267
        , -261 , -256 , -250 , -244 , -239 , -233 , -228 , -222 , -216 , -211 , -205 , -199
        , -193 , -188 , -182 , -176 , -171 , -165 , -160 , -154 , -148 , -143 , -137 , -132
        , -126 , -120 , -115 , -109 , -104 , -97 , -92 , -86 , -80 , -75 , -69 , -64 , -58
        , -52 , -47 , -41 , -36 , -30 , -24 , -19 , -13 , -8 , -2 , 4 , 9 , 15 , 20 , 26 , 32
        , 37 , 43 , 48 , 54 , 60 , 65 , 71 , 76 , 82 , 88 , 93 , 99 , 105 , 111 , 116 , 122
        , 128 , 133 , 139 , 144 , 150 , 156 , 161 , 167 , 172 , 178 , 184 , 189 , 195 , 201
        , 207 , 212 , 218 , 224 , 229 , 235 , 240 , 246 , 252 , 257 , 263 , 268 , 274 , 280
        , 285 , 291 , 296 , 303 , 308 , 314 , 320 , 325 , 331 , 336 , 342 , 348 , 353 , 359
        , 364 , 370 , 376 , 381 , 387 , 392 , 398, 0
    };
    // clang-format on

    rz_i32 m = year % 400;
    for (unsigned i = 0; i < RZ_ARRAY_LEN(__MATCHES); ++i)
        if (__MATCHES[i] == m) return 53;
    return 52;
}

rz_u8 rz_time_days_in_month(rz_u8 month, rz_i32 year) {
    return rz_time_days_in_month_leap(month, rz_time_is_leap_year(year));
}

rz_u8 rz_time_days_in_month_leap(rz_u8 month, bool is_leap_year) {
    RZ_ASSERT(month >= 1), RZ_ASSERT(month <= 12);
    return (month == 2) ? ((is_leap_year) ? 29 : 28) : ((30 | month) ^ (month >> 3));
}
