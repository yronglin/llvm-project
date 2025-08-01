// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___FORMAT_RANGE_DEFAULT_FORMATTER_H
#define _LIBCPP___FORMAT_RANGE_DEFAULT_FORMATTER_H

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#include <__algorithm/ranges_copy.h>
#include <__chrono/statically_widen.h>
#include <__config>
#include <__format/concepts.h>
#include <__format/fmt_pair_like.h>
#include <__format/formatter.h>
#include <__format/range_format.h>
#include <__format/range_formatter.h>
#include <__iterator/back_insert_iterator.h>
#include <__ranges/concepts.h>
#include <__ranges/data.h>
#include <__ranges/from_range.h>
#include <__ranges/size.h>
#include <__type_traits/conditional.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/pair.h>
#include <string_view>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 23

template <class _Rp, class _CharT>
concept __const_formattable_range =
    ranges::input_range<const _Rp> && formattable<ranges::range_reference_t<const _Rp>, _CharT>;

template <class _Rp, class _CharT>
using __fmt_maybe_const _LIBCPP_NODEBUG = conditional_t<__const_formattable_range<_Rp, _CharT>, const _Rp, _Rp>;

// There is no definition of this struct, it's purely intended to be used to
// generate diagnostics.
template <class _Rp>
struct __instantiated_the_primary_template_of_format_kind;

template <range_format _Kp, ranges::input_range _Rp, class _CharT>
struct __range_default_formatter;

// Required specializations

template <ranges::input_range _Rp, class _CharT>
struct __range_default_formatter<range_format::sequence, _Rp, _CharT> {
private:
  using __maybe_const_r _LIBCPP_NODEBUG = __fmt_maybe_const<_Rp, _CharT>;
  range_formatter<remove_cvref_t<ranges::range_reference_t<__maybe_const_r>>, _CharT> __underlying_;

public:
  _LIBCPP_HIDE_FROM_ABI constexpr void set_separator(basic_string_view<_CharT> __separator) noexcept {
    __underlying_.set_separator(__separator);
  }
  _LIBCPP_HIDE_FROM_ABI constexpr void
  set_brackets(basic_string_view<_CharT> __opening_bracket, basic_string_view<_CharT> __closing_bracket) noexcept {
    __underlying_.set_brackets(__opening_bracket, __closing_bracket);
  }

  template <class _ParseContext>
  _LIBCPP_HIDE_FROM_ABI constexpr typename _ParseContext::iterator parse(_ParseContext& __ctx) {
    return __underlying_.parse(__ctx);
  }

  template <class _FormatContext>
  _LIBCPP_HIDE_FROM_ABI typename _FormatContext::iterator
  format(__maybe_const_r& __range, _FormatContext& __ctx) const {
    return __underlying_.format(__range, __ctx);
  }
};

template <ranges::input_range _Rp, class _CharT>
struct __range_default_formatter<range_format::map, _Rp, _CharT> {
private:
  using __maybe_const_map _LIBCPP_NODEBUG = __fmt_maybe_const<_Rp, _CharT>;
  using __element_type _LIBCPP_NODEBUG    = remove_cvref_t<ranges::range_reference_t<__maybe_const_map>>;
  range_formatter<__element_type, _CharT> __underlying_;

public:
  _LIBCPP_HIDE_FROM_ABI constexpr __range_default_formatter()
    requires(__fmt_pair_like<__element_type>)
  {
    __underlying_.set_brackets(_LIBCPP_STATICALLY_WIDEN(_CharT, "{"), _LIBCPP_STATICALLY_WIDEN(_CharT, "}"));
    __underlying_.underlying().set_brackets({}, {});
    __underlying_.underlying().set_separator(_LIBCPP_STATICALLY_WIDEN(_CharT, ": "));
  }

  template <class _ParseContext>
  _LIBCPP_HIDE_FROM_ABI constexpr typename _ParseContext::iterator parse(_ParseContext& __ctx) {
    return __underlying_.parse(__ctx);
  }

  template <class _FormatContext>
  _LIBCPP_HIDE_FROM_ABI typename _FormatContext::iterator
  format(__maybe_const_map& __range, _FormatContext& __ctx) const {
    return __underlying_.format(__range, __ctx);
  }
};

template <ranges::input_range _Rp, class _CharT>
struct __range_default_formatter<range_format::set, _Rp, _CharT> {
private:
  using __maybe_const_set _LIBCPP_NODEBUG = __fmt_maybe_const<_Rp, _CharT>;
  using __element_type _LIBCPP_NODEBUG    = remove_cvref_t<ranges::range_reference_t<__maybe_const_set>>;
  range_formatter<__element_type, _CharT> __underlying_;

public:
  _LIBCPP_HIDE_FROM_ABI constexpr __range_default_formatter() {
    __underlying_.set_brackets(_LIBCPP_STATICALLY_WIDEN(_CharT, "{"), _LIBCPP_STATICALLY_WIDEN(_CharT, "}"));
  }

  template <class _ParseContext>
  _LIBCPP_HIDE_FROM_ABI constexpr typename _ParseContext::iterator parse(_ParseContext& __ctx) {
    return __underlying_.parse(__ctx);
  }

  template <class _FormatContext>
  _LIBCPP_HIDE_FROM_ABI typename _FormatContext::iterator
  format(__maybe_const_set& __range, _FormatContext& __ctx) const {
    return __underlying_.format(__range, __ctx);
  }
};

template <range_format _Kp, ranges::input_range _Rp, class _CharT>
  requires(_Kp == range_format::string || _Kp == range_format::debug_string)
struct __range_default_formatter<_Kp, _Rp, _CharT> {
private:
  // This deviates from the Standard, there the exposition only type is
  //   formatter<basic_string<charT>, charT> underlying_;
  // Using a string_view allows the format function to avoid a copy of the
  // input range when it is a contigious range.
  formatter<basic_string_view<_CharT>, _CharT> __underlying_;

public:
  template <class _ParseContext>
  _LIBCPP_HIDE_FROM_ABI constexpr typename _ParseContext::iterator parse(_ParseContext& __ctx) {
    typename _ParseContext::iterator __i = __underlying_.parse(__ctx);
    if constexpr (_Kp == range_format::debug_string)
      __underlying_.set_debug_format();
    return __i;
  }

  template <class _FormatContext>
  _LIBCPP_HIDE_FROM_ABI typename _FormatContext::iterator
  format(conditional_t<ranges::input_range<const _Rp>, const _Rp&, _Rp&> __range, _FormatContext& __ctx) const {
    // When the range is contiguous use a basic_string_view instead to avoid a
    // copy of the underlying data. The basic_string_view formatter
    // specialization is the "basic" string formatter in libc++.
    if constexpr (ranges::contiguous_range<_Rp> && std::ranges::sized_range<_Rp>)
      return __underlying_.format(basic_string_view<_CharT>{ranges::data(__range), ranges::size(__range)}, __ctx);
    else
      return __underlying_.format(basic_string<_CharT>{from_range, __range}, __ctx);
  }
};

template <ranges::input_range _Rp, class _CharT>
  requires(format_kind<_Rp> != range_format::disabled && formattable<ranges::range_reference_t<_Rp>, _CharT>)
struct formatter<_Rp, _CharT> : __range_default_formatter<format_kind<_Rp>, _Rp, _CharT> {};

#endif // _LIBCPP_STD_VER >= 23

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___FORMAT_RANGE_DEFAULT_FORMATTER_H
