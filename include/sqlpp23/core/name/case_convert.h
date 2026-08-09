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

#include <sqlpp23/core/detail/fixed_string.h>

#include <array>
#include <string_view>
#include <cstddef>
#include <optional>
#include <ranges>
#include <algorithm>

namespace sqlpp{
	enum struct naming_scheme{
		keep,
		trim, // remove '_' on both sides and "m_"
		snake_case, // snake_case
		pascal_case, // PascalCase
		camel_case, // camelCase
		macro_case // MACRO_CASE
	};
}

namespace sqlpp::detail{
	template<::sqlpp::naming_scheme Scheme>
	struct case_convert;
}

namespace sqlpp{
	template<::sqlpp::naming_scheme Scheme, ::sqlpp::detail::fixed_string Name>
	inline consteval auto convert_name(
		::std::constant_wrapper<Name> name = {},
		::std::constant_wrapper<Scheme> /*naming_scheme*/ = {}
	){
		return ::sqlpp::detail::case_convert<Scheme>::convert(name);
	}
	// callable as (`using namespace sqlpp::detail;`, `using namespace sqlpp::literals;`):
	// - `convert_name<naming_scheme::snake_case, "my_string">()`
	// - `convert_name<naming_scheme::snake_case>("my_string"_sc)`
	// - `convert_name<naming_scheme::snake_case>(std::cw<fixed_string{"my_string"}>)`
	// - `convert_name("my_string"_sc, std:cw<naming_scheme::snake_case>)`
	// - `convert_name(std::cw<fixed_string{"my_string"}>, std:cw<naming_scheme::snake_case>)`
}

namespace sqlpp::detail{
	inline constexpr ::std::array separator_characters{'_', '-'};

	inline consteval auto trim(
		::std::string_view name
	) -> ::std::string_view{
		static constexpr auto is_separator{+[](char character) -> bool{
			return ::std::ranges::find(::sqlpp::detail::separator_characters, character) != ::std::end(::sqlpp::detail::separator_characters);
		}};

		auto first{::std::begin(name)};
		auto last{::std::end(name)};

		// remove leading '_'
		first = ::std::ranges::find_if_not(name, is_separator);
		// remove trailing '_'
		last = ::std::ranges::find_if_not(name | ::std::views::reverse, is_separator).base();

		// remove trailing "m__..."
		if(::std::distance(first, last) > 2){
			char a{*::std::next(first, 0uz)};
			char b{*::std::next(first, 1uz)};
			switch(a){
			case 'm':
			case 'M':
				if(is_separator(b)){
					first = ::std::next(first, 1uz);
					first = ::std::ranges::find_if_not(first, last, is_separator);
				}
				break;
			default:
				break;
			}
		}
		return ::std::string_view{first, last};
	}

	inline consteval auto has_word_split_at(
		::std::string_view name,
		::std::size_t index
	) -> bool{
		if(index > name.size()){
			return false;
		}

		static constexpr auto is_upper{+[](char character) -> bool{
			return character >= 'A' and character <= 'Z';
		}};
		static constexpr auto is_lower{+[](char character) -> bool{
			return character >= 'a' and character <= 'z';
		}};
		static constexpr auto is_alpha_numeric{+[](char character) -> bool{
			if(character >= 'a' and character <= 'z'){
				return true;
			}
			if(character >= 'A' and character <= 'Z'){
				return true;
			}
			if(character >= '0' and character <= '9'){
				return true;
			}

			return false;
		}};
		static constexpr auto is_separator{+[](char character) -> bool{
			return ::std::ranges::find(::sqlpp::detail::separator_characters, character) != ::std::end(::sqlpp::detail::separator_characters);
		}};
		static constexpr auto optional_is{[]<typename F>(F&& predicate, ::std::optional<char> character, bool or_value = false) -> bool{
			if(not character.has_value()){
				return or_value;
			}
			return predicate(character.value());
		}};

		::std::optional<char> previous{
			(index > 0uz)
				? ::std::optional<char>{::std::in_place, name[index - 1uz]}
				: ::std::optional<char>{::std::nullopt}
		};
		::std::optional<char> next{
			((index + 1uz) < name.size())
				? ::std::optional<char>{::std::in_place, name[index + 1uz]}
				: ::std::optional<char>{::std::nullopt}
		};
		char current{name[index]};

		// e.g. "SQLName" -> with 'N' as current, ["SQL", "Name"] as separate words
		if(optional_is(is_upper, previous) and is_upper(current) and optional_is(is_lower, next)){
			return true;
		}

		// e.g. "..._abc" -> current 'a' starts new word
		if(optional_is(is_separator, previous) and is_alpha_numeric(current)) {
			return true;
		}

		// e.g. "helloWorld" -> current 'W' starts a new word
		if(optional_is(is_lower, previous) and is_upper(current)){
			return true;
		}

		return false;
	}

