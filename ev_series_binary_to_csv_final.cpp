// Decode an EV Series binary log and write supported records to CSV.
// Build:   g++ -std=c++17 -O2 decode_ev_series_to_csv.cpp -o decode_ev_series_to_csv
// Run:     ./decode_ev_series_to_csv input.bin output_prefix

#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct Record {
    std::string type;
    std::map<std::string, std::string> values;
};

struct Stats {
    std::uint64_t crc_errors = 0;
    std::uint64_t length_errors = 0;
    std::uint64_t unknown_commands = 0;
    std::uint64_t discarded_bytes = 0;
};

const std::map<std::string, std::vector<std::string>> columns_by_type = {
    {"euler", {"device_timestamp_ms", "yaw_deg", "pitch_deg", "roll_deg"}},
    {"quaternion", {"device_timestamp_ms", "w", "x", "y", "z"}},
    {"imu", {"device_timestamp_ms", "gx", "gy", "gz", "ax", "ay", "az",
             "temperature_raw", "temperature_c"}},
    {"gnss", {"velocity", "up_velocity", "latitude_raw", "latitude_deg",
              "longitude_raw", "longitude_deg", "altitude", "course", "data_exist",
              "fix_mode", "sv_type", "hvar", "vvar", "hvar_speed", "vver_speed",
              "hvar_scaled", "vvar_scaled", "hvar_speed_scaled", "vver_speed_scaled",
              "pdop_raw", "hdop_raw", "vdop_raw", "pdop", "hdop", "vdop", "svnum",
              "major_axis", "minor_axis", "orient_raw", "orient_deg", "year", "month",
              "day", "hour", "minute", "second", "subsec_raw", "subsec_sec",
              "device_timestamp_ms"}},
    {"fusion", {"device_timestamp_ms", "latitude_raw", "longitude_raw",
                "latitude_deg", "longitude_deg", "altitude", "course", "velocity",
                "confidence_raw", "reliability_raw", "confidence", "reliability"}}
};

std::uint16_t read_u16(const std::vector<std::uint8_t>& b, std::size_t p) {
    return static_cast<std::uint16_t>(b[p]) |
           (static_cast<std::uint16_t>(b[p + 1]) << 8);
}

std::uint32_t read_u32(const std::vector<std::uint8_t>& b, std::size_t p) {
    return static_cast<std::uint32_t>(b[p]) |
           (static_cast<std::uint32_t>(b[p + 1]) << 8) |
           (static_cast<std::uint32_t>(b[p + 2]) << 16) |
           (static_cast<std::uint32_t>(b[p + 3]) << 24);
}

std::int32_t read_i32(const std::vector<std::uint8_t>& b, std::size_t p) {
    const std::uint32_t u = read_u32(b, p);
    std::int32_t result;
    std::memcpy(&result, &u, sizeof(result));
    return result;
}

std::int16_t read_i16(const std::vector<std::uint8_t>& b, std::size_t p) {
    const std::uint16_t u = read_u16(b, p);
    std::int16_t result;
    std::memcpy(&result, &u, sizeof(result));
    return result;
}

float read_f32(const std::vector<std::uint8_t>& b, std::size_t p) {
    const std::uint32_t bits = read_u32(b, p);
    float result;
    static_assert(sizeof(result) == sizeof(bits),
                  "This program requires 32-bit float.");
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}

std::uint16_t crc16_ccitt_false(const std::vector<std::uint8_t>& bytes,
                                std::size_t begin,
                                std::size_t end) {
    std::uint16_t crc = 0xFFFF;

    for (std::size_t i = begin; i < end; ++i) {
        crc ^= static_cast<std::uint16_t>(bytes[i]) << 8;

        for (int bit = 0; bit < 8; ++bit) {
            if (crc & 0x8000) {
                crc = static_cast<std::uint16_t>((crc << 1) ^ 0x1021);
            } else {
                crc = static_cast<std::uint16_t>(crc << 1);
            }
        }
    }

    return crc;
}

std::string number(double value) {
    std::ostringstream out;
    out << std::setprecision(10) << value;
    return out.str();
}

void put(Record& record, const std::string& key, double value) {
    record.values[key] = number(value);
}

