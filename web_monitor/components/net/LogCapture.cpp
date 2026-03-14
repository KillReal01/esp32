#include "LogCapture.h"

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

#include <cstdarg>
#include <cstdio>
#include <vector>

namespace {
constexpr size_t kLogBufferSize = 8192;

char s_logBuffer[kLogBufferSize] = {};
size_t s_logHead = 0;
size_t s_logSize = 0;
portMUX_TYPE s_logMux = portMUX_INITIALIZER_UNLOCKED;
vprintf_like_t s_prevVprintf = nullptr;
bool s_logInitialized = false;

void logAppend(const char *data, size_t len)
{
    if (!data || len == 0) return;
    portENTER_CRITICAL(&s_logMux);
    for (size_t i = 0; i < len; ++i) {
        s_logBuffer[s_logHead] = data[i];
        s_logHead = (s_logHead + 1) % kLogBufferSize;
        if (s_logSize < kLogBufferSize) {
            ++s_logSize;
        }
    }
    portEXIT_CRITICAL(&s_logMux);
}

int logVprintf(const char *fmt, va_list args)
{
    char stackBuf[256];
    va_list argsCopy;
    va_copy(argsCopy, args);
    const int len = vsnprintf(stackBuf, sizeof(stackBuf), fmt, argsCopy);
    va_end(argsCopy);

    if (len > 0) {
        if (static_cast<size_t>(len) < sizeof(stackBuf)) {
            logAppend(stackBuf, static_cast<size_t>(len));
        } else {
            std::vector<char> heapBuf(static_cast<size_t>(len) + 1);
            va_list argsCopy2;
            va_copy(argsCopy2, args);
            vsnprintf(heapBuf.data(), heapBuf.size(), fmt, argsCopy2);
            va_end(argsCopy2);
            logAppend(heapBuf.data(), static_cast<size_t>(len));
        }
    }

    if (s_prevVprintf) {
        va_list argsCopy3;
        va_copy(argsCopy3, args);
        const int ret = s_prevVprintf(fmt, argsCopy3);
        va_end(argsCopy3);
        return ret;
    }

    return len;
}

std::string getLogSnapshotInternal()
{
    portENTER_CRITICAL(&s_logMux);
    const size_t size = s_logSize;
    const size_t head = s_logHead;
    portEXIT_CRITICAL(&s_logMux);

    std::string out(size, '\0');
    portENTER_CRITICAL(&s_logMux);
    for (size_t i = 0; i < size; ++i) {
        const size_t index = (head + kLogBufferSize - size + i) % kLogBufferSize;
        out[i] = s_logBuffer[index];
    }
    portEXIT_CRITICAL(&s_logMux);
    return out;
}
} // namespace

LogCapture& LogCapture::instance()
{
    static LogCapture inst;
    return inst;
}

void LogCapture::init()
{
    if (s_logInitialized)
        return;
    s_prevVprintf = esp_log_set_vprintf(logVprintf);
    s_logInitialized = true;
}

std::string LogCapture::getSnapshot() const
{
    return getLogSnapshotInternal();
}