	inline consteval auto word_count_of(
		::std::string_view name
	) -> ::std::size_t{
      		if(name.size() == 0uz){
			return 0uz;
      		}

		::std::size_t word_count{1uz};
      		::std::size_t current_size{0uz}; // make sure we don't have any empty words
		for(::std::size_t i{0uz}; i < name.size(); ++i){
			if((current_size > 0uz) and ::sqlpp::detail::has_word_split_at(name, i)){
				current_size = 0uz;
				++word_count;
			}
			++current_size;
		}

		return word_count;
	}

	template<::sqlpp::detail::fixed_string Name>
	inline consteval auto words_of(
		::std::constant_wrapper<Name> /*name*/ = {}
	){
		constexpr ::std::string_view trimmed{::sqlpp::detail::trim(static_cast<::std::string_view>(Name))};
		constexpr ::std::size_t word_count{::sqlpp::detail::word_count_of(trimmed)};
		::std::array<::std::string_view, word_count> result{};

		static constexpr auto is_separator{+[](char character) -> bool{
			return ::std::ranges::find(::sqlpp::detail::separator_characters, character) != ::std::end(::sqlpp::detail::separator_characters);
		}};

		::std::string_view remaining{trimmed};
		for(::std::size_t word_index{0uz}; word_index < word_count; ++word_index){
			::std::size_t skip{0uz};
			for(char c : remaining){
				if(not is_separator(c)){
					break;
				}
				++skip;
			}

			remaining = remaining.substr(skip);

			::std::size_t size{0uz};
			for(::std::size_t i{0uz}; i < remaining.size(); ++i){
				if((size > 0uz) and ::sqlpp::detail::has_word_split_at(remaining, i)){
					break;
				}
				if(is_separator(remaining[i])){
					break;
				}
				++size;
			}

			result[word_index] = remaining.substr(0uz, size);
			remaining = remaining.substr(size);
		}

		return result;
	}
}

template<>
struct sqlpp::detail::case_convert<::sqlpp::naming_scheme::keep>{
	template<::sqlpp::detail::fixed_string Name>
	static consteval auto convert(
		::std::constant_wrapper<Name> /*name*/ = {}
	){
		return Name;
	}
};
static_assert(static_cast<::std::string_view>(::sqlpp::convert_name<::sqlpp::naming_scheme::keep>(::std::cw<::sqlpp::detail::fixed_string{"SQLName"}>)) == "SQLName");

template<>
struct sqlpp::detail::case_convert<::sqlpp::naming_scheme::trim>{
	template<::sqlpp::detail::fixed_string Name>
	static consteval auto convert(
		::std::constant_wrapper<Name> /*name*/ = {}
	){
		constexpr ::std::string_view trimmed{::sqlpp::detail::trim(static_cast<::std::string_view>(Name))};
		return ::sqlpp::detail::fixed_string<trimmed.size() + 1uz>{::std::from_range, trimmed};
	}
};
static_assert(static_cast<::std::string_view>(::sqlpp::convert_name<::sqlpp::naming_scheme::trim>(::std::cw<::sqlpp::detail::fixed_string{"SQLName"}>)) == "SQLName");

