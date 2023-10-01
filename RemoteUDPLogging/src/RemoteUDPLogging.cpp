#include "RemoteUDPLogging.h"

RemoteLogging RemoteLog;

void RemoteLogging::begin(IPAddress addr, uint16_t port)
{
    udp.connect(addr, port);
    xTaskCreate(task, "udp_log", 4096, this, 0, NULL);
}

void RemoteLogging::log(esp_log_level_t level, const char* tag, const char* format, ...) {
    if (level > log_level)
        return;

    va_list args;
    va_start(args, format);

    size_t size_string = snprintf(NULL, 0, format, args) + 4;
    char* string = (char*)malloc(size_string);
    vsnprintf(string, size_string, format, args);

    String level_str[] = { "INVALID", "ERROR", "WARN", "INFO", "DEBUG", "VERBOSE" };

    size_t size_log_string = snprintf(NULL, 0, "[%6u] [%s] [%s]: %s\n", millis(), tag, level_str[level].c_str(), string) + 4;
    char* log_string = (char*)malloc(size_log_string);
    snprintf(log_string, size_log_string, "[%6u] [%s] [%s]: %s\n", millis(), tag, level_str[level].c_str(), string);

    log_printf(log_string);

    if (!buffer.isFull())
        buffer.push(log_string);
    else
        free(log_string);
    //esp_log_write(level, tag, (String(format) + "\n").c_str(), args);

    va_end(args);
    free(string);
}

void RemoteLogging::task(void* param)
{
    RemoteLogging* this_ = (RemoteLogging*)param;
    while (true) {
        this_->flush_buffer();
        delay(50);
    }
}

void RemoteLogging::flush_buffer()
{
    if (!udp.connected())
        return;

    while (!buffer.isEmpty()) {
        char* string = buffer.shift();
        udp.print(string);
        free(string);
    }
}
