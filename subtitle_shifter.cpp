#include "subtitle_shifter.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <regex>
#include <fstream>

#include "time_stamp.h"

namespace fs = std::filesystem;
using std::cout, std::cerr, std::stoi, std::invalid_argument, std::out_of_range, std::regex, std::ifstream,
        std::ofstream, std::string, std::getline, std::smatch;

SubtitleShifter::SubtitleShifter(int numberOfFiles) : mPaths() {
    mPaths.reserve(std::max(numberOfFiles, 0));
}

bool SubtitleShifter::parseArguments(int argc, char **argv) {
    if (argc < 3) {
        cout << "Usage: " << argv[0] << " [-m] <offset-ms> <filepath>...\n";
        return mAreArgumentsValid = false;
    }

    int i;
    for (i = 1; i < argc; ++i) {
        if (argv[i][0] != '-')
            break;

        bool doBreak = false;
        for (int j = 1, len = (int) strlen(argv[i]); j < len; ++j) {
            auto flag = argv[i][j];
            if (isdigit(flag)) {
                doBreak = true;
                break;
            }

            // TODO: Implement support for destination file path with "-d" flag
            if (flag == 'm') {
                mDoModify = true;

            } else {
                cerr << "Unknown switch '" << flag << "'\n";
                return mAreArgumentsValid = false;
            }
        }

        if (doBreak)
            break;
    }

    try {
        mMillisecondsOffset = stoi(argv[i]);

    } catch (invalid_argument &exception) {
        cerr << "Invalid number \"" << argv[i] << "\"\n";
        return mAreArgumentsValid = false;

    } catch (out_of_range &exception) {
        cerr << "Number is out of range: " << argv[i] << '\n';
        return mAreArgumentsValid = false;
    }

    for (++i; i < argc; ++i) {

        auto path = mPaths.emplace_back(argv[i]);
        if (!fs::is_regular_file(path)) {
            cerr << "File " << path << " does not exist\n";
            return mAreArgumentsValid = false;

        } else if (path.extension() != ".srt") {
            cerr << "File type " << path.extension() << " is not supported.\n"
                                                        "Supported file types are: .srt\n";
            return mAreArgumentsValid = false;

        } else {
            auto status = fs::status(path);

            if ((status.permissions() & fs::perms::group_read) == fs::perms::none) {
                cerr << "There are insufficient permissions to read file " << path << '\n';
                return mAreArgumentsValid = false;
            }

            if (mDoModify && (status.permissions() & fs::perms::group_write) == fs::perms::none) {
                cerr << "There are insufficient permissions to modify file " << path << '\n';
                return mAreArgumentsValid = false;
            }
        }
    }

    if (mPaths.empty()) {
        cerr << "No files provided\n";
        return mAreArgumentsValid = false;
    }

    return mAreArgumentsValid = true;
}

void SubtitleShifter::shift() {
    if (!mAreArgumentsValid || mPaths.empty())
        return;

    // https://regex101.com/r/Qus4qM/1
    const regex srtTimeStampRegex(
            R"(((?:00|\d+)):([0-5]\d):([0-5]\d),(\d{3})\W*-->\W*((?:00|\d+)):([0-5]\d):([0-5]\d),(\d{3}))");

    const TimeStamp offset(mMillisecondsOffset);

    for (const auto &path: mPaths) {

        ifstream inStream(path);
        if (!inStream.is_open()) {
            cerr << "Cannot open input file " << path << "\n";
            return;
        }

        auto outputPath = path.stem().string() + "_shifted" + path.extension().string();
        ofstream outStream(outputPath);
        if (!outStream.is_open()) {
            cerr << "Cannot open output file \"" << outputPath << "\"\n";
            return;
        }

        string line;
        int lineNumber = 1;
        while (getline(inStream, line)) {
            if (line.length() > 0 && line[line.length() - 1] == '\r')
                line.erase(line.end() - 1);

            smatch timeStampMatches;
            if (regex_match(line, timeStampMatches, srtTimeStampRegex)) {
                TimeStamp from(stoi(timeStampMatches[1]), stoi(timeStampMatches[2]),
                               stoi(timeStampMatches[3]), stoi(timeStampMatches[4]));
                TimeStamp to(stoi(timeStampMatches[5]), stoi(timeStampMatches[6]),
                             stoi(timeStampMatches[7]), stoi(timeStampMatches[8]));

                from += offset;
                to += offset;

                if (from.isNegative() || to.isNegative())
                    cout << "Warning: Shifting has produced negative timestamp in file \"" << outputPath << "\", line #"
                         << lineNumber << '\n';

                outStream << from << " --> " << to;

            } else if (line.length() > 0) {
                outStream << line;
            }

            outStream << '\n';
            ++lineNumber;
        }

        if (mDoModify) {
            inStream.close();
            outStream.close();

            try {
                fs::rename(outputPath, path);
                fs::remove(outputPath);

            } catch (fs::filesystem_error &error) {
                cerr << error.what() << '\n';
                return;
            }
        }
    }
}
