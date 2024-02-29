#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <hypertracer.h>
#include <hypertracer>
#include <initializer_list>
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

static void htCsvReadError_throw(htCsvReadError &err);

CsvReader::CsvReader(const std::filesystem::path &path) {
#ifdef WIN32
    FILE *file = _wfopen(path.c_str(), L"rb");
#else
    FILE *file = std::fopen(path.c_str(), "rb");
#endif
    if (!file) {
        htCsvReadError err;
        err.code = HTErrIO;
        err.io.libc_errno = errno;
        htCsvReadError_throw(err);
    }
    auto err = htCsvReader_new(&reader, file);
    if (err.code != HTNoError) {
        std::fclose(file);
        htCsvReadError_throw(err);
    }
    this->file = file;
}

CsvReader::~CsvReader() {
    htCsvReader_free(reader);
    std::fclose(static_cast<FILE *>(file));
}

void CsvReader::read_row(bool &eof) {
    auto err = htCsvReader_read_row(reader, &eof);
    htCsvReadError_throw(err);
}

std::size_t CsvReader::num_columns() const {
    return htCsvReader_num_columns(reader);
}

std::string_view CsvReader::column_name_by_index(std::size_t column) const {
    auto result = htCsvReader_column_name_by_index(reader, column);
    return std::string_view(result.buf, result.len);
}

std::optional<std::size_t> CsvReader::column_index_by_name(std::string_view column) const {
    std::size_t out;
    if (htCsvReader_column_index_by_name(reader, &out, column.data(), column.length())) {
        return out;
    }
    return std::nullopt;
}

std::string_view CsvReader::value_by_column_index(std::size_t column) const {
    auto result = htCsvReader_value_by_column_index(reader, column);
    return std::string_view(result.buf, result.len);
}

std::optional<std::string_view> CsvReader::value_by_column_name(std::string_view column) const {
    htStrView out;
    if (htCsvReader_value_by_column_name(reader, &out, column.data(), column.length())) {
        return std::string_view(out.buf, out.len);
    }
    return std::nullopt;
}

static void htCsvWriteError_throw(htCsvWriteError &err);

static std::unique_ptr<htStrView[]> htCsvWriter_header_to_htStrView(std::initializer_list<std::string_view> header);
static std::unique_ptr<htStrView[]> htCsvWriter_header_to_htStrView(std::span<std::string_view> header);

CsvWriter::CsvWriter(const std::filesystem::path &path, std::initializer_list<std::string_view> header) {
#ifdef WIN32
    FILE *file = _wfopen(path.c_str(), L"wb");
#else
    FILE *file = std::fopen(path.c_str(), "wb");
#endif
    if (!file) {
        htCsvWriteError err;
        err.code = HTErrIO;
        err.io.libc_errno = errno;
        htCsvWriteError_throw(err);
    }
    auto header_strview = htCsvWriter_header_to_htStrView(header);
    auto err = htCsvWriter_new(&writer, file, header_strview.get(), header.size());
    if (err.code != HTNoError) {
        std::fclose(file);
        htCsvWriteError_throw(err);
    }
    this->file = file;
}

CsvWriter::CsvWriter(const std::filesystem::path &path, std::span<std::string_view> header) {
#ifdef WIN32
    FILE *file = _wfopen(path.c_str(), L"wb");
#else
    FILE *file = std::fopen(path.c_str(), "wb");
#endif
    if (!file) {
        htCsvWriteError err;
        err.code = HTErrIO;
        err.io.libc_errno = errno;
        htCsvWriteError_throw(err);
    }
    auto header_strview = htCsvWriter_header_to_htStrView(header);
    auto err = htCsvWriter_new(&writer, file, header_strview.get(), header.size());
    if (err.code != HTNoError) {
        std::fclose(file);
        htCsvWriteError_throw(err);
    }
    this->file = file;
}

CsvWriter::CsvWriter(const LogFile &log_file, std::initializer_list<std::string_view> header) :
    file(nullptr) {
    auto header_strview = htCsvWriter_header_to_htStrView(header);
    auto err = htCsvWriter_new(&writer, log_file.log_file->file, header_strview.get(), header.size());
    htCsvWriteError_throw(err);
}

CsvWriter::CsvWriter(const LogFile &log_file, std::span<std::string_view> header) :
    file(nullptr) {
    auto header_strview = htCsvWriter_header_to_htStrView(header);
    auto err = htCsvWriter_new(&writer, log_file.log_file->file, header_strview.get(), header.size());
    htCsvWriteError_throw(err);
}

