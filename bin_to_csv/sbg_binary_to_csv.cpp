// sbgEComのバイナリログをデコードし、対応するメッセージ種別ごとにCSVを出力する。
// ビルド方法: g++ -std=c++17 -O2 sbg_binary_to_csv.cpp -o sbg_binary_to_csv
// 実行方法:   ./sbg_binary_to_csv input.bin output_prefix

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
    {15, "gps1_heading"}, {18, "gps2_heading"},
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
    {"gps1_heading", {"device_timestamp_us", "status", "heading_status", "baseline_valid",
                        "gps_time_of_week_ms", "true_heading_deg", "true_heading_accuracy_deg",
                        "pitch_deg", "pitch_accuracy_deg", "baseline_m", "num_sv_tracked", "num_sv_used"}},
    {"gps2_heading", {"device_timestamp_us", "status", "heading_status", "baseline_valid",
                        "gps_time_of_week_ms", "true_heading_deg", "true_heading_accuracy_deg",
                        "pitch_deg", "pitch_accuracy_deg", "baseline_m", "num_sv_tracked", "num_sv_used"}},
    {"gps1_position", {"device_timestamp_us", "status_type", "gps_time_of_week_ms",
                        "position_status", "position_type", "position_type_name",
                        "latitude_deg", "longitude_deg", "altitude_msl_m", "undulation_m",
                        "latitude_accuracy_m", "longitude_accuracy_m", "altitude_accuracy_m",
                        "svnum", "base_station_id", "differential_age_0_01s",
                        "num_sv_tracked", "status_ext", "gps_l1_used", "gps_l2_used", "gps_l5_used",
                        "glonass_l1_used", "glonass_l2_used", "glonass_l3_used", "galileo_e1_used",
                        "galileo_e5a_used", "galileo_e5b_used", "galileo_e5alt_used", "galileo_e6_used",
                        "beidou_b1_used", "beidou_b2_used", "beidou_b3_used", "qzss_l1_used",
                        "qzss_l2_used", "qzss_l5_used", "qzss_l6_used"}},
    {"gps2_position", {"device_timestamp_us", "status_type", "gps_time_of_week_ms",
                        "position_status", "position_type", "position_type_name",
                        "latitude_deg", "longitude_deg", "altitude_msl_m", "undulation_m",
                        "latitude_accuracy_m", "longitude_accuracy_m", "altitude_accuracy_m",
                        "svnum", "base_station_id", "differential_age_0_01s",
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
    } else if (id == 15 || id == 18) {
        // HDTペイロードの配置は、仕様で定められた各フィールドのオフセットに従う。
        v.push_back(read_u32(b, p));
        const std::uint16_t status = read_u16(b, p + 4);
        v.push_back(status);
        v.push_back(status & 0x3F);
        v.push_back((status >> 6) & 1);
        v.push_back(read_u32(b, p + 6));
        for (std::size_t off = 10; off <= 26; off += 4) v.push_back(read_f32(b, p + off));
        v.push_back(b[p + 30]);
        v.push_back(b[p + 31]);
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

} // 無名名前空間

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
        // 固定長ログは、仕様で定められたペイロード長と照合する。
        // GPS1_SAT (50) と GPS2_SAT (51) は衛星・信号のグループを繰り返し含むため、
        // ペイロード長はフレーム内の個数に応じて変化する。
        std::size_t expected_size = 0;
        switch (msg_id) {
        case 3: expected_size = 58; break;
        case 7: expected_size = 44; break;
        case 8: expected_size = 72; break;
        case 13: case 16: expected_size = 44; break;
        case 14: case 17: expected_size = 62; break;
        case 15: case 18: expected_size = 32; break;
        // ここでの0は「ペイロード長が0」という意味ではなく、可変長を表す値。
        // これらのIDは後で内部の個数情報を使って解析するため、
        // 固定のペイロード長とは照合しない。
        case 50: case 51: expected_size = 0; break;
        }
        if (msg_id == 50 || msg_id == 51) {
            // ペイロード内の配置（オフセットは`payload`からの相対位置）:
            //   0..3: タイムスタンプ、4..7: 予約領域、8: 衛星数。
            // 9バイトの固定部分の後に、衛星ごとのSatDataグループが続く。
            // 各SatDataは7バイトで始まり、その後に`nrSignals`個の
            // 3バイトSignalDataグループが続く。
            // 固定部分はタイムスタンプ(4) + 予約領域(4) + 衛星数(1)の計9バイト。
            // これより短いと衛星数も安全に読み出せないため、配列を参照する前に
            // 不正なフレームとして処理を中止する。
            if (payload_size < 9) {
                ++length_errors;
                pos += frame_size;
                continue;
            }
            const std::uint32_t stamp = read_u32(bytes, payload); // ペイロード内オフセット0: uint32型のtimeStamp。
            const std::uint8_t satellite_count = bytes[payload + 8]; // オフセット8: uint8型のnrSatellites。
            const std::size_t payload_end = payload + payload_size;
            std::size_t cursor = payload + 9; // 4+4+1バイトの固定部分の直後が最初のSatData。
            // 可変長フレーム全体の検証が終わるまでは行を一時保存する。
            // 後続の衛星データが不正でも、CSVに一部の行だけが残らないようにする。
            std::vector<std::vector<double>> frame_rows;
            bool valid = satellite_count <= 64; // sbgEComの仕様では、1メッセージあたりの衛星数は0～64。
            auto& output = outputs.at(known->second);
            for (std::uint8_t sat = 0; sat < satellite_count && valid; ++sat) {
                // SatDataは7バイトで、その後にSignalDataの繰り返し部分が続く:
                // satelliteId uint8型 @+0、elevation int8型 @+1、azimuth uint16型 @+2、
                // satFlags uint16型 @+4、nrSignals uint8型 @+6。
                if (cursor + 7 > payload_end) { valid = false; break; }
                const std::uint8_t satellite_id = bytes[cursor];
                const std::int8_t elevation = static_cast<std::int8_t>(bytes[cursor + 1]);
                const std::uint16_t azimuth = read_u16(bytes, cursor + 2); // SatDataの先頭から+2バイト。
                const std::uint16_t sat_flags = read_u16(bytes, cursor + 4);
                const std::uint8_t signal_count = bytes[cursor + 6];
                cursor += 7; // SatDataのヘッダーを読み終え、最初のSignalDataの位置へ進める。
                // sbgEComの仕様では、衛星ごとのnrSignalsは0～8。
                // これを超える値は不正であり、繰り返し部分の読み出しが
                // ペイロード範囲を越えるおそれがあるため、このフレームの解析を中止する。
                if (signal_count > 8) { valid = false; break; }
                for (std::uint8_t sig = 0; sig < signal_count; ++sig) {
                    // SignalDataは3バイト:
                    // signalId uint8型 @+0、sigFlags uint8型 @+1、SNR uint8型 @+2 (dB)。
                    if (cursor + 3 > payload_end) { valid = false; break; }
                    const std::uint8_t signal_id = bytes[cursor];
                    const std::uint8_t sig_flags = bytes[cursor + 1];
                    const std::uint8_t snr = bytes[cursor + 2]; // このSignalDataの先頭から+2バイト。
                    cursor += 3; // 次の信号、または次の衛星グループの位置へ進める。
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
                    frame_rows.push_back(row);
                }
            }
            if (!valid || cursor != payload_end) {
                ++length_errors;
                pos += frame_size;
                continue;
            }
            for (const auto& row : frame_rows) {
                write_row(output.stream, row);
                ++output.rows;
            }
            pos += frame_size;
            continue;
        }
        // 可変長の衛星ログは上で処理済み。
        // それ以外の対応メッセージは、ペイロード長が固定レイアウトと一致する必要がある。
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