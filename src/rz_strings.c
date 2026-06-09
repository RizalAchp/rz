#include "rz_strings.h"

/// BEGIN
#define ASCII_CASE_MASK ((rz_char)0b00100000)

RZ_DEF bool rz_is_ascii_whitespace(rz_char ch) {
    return ch == ' ' || (unsigned)ch - '\t' < 5;
}

RZ_DEF bool rz_is_ascii_alphabetic(rz_char ch) {
    return ((unsigned)ch | 32) - 'a' < 26;
}

RZ_DEF bool rz_is_ascii_digit(rz_char ch) {
    return (unsigned)ch - '0' < 10;
}

RZ_DEF bool rz_is_ascii_hexdigit(rz_char ch) {
    return rz_is_ascii_digit(ch) || ((unsigned)ch | 32) - 'a' < 6;
}

RZ_DEF bool rz_is_ascii_alphanumeric(rz_char ch) {
    return rz_is_ascii_alphabetic(ch) || rz_is_ascii_digit(ch);
}

RZ_DEF bool rz_is_ascii_lower(rz_char ch) {
    return (unsigned)ch - 'a' < 26;
}

RZ_DEF bool rz_is_ascii_upper(rz_char ch) {
    return (unsigned)ch - 'A' < 26;
}

RZ_DEF rz_char rz_ascii_lower(rz_char ch) {
    return ch | ((rz_char)rz_is_ascii_upper(ch) * ASCII_CASE_MASK);
}

RZ_DEF rz_char rz_ascii_upper(rz_char ch) {
    return ch ^ ((rz_char)rz_is_ascii_lower(ch) * ASCII_CASE_MASK);
}

#ifdef RZ_STRING_IMPL

RZ_DEF RZ_Str rz_str_sized_alloc(const rz_char *cstr, rz_usize size, RZ_Allocator a) {
    RZ_Str str = {.allocator = a};
    rz_str_append_sized_str(&str, cstr, size);
    return str;
}

#    ifdef RZ_SPRINTF_IMPL
#        include "rz_sprintf.h"
RZ_DEF RZ_Str rz_vstrf(RZ_Allocator a, const rz_char *fmt, va_list arg) {
    RZ_Str   str  = {.allocator = a};
    rz_char *cstr = rz_avsprintf(str.allocator, fmt, arg);
    rz_str_append_cstr(&str, cstr);
    return str;
}
RZ_DEF RZ_Str rz_strf(RZ_Allocator a, const rz_char *fmt, ...) {
    va_list arg;
    va_start(arg, fmt);
    RZ_Str str = rz_vstrf(a, fmt, arg);
    va_end(arg);
    return str;
}

RZ_DEC void rz_str_appendvf(RZ_Str *s, const rz_char *fmt, va_list args) {
    rz_str_append_cstr(s, rz_avsprintf(s->allocator, fmt, args));
}

RZ_DEF void rz_str_appendf(RZ_Str *s, const rz_char *fmt, ...) {
    va_list arg;
    va_start(arg, fmt);
    rz_str_append_cstr(s, rz_avsprintf(s->allocator, fmt, arg));
    va_end(arg);
}
#    endif

RZ_DEC const char *rz_str_to_cstr(RZ_Str *s) {
    rz_str_append_null(s);
    return s->data;
}

RZ_DEF rz_ptrdiff rz_sv_case_cmp(RZ_StrView lhs, RZ_StrView rhs) {
    // Handle empty sv
    bool lhs_empty = rz_arr_is_empty(&lhs);
    bool rhs_empty = rz_arr_is_empty(&rhs);
    if (lhs_empty && rhs_empty) return 0;
    else if (lhs_empty) return -1;
    else if (rhs_empty) return 1;
    else {
        if (lhs.len != rhs.len) return lhs.len - rhs.len;
        return strncasecmp(lhs.data, rhs.data, lhs.len);
    }
}

RZ_DEF rz_ptrdiff rz_sv_cmp(RZ_StrView lhs, RZ_StrView rhs) {
    // Handle empty sv
    bool lhs_empty = rz_arr_is_empty(&lhs);
    bool rhs_empty = rz_arr_is_empty(&rhs);

    if (lhs_empty && rhs_empty) return 0;
    else if (lhs_empty) return -1;
    else if (rhs_empty) return 1;
    else {
        if (lhs.len != rhs.len) return lhs.len - rhs.len;
        return memcmp(lhs.data, rhs.data, lhs.len);
    }
}

