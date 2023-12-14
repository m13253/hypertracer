#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <hypertracer.h>
#include <hypertracer>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>
#ifdef WIN32
#include <wchar.h>
#endif

namespace ht {

using namespace std::literals;

static void HTCsvReadError_throw(HTCsvReadError &err);

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

static void HTCsvWriteError_throw(HTCsvWriteError &err);

static std::unique_ptr<HTStrView[]> HTCsvWriter_header_to_HTStrView(std::span<std::string_view> header);

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
    auto header_strview = HTCsvWriter_header_to_HTStrView(header);
    auto err = HTCsvWriter_new(&writer, file, header_strview.get(), header.size());
    if (err.code != HTNoError) {
        std::fclose(file);
        HTCsvWriteError_throw(err);
    }
    this->file = file;
}

CsvWriter::CsvWriter(const LogFile &log_file, std::span<std::string_view> header) :
    file(nullptr) {
    auto header_strview = HTCsvWriter_header_to_HTStrView(header);
    auto err = HTCsvWriter_new(&writer, log_file.log_file->file, header_strview.get(), header.size());
    HTCsvWriteError_throw(err);
}

static std::unique_ptr<HTStrView[]> HTCsvWriter_header_to_HTStrView(std::span<std::string_view> header) {
    auto result = std::make_unique_for_overwrite<HTStrView[]>(header.size());
    for (size_t i = 0; i < header.size(); i++) {
        result[i].buf = header[i].data();
        result[i].len = header[i].length();
    }
    return result;
}

CsvWriter::~CsvWriter() {
    HTCsvWriter_free(writer);
    if (file) {
        std::fclose(static_cast<FILE *>(file));
    }
}

static void HTCsvWriter_free_managed_string(void *param);

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
        std::ostringstream ss;
        ss << "quote not closed at Line "sv << (err.io.pos_row + 1) << ", Col "sv << (err.io.pos_col + 1);
        auto exception = std::runtime_error(ss.str());
        HTCsvReadError_free(&err);
        throw exception;
    }
    case HTErrColumnDuplicated: {
        std::ostringstream ss;
        ss << "column \""sv << std::string_view(err.column.name.buf, err.column.name.len) << "\" duplicated at both #"sv << err.column.index_a << " and #"sv << err.column.index_b;
        auto exception = std::runtime_error(ss.str());
        HTCsvReadError_free(&err);
        throw exception;
    }
    default: {
        std::ostringstream ss;
        ss << "unknown error "sv << (int) err.code;
        auto exception = std::runtime_error(ss.str());
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
        std::ostringstream ss;
        ss << "unknown error "sv << (int) err.code;
        auto exception = std::runtime_error(ss.str());
        HTCsvWriteError_free(&err);
        throw exception;
    }
    }
}

static void HTCsvWriter_free_managed_string(void *param) {
    delete static_cast<std::string *>(param);
}

LogFile::LogFile(std::string_view prefix, unsigned num_retries) {
    log_file = new HTLogFile;
    if (!HTLogFile_new(log_file, prefix.data(), prefix.length(), num_retries)) {
        delete log_file;
        std::ostringstream ss;
        ss << "failed to create a file with prefix \""sv << prefix << "\" after "sv << num_retries << " retries"sv;
        throw std::runtime_error(ss.str());
    }
}

LogFile::~LogFile() {
    HTLogFile_free(log_file);
    delete log_file;
}

std::string_view LogFile::filename() const {
    return std::string_view(log_file->filename.buf, log_file->filename.len);
}

Tracer::Tracer(const std::filesystem::path &path, std::span<std::string_view> header, size_t buffer_num_rows) {
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
    auto header_strview = HTCsvWriter_header_to_HTStrView(header);
    auto err = HTTracer_new(&tracer, file, header_strview.get(), header.size(), buffer_num_rows);
    if (err.code != HTNoError) {
        std::fclose(file);
        HTCsvWriteError_throw(err);
    }
    this->file = file;
}

Tracer::Tracer(const LogFile &log_file, std::span<std::string_view> header, size_t buffer_num_rows) :
    file(nullptr) {
    auto header_strview = HTCsvWriter_header_to_HTStrView(header);
    auto err = HTTracer_new(&tracer, log_file.log_file->file, header_strview.get(), header.size(), buffer_num_rows);
    HTCsvWriteError_throw(err);
}

Tracer::~Tracer() {
    HTTracer_free(tracer);
    if (file) {
        std::fclose(static_cast<FILE *>(file));
    }
}

void Tracer::write_row(std::span<std::variant<std::string_view, std::string *>> columns) {
    auto line_buffer = std::make_unique_for_overwrite<HTString[]>(columns.size());
    for (size_t i = 0; i < columns.size(); i++) {
        switch (columns[i].index()) {
        case 0: {
            std::string_view value = std::get<0>(columns[i]);
            line_buffer[i].buf = const_cast<char *>(value.data());
            line_buffer[i].len = value.length();
            line_buffer[i].free_func = nullptr;
            line_buffer[i].free_param = nullptr;
            break;
        }
        case 1: {
            std::string *managed_value = std::get<1>(columns[i]);
            line_buffer[i].buf = managed_value->data();
            line_buffer[i].len = managed_value->length();
            line_buffer[i].free_func = HTCsvWriter_free_managed_string;
            line_buffer[i].free_param = managed_value;
            break;
        }
        }
    }
    HTTracer_write_row(tracer, line_buffer.get());
}

} // namespace ht
