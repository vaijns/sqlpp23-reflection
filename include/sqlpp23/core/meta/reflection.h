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
#include <ranges>

#include <sqlpp23/core/meta/annotation.h>
#include <sqlpp23/core/meta/table_info.h>
#include <sqlpp23/core/meta/column_info.h>
#include <sqlpp23/core/meta/table_builder.h>

namespace sqlpp::meta{
	template<::std::meta::info M> requires(is_nonstatic_data_member(M))
	inline consteval auto column_info_from(){
		using column_type = [: type_of(M) :];
		using sql_column_type = [: ::sqlpp::detail::sql_type_override_of<M>() :];
		constexpr ::sqlpp::detail::fixed_string<identifier_of(M).size() + 1uz> name{::std::from_range, identifier_of(M)};
		constexpr auto sql_name{::sqlpp::detail::column_name_override_of<M>()};
		constexpr ::std::size_t sql_name_size{::std::remove_cvref_t<decltype(sql_name)>::value_type::size};

		return ::sqlpp::meta::column_info<column_type, name.size, sql_name_size, sql_column_type>{
			.sql_name_override = sql_name,
			.naming_scheme_override = ::sqlpp::detail::naming_scheme_of<M>(),
			.name = name,
			.is_primary_key = ::sqlpp::detail::has_primary_key_annotation<M>(),
			.is_auto_incremented = ::sqlpp::detail::has_auto_increment_annotation<M>(),
			.has_default_initializer = has_default_member_initializer(M)
		};
	}

	template<::std::meta::info M> requires(is_type(M))
	inline consteval auto table_info_from(){
		using table_type = [: M :];
		constexpr ::sqlpp::detail::fixed_string<identifier_of(M).size() + 1uz> name{::std::from_range, identifier_of(M)};
		constexpr auto sql_name{::sqlpp::detail::table_name_override_of<M>()};
		constexpr ::std::size_t sql_name_size{::std::remove_cvref_t<decltype(sql_name)>::value_type::size};
		return ::sqlpp::meta::table_info<table_type, name.size, sql_name_size>{
			.sql_name_override = sql_name,
			.naming_scheme_override = ::sqlpp::detail::naming_scheme_of<M>(),
			.name = name
		};
	}
}

namespace sqlpp::detail{
	template<::std::meta::info M, ::std::meta::info... ColumnMs> requires((is_type(M) and (is_nonstatic_data_member(ColumnMs) and ...)))
	inline consteval auto table_from_reflection_columns() -> ::std::meta::info{
		constexpr ::sqlpp::meta::table_info table_info{::sqlpp::meta::table_info_from<M>()};
		return ::sqlpp::meta::build_table<table_info, ::sqlpp::meta::column_info_from<ColumnMs>()...>();
	}

	template<::std::meta::info M> requires(is_type(M))
	inline consteval auto table_from_reflection() -> ::std::meta::info{
		constexpr ::std::span members{::std::define_static_array(nonstatic_data_members_of(M, std::meta::access_context::current().via(M)))};
		return [&]<::std::size_t... Is>(::std::index_sequence<Is...> /*index_sequence*/){
			// TODO: filter out columns with the ignore annotation
			return ::sqlpp::detail::table_from_reflection_columns<M, members[Is]...>();
		}(::std::make_index_sequence<members.size()>{});
	}
}

namespace sqlpp{
	template<typename T>
	using reflect_table = [: (::sqlpp::detail::table_from_reflection<^^T>()) :];
}