RZ_DEF rz_usize rz_sv_find(RZ_StrView sv, RZ_StrView needle) {
    if (needle.len == 0) return sv.len; // common convention: empty needle => end
    if (needle.len > sv.len) return RZ_NOT_FOUND;

    for (rz_usize i = 0; (i + needle.len) < sv.len; ++i, sv.data++, sv.len--) {
        if (rz_sv_eq(rz_sv_sized(sv.data, needle.len), needle)) return i;
    }
    return RZ_NOT_FOUND;
}

RZ_DEF rz_usize rz_sv_find_char(RZ_StrView sv, rz_char needle) {
    rz_foreach_idx(i, ch, &sv, if (needle == *ch) return i;);
    return RZ_NOT_FOUND;
}

RZ_DEF rz_usize rz_sv_find_by(RZ_StrView sv, RZ_StrViewCharPredicate fn) {
    if (1 > sv.len) return RZ_NOT_FOUND;

    for (rz_usize i = 0; (i + 1) < sv.len; ++i, sv.data++, sv.len--) {
        if (fn(*sv.data)) return i;
    }
    return RZ_NOT_FOUND;
}

RZ_DEF rz_usize rz_sv_rfind(RZ_StrView sv, RZ_StrView needle) {
    if (needle.len == 0) return sv.len; // common convention: empty needle => end
    if (needle.len > sv.len) return RZ_NOT_FOUND;

    for (rz_usize i = (rz_isize)(sv.len - needle.len); i-- > 0;) {
        RZ_StrView sub = rz_sv_sized(sv.data + (size_t)i, needle.len);
        if (rz_sv_eq(sub, needle)) return (rz_usize)i;
    }
    return RZ_NOT_FOUND;
}

RZ_DEF rz_usize rz_sv_rfind_char(RZ_StrView sv, rz_char needle) {
    for (rz_usize i = sv.len; i-- > 0;) {
        if (sv.data[i] == needle) return i;
    }
    return RZ_NOT_FOUND;
}

RZ_DEF rz_usize rz_sv_rfind_by(RZ_StrView sv, RZ_StrViewCharPredicate fn) {
    if (1 > sv.len) return RZ_NOT_FOUND;

    for (rz_usize i = sv.len; i-- > 0;) {
        if (fn(sv.data[i])) return i;
    }
    return RZ_NOT_FOUND;
}

bool rz_sv_starts_with(RZ_StrView sv, RZ_StrView starts_sv) {
    if (sv.len < starts_sv.len) return false;
    sv.len = starts_sv.len;
    return rz_sv_eq(sv, starts_sv);
}

bool rz_sv_ends_with(RZ_StrView sv, RZ_StrView ends_sv) {
    if (sv.len < ends_sv.len) return false;
    sv.data = sv.data + (sv.len - ends_sv.len);
    sv.len  = ends_sv.len;
    return rz_sv_eq(sv, ends_sv);
}

RZ_DEF RZ_StrView rz_sv_trim_prefix_char(RZ_StrView sv, rz_char ch) {
    size_t i = 0;
    while (i < sv.len && sv.data[i] == ch) i += 1;
    return rz_sv_sized(sv.data + i, sv.len - i);
}

RZ_DEF RZ_StrView rz_sv_trim_suffix_char(RZ_StrView sv, rz_char ch) {
    size_t i = 0;
    while (i < sv.len && sv.data[sv.len - 1 - i] == ch) {
        i += 1;
    }
    return rz_sv_sized(sv.data, sv.len - i);
}

RZ_DEF RZ_StrView rz_sv_trim_char(RZ_StrView sv, rz_char ch) {
    return rz_sv_trim_suffix_char(rz_sv_trim_prefix_char(sv, ch), ch);
}

RZ_DEF RZ_StrView rz_sv_trim_prefix_by(RZ_StrView sv, RZ_StrViewCharPredicate fn) {
    size_t i = 0;
    while (i < sv.len && fn(sv.data[i])) i += 1;
    return rz_sv_sized(sv.data + i, sv.len - i);
}

