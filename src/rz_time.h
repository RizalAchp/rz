#pragma once
#ifndef RZ_TIME_H
#    define RZ_TIME_H
#    include "rz_common.h"

#    define RZ_TIME_NANOS_RANGE_MIN 0
#    define RZ_TIME_NANOS_RANGE_MAX 999999999 // 999.999.999

#    define RZ_TIME_NANOS_PER_SEC   1000000000
#    define RZ_TIME_NANOS_PER_MILLI 1000000
#    define RZ_TIME_NANOS_PER_MICRO 1000

#    define RZ_TIME_MILLIS_PER_SEC  1000
#    define RZ_TIME_MICROS_PER_SEC  1000000

#    define RZ_TIME_SECS_PER_MINUTE 60
#    define RZ_TIME_MINS_PER_HOUR   60
#    define RZ_TIME_HOURS_PER_DAY   24
#    define RZ_TIME_DAYS_PER_WEEK   7

#    if RZ_TARGET_FAMILY_UNIX
#        define RZ_OsInstantTime struct timespec
#        define RZ_OsSystemTime  struct timespec
#    elif RZ_TARGET_OS_WINDOWS
/// This duration is relative to an arbitrary microsecond
/// epoch from the winapi QueryPerformanceCounter function.
#        define RZ_OsInstantTime RZ_Duration
#        define RZ_OsSystemTime  FILETIME
#    endif

