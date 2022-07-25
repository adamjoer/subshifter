#pragma once

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

    ~SubtitleShifter() = default;

    ParseStatus parseArguments(int argc, char *argv[]);

    void shift();

private:
    std::string mExecutableName;

    int mMillisecondsOffset = 0;

    std::vector<std::filesystem::path> mPaths;

    bool mAreArgumentsValid = false;

    bool mDoModify = false;

    bool mIsDestinationPathSpecified = false;

    bool mDoRecurse = false;

    bool mIgnoreInvalidFiles = false;

    std::filesystem::path mDestinationPath;

    void printUsage();

    [[nodiscard]] bool isFileValid(const std::filesystem::path &path) const;
};