bool decode_frame(const std::vector<std::uint8_t>& bytes,
                  std::size_t position,
                  std::uint8_t command,
                  Record& record) {
    switch (command) {
    case 0x10: // Euler angles
        record.type = "euler";
        put(record, "device_timestamp_ms", read_u32(bytes, position + 4));
        put(record, "yaw_deg", read_f32(bytes, position + 8));
        put(record, "pitch_deg", read_f32(bytes, position + 12));
        put(record, "roll_deg", read_f32(bytes, position + 16));
        return true;

    case 0x11: // Quaternion
        record.type = "quaternion";
        put(record, "device_timestamp_ms", read_u32(bytes, position + 4));
        put(record, "w", read_f32(bytes, position + 8));
        put(record, "x", read_f32(bytes, position + 12));
        put(record, "y", read_f32(bytes, position + 16));
        put(record, "z", read_f32(bytes, position + 20));
        return true;

    case 0x12: { // IMU
        record.type = "imu";
        put(record, "device_timestamp_ms", read_u32(bytes, position + 4));
        put(record, "gx", read_f32(bytes, position + 8));
        put(record, "gy", read_f32(bytes, position + 12));
        put(record, "gz", read_f32(bytes, position + 16));
        put(record, "ax", read_f32(bytes, position + 20));
        put(record, "ay", read_f32(bytes, position + 24));
        put(record, "az", read_f32(bytes, position + 28));

        const auto temperature = read_i16(bytes, position + 32);
        put(record, "temperature_raw", temperature);
        put(record, "temperature_c", temperature * 0.01);
        return true;
    }

    case 0x13: { // GNSS
        record.type = "gnss";
        put(record, "velocity", read_f32(bytes, position + 4));
        put(record, "up_velocity", read_f32(bytes, position + 8));

        const auto latitude = read_i32(bytes, position + 12);
        const auto longitude = read_i32(bytes, position + 16);
        put(record, "latitude_raw", latitude);
        put(record, "latitude_deg", latitude * 1e-7);
        put(record, "longitude_raw", longitude);
        put(record, "longitude_deg", longitude * 1e-7);

        put(record, "altitude", read_f32(bytes, position + 20));
        put(record, "course", read_f32(bytes, position + 24));
        put(record, "data_exist", bytes[position + 28]);
        put(record, "fix_mode", bytes[position + 29]);
        put(record, "sv_type", read_u16(bytes, position + 30));

        put(record, "hvar", read_f32(bytes, position + 32));
        put(record, "vvar", read_f32(bytes, position + 36));
        put(record, "hvar_speed", read_f32(bytes, position + 40));
        put(record, "vver_speed", read_f32(bytes, position + 44));

        put(record, "hvar_scaled", read_f32(bytes, position + 32) * 0.1);
        put(record, "vvar_scaled", read_f32(bytes, position + 36) * 0.1);
        put(record, "hvar_speed_scaled", read_f32(bytes, position + 40) * 0.1);
        put(record, "vver_speed_scaled", read_f32(bytes, position + 44) * 0.1);

        const auto pdop = bytes[position + 48];
        const auto hdop = bytes[position + 49];
        const auto vdop = bytes[position + 50];
        put(record, "pdop_raw", pdop);
        put(record, "hdop_raw", hdop);
        put(record, "vdop_raw", vdop);
        put(record, "pdop", pdop * 0.1);
        put(record, "hdop", hdop * 0.1);
        put(record, "vdop", vdop * 0.1);
        put(record, "svnum", bytes[position + 51]);

        put(record, "major_axis", read_f32(bytes, position + 52));
        put(record, "minor_axis", read_f32(bytes, position + 56));

        const auto orient = read_i16(bytes, position + 60);
        put(record, "orient_raw", orient);
        put(record, "orient_deg", orient * 0.01);

        put(record, "year", read_u16(bytes, position + 62));
        put(record, "month", bytes[position + 64]);
        put(record, "day", bytes[position + 65]);
        put(record, "hour", bytes[position + 66]);
        put(record, "minute", bytes[position + 67]);
        put(record, "second", bytes[position + 68]);
        put(record, "subsec_raw", bytes[position + 69]);
        put(record, "subsec_sec", bytes[position + 69] * 0.1);
        put(record, "device_timestamp_ms", read_u32(bytes, position + 70));
        return true;
    }

    case 0x14: { // Fused position
        record.type = "fusion";
        put(record, "device_timestamp_ms", read_u32(bytes, position + 4));

        const auto latitude = read_i32(bytes, position + 8);
        const auto longitude = read_i32(bytes, position + 12);
        put(record, "latitude_raw", latitude);
        put(record, "longitude_raw", longitude);
        put(record, "latitude_deg", latitude * 1e-7);
        put(record, "longitude_deg", longitude * 1e-7);

        put(record, "altitude", read_f32(bytes, position + 16));
        put(record, "course", read_f32(bytes, position + 20));
        put(record, "velocity", read_f32(bytes, position + 24));
        put(record, "confidence_raw", bytes[position + 28]);
        put(record, "reliability_raw", bytes[position + 29]);
        put(record, "confidence", bytes[position + 28] / 255.0);
        put(record, "reliability", bytes[position + 29] / 255.0);
        return true;
    }

    default:
        return false;
    }
}

