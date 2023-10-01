#ifndef RemoteLogging_h
#define RemoteLogging_h

#include <CircularBuffer.h>
#include <AsyncUDP.h>
#include <WiFi.h>

class RemoteLogging {
public:
    esp_log_level_t log_level = ESP_LOG_DEBUG;
    AsyncUDP udp;
    void begin(IPAddress addr, uint16_t port);
    void log(esp_log_level_t level, const char* tag, const char* format, ...);

private:
    CircularBuffer<char*, 32> buffer;
    static void task(void* param);
    void flush_buffer();
};

extern RemoteLogging RemoteLog;

#endif