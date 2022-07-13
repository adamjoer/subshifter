#pragma once

#include <vector>
#include <filesystem>

class SubtitleShifter {
public:
    SubtitleShifter() = default;

    ~SubtitleShifter() = default;

    bool parseArguments(int argc, char *const argv[]);

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