void write_csv_header(std::ofstream& out,
                      const std::vector<std::string>& columns) {
    for (std::size_t i = 0; i < columns.size(); ++i) {
        if (i != 0) {
            out << ',';
        }
        out << columns[i];
    }
    out << '\n';
}

void write_csv_row(std::ofstream& out,
                   const Record& record,
                   const std::vector<std::string>& columns) {
    for (std::size_t i = 0; i < columns.size(); ++i) {
        if (i != 0) {
            out << ',';
        }

        const auto found = record.values.find(columns[i]);
        if (found != record.values.end()) {
            out << found->second;
        }
    }
    out << '\n';
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0]
                  << " input.bin output_prefix\n";
        return 1;
    }

    std::ifstream input(argv[1], std::ios::binary);
    if (!input) {
        std::cerr << "Cannot open input file: " << argv[1] << '\n';
        return 1;
    }

    std::string output_prefix = argv[2];
    if (output_prefix.size() >= 4 &&
        output_prefix.substr(output_prefix.size() - 4) == ".csv") {
        output_prefix.erase(output_prefix.size() - 4);
    }

    const std::map<std::string, std::string> suffix_by_type = {
        {"euler", "euler"},
        {"quaternion", "quaternion"},
        {"imu", "imu"},
        {"gnss", "gnss"},
        {"fusion", "fusion"}
    };

    std::map<std::string, std::ofstream> outputs;

    for (const auto& item : suffix_by_type) {
        const std::string path =
            output_prefix + "_" + item.second + ".csv";

        auto& out = outputs[item.first];
        out.open(path);

        if (!out) {
            std::cerr << "Cannot create output file: " << path << '\n';
            return 1;
        }

        write_csv_header(out, columns_by_type.at(item.first));
    }

    std::vector<std::uint8_t> bytes(
        (std::istreambuf_iterator<char>(input)), {});

    Stats stats;
    std::map<std::string, std::uint64_t> counts;
    std::size_t position = 0;

    while (position + 3 < bytes.size()) {
        if (bytes[position] != 0xAA || bytes[position + 1] != 0x55) {
            ++stats.discarded_bytes;
            ++position;
            continue;
        }

        const std::uint8_t payload_length = bytes[position + 2];
        if (payload_length < 3) {
            ++stats.length_errors;
            ++position;
            continue;
        }

        const std::size_t frame_size = 3 + payload_length;
        if (position + frame_size > bytes.size()) {
            break;
        }

        const auto expected_crc =
            read_u16(bytes, position + frame_size - 2);
        const auto actual_crc =
            crc16_ccitt_false(bytes, position, position + frame_size - 2);

        if (actual_crc != expected_crc) {
            ++stats.crc_errors;
            ++position;
            continue;
        }

        const std::uint8_t command = bytes[position + 3];

        const std::map<std::uint8_t, std::uint8_t> expected_lengths = {
            {0x10, 19},
            {0x11, 23},
            {0x12, 33},
            {0x13, 73},
            {0x14, 29}
        };

        const auto expected = expected_lengths.find(command);
        if (expected == expected_lengths.end()) {
            ++stats.unknown_commands;
            position += frame_size;
            continue;
        }

        if (payload_length != expected->second) {
            ++stats.length_errors;
            position += frame_size;
            continue;
        }

        Record record;
        if (decode_frame(bytes, position, command, record)) {
            ++counts[record.type];
            write_csv_row(
                outputs.at(record.type),
                record,
                columns_by_type.at(record.type));
        }

        position += frame_size;
    }

    std::cout << "Saved CSV files with prefix: "
              << output_prefix << '\n';

    std::uint64_t total = 0;
    for (const auto& item : counts) {
        std::cout << item.first << ": "
                  << item.second << " rows\n";
        total += item.second;
    }

    std::cout << "Total decoded rows: " << total << '\n'
              << "CRC errors: " << stats.crc_errors << '\n'
              << "Length errors: " << stats.length_errors << '\n'
              << "Unknown commands skipped: "
              << stats.unknown_commands << '\n'
              << "Bytes skipped while searching for frames: "
              << stats.discarded_bytes << '\n';

    return 0;
}