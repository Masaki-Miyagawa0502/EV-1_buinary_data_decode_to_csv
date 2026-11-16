// Decode sbgECom binary logs and write one CSV per supported message type.
// Build: g++ -std=c++17 -O2 sbg_binary_to_csv.cpp -o sbg_binary_to_csv
// Run:   ./sbg_binary_to_csv input.bin output_prefix

#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

struct CsvOutput {
    std::ofstream stream;
    std::vector<std::string> columns;
    std::uint64_t rows = 0;
};

const std::map<std::uint8_t, std::string> names = {
    {3, "imu"}, {7, "quaternion"}, {8, "navigation"},
    {13, "gps1_velocity"}, {14, "gps1_position"},
    {16, "gps2_velocity"}, {17, "gps2_position"},
    {50, "gps1_signals"}, {51, "gps2_signals"}
};

const std::map<std::string, std::vector<std::string>> columns = {
    {"imu", {"device_timestamp_us", "imu_status", "ax", "ay", "az",
             "gx", "gy", "gz", "temperature_c",
             "accel1_x_mps2", "accel1_y_mps2", "accel1_z_mps2",
             "gyro1_x_rad_s", "gyro1_y_rad_s", "gyro1_z_rad_s"}},
    {"quaternion", {"device_timestamp_us", "q0_w", "q1_x", "q2_y", "q3_z",
                    "roll_accuracy_rad", "pitch_accuracy_rad", "yaw_accuracy_rad",
                    "solution_status", "magnetic_declination_rad",
                    "magnetic_inclination_rad"}},
    {"navigation", {"device_timestamp_us", "velocity_n_mps", "velocity_e_mps",
                     "velocity_d_mps", "velocity_n_accuracy_mps",
                     "velocity_e_accuracy_mps", "velocity_d_accuracy_mps",
                     "latitude_deg", "longitude_deg", "altitude_msl_m",
                     "undulation_m", "latitude_accuracy_m", "longitude_accuracy_m",
                     "altitude_accuracy_m", "solution_status"}},
    {"gps1_velocity", {"device_timestamp_us", "status_type", "gps_time_of_week_ms",
                        "velocity_n_mps", "velocity_e_mps", "velocity_d_mps",
                        "velocity_n_accuracy_mps", "velocity_e_accuracy_mps",
                        "velocity_d_accuracy_mps", "course_deg", "course_accuracy_deg"}},
    {"gps2_velocity", {"device_timestamp_us", "status_type", "gps_time_of_week_ms",
                        "velocity_n_mps", "velocity_e_mps", "velocity_d_mps",
                        "velocity_n_accuracy_mps", "velocity_e_accuracy_mps",
                        "velocity_d_accuracy_mps", "course_deg", "course_accuracy_deg"}},
    {"gps1_position", {"device_timestamp_us", "status_type", "gps_time_of_week_ms",
                        "position_status", "position_type", "position_type_name",
                        "latitude_deg", "longitude_deg", "altitude_msl_m", "undulation_m",
                        "latitude_accuracy_m", "longitude_accuracy_m", "altitude_accuracy_m",
                        "num_sv_used", "base_station_id", "differential_age_0_01s",
                        "num_sv_tracked", "status_ext", "gps_l1_used", "gps_l2_used", "gps_l5_used",
                        "glonass_l1_used", "glonass_l2_used", "glonass_l3_used", "galileo_e1_used",
                        "galileo_e5a_used", "galileo_e5b_used", "galileo_e5alt_used", "galileo_e6_used",
                        "beidou_b1_used", "beidou_b2_used", "beidou_b3_used", "qzss_l1_used",
                        "qzss_l2_used", "qzss_l5_used", "qzss_l6_used"}},
    {"gps2_position", {"device_timestamp_us", "status_type", "gps_time_of_week_ms",
                        "position_status", "position_type", "position_type_name",
                        "latitude_deg", "longitude_deg", "altitude_msl_m", "undulation_m",
                        "latitude_accuracy_m", "longitude_accuracy_m", "altitude_accuracy_m",
                        "num_sv_used", "base_station_id", "differential_age_0_01s",
                        "num_sv_tracked", "status_ext", "gps_l1_used", "gps_l2_used", "gps_l5_used",
                        "glonass_l1_used", "glonass_l2_used", "glonass_l3_used", "galileo_e1_used",
                        "galileo_e5a_used", "galileo_e5b_used", "galileo_e5alt_used", "galileo_e6_used",
                        "beidou_b1_used", "beidou_b2_used", "beidou_b3_used", "qzss_l1_used",
                        "qzss_l2_used", "qzss_l5_used", "qzss_l6_used"}},
    {"gps1_signals", {"device_timestamp_us", "satellite_id", "elevation_deg", "azimuth_deg",
                       "sat_flags", "sat_tracking_status", "sat_health_status", "sat_elevation_status",
                       "constellation", "signal_count", "signal_id", "signal_flags",
                       "signal_tracking_status", "signal_health_status", "snr_db", "snr_valid"}},
    {"gps2_signals", {"device_timestamp_us", "satellite_id", "elevation_deg", "azimuth_deg",
                       "sat_flags", "sat_tracking_status", "sat_health_status", "sat_elevation_status",
                       "constellation", "signal_count", "signal_id", "signal_flags",
                       "signal_tracking_status", "signal_health_status", "snr_db", "snr_valid"}}
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

float read_f32(const std::vector<std::uint8_t>& b, std::size_t p) {
    const std::uint32_t bits = read_u32(b, p);
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

double read_f64(const std::vector<std::uint8_t>& b, std::size_t p) {
    const std::uint64_t bits = static_cast<std::uint64_t>(read_u32(b, p)) |
        (static_cast<std::uint64_t>(read_u32(b, p + 4)) << 32);
    double value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

std::uint16_t crc_sbg(const std::vector<std::uint8_t>& b,
                      std::size_t begin, std::size_t end) {
    std::uint16_t crc = 0;
    for (std::size_t p = begin; p < end; ++p) {
        crc ^= b[p];
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 1) ? static_cast<std::uint16_t>((crc >> 1) ^ 0x8408)
                            : static_cast<std::uint16_t>(crc >> 1);
        }
    }
    return crc;
}

void write_header(std::ofstream& out, const std::vector<std::string>& cols) {
    for (std::size_t i = 0; i < cols.size(); ++i) {
        if (i) out << ',';
        out << cols[i];
    }
    out << '\n';
}

void write_row(std::ofstream& out, const std::vector<double>& values) {
    out << std::setprecision(17);
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i) out << ',';
        if (std::isfinite(values[i])) out << values[i];
    }
    out << '\n';
}