RZ_DEF RZ_StrView rz_sv_trim_suffix_by(RZ_StrView sv, RZ_StrViewCharPredicate fn) {
    size_t i = 0;
    while (i < sv.len && fn(sv.data[sv.len - 1 - i])) {
        i += 1;
    }
    return rz_sv_sized(sv.data, sv.len - i);
}

RZ_DEF RZ_StrView rz_sv_trim_by(RZ_StrView sv, RZ_StrViewCharPredicate fn) {
    return rz_sv_trim_suffix_by(rz_sv_trim_prefix_by(sv, fn), fn);
}

RZ_DEF RZ_StrView rz_sv_strip_nprefix(RZ_StrView *sv, rz_usize n) {
    RZ_StrView result = {0};
    rz_arr_split_at(sv, n, &result, sv);
    return result;
}

RZ_DEF RZ_StrView rz_sv_strip_nsuffix(RZ_StrView *sv, rz_usize n) {
    RZ_StrView result = {0};
    rz_arr_split_at(sv, n, sv, &result);
    return result;
}

RZ_DEF RZ_StrView rz_sv_strip_prefix_while(RZ_StrView *sv, RZ_StrViewCharPredicate pred) {
    size_t i = 0;
    while (i < sv->len && pred(sv->data[i])) {
        i += 1;
    }

    return rz_sv_strip_nprefix(sv, i);
}

RZ_DEF RZ_StrView rz_sv_strip_suffix_while(RZ_StrView *sv, RZ_StrViewCharPredicate pred) {
    size_t i = 0;
    while (i < sv->len && pred(sv->data[sv->len - 1 - i])) {
        i += 1;
    }

    return rz_sv_strip_nsuffix(sv, i);
}

RZ_DEF bool rz_sv_strip_prefix(RZ_StrView *sv, RZ_StrView prefix) {
    if (rz_sv_starts_with(*sv, prefix)) {
        rz_sv_strip_nprefix(sv, prefix.len);
        return true;
    }
    return false;
}

RZ_DEF bool rz_sv_strip_suffix(RZ_StrView *sv, RZ_StrView suffix) {
    if (rz_sv_ends_with(*sv, suffix)) {
        rz_sv_strip_nsuffix(sv, suffix.len);
        return true;
    }
    return false;
}

RZ_DEF bool rz__sv_split(RZ_StrView *sv, RZ_StrView delim, RZ_StrView *result, bool inclusive) {
    RZ_ASSERT_NOT_NULL(sv);
    RZ_ASSERT_NOT_NULL(result);
    if (rz_arr_is_empty(sv)) return false;

    rz_usize i = rz_sv_find(*sv, delim);
    if (i == RZ_NOT_FOUND) {
        result->len  = sv->len;
        result->data = sv->data;
        sv->len      = 0;
        sv->data     = 0;
        return true;
    }

    result->data = sv->data;
    result->len  = i + ((rz_usize)inclusive * delim.len);

    sv->len -= i + delim.len;
    sv->data += i + delim.len;
    return true;
}

RZ_DEF bool rz__sv_split_char(RZ_StrView *sv, char delim, RZ_StrView *result, bool inclusive) {
    RZ_ASSERT_NOT_NULL(sv);
    RZ_ASSERT_NOT_NULL(result);
    if (rz_arr_is_empty(sv)) return false;

    rz_usize i = rz_sv_find_char(*sv, delim);
    if (i == RZ_NOT_FOUND) {
        result->len  = sv->len;
        result->data = sv->data;
        sv->len      = 0;
        sv->data     = 0;
        return true;
    }
    result->data = sv->data;
    result->len  = i + (rz_usize)inclusive;

    sv->len -= i + 1;
    sv->data += i + 1;
    return true;
}

RZ_DEF bool rz__sv_split_by(RZ_StrView *sv, RZ_StrViewCharPredicate fn, RZ_StrView *result, bool inclusive) {
    RZ_ASSERT_NOT_NULL(sv);
    RZ_ASSERT_NOT_NULL(result);
    if (rz_arr_is_empty(sv)) return false;

    rz_usize i = rz_sv_find_by(*sv, fn);
    if (i == RZ_NOT_FOUND) {
        result->len  = sv->len;
        result->data = sv->data;
        sv->len      = 0;
        sv->data     = 0;
        return true;
    }
    result->data = sv->data;
    result->len  = i + (rz_usize)inclusive;

    sv->len -= i + 1;
    sv->data += i + 1;
    return true;
}