template<>
struct sqlpp::detail::case_convert<::sqlpp::naming_scheme::snake_case>{
	static consteval auto to_lower(
		char character
	) -> char{
		if(character >= 'A' and character <= 'Z'){
			return character + ('a' - 'A');
		}
		return character;
	}

	template<::sqlpp::detail::fixed_string Name>
	static consteval auto convert(
		::std::constant_wrapper<Name> name = {}
	){
		constexpr auto words{::sqlpp::detail::words_of(name)};
		constexpr ::std::size_t required_size{::std::ranges::fold_left(
			words,
			1uz /* null-terminator */,
			[](::std::size_t agg, ::std::string_view str) -> ::std::size_t{ return agg + str.size(); }
		) + (words.size() - 1uz /* underscore for every word except the first */)};
		::std::array<char, required_size> result{'\0'};

		::std::size_t current_index{0uz};
		if(words.size() > 0uz){
			::std::string_view word{words[0uz]};
			for(::std::size_t i{0uz}; i < word.size(); ++i){
				char character{word[i]};
				result[current_index++] = ::sqlpp::detail::case_convert<::sqlpp::naming_scheme::snake_case>::to_lower(character);
			}
		}

		for(::std::size_t word_index{1uz}; word_index < words.size(); ++word_index){
			::std::string_view word{words[word_index]};
			result[current_index++] = '_';
			for(::std::size_t i{0uz}; i < word.size(); ++i){
				char character{word[i]};
				result[current_index++] = ::sqlpp::detail::case_convert<::sqlpp::naming_scheme::snake_case>::to_lower(character);
			}
		}

		return ::sqlpp::detail::fixed_string<required_size>{result};
	}
};
static_assert(static_cast<::std::string_view>(::sqlpp::convert_name<::sqlpp::naming_scheme::snake_case>(::std::cw<::sqlpp::detail::fixed_string{"SQLName"}>)) == "sql_name");

template<>
struct sqlpp::detail::case_convert<::sqlpp::naming_scheme::camel_case>{
	static consteval auto to_lower(
		char character
	) -> char{
		if(character >= 'A' and character <= 'Z'){
			return character + ('a' - 'A');
		}
		return character;
	}

	static consteval auto to_upper(
		char character
	) -> char{
		if(character >= 'a' and character <= 'z'){
			return character - ('a' - 'A');
		}
		return character;
	}

	template<::sqlpp::detail::fixed_string Name>
	static consteval auto convert(
		::std::constant_wrapper<Name> name = {}
	){
		constexpr auto words{::sqlpp::detail::words_of(name)};
		constexpr ::std::size_t required_size{::std::ranges::fold_left(
			words,
			1uz /* null-terminator */,
			[](::std::size_t agg, ::std::string_view str) -> ::std::size_t{ return agg + str.size(); }
		)};
		::std::array<char, required_size> result{'\0'};

		::std::size_t current_index{0uz};
		if(words.size() > 0uz){
			::std::string_view word{words[0uz]};
			for(::std::size_t i{0uz}; i < words[0uz].size(); ++i){
				char character{word[i]};
				result[current_index++] = ::sqlpp::detail::case_convert<::sqlpp::naming_scheme::camel_case>::to_lower(character);
			}
		}

		for(::std::size_t word_index{1uz}; word_index < words.size(); ++word_index){
			::std::string_view word{words[word_index]};
			if(word.size() > 0uz){
				char character{word[0uz]};
				result[current_index++] = ::sqlpp::detail::case_convert<::sqlpp::naming_scheme::camel_case>::to_upper(character);
			}
			for(::std::size_t i{1uz}; i < word.size(); ++i){
				char character{word[i]};
				result[current_index++] = ::sqlpp::detail::case_convert<::sqlpp::naming_scheme::camel_case>::to_lower(character);
			}
		}

		return ::sqlpp::detail::fixed_string<required_size>{result};
	}
};
static_assert(static_cast<::std::string_view>(::sqlpp::convert_name<::sqlpp::naming_scheme::camel_case>(::std::cw<::sqlpp::detail::fixed_string{"SQLName"}>)) == "sqlName");

