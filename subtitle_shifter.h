#pragma once

#include <string>
#include <vector>
#include <filesystem>

enum class ParseStatus {
    Continue,
    Exit,
    Error,
};

class SubtitleShifter {
public:
    SubtitleShifter() = default;

    explicit SubtitleShifter(int millisecondsOffset) : mMillisecondsOffset(millisecondsOffset) {
    }

    ~SubtitleShifter() = default;

    ParseStatus parseArguments(int argc, const char *const argv[]);

    void addFilePath(const std::filesystem::path &path);

    void shift();

    void setMillisecondsOffset(int millisecondsOffset) { mMillisecondsOffset = millisecondsOffset; }

    void setDoModify(bool doModify) { mDoRecurse = doModify; }

    void setDoRecurse(bool doRecurse) { mDoRecurse = doRecurse; }

    void setIgnoreInvalidFiles(bool ignoreInvalidFiles) { mIgnoreInvalidFiles = ignoreInvalidFiles; }

    void setDestinationPath(const std::filesystem::path &destinationPath) { mDestinationPath = destinationPath; }

private:
    std::string mExecutableName;

    std::vector<std::filesystem::path> mPaths;

    bool mDoModify = false;

    bool mDoRecurse = false;

    bool mIgnoreInvalidFiles = false;

    int mMillisecondsOffset = 0;

    std::filesystem::path mDestinationPath = ".";

    [[nodiscard]] bool isFileValid(const std::filesystem::path &path) const;
};
