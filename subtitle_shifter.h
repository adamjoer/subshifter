#pragma once

#include <vector>
#include <filesystem>
#include <ostream>

class SubtitleShifter {

private:
    int mMillisecondsOffset = 0;

    std::vector<std::filesystem::path> mPaths;

    bool mAreArgumentsValid = true;

    bool mDoModify = false;

    bool mIsDestinationPathSpecified = false;

    std::filesystem::path mDestinationPath;

    static void printUsage(char *programName);

public:
    explicit SubtitleShifter(int numberOfFiles);

    bool parseArguments(int argc, char *argv[]);

    void shift();
};