template<>
struct sqlpp::detail::case_convert<::sqlpp::naming_scheme::pascal_case>{
	static consteval auto to_lower(
		char character
	) -> char{
		if(character >= 'A' and character <= 'Z'){
			return character + ('a' - 'A');
		}
		return character;
	}

	static consteval auto to_upper(
		char character
	) -> char{
		if(character >= 'a' and character <= 'z'){
			return character - ('a' - 'A');
		}
		return character;
	}

	template<::sqlpp::detail::fixed_string Name>
	static consteval auto convert(
		::std::constant_wrapper<Name> name = {}
	){
		constexpr auto words{::sqlpp::detail::words_of(name)};
		constexpr ::std::size_t required_size{::std::ranges::fold_left(
			words,
			1uz /* null-terminator */,
			[](::std::size_t agg, ::std::string_view str) -> ::std::size_t{ return agg + str.size(); }
		)};
		::std::array<char, required_size> result{'\0'};

		::std::size_t current_index{0uz};
		for(::std::string_view word : words){
			if(word.size() > 0uz){
				char character{word[0uz]};
				result[current_index++] = ::sqlpp::detail::case_convert<::sqlpp::naming_scheme::pascal_case>::to_upper(character);
			}
			for(::std::size_t i{1uz}; i < word.size(); ++i){
				char character{word[i]};
				result[current_index++] = ::sqlpp::detail::case_convert<::sqlpp::naming_scheme::pascal_case>::to_lower(character);
			}
		}

		return ::sqlpp::detail::fixed_string<required_size>{result};
	}
};
static_assert(static_cast<::std::string_view>(::sqlpp::convert_name<::sqlpp::naming_scheme::pascal_case>(::std::cw<::sqlpp::detail::fixed_string{"SQLName"}>)) == "SqlName");

template<>
struct sqlpp::detail::case_convert<::sqlpp::naming_scheme::macro_case>{
	static consteval auto to_upper(
		char character
	) -> char{
		if(character >= 'a' and character <= 'z'){
			return character - ('a' - 'A');
		}
		return character;
	}

	template<::sqlpp::detail::fixed_string Name>
	static consteval auto convert(
		::std::constant_wrapper<Name> name = {}
	){
		constexpr auto words{::sqlpp::detail::words_of(name)};
		constexpr ::std::size_t required_size{::std::ranges::fold_left(
			words,
			1uz /* null-terminator */,
			[](::std::size_t agg, ::std::string_view str) -> ::std::size_t{ return agg + str.size(); }
		) + (words.size() - 1uz /* underscore for every word except the first */)};
		::std::array<char, required_size> result{'\0'};

		::std::size_t current_index{0uz};
		if(words.size() > 0uz){
			::std::string_view word{words[0uz]};
			for(::std::size_t i{0uz}; i < word.size(); ++i){
				char character{word[i]};
				result[current_index++] = ::sqlpp::detail::case_convert<::sqlpp::naming_scheme::macro_case>::to_upper(character);
			}
		}

		for(::std::size_t word_index{1uz}; word_index < words.size(); ++word_index){
			::std::string_view word{words[word_index]};
			result[current_index++] = '_';
			for(::std::size_t i{0uz}; i < word.size(); ++i){
				char character{word[i]};
				result[current_index++] = ::sqlpp::detail::case_convert<::sqlpp::naming_scheme::macro_case>::to_upper(character);
			}
		}

		return ::sqlpp::detail::fixed_string<required_size>{result};
	}
};
static_assert(static_cast<::std::string_view>(::sqlpp::convert_name<::sqlpp::naming_scheme::macro_case>(::std::cw<::sqlpp::detail::fixed_string{"SQLName"}>)) == "SQL_NAME");
