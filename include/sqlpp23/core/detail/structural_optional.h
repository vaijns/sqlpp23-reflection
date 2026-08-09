#pragma once

/*
 * Copyright (c) 2013-2015, Roland Bock
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 *   Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <optional>
#include <type_traits>
#include <functional>
#include <concepts>
#include <stdexcept>

namespace sqlpp::detail{
	template<typename T>
	struct structural_optional{
		using value_type = T;

		template<typename TFwd>
		inline constexpr structural_optional(::std::in_place_t, TFwd&& value)
			: m_value{.value{::std::forward<TFwd>(value)}}, m_has_value{true}{}

		inline constexpr structural_optional(::std::nullopt_t)
			: m_value{.nullopt{::std::nullopt}}, m_has_value{false}{}

		inline constexpr structural_optional(::sqlpp::detail::structural_optional<T> const& /*other*/) = default;
		inline constexpr structural_optional(::sqlpp::detail::structural_optional<T>&& /*other*/) = default;
		inline constexpr auto operator=(::sqlpp::detail::structural_optional<T> const& /*other*/) -> ::sqlpp::detail::structural_optional<T>& = default;
		inline constexpr auto operator=(::sqlpp::detail::structural_optional<T>&& /*other*/) -> ::sqlpp::detail::structural_optional<T>& = default;

		template<typename F>
		inline constexpr auto transform(F&& f) const -> ::sqlpp::detail::structural_optional<::std::invoke_result_t<F, T>>{
			if(not m_has_value){
				return ::sqlpp::detail::structural_optional<::std::invoke_result_t<F, T>>{::std::nullopt};
			}
			return ::sqlpp::detail::structural_optional<::std::invoke_result_t<F, T>>{
				::std::in_place,
				::std::invoke(::std::forward<F>(f), m_value.value)
			};
		}

		template<typename F>
		inline constexpr auto and_then(F&& f) const -> ::sqlpp::detail::structural_optional<typename ::std::invoke_result_t<F, T>::value_type>{
			if(not m_has_value){
				return ::sqlpp::detail::structural_optional<typename ::std::invoke_result_t<F, T>::value_type>{::std::nullopt};
			}
			return ::std::invoke(::std::forward<F>(f), m_value.value);
		}

		template<typename F> requires(::std::same_as<::std::invoke_result_t<F>, ::sqlpp::detail::structural_optional<T>>)
		inline constexpr auto or_else(F&& f) const -> ::sqlpp::detail::structural_optional<T>{
			if(m_has_value){
				return *this;
			}

			return ::std::invoke(::std::forward<F>(f));
		}

		template<typename TFwd>
		inline constexpr auto value_or(TFwd&& value) const -> ::sqlpp::detail::structural_optional<T>{
			if(m_has_value){
				return m_value.value;
			}

			return value;
		}

		inline constexpr auto has_value() const -> bool{
			return m_has_value;
		}

		inline constexpr auto value() const -> T{
			if(not m_has_value){
				throw ::std::logic_error{"trying to access the value of a nullopt structural_optional"};
			}
			return m_value.value;
		}

		union{
			::std::nullopt_t nullopt;
			T value;
		} m_value;
		bool m_has_value;
	};
}