static std::unique_ptr<htStrView[]> htCsvWriter_header_to_htStrView(std::initializer_list<std::string_view> header) {
    auto result = std::make_unique_for_overwrite<htStrView[]>(header.size());
    std::size_t i = 0;
    for (const auto &column : header) {
        result[i].buf = column.data();
        result[i].len = column.length();
        i++;
    }
    return result;
}

static std::unique_ptr<htStrView[]> htCsvWriter_header_to_htStrView(std::span<std::string_view> header) {
    auto result = std::make_unique_for_overwrite<htStrView[]>(header.size());
    for (std::size_t i = 0; i < header.size(); i++) {
        result[i].buf = header[i].data();
        result[i].len = header[i].length();
    }
    return result;
}

CsvWriter::~CsvWriter() {
    htCsvWriter_free(writer);
    if (file) {
        std::fclose(static_cast<FILE *>(file));
    }
}

static void htCsvWriter_free_managed_string(void *param);

void CsvWriter::set_string_by_column_index(std::size_t column, std::string &&value) {
    std::string *managed_value = new std::string(std::move(value));
    htCsvWriter_set_string_by_column_index(writer, column, managed_value->data(), managed_value->length(), htCsvWriter_free_managed_string, managed_value);
}

void CsvWriter::set_strview_by_column_index(std::size_t column, std::string_view value) {
    htCsvWriter_set_strview_by_column_index(writer, column, value.data(), value.length());
}

void CsvWriter::set_string_by_column_name(std::string_view column, std::string &&value) {
    std::string *managed_value = new std::string(std::move(value));
    htCsvWriter_set_string_by_column_name(writer, column.data(), column.length(), managed_value->data(), managed_value->length(), htCsvWriter_free_managed_string, managed_value);
}

void CsvWriter::set_strview_by_column_name(std::string_view column, std::string_view value) {
    htCsvWriter_set_strview_by_column_name(writer, column.data(), column.length(), value.data(), value.length());
}

void CsvWriter::write_row() {
    auto err = htCsvWriter_write_row(writer);
    htCsvWriteError_throw(err);
}

static void htCsvReadError_throw(htCsvReadError &err) {
    switch (err.code) {
    case HTNoError:
        htCsvReadError_free(&err);
        return;
    case HTErrIO: {
        auto exception = std::system_error((int) err.io.libc_errno, std::generic_category());
        htCsvReadError_free(&err);
        throw exception;
    }
    case HTErrQuoteNotClosed: {
        std::ostringstream ss;
        ss << "quote not closed at Line "sv << (err.io.pos_row + 1) << ", Col "sv << (err.io.pos_col + 1);
        auto exception = std::runtime_error(ss.str());
        htCsvReadError_free(&err);
        throw exception;
    }
    case HTErrColumnDuplicated: {
        std::ostringstream ss;
        ss << "column \""sv << std::string_view(err.column.name.buf, err.column.name.len) << "\" duplicated at both #"sv << err.column.index_a << " and #"sv << err.column.index_b;
        auto exception = std::runtime_error(ss.str());
        htCsvReadError_free(&err);
        throw exception;
    }
    default: {
        std::ostringstream ss;
        ss << "unknown error "sv << (int) err.code;
        auto exception = std::runtime_error(ss.str());
        htCsvReadError_free(&err);
        throw exception;
    }
    }
}

static void htCsvWriteError_throw(htCsvWriteError &err) {
    switch (err.code) {
    case HTNoError:
        htCsvWriteError_free(&err);
        return;
    case HTErrIO: {
        auto exception = std::system_error((int) err.io.libc_errno, std::generic_category());
        htCsvWriteError_free(&err);
        throw exception;
    }
    default: {
        std::ostringstream ss;
        ss << "unknown error "sv << (int) err.code;
        auto exception = std::runtime_error(ss.str());
        htCsvWriteError_free(&err);
        throw exception;
    }
    }
}

static void htCsvWriter_free_managed_string(void *param) {
    delete static_cast<std::string *>(param);
}

