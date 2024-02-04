// SPDX-FileCopyrightText: © Sebastián Mancilla <smancill@smancill.dev>
//
// SPDX-License-Identifier: MIT-0

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <thread>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>


constexpr auto max_cities = 10'000;
constexpr auto data_sep = ';';


struct StringHash {
    using is_transparent = void;

    [[nodiscard]] auto operator()(const char* arg) const noexcept -> std::size_t
    {
        return hash(arg);
    }

    [[nodiscard]] auto operator()(std::string_view arg) const noexcept -> std::size_t
    {
        return hash(arg);
    }

    [[nodiscard]] auto operator()(const std::string& arg) const noexcept -> std::size_t
    {
        return hash(arg);
    }

private:
    // FNV-1a 32bits
    static auto hash(std::string_view arg) noexcept -> std::size_t
    {
        constexpr auto prime = 0x01000193U;
        constexpr auto offset = 0x811C9DC5U;

        auto hash = offset;
        for (auto c : arg) {
            hash ^= static_cast<unsigned>(c);
            hash *= prime;
        }
        return hash;
    }
};


class MappedFile {
public:
    MappedFile(const std::filesystem::path& path)
      : path_{path}
      , file_{path}
      , size_{std::filesystem::file_size(path)}
      , data_{map_file(file_.fd, size_)}
    {
    }

    ~MappedFile() noexcept
    {
        munmap(data_, size_);
    }

    MappedFile(const MappedFile&) = delete;
    MappedFile(MappedFile&&) noexcept = default;

    auto operator=(const MappedFile&) -> MappedFile& = delete;
    auto operator=(MappedFile&&) noexcept -> MappedFile& = default;

    auto data() const noexcept -> const char*
    {
        return data_;
    }

    auto size() const noexcept -> std::size_t
    {
        return size_;
    }

    explicit operator std::string_view() const noexcept
    {
        return {data_, size_};
    }

private:
    struct Descriptor {
        explicit Descriptor(const std::filesystem::path& path)
          : fd{::open(path.c_str(), O_RDONLY, 0)}
        {
            if (fd < 0) {
                throw std::runtime_error{"could not open file"};
            }
        }

        ~Descriptor() noexcept
        {
            ::close(fd);
        }

        int fd;
    };

    static auto map_file(int file, std::size_t size) -> char*
    {
        auto data = ::mmap(nullptr, size, PROT_READ, MAP_SHARED, file, 0);
        if (data == MAP_FAILED) {
            throw std::runtime_error{"could not memory-map file"};
        }
        return static_cast<char*>(data);
    }

private:
    std::filesystem::path path_;
    Descriptor file_;
    std::size_t size_;
    char* data_;
};


class Reader
{
public:
    explicit Reader(std::string_view data) noexcept
      : data_{data}
    {}

    auto getline(std::string_view& line) noexcept -> bool
    {
        auto nl = data_.find('\n', pos_);
        if (nl == data_.npos) {
            return false;
        }
        line = data_.substr(pos_, nl - pos_);
        pos_ = nl + 1;
        return true;
    }

private:
    std::string_view data_;
    std::size_t pos_ = 0;
};


class Stats
{
public:
    auto update(std::int16_t temp) noexcept -> void
    {
        min_ = std::min(min_, temp);
        max_ = std::max(max_, temp);
        sum_ += temp;
        count_++;
    }

    auto merge(const Stats& o) noexcept -> void
    {
        min_ = std::min(min_, o.min_);
        max_ = std::max(max_, o.max_);
        sum_ += o.sum_;
        count_ += o.count_;
    }

    [[nodiscard]] auto min() const noexcept -> float
    {
        return min_ / 10.0;
    }

    [[nodiscard]] auto max() const noexcept -> float
    {
        return max_ / 10.0;
    }

    [[nodiscard]] auto avg() const noexcept -> float
    {
        return static_cast<float>(sum_ / 10.0 / count_);
    }

private:
    std::int16_t min_ = std::numeric_limits<std::int16_t>::max();
    std::int16_t max_ = std::numeric_limits<std::int16_t>::min();
    std::int64_t sum_ = 0;
    std::int32_t count_ = 0;
};


template<>
struct std::formatter<Stats>
{
    constexpr auto parse(std::format_parse_context& ctx) const
    {
        return ctx.begin();
    }