RZ_DEF bool rz__sv_rsplit(RZ_StrView *sv, RZ_StrView delim, RZ_StrView *result, bool inclusive) {
    RZ_ASSERT_NOT_NULL(sv);
    RZ_ASSERT_NOT_NULL(result);
    if (rz_arr_is_empty(sv)) return false;

    rz_usize i = rz_sv_rfind(*sv, delim);
    if (i == RZ_NOT_FOUND) {
        result->len  = sv->len;
        result->data = sv->data;
        sv->len      = 0;
        sv->data     = 0;
        return true;
    }

    rz_usize keep = i + ((rz_usize)!inclusive * delim.len); // if inclusive the result is  i + 0
    result->data  = sv->data + keep;
    result->len   = sv->len - keep;

    sv->len -= i;

    return true;
}

RZ_DEF bool rz__sv_rsplit_char(RZ_StrView *sv, char delim, RZ_StrView *result, bool inclusive) {
    RZ_ASSERT_NOT_NULL(sv);
    RZ_ASSERT_NOT_NULL(result);
    if (rz_arr_is_empty(sv)) return false;

    rz_usize i = rz_sv_rfind_char(*sv, delim);
    if (i == RZ_NOT_FOUND) {
        result->len  = sv->len;
        result->data = sv->data;
        sv->len      = 0;
        sv->data     = 0;
        return true;
    }

    rz_usize keep = i + ((rz_usize)!inclusive); // if inclusive the result is  i + 0
    result->data  = sv->data + keep;
    result->len   = sv->len - keep;

    sv->len -= i;

    return true;
}

RZ_DEF bool rz__sv_rsplit_by(RZ_StrView *sv, RZ_StrViewCharPredicate fn, RZ_StrView *result, bool inclusive) {
    RZ_ASSERT_NOT_NULL(sv);
    RZ_ASSERT_NOT_NULL(result);
    if (rz_arr_is_empty(sv)) return false;

    rz_usize i = rz_sv_rfind_by(*sv, fn);
    if (i == RZ_NOT_FOUND) {
        result->len  = sv->len;
        result->data = sv->data;
        sv->len      = 0;
        sv->data     = 0;
        return true;
    }

    rz_usize keep = i + ((rz_usize)!inclusive); // if inclusive the result is  i + 0
    result->data  = sv->data + keep;
    result->len   = sv->len - keep;

    sv->len -= i;

    return true;
}

RZ_DEF bool rz__sv_split_once(RZ_StrView sv, RZ_StrView delim, RZ_StrView *left_result, RZ_StrView *right_result, bool inclusive) {
    RZ_DBG_ASSERT(left_result != NULL && right_result != NULL);
    *right_result = sv;
    bool res      = rz__sv_split(right_result, delim, left_result, inclusive);
    return res && !rz_arr_is_empty(right_result);
}
RZ_DEF bool rz__sv_split_once_char(RZ_StrView sv, rz_char delim, RZ_StrView *left_result, RZ_StrView *right_result, bool inclusive) {
    RZ_DBG_ASSERT(left_result != NULL && right_result != NULL);
    *right_result = sv;
    bool res      = rz__sv_split_char(right_result, delim, left_result, inclusive);
    return res && !rz_arr_is_empty(right_result);
}
RZ_DEF bool rz__sv_split_once_by(RZ_StrView sv, RZ_StrViewCharPredicate fn, RZ_StrView *left_result, RZ_StrView *right_result, bool inclusive) {
    RZ_DBG_ASSERT(left_result != NULL && right_result != NULL);
    *right_result = sv;
    bool res      = rz__sv_split_by(right_result, fn, left_result, inclusive);
    return res && !rz_arr_is_empty(right_result);
}

