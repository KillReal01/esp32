#ifndef LOGCAPTURE_H
#define LOGCAPTURE_H

#include <string>

class LogCapture {
public:
    static LogCapture& instance();

    void init();
    std::string getSnapshot() const;

private:
    LogCapture() = default;
};

#endif // LOGCAPTURE_H