    auto format(const Stats& obj, std::format_context& ctx) const
    {
        constexpr auto fmt = "{:.1f}/{:.1f}/{:.1f}";
        return std::format_to(ctx.out(), fmt, obj.min(), obj.avg(), obj.max());
    }
};


using StatsMap = std::unordered_map<std::string, Stats, StringHash, std::equal_to<>>;


auto make_map(std::size_t capacity = max_cities) -> StatsMap
{
    auto map = StatsMap{};
    map.reserve(capacity);
    return map;
}


auto number_of_threads() -> int
{
    if (auto nthreads = std::getenv("NUM_THREADS"); nthreads) {
        return std::stoi(nthreads);
    }
    return std::thread::hardware_concurrency();
}


auto split_input(std::string_view data, int chunks) -> std::vector<std::string_view>
{
    auto segments = std::vector<std::string_view>(chunks);
    auto size = (data.size() + chunks - 1) / chunks;

    for (auto i = 0; i < chunks; ++i) {
        auto start = i * size;
        auto end = start + size;

        // start at next line
        if (i > 0 && data[start - 1] != '\n') {
            start = data.find('\n', start) + 1;
        }
        // end at eol
        if (i < chunks && data[end - 1] != '\n') {
            end = data.find('\n', end) + 1;
        }

        segments[i] = data.substr(start, end - start);
    }

    return segments;
}


auto get_temperature(std::string_view value) -> std::int16_t
{
    auto sign = 1;
    if (value[0] == '-') {
        sign = -1;
        value.remove_prefix(1);
    }
    // assume values in "d.d" or "dd.d" format
    if (value.size() == 3) {
        return sign * (10 * value[0] + value[2] - '0' * 11);
    }
    return sign * (100 * value[0] + 10 * value[1] + value[3] - '0' * 111);
}


auto merge_results(std::span<StatsMap> results) -> StatsMap
{
    auto merged = std::move(results.front());
    for (const auto& cities : results.subspan(1)) {
        for (const auto& [city, stats] : cities) {
            merged[city].merge(stats);
        }
    };
    return merged;
}


auto dump_results(const StatsMap& cities) -> void
{
    auto sorted = std::map{cities.begin(), cities.end()};

    auto out = std::ostream_iterator<char>(std::cout);
    std::cout << "{";

    auto it = std::begin(sorted);
    std::format_to(out, "{}={}", it->first, it->second);
    std::ranges::for_each(sorted | std::views::drop(1), [&out](const auto& elem) {
        std::format_to(out, ", {}={}", elem.first, elem.second);
    });

    std::cout << "}\n";
}


auto process_file(const std::filesystem::path& data_file) -> void
{
    auto file = MappedFile{data_file};

    const auto chunks = number_of_threads();
    const auto segments = split_input(std::string_view{file}, chunks);

    auto process_segment = [](std::string_view data) {
        auto cities = make_map();
        auto reader = Reader{data};
        for (auto line = std::string_view{}; reader.getline(line); ) {
            auto sep = line.find(data_sep);
            auto city = line.substr(0, sep);
            auto temp = get_temperature(line.substr(sep + 1));

            auto it = cities.find(city);
            if (it == cities.end()) {
                it = cities.emplace(city, Stats{}).first;
            }
            it->second.update(temp);
        }
        return cities;
    };

    auto process_all = [process_segment](std::span<const std::string_view> segments) {
        auto results = std::vector<StatsMap>(segments.size());
        auto threads = std::vector<std::jthread>{};
        for (auto i = 0; i < std::ssize(segments); ++i) {
            threads.emplace_back([=, &segments, &results]() {
                results[i] = process_segment(segments[i]);
            });
        }
        std::ranges::for_each(threads, &std::jthread::join);
        return merge_results(results);
    };

    const auto cities = process_all(segments);

    dump_results(cities);
}


auto main(int argc, char** argv) -> int
{
    if (argc > 2) {
        std::cerr << "usage: main [ <input_file> ]" << '\n';
        return EXIT_FAILURE;
    }

    auto data_file = std::filesystem::path{argc == 2 ? argv[1] : "measurements.txt"};
    process_file(data_file);

    return EXIT_SUCCESS;
}
