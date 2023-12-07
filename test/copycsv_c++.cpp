#include <cstdlib>
#include <filesystem>
#include <hypertracer>
#include <iostream>
#include <string_view>
#include <vector>

using namespace std::literals;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cout << "Usage "sv << argv[0] << " INPUT.csv OUTPUT.csv"sv << std::endl;
        std::exit(1);
    }
    auto reader = ht::CsvReader(std::filesystem::path(argv[1]));

    size_t num_columns = reader.num_columns();
    size_t num_empty_columns = 0;
    std::vector<std::string_view> column_names;
    column_names.reserve(num_columns);
    for (size_t i = 0; i < num_columns; i++) {
        auto name = reader.column_name_by_index(i);
        // We want at most one empty column
        if (name.empty()) {
            if (num_empty_columns == 0) {
                column_names.push_back(name);
            }
            num_empty_columns++;
        } else {
            column_names.push_back(name);
        }
    }

    auto writer = ht::CsvWriter(std::filesystem::path(argv[2]), column_names);

    for (;;) {
        {
            bool eof;
            reader.read_row(eof);
            if (eof) {
                break;
            }
        }
        for (const auto &i : column_names) {
            auto str = reader.value_by_column_name(i);
            if (!str) {
                std::abort();
            }
            writer.set_string_by_column_name(i, std::string(*str));
        }
        writer.write_row();
    }
    return 0;
}