RZ_DEF bool rz__sv_rsplit_once(RZ_StrView sv, RZ_StrView delim, RZ_StrView *left_result, RZ_StrView *right_result, bool inclusive) {
    RZ_DBG_ASSERT(left_result != NULL && right_result != NULL);
    *left_result = sv;
    bool res     = rz__sv_rsplit(left_result, delim, right_result, inclusive);
    return res && !rz_arr_is_empty(left_result);
}
RZ_DEF bool rz__sv_rsplit_once_char(RZ_StrView sv, rz_char delim, RZ_StrView *left_result, RZ_StrView *right_result, bool inclusive) {
    RZ_DBG_ASSERT(left_result != NULL && right_result != NULL);
    *left_result = sv;
    bool res     = rz__sv_rsplit_char(left_result, delim, right_result, inclusive);
    return res && !rz_arr_is_empty(left_result);
}
RZ_DEF bool rz__sv_rsplit_once_by(RZ_StrView sv, RZ_StrViewCharPredicate fn, RZ_StrView *left_result, RZ_StrView *right_result, bool inclusive) {
    RZ_DBG_ASSERT(left_result != NULL && right_result != NULL);
    *left_result = sv;
    bool res     = rz__sv_rsplit_by(left_result, fn, right_result, inclusive);
    return res && !rz_arr_is_empty(left_result);
}

RZ_DEF void rz_sv_to_ascii_lowercase(RZ_StrView *sv) {
    rz_foreach(c, sv) *c = rz_ascii_lower(*c);
}

RZ_DEF void rz_sv_to_ascii_uppercase(RZ_StrView *sv) {
    rz_foreach(c, sv) *c = rz_ascii_upper(*c);
}

RZ_DEF rz_usize rz_sv_levenshtein_distance(RZ_StrView a, RZ_StrView b) {
    if (b.len == 0) return a.len;
    if (a.len == 0) return b.len;

    auto save                = rz_temp_snapshot();
    auto b_len               = b.len;

    RZ_Array(rz_usize) cache = {.allocator = rz_temp_allocator()};
    rz_arr_resize(&cache, b_len);
    for (rz_usize i = 1; i <= b_len; ++i) cache.data[i] = i;

    auto result = b_len;

    rz_foreach_idx(i, a_elem, &a, {
        result          = i + 1;
        auto distance_b = i;

        rz_foreach_idx(j, b_elem, &b, {
            auto cost       = (rz_usize)(*a_elem != *b_elem);
            auto distance_a = distance_b + cost;
            distance_b      = cache.data[j];
            result          = RZ_MIN(result + 1, RZ_MIN(distance_a, distance_b + 1));
            cache.data[j]   = result;
        });
    });

    rz_temp_rewind(save);
    return result;
}

RZ_DEF rz_usize rz_sv_osa_distance(RZ_StrView a, RZ_StrView b) {
    auto b_len            = b.len;

    rz_usize distance_len = b_len + 1;

    // 0..=b_len behaves like 0..b_len.saturating_add(1) which could be a different size
    // this leads to significantly worse code gen when swapping the vectors below
    rz_usize prev_two_distances[distance_len];
    rz_usize prev_distances[distance_len];
    for (rz_usize i = 0; i <= b_len; ++i) {
        prev_two_distances[i] = i;
        prev_distances[i]     = i;
    }
    rz_usize curr_distances[distance_len];
    rz_memzero(curr_distances, distance_len);

    rz_char prev_a_char = RZ_I8_MAX;
    rz_char prev_b_char = RZ_I8_MAX;

    rz_foreach_idx(i, a_char, &a, {
        curr_distances[0] = i + 1;

        rz_foreach_idx(j, b_char, &b, {
            auto cost             = (rz_usize)(*a_char != *b_char);
            curr_distances[j + 1] = RZ_MIN(curr_distances[j] + 1, RZ_MIN(prev_distances[j + 1] + 1, prev_distances[j] + cost));
            if ((i > 0) && (j > 0) && (*a_char != *b_char) && (*a_char == prev_b_char) && (*b_char == prev_a_char)) {
                curr_distances[j + 1] = RZ_MIN(curr_distances[j + 1], prev_two_distances[j - 1] + 1);
            }

            prev_b_char = *b_char;
        });

        rz_memswap(&prev_two_distances, &prev_distances, distance_len * sizeof(rz_usize));
        rz_memswap(&prev_distances, &curr_distances, distance_len * sizeof(rz_usize));
        prev_a_char = *a_char;
    });

    // access prev_distances instead of curr_distances since we swapped
    // them above. In case a is empty this would still contain the correct value
    // from initializing the last element to b_len
    return prev_distances[b_len];
}

#endif /* ifdef RZ_STRING_IMPL */
/// END