#    ifdef __cplusplus
extern "C" {
#    endif

/// A `RZ_Duration` type to represent a span of time, typically used for system timeouts.
/// Each `RZ_Duration` is composed of a whole number of seconds and a fractional part
/// represented in nanoseconds. If the underlying system does not support
/// nanosecond-level precision, APIs binding a system timeout will typically round up
/// the number of nanoseconds.
typedef struct RZ_Duration {
    rz_u64 secs;
    rz_u32 nanos;
} RZ_Duration;

#    define rz_duration(_secs, _nanos) ((RZ_Duration){.secs = (_secs), .nanos = (_nanos)})
#    define rz_duration_secs(_secs)    ((RZ_Duration){.secs = (_secs), .nanos = 0})

RZ_DEC RZ_Duration rz_duration_secs_f32(rz_f32 secs);
RZ_DEC RZ_Duration rz_duration_secs_f64(rz_f64 secs);
RZ_DEC RZ_Duration rz_duration_millis(rz_u64 millis);
RZ_DEC RZ_Duration rz_duration_micros(rz_u64 micros);
RZ_DEC RZ_Duration rz_duration_nanos(rz_u64 nanos);
RZ_DEC RZ_Duration rz_duration_weeks(rz_u64 weeks);
RZ_DEC RZ_Duration rz_duration_days(rz_u64 days);
RZ_DEC RZ_Duration rz_duration_hours(rz_u64 hours);
RZ_DEC RZ_Duration rz_duration_mins(rz_u64 hours);
#    define rz_duration_subsec_millis(dur)      ((dur).nanos / RZ_TIME_NANOS_PER_MILLI)
#    define rz_duration_subsec_micros(dur)      ((dur).nanos / RZ_TIME_NANOS_PER_MICRO)
#    define rz_duration_subsec_nanos(dur)       ((dur).nanos)
#    define rz_duration_as_secs(FLOAT_T, dur)   ((FLOAT_T)(dur).secs) + ((FLOAT_T)(dur).nanos) / ((FLOAT_T)RZ_TIME_NANOS_PER_SEC)
#    define rz_duration_as_millis(FLOAT_T, dur) (((FLOAT_T)(dur).secs) * ((FLOAT_T)RZ_TIME_MILLIS_PER_SEC) + ((FLOAT_T)(dur).nanos) / ((FLOAT_T)RZ_TIME_NANOS_PER_MILLI))

RZ_DEC RZ_Duration rz_duration_add(RZ_Duration lhs, RZ_Duration rhs);
RZ_DEC bool        rz_duration_checked_add(RZ_Duration lhs, RZ_Duration rhs, RZ_Duration *result);
RZ_DEC RZ_Duration rz_duration_saturating_add(RZ_Duration lhs, RZ_Duration rhs);
RZ_DEC RZ_Duration rz_duration_sub(RZ_Duration lhs, RZ_Duration rhs);
RZ_DEC bool        rz_duration_checked_sub(RZ_Duration lhs, RZ_Duration rhs, RZ_Duration *result);
RZ_DEC RZ_Duration rz_duration_saturating_sub(RZ_Duration lhs, RZ_Duration rhs);
RZ_DEC RZ_Duration rz_duration_abs_diff(RZ_Duration lhs, RZ_Duration rhs);

/// return < 0 if lhs < rhs.
/// return > 0 if lhs == rhs.
/// return == 0 if lhs > rhs.
RZ_DEC rz_ptrdiff rz_duration_cmp(RZ_Duration lhs, RZ_Duration rhs);

typedef struct {
    RZ_Duration t;
} RZ_InstantTime;

RZ_DEC RZ_InstantTime rz_instant_now(void);
RZ_DEC RZ_Duration    rz_instant_duration_since(RZ_InstantTime self, RZ_InstantTime earlier);
RZ_DEC bool           rz_instant_checked_duration_since(RZ_InstantTime self, RZ_InstantTime earlier, RZ_Duration *result);
RZ_DEC RZ_Duration    rz_instant_elapsed(RZ_InstantTime self);
RZ_DEC bool           rz_instant_checked_add(RZ_InstantTime self, RZ_Duration duration, RZ_InstantTime *result);
RZ_DEC bool           rz_instant_checked_sub(RZ_InstantTime self, RZ_Duration duration, RZ_InstantTime *result);
RZ_DEC RZ_InstantTime rz_instant_add(RZ_InstantTime self, RZ_Duration duration);
RZ_DEC RZ_InstantTime rz_instant_sub(RZ_InstantTime self, RZ_Duration duration);

typedef struct {
    RZ_Duration t;
} RZ_SystemTime;

#    define RZ_SYSTIME_FAILED             ((RZ_SystemTime){-1, -1})
#    define rz_systime_is_failed(systime) ((systime).t.secs == (RZ_TYPEOF((systime).t.secs))(-1))

extern const RZ_SystemTime RZ_TIME_UNIX_EPOCH;
RZ_DEC RZ_SystemTime       rz_systemtime_from_os(RZ_OsSystemTime os);
#    if RZ_TARGET_FAMILY_UNIX
RZ_DEC RZ_SystemTime rz_systemtime_from(rz_i64 tv_sec, rz_i64 tv_nsec);
#    endif

RZ_DEC RZ_SystemTime rz_systemtime_now(void);
RZ_DEC RZ_Duration   rz_systemtime_duration_since(RZ_SystemTime self, RZ_SystemTime earlier);
RZ_DEC bool          rz_systemtime_checked_duration_since(RZ_SystemTime self, RZ_SystemTime earlier, RZ_Duration *result);
RZ_DEC RZ_Duration   rz_systemtime_elapsed(RZ_SystemTime self);
RZ_DEC bool          rz_systemtime_checked_add(RZ_SystemTime self, RZ_Duration duration, RZ_SystemTime *result);
RZ_DEC bool          rz_systemtime_checked_sub(RZ_SystemTime self, RZ_Duration duration, RZ_SystemTime *result);
RZ_DEC RZ_SystemTime rz_systemtime_add(RZ_SystemTime self, RZ_Duration duration);
RZ_DEC RZ_SystemTime rz_systemtime_sub(RZ_SystemTime self, RZ_Duration duration);

#    if (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L))
#        define rz_time_duration_since(self, earlier) _Generic((self), RZ_InstantTime: rz_instant_duration_since, RZ_SystemTime: rz_systemtime_duration_since)(self, earlier)
#        define rz_time_checked_duration_since(self, earlier, result)                                                                                       \
            _Generic((self), RZ_InstantTime: rz_instant_checked_duration_since, RZ_SystemTime: rz_systemtime_checked_duration_since)(self, earlier, result)
#        define rz_time_elapsed(self) _Generic((self), RZ_InstantTime: rz_instant_elapsed, RZ_SystemTime: rz_systemtime_elapsed)(self)
#        define rz_time_checked_add(self, duration, result)                                                                           \
            _Generic((self), RZ_InstantTime: rz_instant_checked_add, RZ_SystemTime: rz_systemtime_checked_add)(self, earlier, result)
#        define rz_time_checked_sub(self, duration, result)                                                                           \
            _Generic((self), RZ_InstantTime: rz_instant_checked_sub, RZ_SystemTime: rz_systemtime_checked_sub)(self, earlier, result)
#        define rz_time_add(self, duration) _Generic((self), RZ_InstantTime: rz_instant_add, RZ_SystemTime: rz_systemtime_add)(self, earlier)
#        define rz_time_sub(self, duration) _Generic((self), RZ_InstantTime: rz_instant_sub, RZ_SystemTime: rz_systemtime_sub)(self, earlier)
#    endif

#    ifdef __cplusplus
} /* extern "C" */
#    endif

#endif /* end of include guard: RZ_TIME_H */
