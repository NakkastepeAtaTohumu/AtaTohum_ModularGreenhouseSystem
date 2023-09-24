#include "fGMSServer.h"

AsyncWebServer* fGMSServer::server;
AsyncEventSource* fGMSServer::events;
FtpServer* fGMSServer::ftpsrv;
int fGMSServer::bytes_sent = 0;
long fGMSServer::restartMS = 0;

uint8_t palette[768] = { 0,0,0, 85, 85, 85, 170, 170, 170, 255, 255,255 };

int GetPNGImage(uint8_t* buffer) {
    ESP_LOGI("fGUI", "Get image PNG");
    PNG& png = *new PNG();

    if (buffer == NULL) {
        ESP_LOGE("fGUI", "Image buffer malloc failed!");
        return 0;
    }

    int result = png.open(buffer, 8192);

    if (result != PNG_SUCCESS) {
        ESP_LOGE("fGUI", "PNG open failed!");
        return 0;
    }

    ESP_LOGI("fGUI", "Begin encode");
    result = png.encodeBegin(fGUI::sp->width(), fGUI::sp->height(), PNG_PIXEL_INDEXED, 2, palette, 3);

    if (result != PNG_SUCCESS) {
        ESP_LOGE("fGUI", "PNG encode begin failed!");
        return 0;
    }

    uint8_t line[256];
    //void* line_buffer = malloc(768);

    for (int y = 0; y < fGUI::sp->height(); y++) {
        for (int x = 0; x < fGUI::sp->width() - 3; x+=4) {
            uint8_t pixel;

            for (int i = 0; i < 4; i++) {
                uint16_t col_565 = fGUI::sp->readPixel(x + i, y);
                uint32_t col_888 = fGUI::sp->color16to24(col_565);

                uint8_t col_brightness = (((uint8_t*)&col_888)[0] + ((uint8_t*)&col_888)[1] + ((uint8_t*)&col_888)[2]) / 3;

                uint8_t col = col_brightness / 64;

                pixel = pixel << 2;
                pixel |= col;
            }

            line[x / 4] = pixel;
        }

        //ESP_LOGI("fGUI", "Add line #%d", y);
        png.addLine(line);
    }
    //free(line_buffer);

    int length = png.close();

    delete& png;
    return length;
}

void GetBinaryImage(uint8_t* buffer, size_t length, size_t offset) {
    ESP_LOGV("fGUI", "Get image binary, %d bytes, start at byte #%d", length, offset);

    int pixel_offset = offset * 4;
    int y = pixel_offset / fGUI::sp->width();
    int x = pixel_offset % fGUI::sp->width();

    ESP_LOGV("fGUI", "X, Y of first pixel: %d, %d", x, y);

    int count = 0;

    for (; y < fGUI::sp->height(); y++) {
        for (x = 0; x < fGUI::sp->width() - 3; x += 4) {
            if (count >= length) {
                ESP_LOGV("fGUI", "Filled buffer, wrote %d bytes.", count);
                return;
            }

            uint8_t pixel = 0;
            for (int i = 0; i < 4; i++) {
                uint16_t col_565 = fGUI::sp->readPixel(x + i, y);
                uint32_t col_888 = fGUI::sp->color16to24(col_565);

                uint8_t col_brightness = (((uint8_t*)&col_888)[0] + ((uint8_t*)&col_888)[1] + ((uint8_t*)&col_888)[2]) / 3;

                uint8_t col = col_brightness / 64;

                pixel = pixel << 2;
                pixel |= col;
            }
            buffer[count] = pixel;
            count++;

            //ESP_LOGI("fGUI", "Current x: %d-%d, y: %d", x, x + 4, y);
        }
    }

    ESP_LOGV("fGUI", "End of image, wrote %d bytes.", count);
}