// PHẦN XỬ LÝ DỮ LIỆU LIDAR
uint8_t start_count = 0; // tìm 2 byte đầu tiên của packet (0xFA và 0xA0)
bool got_scan = false; // cờ báo tìm được phần bắt đầu hợp lệ

boost::array<uint8_t, 660> raw_bytes; // buffer chứa toàn bộ 30 packets x 22 bytes
uint8_t good_sets = 0; // đếm số packet hợp lệ
int index;

while (!shutting_down_ && !got_scan) {
    // Tìm điểm bắt dầu
    boost::asio::read(serial_, boost::asio::buffer(&raw_bytes[start_count], 1));
    if (start_count == 0) {
    if (raw_bytes[start_count] == 0xFA) {
        start_count = 1;
        std::cout << "Found start byte" << std::endl;
    }
    } else if (start_count == 1) {
    if (raw_bytes[start_count] == 0xA0) {
        start_count = 0;
        std::cout << "Found sync byte" << std::endl;
        got_scan = true;
        // khi đã tìm được điểm bắt đầu thì đọc 658 bytes còn lại
        boost::asio::read(serial_, boost::asio::buffer(&raw_bytes[2], 658));
        std::cout << "Read " << 658 << " bytes" << std::endl;

        // thiết lập thông số: góc tối thiểu/tối đa, bước góc, khoảng cách hợp lệ, cấp phát mảng chứa 120 điểm quét và cường độ
        scan->angle_min = 46.0 * M_PI / 180.0;
        scan->angle_max = -43.0 * M_PI / 180.0;
        scan->angle_increment = -(2.0 * M_PI / 360.0);
        scan->range_min = 0.15;
        scan->range_max = 6.0;
        scan->ranges.resize(120);
        scan->intensities.resize(120);

        // duyệt 30 packets, mỗi packet 22 bytes
        for (uint16_t i = 0; i < raw_bytes.size(); i += 22) {
        if (raw_bytes[i] == 0xFA && raw_bytes[i + 1] == (0xA0 + i / 22)) {
            good_sets++;

            // duyệt mỗi data
            for (uint16_t j = i + 4; j < i + 20; j += 4) {
            index = (4 * i) / 22 + (j - 4 - i) / 4;
            uint8_t byte0 = raw_bytes[j];
            uint8_t byte1 = raw_bytes[j + 1];
            uint8_t byte2 = raw_bytes[j + 2];
            uint16_t range = ((byte1 & 0x3F) << 8) + byte0;
            uint16_t intensity_width = byte2;
            
            // tính vị trí và cường độ tương ứng với index
            scan->ranges[index] = range / 1000.0;
            scan->intensities[index] = 100 * intensity_width;
            }
        }
        }
        if (scan->ranges.empty()) {
        std::cerr << "No valid data in scan->ranges" << std::endl;
        }
    }
    }
}

