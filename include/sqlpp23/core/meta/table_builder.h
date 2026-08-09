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
#include <type_traits>
#include <vector>

#include <sqlpp23/core/meta/column_info.h>
#include <sqlpp23/core/meta/table_info.h>
#include <sqlpp23/core/name/create_reflection_name_tag.h>
#include <sqlpp23/core/basic/table_columns.h>
#include <sqlpp23/core/detail/type_set.h>
#include <sqlpp23/core/type_traits/optional.h>
#include <sqlpp23/core/name/case_convert.h>
#include <sqlpp23/core/basic/table.h>

namespace sqlpp::detail{
	template<typename SqlType, ::sqlpp::detail::fixed_string CppName, bool HasDefault, ::sqlpp::detail::fixed_string SqlName = CppName>
	struct builder_column{
		using _sqlpp_name_tag = ::sqlpp::meta::reflection_alias<CppName, SqlName>::_sqlpp_name_tag;

		using data_type = SqlType;

		using has_default = ::std::bool_constant<HasDefault>;
	};

	template<typename T>
	struct is_required_insert_column : ::std::false_type{};

	template<typename Table, typename SqlType, ::sqlpp::detail::fixed_string CppName, ::sqlpp::detail::fixed_string SqlName>
	struct is_required_insert_column<::sqlpp::column_t<Table, ::sqlpp::detail::builder_column<SqlType, CppName, false, SqlName>>> : ::std::bool_constant<not ::sqlpp::is_optional<SqlType>::value>{};

	template<::sqlpp::detail::fixed_string CppName, ::sqlpp::detail::fixed_string SqlName, typename... Columns>
	struct builder_table{
		using _sqlpp_name_tag = ::sqlpp::meta::reflection_alias<CppName, SqlName>::_sqlpp_name_tag;

		template<typename T>
		using _table_columns = ::sqlpp::table_columns<T, Columns...>;

		using _required_insert_columns = ::sqlpp::detail::make_type_set_if_t<
			::sqlpp::detail::is_required_insert_column,
			::sqlpp::column_t<::sqlpp::table_t<::sqlpp::detail::builder_table<CppName, SqlName, Columns...>>, Columns>...
		>;
	};

	template<::sqlpp::meta::table_info TableInfo, ::sqlpp::meta::column_info ColumnInfo>
	inline consteval auto column_name_of(){
		if constexpr(ColumnInfo.sql_name_override.has_value()){
			return ColumnInfo.sql_name_override.value();
		} else if constexpr(ColumnInfo.naming_scheme_override.has_value()){
			return ::sqlpp::convert_name<ColumnInfo.naming_scheme_override.value(), ColumnInfo.name>();
		} else if constexpr(TableInfo.naming_scheme_override.has_value()){
			return ::sqlpp::convert_name<TableInfo.naming_scheme_override.value(), ColumnInfo.name>();
		} else{
			return ::sqlpp::convert_name<::sqlpp::naming_scheme::trim, ColumnInfo.name>();
		}
	}

	template<::sqlpp::meta::table_info TableInfo>
	inline consteval auto table_name_of(){
		if constexpr(TableInfo.sql_name_override.has_value()){
			return TableInfo.sql_name_override.value();
		} else if constexpr(TableInfo.naming_scheme_override.has_value()){
			return ::sqlpp::convert_name<TableInfo.naming_scheme_override.value(), TableInfo.name>();
		} else{
			return ::sqlpp::convert_name<::sqlpp::naming_scheme::trim, TableInfo.name>();
		}
	}
}

namespace sqlpp::meta{
	template<::sqlpp::meta::table_info TableInfo, ::sqlpp::meta::column_info ColumnInfo>
	inline consteval auto build_column(
		::std::constant_wrapper<TableInfo> /*table_info*/ = {},
		::std::constant_wrapper<ColumnInfo> /*column_info*/ = {}
	) -> ::std::meta::info{
		using column_info_type = ::std::remove_cvref_t<decltype(ColumnInfo)>;
		using column_type = ::std::conditional_t<
			::std::is_same_v<typename column_info_type::sql_type_override, ::std::nullopt_t>,
			/*?*/ typename column_info_type::type,
			/*:*/ typename column_info_type::sql_type_override
		>;
		constexpr ::sqlpp::detail::fixed_string cpp_name{ColumnInfo.name};
		constexpr ::sqlpp::detail::fixed_string sql_name{::sqlpp::detail::column_name_of<TableInfo, ColumnInfo>()};
		constexpr bool has_default{ColumnInfo.is_auto_incremented or ColumnInfo.has_default_initializer/* or ::sqlpp::is_optional<column_type>::value*/};

		return ^^::sqlpp::detail::builder_column<column_type, cpp_name, has_default, sql_name>;
	}

	template<::sqlpp::meta::table_info TableInfo, ::sqlpp::meta::column_info... ColumnInfos>
	inline consteval auto build_table() -> ::std::meta::info{
		constexpr ::sqlpp::detail::fixed_string cpp_name{TableInfo.name};
		constexpr ::sqlpp::detail::fixed_string sql_name{::sqlpp::detail::table_name_of<TableInfo>()};
		return substitute(^^::sqlpp::table_t, ::std::vector{substitute(^^::sqlpp::detail::builder_table, ::std::vector{^^cpp_name, ^^sql_name, ::sqlpp::meta::build_column<TableInfo, ColumnInfos>()...})});
	}
}