LogFile::LogFile(std::string_view prefix, unsigned num_retries) {
    log_file = new htLogFile;
    if (!htLogFile_new(log_file, prefix.data(), prefix.length(), num_retries)) {
        delete log_file;
        std::ostringstream ss;
        ss << "failed to create a file with prefix \""sv << prefix << "\" after "sv << num_retries << " retries"sv;
        throw std::runtime_error(ss.str());
    }
}

LogFile::~LogFile() {
    htLogFile_free(log_file);
    delete log_file;
}

std::string_view LogFile::filename() const {
    return std::string_view(log_file->filename.buf, log_file->filename.len);
}

Tracer::Tracer(const std::filesystem::path &path, std::initializer_list<std::string_view> header, std::size_t buffer_num_rows) {
#ifdef WIN32
    FILE *file = _wfopen(path.c_str(), L"wb");
#else
    FILE *file = std::fopen(path.c_str(), "wb");
#endif
    if (!file) {
        htCsvWriteError err;
        err.code = HTErrIO;
        err.io.libc_errno = errno;
        htCsvWriteError_throw(err);
    }
    auto header_strview = htCsvWriter_header_to_htStrView(header);
    auto err = htTracer_new(&tracer, file, header_strview.get(), header.size(), buffer_num_rows);
    if (err.code != HTNoError) {
        std::fclose(file);
        htCsvWriteError_throw(err);
    }
    this->file = file;
}

Tracer::Tracer(const std::filesystem::path &path, std::span<std::string_view> header, std::size_t buffer_num_rows) {
#ifdef WIN32
    FILE *file = _wfopen(path.c_str(), L"wb");
#else
    FILE *file = std::fopen(path.c_str(), "wb");
#endif
    if (!file) {
        htCsvWriteError err;
        err.code = HTErrIO;
        err.io.libc_errno = errno;
        htCsvWriteError_throw(err);
    }
    auto header_strview = htCsvWriter_header_to_htStrView(header);
    auto err = htTracer_new(&tracer, file, header_strview.get(), header.size(), buffer_num_rows);
    if (err.code != HTNoError) {
        std::fclose(file);
        htCsvWriteError_throw(err);
    }
    this->file = file;
}

Tracer::Tracer(const LogFile &log_file, std::initializer_list<std::string_view> header, std::size_t buffer_num_rows) :
    file(nullptr) {
    auto header_strview = htCsvWriter_header_to_htStrView(header);
    auto err = htTracer_new(&tracer, log_file.log_file->file, header_strview.get(), header.size(), buffer_num_rows);
    htCsvWriteError_throw(err);
}

Tracer::Tracer(const LogFile &log_file, std::span<std::string_view> header, std::size_t buffer_num_rows) :
    file(nullptr) {
    auto header_strview = htCsvWriter_header_to_htStrView(header);
    auto err = htTracer_new(&tracer, log_file.log_file->file, header_strview.get(), header.size(), buffer_num_rows);
    htCsvWriteError_throw(err);
}

Tracer::~Tracer() {
    htTracer_free(tracer);
    if (file) {
        std::fclose(static_cast<FILE *>(file));
    }
}

void Tracer::write_row(std::initializer_list<std::variant<std::string_view, std::string *>> columns) {
    auto line_buffer = std::make_unique_for_overwrite<htString[]>(columns.size());
    std::size_t i = 0;
    for (const auto &column : columns) {
        switch (column.index()) {
        case 0: {
            std::string_view value = std::get<0>(column);
            line_buffer[i].buf = const_cast<char *>(value.data());
            line_buffer[i].len = value.length();
            line_buffer[i].free_func = nullptr;
            line_buffer[i].free_param = nullptr;
            break;
        }
        case 1: {
            std::string *managed_value = std::get<1>(column);
            line_buffer[i].buf = managed_value->data();
            line_buffer[i].len = managed_value->length();
            line_buffer[i].free_func = htCsvWriter_free_managed_string;
            line_buffer[i].free_param = managed_value;
            break;
        }
        }
        i++;
    }
    htTracer_write_row(tracer, line_buffer.get());
}

void Tracer::write_row(std::span<std::variant<std::string_view, std::string *>> columns) {
    auto line_buffer = std::make_unique_for_overwrite<htString[]>(columns.size());
    for (std::size_t i = 0; i < columns.size(); i++) {
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
            line_buffer[i].free_func = htCsvWriter_free_managed_string;
            line_buffer[i].free_param = managed_value;
            break;
        }
        }
    }
    htTracer_write_row(tracer, line_buffer.get());
}

} // namespace ht
