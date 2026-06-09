#pragma once

#ifndef RZ_STRINGS_H
#    define RZ_STRINGS_H
#    include "rz_collections.h"
#    include "rz_common.h"

#    if defined(__cplusplus)
extern "C" {
#    endif /* ifndef  defined(__cplusplus) */

/// TODO: implement utf8

RZ_DEC bool    rz_is_ascii_whitespace(rz_char ch);
RZ_DEC bool    rz_is_ascii_alphabetic(rz_char ch);
RZ_DEC bool    rz_is_ascii_digit(rz_char ch);
RZ_DEC bool    rz_is_ascii_hexdigit(rz_char ch);
RZ_DEC bool    rz_is_ascii_alphanumeric(rz_char ch);
RZ_DEC bool    rz_is_ascii_lower(rz_char ch);
RZ_DEC bool    rz_is_ascii_upper(rz_char ch);
RZ_DEC rz_char rz_ascii_lower(rz_char ch);
RZ_DEC rz_char rz_ascii_upper(rz_char ch);

/// Dynamic String or generally called String Builder
typedef RZ_Array(rz_char) RZ_Str, RZ_StrBuilder;

RZ_DEC RZ_Str rz_str_sized_alloc(const rz_char *cstr, rz_usize size, RZ_Allocator a);
#    define rz_str_sized(cstr, size)        rz_str_sized_alloc(cstr, size, NULL)
#    define rz_str_sized_temp(cstr, size)   rz_str_sized_alloc(cstr, size, rz_temp_allocator())

#    define rz_str_alloc(cstr, alloc)       rz_str_sized_alloc(cstr, strlen(cstr), alloc)
#    define rz_str(cstr)                    rz_str_sized(cstr, strlen(cstr))
#    define rz_str_temp(cstr)               rz_str_sized_temp(cstr, strlen(cstr))

#    define rz_str_from_sv_alloc(sv, alloc) rz_str_sized_alloc((sv).data, (sv).len, alloc)
#    define rz_str_from_sv(sv)              rz_str_sized((sv).data, (sv).len)
#    define rz_str_from_sv_temp(sv)         rz_str_sized_temp((sv).data, (sv).len)

RZ_DEC RZ_Str rz_vstrf(RZ_Allocator a, const rz_char *fmt, va_list arg);
RZ_DEC RZ_Str rz_strf(RZ_Allocator a, const rz_char *fmt, ...) RZ_PRINTF_FORMAT(2, 3);
#    define rz_tvstrf(fmt, arg) rz_vstrf(rz_temp_allocator(), fmt, arg)
#    define rz_tstrf(fmt, ...)  rz_strf(rz_temp_allocator(), fmt, __VA_ARGS__)

#    define rz_str_from(a, val) rz_strf(a, rz_fmt(value), (_Generic(val, bool: ((val) ? "true" : "false"), default: val)))

#    define rz_str_append       rz_arr_append
#    define rz_str_free         rz_arr_free

#    define rz_str_append_null(s)   \
        do {                        \
            rz_arr_append(s, '\0'); \
            (s)->len--;             \
        } while (0)
#    define rz_str_append_sized_str(s, str, size) rz_arr_append_many(s, str, size)
#    define rz_str_append_cstr(s, cstr)           rz_str_append_sized_str(s, cstr, strlen(cstr))
#    define rz_str_append_sv(s, sv)               rz_str_append_sized_str(s, (sv).data, (sv).len)

#    define rz_str2sv(str)                        ((RZ_StrView){.data = (str)->data, .len = (str)->len})

RZ_DEC void rz_str_appendvf(RZ_Str *s, const rz_char *fmt, va_list args);
RZ_DEC void rz_str_appendf(RZ_Str *s, RZ_PRINTF_FMT(const rz_char *fmt), ...) RZ_PRINTF_FORMAT(2, 3);

RZ_DEC const char *rz_str_to_cstr(RZ_Str *s);

/// String View (slice of string or sized string)
typedef RZ_ArrayView(rz_char) RZ_StrView;

#    define rz_sv(cstr)             ((RZ_StrView){.data = (char *)cstr, .len = (cstr) ? strlen(cstr) : 0})
#    define rz_sv_sized(cstr, size) ((RZ_StrView){.data = (char *)cstr, .len = size})
#    define rz_sv_from_str(str)     ((RZ_StrView){.data = (char *)(str)->data, .len = (str)->len})
#    define rz_sv_empty             rz_sv_sized(NULL, 0)
#    define rz_sv_static(s_cstr)    ((RZ_StrView){.data = (char *)s_cstr, .len = sizeof(s_cstr) - 1})

#    define rz_sv_is_empty(sv)      (((sv).len == 0) || ((sv).data == NULL))

RZ_DEC rz_ptrdiff rz_sv_case_cmp(RZ_StrView lhs, RZ_StrView rhs);
#    define rz_sv_case_cmp_cstr(lhs, rhs_cstr) rz_sv_case_cmp(lhs, rz_sv(rhs_cstr))
#    define rz_sv_case_eq(lhs, rhs)            (rz_sv_case_cmp(lhs, rhs) == 0)
#    define rz_sv_case_eq_cstr(lhs, rhs_cstr)  rz_sv_case_eq(lhs, rz_sv(rhs_cstr))

RZ_DEC rz_ptrdiff rz_sv_cmp(RZ_StrView lhs, RZ_StrView rhs);
#    define rz_sv_cmp_cstr(lhs, rhs_cstr) rz_sv_cmp(lhs, rz_sv(rhs_cstr))
#    define rz_sv_eq(lhs, rhs)            (rz_sv_cmp(lhs, rhs) == 0)
#    define rz_sv_eq_cstr(lhs, rhs_cstr)  rz_sv_eq(lhs, rz_sv(rhs_cstr))

typedef bool (*RZ_StrViewCharPredicate)(rz_char ch);

RZ_DEC rz_usize rz_sv_find(RZ_StrView sv, RZ_StrView needle);
RZ_DEC rz_usize rz_sv_find_char(RZ_StrView sv, rz_char needle);
RZ_DEC rz_usize rz_sv_find_by(RZ_StrView sv, RZ_StrViewCharPredicate fn);
#    define rz_sv_find_cstr(sv, cstr) rz_sv_find(sv, rz_sv(cstr))

RZ_DEC rz_usize rz_sv_rfind(RZ_StrView sv, RZ_StrView needle);
RZ_DEC rz_usize rz_sv_rfind_char(RZ_StrView sv, rz_char needle);
RZ_DEC rz_usize rz_sv_rfind_by(RZ_StrView sv, RZ_StrViewCharPredicate fn);
#    define rz_sv_rfind_cstr(sv, cstr)    rz_sv_rfind(sv, rz_sv(cstr))

#    define rz_sv_contains(sv, pat)       (rz_sv_find(sv, pat) != RZ_NOT_FOUND)
#    define rz_sv_contains_cstr(sv, cstr) rz_sv_contains(sv, rz_sv(cstr))
#    define rz_sv_contains_char(sv, ch)   (rz_sv_find_char(sv, ch) != RZ_NOT_FOUND)

RZ_DEC bool rz_sv_starts_with(RZ_StrView sv, RZ_StrView starts_sv);
#    define rz_sv_starts_with_cstr(sv, starts) rz_sv_starts_with(sv, rz_sv(starts))
#    define rz_sv_starts_with_char(sv, ch)     (((sv).len == 0 || (sv).data == NULL) ? false : ((sv).data[0] == ch))

RZ_DEC bool rz_sv_ends_with(RZ_StrView sv, RZ_StrView ends_sv);
#    define rz_sv_ends_with_cstr(sv, ends) rz_sv_ends_with(sv, rz_sv(ends))
#    define rz_sv_ends_with_char(sv, ch)   ((sv.len == 0 || sv.data == NULL) ? false : (sv.data[sv.len - 1] == ch))

RZ_DEC RZ_StrView rz_sv_trim_prefix_char(RZ_StrView sv, rz_char ch);
RZ_DEC RZ_StrView rz_sv_trim_suffix_char(RZ_StrView sv, rz_char ch);
RZ_DEC RZ_StrView rz_sv_trim_char(RZ_StrView sv, rz_char ch);

RZ_DEC RZ_StrView rz_sv_trim_prefix_by(RZ_StrView sv, RZ_StrViewCharPredicate fn);
RZ_DEC RZ_StrView rz_sv_trim_suffix_by(RZ_StrView sv, RZ_StrViewCharPredicate fn);
RZ_DEC RZ_StrView rz_sv_trim_by(RZ_StrView sv, RZ_StrViewCharPredicate fn);

#    define rz_sv_trim_prefix(sv) rz_sv_trim_prefix_by(sv, rz_is_ascii_whitespace)
#    define rz_sv_trim_suffix(sv) rz_sv_trim_suffix_by(sv, rz_is_ascii_whitespace)
#    define rz_sv_trim(sv)        rz_sv_trim_by(sv, rz_is_ascii_whitespace)

RZ_DEC RZ_StrView rz_sv_strip_nprefix(RZ_StrView *sv, rz_usize n);
RZ_DEC RZ_StrView rz_sv_strip_nsuffix(RZ_StrView *sv, rz_usize n);
#    define rz_sv_split_at  rz_sv_strip_nprefix
#    define rz_sv_rsplit_at rz_sv_strip_nsuffix

RZ_DEC RZ_StrView rz_sv_strip_prefix_while(RZ_StrView *sv, RZ_StrViewCharPredicate pred);
RZ_DEC RZ_StrView rz_sv_strip_suffix_while(RZ_StrView *sv, RZ_StrViewCharPredicate pred);

RZ_DEC bool rz_sv_strip_prefix(RZ_StrView *sv, RZ_StrView prefix);
RZ_DEC bool rz_sv_strip_suffix(RZ_StrView *sv, RZ_StrView suffix);

RZ_DEC bool rz__sv_split(RZ_StrView *sv, RZ_StrView delim, RZ_StrView *result, bool inclusive);
RZ_DEC bool rz__sv_split_char(RZ_StrView *sv, char delim, RZ_StrView *result, bool inclusive);
RZ_DEC bool rz__sv_split_by(RZ_StrView *sv, RZ_StrViewCharPredicate fn, RZ_StrView *result, bool inclusive);

#    define rz_sv_split(sv, delim, result)                     rz__sv_split(sv, delim, result, false)
#    define rz_sv_split_cstr(sv, delim_cstr, result)           rz__sv_split(sv, rz_sv(delim_cstr), result, false)
#    define rz_sv_split_char(sv, delim, result)                rz__sv_split_char(sv, delim, result, false)
#    define rz_sv_split_by(sv, predicate, result)              rz__sv_split_by(sv, predicate, result, false)

#    define rz_sv_split_inclusive(sv, delim, result)           rz__sv_split(sv, delim, result, true)
#    define rz_sv_split_inclusive_cstr(sv, delim_cstr, result) rz__sv_split(sv, rz_sv(delim_cstr), result, true)
#    define rz_sv_split_inclusive_char(sv, delim, result)      rz__sv_split_char(sv, delim, result, true)
#    define rz_sv_split_inclusive_by(sv, predicate, result)    rz__sv_split_by(sv, predicate, result, true)

RZ_DEC bool rz__sv_rsplit(RZ_StrView *sv, RZ_StrView delim, RZ_StrView *result, bool inclusive);
RZ_DEC bool rz__sv_rsplit_char(RZ_StrView *sv, char delim, RZ_StrView *result, bool inclusive);
RZ_DEC bool rz__sv_rsplit_by(RZ_StrView *sv, RZ_StrViewCharPredicate fn, RZ_StrView *result, bool inclusive);

#    define rz_sv_rsplit(sv, delim, result)                     rz__sv_rsplit(sv, delim, result, false)
#    define rz_sv_rsplit_cstr(sv, delim_cstr, result)           rz__sv_rsplit(sv, rz_sv(delim_cstr), result, false)
#    define rz_sv_rsplit_char(sv, delim, result)                rz__sv_rsplit_char(sv, delim, result, false)
#    define rz_sv_rsplit_by(sv, predicate, result)              rz__sv_rsplit_by(sv, predicate, result, false)

#    define rz_sv_rsplit_inclusive(sv, delim, result)           rz__sv_rsplit(sv, delim, result, true)
#    define rz_sv_rsplit_inclusive_cstr(sv, delim_cstr, result) rz__sv_rsplit(sv, rz_sv(delim_cstr), result, true)
#    define rz_sv_rsplit_inclusive_char(sv, delim, result)      rz__sv_rsplit_char(sv, delim, result, true)
#    define rz_sv_rsplit_inclusive_by(sv, predicate, result)    rz__sv_rsplit_by(sv, predicate, result, true)

RZ_DEC bool rz__sv_split_once(RZ_StrView sv, RZ_StrView delim, RZ_StrView *left_result, RZ_StrView *right_result, bool inclusive);
RZ_DEC bool rz__sv_split_once_char(RZ_StrView sv, rz_char delim, RZ_StrView *left_result, RZ_StrView *right_result, bool inclusive);
RZ_DEC bool rz__sv_split_once_by(RZ_StrView sv, RZ_StrViewCharPredicate fn, RZ_StrView *left_result, RZ_StrView *right_result, bool inclusive);

#    define rz_sv_split_once(sv, delim, left_result, right_result)                     rz__sv_split_once(sv, delim, left_result, right_result, false)
#    define rz_sv_split_once_cstr(sv, delim_cstr, left_result, right_result)           rz__sv_split_once(sv, rz_sv(delim_cstr), left_result, right_result, false)
#    define rz_sv_split_once_char(sv, delim, left_result, right_result)                rz__sv_split_once_char(sv, delim, left_result, right_result, false)
#    define rz_sv_split_once_by(sv, predicate, left_result, right_result)              rz__sv_split_once_by(sv, predicate, left_result, right_result, false)
#    define rz_sv_split_once_inclusive(sv, delim, left_result, right_result)           rz__sv_split_once(sv, delim, left_result, right_result, true)
#    define rz_sv_split_once_inclusive_cstr(sv, delim_cstr, left_result, right_result) rz__sv_split_once(sv, rz_sv(delim_cstr), left_result, right_result, true)
#    define rz_sv_split_once_inclusive_char(sv, delim, left_result, right_result)      rz__sv_split_once_char(sv, delim, left_result, right_result, true)
#    define rz_sv_split_once_inclusive_by(sv, predicate, left_result, right_result)    rz__sv_split_once_by(sv, predicate, left_result, right_result, true)

RZ_DEC bool rz__sv_rsplit_once(RZ_StrView sv, RZ_StrView delim, RZ_StrView *left_result, RZ_StrView *right_result, bool inclusive);
RZ_DEC bool rz__sv_rsplit_once_char(RZ_StrView sv, rz_char delim, RZ_StrView *left_result, RZ_StrView *right_result, bool inclusive);
RZ_DEC bool rz__sv_rsplit_once_by(RZ_StrView sv, RZ_StrViewCharPredicate fn, RZ_StrView *left_result, RZ_StrView *right_result, bool inclusive);

#    define rz_sv_rsplit_once(sv, delim, left_result, right_result)                     rz__sv_rsplit_once(sv, delim, left_result, right_result, false)
#    define rz_sv_rsplit_once_cstr(sv, delim_cstr, left_result, right_result)           rz__sv_rsplit_once(sv, rz_sv(delim_cstr), left_result, right_result, false)
#    define rz_sv_rsplit_once_char(sv, delim, left_result, right_result)                rz__sv_rsplit_once_char(sv, delim, left_result, right_result, false)
#    define rz_sv_rsplit_once_by(sv, predicate, left_result, right_result)              rz__sv_rsplit_once_by(sv, predicate, left_result, right_result, false)
#    define rz_sv_rsplit_once_inclusive(sv, delim, left_result, right_result)           rz__sv_rsplit_once(sv, delim, left_result, right_result, true)
#    define rz_sv_rsplit_once_inclusive_cstr(sv, delim_cstr, left_result, right_result) rz__sv_rsplit_once(sv, rz_sv(delim_cstr), left_result, right_result, true)
#    define rz_sv_rsplit_once_inclusive_char(sv, delim, left_result, right_result)      rz__sv_rsplit_once_char(sv, delim, left_result, right_result, true)
#    define rz_sv_rsplit_once_inclusive_by(sv, predicate, left_result, right_result)    rz__sv_rsplit_once_by(sv, predicate, left_result, right_result, true)

RZ_DEC void rz_sv_to_ascii_lowercase(RZ_StrView *sv);
RZ_DEC void rz_sv_to_ascii_uppercase(RZ_StrView *sv);

/// Calculates the minimum number of insertions, deletions, and substitutions
/// required to change one string into the other.
RZ_DEC rz_usize rz_sv_levenshtein_distance(RZ_StrView pattern, RZ_StrView txt);
#    define rz_sv_levenshtein_distance_cstr(pattern, txt) rz_sv_levenshtein_distance(pattern, rz_sv(txt))

/// Like Levenshtein but allows for adjacent transpositions. Each substring can
/// only be edited once.
RZ_DEC rz_usize rz_sv_osa_distance(RZ_StrView pattern, RZ_StrView txt);
#    define rz_sv_osa_distance_cstr(pattern, txt) rz_sv_osa_distance(pattern, rz_sv(txt))

#    if defined(__cplusplus)
}
#    endif /* ifndef __cplusplus */
#endif     /* end of include guard: RZ_STRINGS_H */
