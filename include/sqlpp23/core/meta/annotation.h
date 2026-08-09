#pragma once

/*
 * Copyright (c) 2026, Roland Bock
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

#include <meta>
#include <cstddef>
#include <cstdlib>

#include <sqlpp23/core/detail/fixed_string.h>
#include <sqlpp23/core/name/case_convert.h>
#include <sqlpp23/core/detail/structural_optional.h>

namespace sqlpp::detail{
	template<::std::size_t N>
	struct table_name_t{
		static constexpr ::std::constant_wrapper<N, ::std::size_t> size{};
		::sqlpp::detail::fixed_string<N> name;
	};

	template<::std::size_t N>
	struct column_name_t{
		static constexpr ::std::constant_wrapper<N, ::std::size_t> size{};
		::sqlpp::detail::fixed_string<N> name;
	};

	struct primary_key_t{};

	struct auto_increment_t{};

	struct ignore_t{};

	template<typename T>
	struct sql_type_override_t{
		using type = T;
	};
}

namespace sqlpp::meta::inline annotation{
	template<::std::size_t N>
	inline constexpr auto table(char const (&name)[N]) noexcept -> ::sqlpp::detail::table_name_t<N>{
		return {
			.name{::sqlpp::detail::fixed_string<N>{name}}
		};
	}

	template<::std::size_t N>
	inline constexpr auto column(char const (&name)[N]) noexcept -> ::sqlpp::detail::column_name_t<N>{
		return {
			.name{::sqlpp::detail::fixed_string<N>{name}}
		};
	}

	inline constexpr ::sqlpp::detail::primary_key_t primary_key{};
	inline constexpr ::sqlpp::detail::auto_increment_t auto_increment{};
	inline constexpr ::sqlpp::detail::ignore_t ignore{};

	template<typename T>
	inline constexpr ::sqlpp::detail::sql_type_override_t<T> sql_type{};
}

namespace sqlpp::detail{
	template<typename T, template<typename...> typename Template>
	struct is_specialization_of : ::std::false_type{};

	template<template<typename...> typename Template, typename... Args>
	struct is_specialization_of<Template<Args...>, Template> : ::std::true_type{};

	template<typename T, template<typename...> typename Template>
	concept specialization_of = ::sqlpp::detail::is_specialization_of<T, Template>::value;

	template<typename T, template<auto...> typename Template>
	struct is_value_specialization_of : ::std::false_type{};

	template<template<auto...> typename Template, auto... Args>
	struct is_value_specialization_of<Template<Args...>, Template> : ::std::true_type{};

	template<typename T, template<auto...> typename Template>
	concept value_specialization_of = ::sqlpp::detail::is_value_specialization_of<T, Template>::value;

	template<::std::meta::info M> requires(is_type(M) or is_nonstatic_data_member(M))
	inline consteval auto naming_scheme_of() -> ::sqlpp::detail::structural_optional<::sqlpp::naming_scheme>{
		template for(constexpr auto annotation : ::std::define_static_array(annotations_of_with_type(M, ^^::sqlpp::naming_scheme))) {
			return ::sqlpp::detail::structural_optional<::sqlpp::naming_scheme>{::std::in_place, extract<::sqlpp::naming_scheme>(annotation)};
		}

		return ::sqlpp::detail::structural_optional<::sqlpp::naming_scheme>{::std::nullopt};
	}

	template<::std::meta::info M> requires(is_nonstatic_data_member(M))
	inline consteval auto column_annotation_of() -> ::std::optional<::std::meta::info>{
		template for(constexpr auto annotation : ::std::define_static_array(annotations_of(M))) {
			using const_annotation_type = [: type_of(annotation) :];
			using annotation_type = ::std::remove_cvref_t<const_annotation_type>;
			constexpr bool is_column_name_specialization = ::sqlpp::detail::value_specialization_of<annotation_type, ::sqlpp::detail::column_name_t>;

			if constexpr(is_column_name_specialization) {
				return ::std::optional<::std::meta::info>{::std::in_place, annotation};
			}
		}

		return ::std::optional<::std::meta::info>{::std::nullopt};
	}

	template<::std::meta::info M> requires(is_nonstatic_data_member(M))
	inline consteval auto column_name_override_of(){
		constexpr ::std::optional<::std::meta::info> column_annotation{::sqlpp::detail::column_annotation_of<M>()};
		if constexpr(column_annotation.has_value()){
			constexpr ::std::meta::info annotation{column_annotation.value()};
			using const_annotation_type = [: type_of(annotation) :];
			using annotation_type = ::std::remove_cvref_t<const_annotation_type>;
			constexpr ::sqlpp::detail::column_name_t column_name_annotation = extract<annotation_type>(annotation);
			constexpr ::std::size_t name_size = decltype(column_name_annotation)::size;
			constexpr ::sqlpp::detail::fixed_string<name_size> column_name = column_name_annotation.name;
			return ::sqlpp::detail::structural_optional<::sqlpp::detail::fixed_string<name_size>>{::std::in_place, column_name};
		} else{
			return ::sqlpp::detail::structural_optional<::sqlpp::detail::fixed_string<0uz>>{::std::nullopt};
		}
	}

	template<::std::meta::info M> requires(is_type(M))
	inline consteval auto table_annotation_of() -> ::std::optional<::std::meta::info>{
		template for(constexpr auto annotation : ::std::define_static_array(annotations_of(M))) {
			using const_annotation_type = [: type_of(annotation) :];
			using annotation_type = ::std::remove_cvref_t<const_annotation_type>;
			constexpr bool is_table_name_specialization = ::sqlpp::detail::value_specialization_of<annotation_type, ::sqlpp::detail::table_name_t>;

			if constexpr(is_table_name_specialization) {
				return ::std::optional<::std::meta::info>{::std::in_place, annotation};
			}
		}

		return ::std::optional<::std::meta::info>{::std::nullopt};
	}

	template<::std::meta::info M> requires(is_type(M))
	inline consteval auto table_name_override_of(){
		constexpr ::std::optional<::std::meta::info> table_annotation{::sqlpp::detail::table_annotation_of<M>()};
		if constexpr(table_annotation.has_value()){
			constexpr ::std::meta::info annotation{table_annotation.value()};
			using const_annotation_type = [: type_of(annotation) :];
			using annotation_type = ::std::remove_cvref_t<const_annotation_type>;
			constexpr ::sqlpp::detail::table_name_t table_name_annotation = extract<annotation_type>(annotation);
			constexpr ::std::size_t name_size = decltype(table_name_annotation)::size;
			constexpr ::sqlpp::detail::fixed_string<name_size> table_name = table_name_annotation.name;
			return ::sqlpp::detail::structural_optional<::sqlpp::detail::fixed_string<name_size>>{::std::in_place, table_name};
		} else{
			return ::sqlpp::detail::structural_optional<::sqlpp::detail::fixed_string<0uz>>{::std::nullopt};
		}
	}

	template<::std::meta::info M> requires(is_nonstatic_data_member(M))
	inline consteval auto sql_type_override_of() -> ::std::meta::info{
		template for(constexpr auto annotation : ::std::define_static_array(annotations_of(M))) {
			using const_annotation_type = [: type_of(annotation) :];
			using annotation_type = ::std::remove_cvref_t<const_annotation_type>;
			constexpr bool is_sql_type_override_specialization = ::sqlpp::detail::specialization_of<annotation_type, ::sqlpp::detail::sql_type_override_t>;

			if constexpr(is_sql_type_override_specialization) {
				constexpr ::sqlpp::detail::sql_type_override_t sql_type_override_annotation = extract<annotation_type>(annotation);
				using sql_type = decltype(sql_type_override_annotation)::type;
				return ^^sql_type;
			}
		}

		using default_type = ::std::nullopt_t;
		return ^^default_type;
	}

	template<::std::meta::info M, typename Annotation> requires(is_nonstatic_data_member(M))
	inline consteval auto has_annotation() -> bool{
		template for(constexpr auto annotation : ::std::define_static_array(annotations_of_with_type(M, ^^Annotation))) {
			return true;
		}
		return false;
	}

	template<::std::meta::info M> requires(is_nonstatic_data_member(M))
	inline consteval auto has_primary_key_annotation() -> bool{
		return ::sqlpp::detail::has_annotation<M, ::sqlpp::detail::primary_key_t>();
	}

	template<::std::meta::info M> requires(is_nonstatic_data_member(M))
	inline consteval auto has_auto_increment_annotation() -> bool{
		return ::sqlpp::detail::has_annotation<M, ::sqlpp::detail::auto_increment_t>();
	}

	template<::std::meta::info M> requires(is_nonstatic_data_member(M))
	inline consteval auto has_ignore_annotation() -> bool{
		return ::sqlpp::detail::has_annotation<M, ::sqlpp::detail::ignore_t>();
	}
}
