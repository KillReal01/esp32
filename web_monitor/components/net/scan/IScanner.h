#ifndef I_SCANNER_H
#define I_SCANNER_H

#include <string>

enum class ScanState : uint8_t { Idle, Scanning, Ready, Error };

class IScanner {
public:
    virtual ~IScanner() = default;
    virtual bool start(uint32_t durationSeconds) = 0;
    virtual void stop() = 0;
    virtual ScanState state() const = 0;
    virtual bool getResult(std::string& out) const = 0;
};

#endif // I_SCANNER_H
