// SPDX-FileCopyrightText: © Sebastián Mancilla <smancill@smancill.dev>
//
// SPDX-License-Identifier: MIT-0

#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>


constexpr auto min_temp = -100.0;
constexpr auto max_temp = +100.0;
constexpr auto max_cities = 10'000;
constexpr auto max_line_length = 100;
constexpr auto data_sep = ';';


struct StringHash {
    using is_transparent = void;

    using hash_type = std::hash<std::string_view>;

    [[nodiscard]] auto operator()(const char* arg) const noexcept -> std::size_t
    {
        return hash_type{}(arg);
    }

    [[nodiscard]] auto operator()(std::string_view arg) const noexcept -> std::size_t
    {
        return hash_type{}(arg);
    }

    [[nodiscard]] auto operator()(const std::string& arg) const noexcept -> std::size_t
    {
        return hash_type{}(arg);
    }
};


class Stats
{
public:
    auto update(float temp) noexcept -> void
    {
        min_ = std::min(min_, temp);
        max_ = std::max(max_, temp);
        sum_ += temp;
        count_++;
    }

    [[nodiscard]] auto min() const noexcept -> float
    {
        return min_;
    }

    [[nodiscard]] auto max() const noexcept -> float
    {
        return max_;
    }

    [[nodiscard]] auto avg() const noexcept -> float
    {
        return static_cast<float>(sum_ / count_);
    }

private:
    float min_ = max_temp;
    float max_ = min_temp;
    double sum_ = 0;
    std::size_t count_ = 0;
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


auto make_line_buffer(std::size_t capacity = max_line_length) -> std::string
{
    return std::string(capacity, ' ');
}


auto get_temperature(std::string_view value) -> float
{
    float result;
    std::from_chars(value.data(), value.data() + value.size(), result);
    return result;
}


auto parse_data(std::string_view line) -> std::pair<std::string_view, float>
{
    auto sep = line.find(data_sep);
    auto city = line.substr(0, sep);
    auto temp = get_temperature(line.substr(sep + 1));
    return {city, temp};
}


auto dump_results(const StatsMap& cities) -> void
{
    auto sorted = std::map{cities.begin(), cities.end()};

    auto out = std::ostream_iterator<char>(std::cout);
    std::cout << "{";

    auto it = std::begin(sorted);
    std::format_to(out, "{}={}", it->first, it->second);
    std::for_each(std::next(it), std::end(sorted), [&out](const auto& elem) {
        std::format_to(out, ", {}={}", elem.first, elem.second);
    });

    std::cout << "}\n";
}


auto process_file(const std::filesystem::path& data_file) -> void
{
    auto cities = make_map();

    auto input = std::ifstream{data_file};
    for (auto line = make_line_buffer(); std::getline(input, line); ) {
        auto [city, temp] = parse_data(line);
        auto it = cities.find(city);
        if (it == cities.end()) {
            it = cities.emplace(city, Stats{}).first;
        }
        it->second.update(temp);
    }

    dump_results(cities);
}


auto main(int argc, char** argv) -> int
{
    if (argc > 2) {
        std::cerr << "usage: baseline [ <input_file> ]" << '\n';
        return EXIT_FAILURE;
    }

    auto data_file = std::filesystem::path{argc == 2 ? argv[1] : "measurements.txt"};
    process_file(data_file);

    return EXIT_SUCCESS;
}
