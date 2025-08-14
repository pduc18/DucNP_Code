// PHẦN GIẢI MÃ DỮ LIỆU NHẬN VỀ ("#distance,angle\r\n") VÀ PHÂN TÍCH THÀNH KHOẢNG CÁCH THEO 2 TRỤC

// buffer DMA
extern uint8_t uart_dma_buffer[DMA_BUFFER_SIZE];

// Lưu vị trí cuối cùng được đọc
static uint16_t last_index = 0;

// Bộ tạm lưu dữ liệu trước khi parse
static char temp_line_buffer[TEMP_LINE_BUFFER_SIZE];

// current_write: chỉ số ghi hiện tại của DMA
void UART_ProcessDMAData(uint16_t current_write_index) {
    static uint16_t line_len = 0; // chiều dài dữ liệu trong buffer
    // lưu dữ liệu đọc từ DMA (last index -> current index) và lưu vào buffer
    while (last_index != current_write_index) {
        char c = (char)uart_dma_buffer[last_index];
        // luôn update last index
        last_index = (last_index + 1) % DMA_BUFFER_SIZE;

        if (line_len < TEMP_LINE_BUFFER_SIZE - 1) {
            temp_line_buffer[line_len++] = c;
        } else {
            // overflow
            line_len = 0;
            continue;
        }

        if (c == '\n') {
            // khi gặp ký tự xuống dòng thì bắt đầu xử lý
            temp_line_buffer[line_len - 1] = '\0';
            
            float temp_distance = 0.0f, temp_angle = 0.0f;
            if (sscanf(temp_line_buffer, "#%f,%f", &temp_distance, &temp_angle) == 2) {
                if (temp_distance >= 0.0f && temp_distance <= 10.0f && 
                    temp_angle >= -180.0f && temp_angle <= 180.0f) {
                    angle = (temp_angle + 43) * M_PI / 180.0f;
                    Dtf_value = temp_distance * cosf(angle) * 1000.0f;
                    Dmis_value = temp_distance * sinf(angle) * 1000.0f;
                }
            }
            line_len = 0;
        }
    }
}


float getDtf() {
    return Dtf_value;
}
float getDmis() {
    return fabs(Dmis_value - dy); // hiệu chỉnh theo thiết kế thực tế
}

// trong main.c
void HAL_UART_IdleCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        // tính vị trí hiện tại
        uint16_t dma_write_index = DMA_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart1_rx);
        UART_ProcessDMAData(dma_write_index);
        Dtf = getDtf();
        Dmis = getDmis();
        target_position = LookupTargetPosition(Dtf, Dmis);

        last_data_update = HAL_GetTick();
        data_valid = true;
    }
}
