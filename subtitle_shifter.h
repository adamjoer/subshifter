#pragma once

#include <vector>
#include <filesystem>

class SubtitleShifter {

private:
    int mMillisecondsOffset = 0;
    std::vector<std::filesystem::path> mPaths;

    bool mAreArgumentsValid = true;

    bool mDoModify = false;

public:
    explicit SubtitleShifter(int numberOfFiles);

    bool parseArguments(int argc, char *argv[]);

    void shift();
};
