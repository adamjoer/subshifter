#pragma once

#include <vector>
#include <filesystem>

class SubtitleShifter {
public:
    SubtitleShifter() = default;

    ~SubtitleShifter() = default;

    bool parseArguments(int argc, char *argv[]);

    void shift();

private:
    int mMillisecondsOffset = 0;

    std::vector<std::filesystem::path> mPaths;

    bool mAreArgumentsValid = true;

    bool mDoModify = false;

    bool mIsDestinationPathSpecified = false;

    bool mDoRecurse = false;

    bool mIgnoreInvalidFiles = false;

    std::filesystem::path mDestinationPath;

    static void printUsage(char *programName);

    [[nodiscard]] bool isFileValid(const std::filesystem::path &path) const;
};
