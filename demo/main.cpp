#include <cstdlib>
#include <sqlite3.h>
#include <sqlpp23/sqlpp23.h>
#include <sqlpp23/core/meta/reflection.h>
#include <sqlpp23/sqlite3/sqlite3.h>
#include <memory>
#include <iostream>

struct [[=sqlpp::naming_scheme::snake_case]] [[=sqlpp::meta::table("my_table")]] user{
	[[=sqlpp::meta::auto_increment]]
	[[=sqlpp::meta::sql_type<sqlpp::integral>]]
	std::uint64_t id;

	[[=sqlpp::meta::column("username")]]
	[[=sqlpp::meta::sql_type<sqlpp::text>]]
	std::string name;

	[[=sqlpp::naming_scheme::pascal_case]]
	[[=sqlpp::meta::sql_type<sqlpp::timestamp>]]
	std::chrono::system_clock::time_point creation_time;
};

using my_table = sqlpp::reflect_table<user>;

inline auto log_cerr(sqlpp::log_category, std::string const& message) -> void{
	std::cerr << message << '\n';
}

auto main(int /*argc*/, char* /*argv*/[]) -> int{
	auto config = std::make_shared<sqlpp::sqlite3::connection_config>();
	config->path_to_database = "./testdb.sqlite3";
	config->flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
	config->debug = sqlpp::debug_logger({sqlpp::log_category::all}, log_cerr);
	sqlpp::sqlite3::connection db;
	db.connect_using(config);
	my_table table{};

	db(sqlpp::insert_into(table).set(table.name = "my_test_user_a", table.creation_time = std::chrono::system_clock::now()));
	db(sqlpp::insert_into(table).set(table.name = "my_test_user_b", table.creation_time = std::chrono::system_clock::now()));
	db(sqlpp::insert_into(table).set(table.name = "my_test_user_c", table.creation_time = std::chrono::system_clock::now()));
	db(sqlpp::insert_into(table).set(table.name = "my_test_user_d", table.creation_time = std::chrono::system_clock::now()));

	for(auto const& row : db(sqlpp::select(table.id, table.name, table.creation_time).from(table).where(table.id != 0u))){
		std::cout << "id: " << row.id << ", username: \"" << row.name << "\"\n";
	}
	return EXIT_SUCCESS;
}