const char* gps_position_type_name(std::uint32_t value) {
    switch (value) {
    case 0: return "SBG_ECOM_POS_NO_SOLUTION";
    case 1: return "SBG_ECOM_POS_UNKNOWN_TYPE";
    case 2: return "SBG_ECOM_POS_SINGLE";
    case 3: return "SBG_ECOM_POS_PSRDIFF";
    case 4: return "SBG_ECOM_POS_SBAS";
    case 5: return "SBG_ECOM_POS_OMNISTAR";
    case 6: return "SBG_ECOM_POS_RTK_FLOAT";
    case 7: return "SBG_ECOM_POS_RTK_INT";
    case 8: return "SBG_ECOM_POS_PPP_FLOAT";
    case 9: return "SBG_ECOM_POS_PPP_INT";
    case 10: return "SBG_ECOM_POS_FIXED";
    default: return "SBG_ECOM_POS_UNRECOGNIZED";
    }
}

void write_gps_position_row(std::ofstream& out,
                            const std::vector<double>& values) {
    out << std::setprecision(17);
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i) out << ',';
        if (i == 5) out << gps_position_type_name(static_cast<std::uint32_t>(values[4])) << ',';
        if (std::isfinite(values[i])) out << values[i];
    }
    out << '\n';
}

std::vector<double> decode(std::uint8_t id,
                           const std::vector<std::uint8_t>& b,
                           std::size_t p) {
    std::vector<double> v;
    if (id == 3) {
        v.push_back(read_u32(b, p));
        v.push_back(read_u16(b, p + 4));
        for (std::size_t off = 6; off < 58; off += 4) v.push_back(read_f32(b, p + off));
    } else if (id == 7) {
        v.push_back(read_u32(b, p));
        for (std::size_t off = 4; off < 32; off += 4) v.push_back(read_f32(b, p + off));
        v.push_back(read_u32(b, p + 32));
        v.push_back(read_f32(b, p + 36));
        v.push_back(read_f32(b, p + 40));
    } else if (id == 8) {
        v.push_back(read_u32(b, p));
        for (std::size_t off = 4; off < 28; off += 4) v.push_back(read_f32(b, p + off));
        v.push_back(read_f64(b, p + 28));
        v.push_back(read_f64(b, p + 36));
        v.push_back(read_f64(b, p + 44));
        for (std::size_t off = 52; off < 68; off += 4) v.push_back(read_f32(b, p + off));
        v.push_back(read_u32(b, p + 68));
    } else if (id == 13 || id == 16) {
        v.push_back(read_u32(b, p));
        v.push_back(read_u32(b, p + 4));
        v.push_back(read_u32(b, p + 8));
        for (std::size_t off = 12; off <= 40; off += 4) v.push_back(read_f32(b, p + off));
    } else if (id == 14 || id == 17) {
        v.push_back(read_u32(b, p));
        const std::uint32_t status_type = read_u32(b, p + 4);
        v.push_back(status_type);
        v.push_back(read_u32(b, p + 8));
        v.push_back(status_type & 0x3F);
        v.push_back((status_type >> 6) & 0x3F);
        v.push_back(read_f64(b, p + 12));
        v.push_back(read_f64(b, p + 20));
        v.push_back(read_f64(b, p + 28));
        for (std::size_t off = 36; off <= 48; off += 4) v.push_back(read_f32(b, p + off));
        v.push_back(b[p + 52]);
        v.push_back(read_u16(b, p + 53));
        v.push_back(read_u16(b, p + 55));
        v.push_back(b[p + 57]);
        v.push_back(read_u32(b, p + 58));
        for (unsigned bit = 12; bit <= 29; ++bit) v.push_back((status_type >> bit) & 1);
    }
    return v;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " input.bin output_prefix\n";
        return 1;
    }

    std::ifstream input(argv[1], std::ios::binary);
    if (!input) {
        std::cerr << "Cannot open input file: " << argv[1] << '\n';
        return 1;
    }
    std::string prefix = argv[2];
    if (prefix.size() >= 4 && prefix.substr(prefix.size() - 4) == ".csv")
        prefix.erase(prefix.size() - 4);

    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)), {});
    std::map<std::string, CsvOutput> outputs;
    for (const auto& item : names) {
        const std::string& name = item.second;
        auto& output = outputs[name];
        output.columns = columns.at(name);
        const std::string path = prefix + "_" + name + ".csv";
        output.stream.open(path);
        if (!output.stream) {
            std::cerr << "Cannot create output file: " << path << '\n';
            return 1;
        }
        write_header(output.stream, output.columns);
    }

    std::uint64_t crc_errors = 0, length_errors = 0, unknown = 0, discarded = 0;
    std::size_t pos = 0;
    while (pos + 9 <= bytes.size()) {
        if (bytes[pos] != 0xFF || bytes[pos + 1] != 0x5A) {
            ++discarded;
            ++pos;
            continue;
        }
        const std::uint8_t msg_id = bytes[pos + 2];
        const std::uint8_t msg_class = bytes[pos + 3];
        const std::uint16_t payload_size = read_u16(bytes, pos + 4);
        if (payload_size > 4086) {
            ++length_errors;
            ++pos;
            continue;
        }
        const std::size_t payload = pos + 6;
        const std::size_t crc_pos = payload + payload_size;
        const std::size_t frame_size = 6 + payload_size + 3;
        if (crc_pos + 3 > bytes.size()) {
            ++length_errors;
            ++pos;
            continue;
        }
        if (bytes[crc_pos + 2] != 0x33 ||
            crc_sbg(bytes, pos + 2, crc_pos) != read_u16(bytes, crc_pos)) {
            ++crc_errors;
            ++pos;
            continue;
        }
        const auto known = names.find(msg_id);
        if (msg_class != 0 || known == names.end()) {
            ++unknown;
            pos += frame_size;
            continue;
        }
        std::size_t expected_size = 0;
        switch (msg_id) {
        case 3: expected_size = 58; break;
        case 7: expected_size = 44; break;
        case 8: expected_size = 72; break;
        case 13: case 16: expected_size = 44; break;
        case 14: case 17: expected_size = 62; break;
        case 50: case 51: expected_size = 0; break; // Variable length.
        }
        if (msg_id == 50 || msg_id == 51) {
            if (payload_size < 9) {
                ++length_errors;
                pos += frame_size;
                continue;
            }
            const std::uint32_t stamp = read_u32(bytes, payload);
            const std::uint8_t satellite_count = bytes[payload + 8];
            const std::size_t payload_end = payload + payload_size;
            std::size_t cursor = payload + 9;
            bool valid = true;
            auto& output = outputs.at(known->second);
            for (std::uint8_t sat = 0; sat < satellite_count && valid; ++sat) {
                if (cursor + 7 > payload_end) { valid = false; break; }
                const std::uint8_t satellite_id = bytes[cursor];
                const std::int8_t elevation = static_cast<std::int8_t>(bytes[cursor + 1]);
                const std::uint16_t azimuth = read_u16(bytes, cursor + 2);
                const std::uint16_t sat_flags = read_u16(bytes, cursor + 4);
                const std::uint8_t signal_count = bytes[cursor + 6];
                cursor += 7;
                if (signal_count > 8) { valid = false; break; }
                for (std::uint8_t sig = 0; sig < signal_count; ++sig) {
                    if (cursor + 3 > payload_end) { valid = false; break; }
                    const std::uint8_t signal_id = bytes[cursor];
                    const std::uint8_t sig_flags = bytes[cursor + 1];
                    const std::uint8_t snr = bytes[cursor + 2];
                    cursor += 3;
                    const std::vector<double> row = {
                        static_cast<double>(stamp), static_cast<double>(satellite_id),
                        static_cast<double>(elevation), static_cast<double>(azimuth),
                        static_cast<double>(sat_flags), static_cast<double>(sat_flags & 7),
                        static_cast<double>((sat_flags >> 3) & 3),
                        static_cast<double>((sat_flags >> 5) & 3),
                        static_cast<double>((sat_flags >> 7) & 15),
                        static_cast<double>(signal_count), static_cast<double>(signal_id),
                        static_cast<double>(sig_flags), static_cast<double>(sig_flags & 7),
                        static_cast<double>((sig_flags >> 3) & 3), static_cast<double>(snr),
                        static_cast<double>((sig_flags >> 5) & 1)
                    };
                    write_row(output.stream, row);
                    ++output.rows;
                }
            }
            if (!valid || cursor != payload_end) ++length_errors;
            pos += frame_size;
            continue;
        }
        if (payload_size != expected_size) {
            ++length_errors;
            pos += frame_size;
            continue;
        }
        auto values = decode(msg_id, bytes, payload);
        bool finite = true;
        for (double value : values) finite = finite && std::isfinite(value);
        const bool is_gps_position = msg_id == 14 || msg_id == 17;
        const std::size_t expected_columns = columns.at(known->second).size();
        if (!finite || values.size() + (is_gps_position ? 1 : 0) != expected_columns) {
            ++length_errors;
            pos += frame_size;
            continue;
        }
        auto& output = outputs.at(known->second);
        if (is_gps_position) write_gps_position_row(output.stream, values);
        else write_row(output.stream, values);
        ++output.rows;
        pos += frame_size;
    }

    std::uint64_t total = 0;
    for (const auto& item : outputs) {
        std::cout << item.first << ": " << item.second.rows << " rows\n";
        total += item.second.rows;
    }
    std::cout << "Total decoded rows: " << total << '\n'
              << "CRC/frame errors: " << crc_errors << '\n'
              << "Length/value errors: " << length_errors << '\n'
              << "Unknown messages skipped: " << unknown << '\n'
              << "Bytes skipped while searching for frames: " << discarded << '\n';
    return 0;
}
