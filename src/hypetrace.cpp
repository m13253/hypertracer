#include <cerrno>
#include <filesystem>
#include <format>
#include <hypetrace>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>
#ifdef WIN32
#include <wchar.h>
#endif

#define _Bool bool
#include <hypetrace.h>

namespace ht {

using namespace std::literals;

static void HTCsvReadError_throw(HTCsvReadError &err);
static void HTCsvWriteError_throw(HTCsvWriteError &err);
static void HTCsvWriter_free_managed_string(void *param);

CsvReader::CsvReader(const std::filesystem::path &path) {
#ifdef WIN32
    FILE *file = _wfopen(path.c_str(), L"rb");
#else
    FILE *file = std::fopen(path.c_str(), "rb");
#endif
    if (!file) {
        HTCsvReadError err;
        err.code = HTErrIO;
        err.io.libc_errno = errno;
        HTCsvReadError_throw(err);
    }
    auto err = HTCsvReader_new(&reader, file);
    if (err.code != HTNoError) {
        std::fclose(file);
        HTCsvReadError_throw(err);
    }
    this->file = file;
}

CsvReader::~CsvReader() {
    HTCsvReader_free(reader);
    std::fclose(static_cast<FILE *>(file));
}

void CsvReader::read_row(bool &eof) {
    auto err = HTCsvReader_read_row(reader, &eof);
    HTCsvReadError_throw(err);
}

size_t CsvReader::num_columns() const {
    return HTCsvReader_num_columns(reader);
}

std::string_view CsvReader::column_name_by_index(size_t column) const {
    auto result = HTCsvReader_column_name_by_index(reader, column);
    return std::string_view(result.buf, result.len);
}

std::optional<size_t> CsvReader::column_index_by_name(std::string_view column) const {
    size_t out;
    if (HTCsvReader_column_index_by_name(reader, &out, column.data(), column.length())) {
        return out;
    }
    return std::nullopt;
}

std::string_view CsvReader::value_by_column_index(size_t column) const {
    auto result = HTCsvReader_value_by_column_index(reader, column);
    return std::string_view(result.buf, result.len);
}

std::optional<std::string_view> CsvReader::value_by_column_name(std::string_view column) const {
    HTStrView out;
    if (HTCsvReader_value_by_column_name(reader, &out, column.data(), column.length())) {
        return std::string_view(out.buf, out.len);
    }
    return std::nullopt;
}

CsvWriter::CsvWriter(const std::filesystem::path &path, std::span<std::string_view> header) {
#ifdef WIN32
    FILE *file = _wfopen(path.c_str(), L"wb");
#else
    FILE *file = std::fopen(path.c_str(), "wb");
#endif
    if (!file) {
        HTCsvWriteError err;
        err.code = HTErrIO;
        err.io.libc_errno = errno;
        HTCsvWriteError_throw(err);
    }
    std::vector<HTStrView> header_vector;
    header_vector.reserve(header.size());
    for (const auto &i : header) {
        header_vector.push_back(HTStrView{
            .buf = i.data(),
            .len = i.length()
        });
    }
    auto err = HTCsvWriter_new(&writer, file, header_vector.data(), header_vector.size());
    if (err.code != HTNoError) {
        std::fclose(file);
        HTCsvWriteError_throw(err);
    }
    this->file = file;
}

CsvWriter::~CsvWriter() {
    HTCsvWriter_free(writer);
    std::fclose(static_cast<FILE *>(file));
}

void CsvWriter::set_string_by_column_index(size_t column, std::string &&value) {
    std::string *managed_value = new std::string(std::move(value));
    HTCsvWriter_set_string_by_column_index(writer, column, managed_value->data(), managed_value->length(), HTCsvWriter_free_managed_string, managed_value);
}

void CsvWriter::set_strview_by_column_index(size_t column, std::string_view value) {
    HTCsvWriter_set_strview_by_column_index(writer, column, value.data(), value.length());
}

void CsvWriter::set_string_by_column_name(std::string_view column, std::string &&value) {
    std::string *managed_value = new std::string(std::move(value));
    HTCsvWriter_set_string_by_column_name(writer, column.data(), column.length(), managed_value->data(), managed_value->length(), HTCsvWriter_free_managed_string, managed_value);
}

void CsvWriter::set_strview_by_column_name(std::string_view column, std::string_view value) {
    HTCsvWriter_set_strview_by_column_name(writer, column.data(), column.length(), value.data(), value.length());
}

void CsvWriter::write_row() {
    auto err = HTCsvWriter_write_row(writer);
    HTCsvWriteError_throw(err);
}

static void HTCsvReadError_throw(HTCsvReadError &err) {
    switch (err.code) {
    case HTNoError:
        HTCsvReadError_free(&err);
        return;
    case HTErrIO: {
        auto exception = std::system_error((int) err.io.libc_errno, std::generic_category());
        HTCsvReadError_free(&err);
        throw exception;
    }
    case HTErrQuoteNotClosed: {
        auto exception = std::runtime_error(std::format("quote not closed at Line {}, Col {}"sv, err.io.pos_row + 1, err.io.pos_col + 1));
        HTCsvReadError_free(&err);
        throw exception;
    }
    case HTErrColumnDuplicated: {
        auto exception = std::runtime_error(std::format("column \"{}\" duplicated at both #{} and #{}\n"sv, std::string_view(err.column.name.buf, err.column.name.len), err.column.index_a, err.column.index_b));
        HTCsvReadError_free(&err);
        throw exception;
    }
    default: {
        auto exception = std::runtime_error(std::format("unknown error {}"sv, (int) err.code));
        HTCsvReadError_free(&err);
        throw exception;
    }
    }
}

static void HTCsvWriteError_throw(HTCsvWriteError &err) {
    switch (err.code) {
    case HTNoError:
        HTCsvWriteError_free(&err);
        return;
    case HTErrIO: {
        auto exception = std::system_error((int) err.io.libc_errno, std::generic_category());
        HTCsvWriteError_free(&err);
        throw exception;
    }
    default: {
        auto exception = std::runtime_error(std::format("unknown error {}"sv, (int) err.code));
        HTCsvWriteError_free(&err);
        throw exception;
    }
    }
}

static void HTCsvWriter_free_managed_string(void *param) {
    delete static_cast<std::string *>(param);
}

} // namespace ht
