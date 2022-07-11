#include "subtitle_shifter.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <regex>
#include <fstream>

#include "time_stamp.h"

namespace fs = std::filesystem;
using std::cout, std::cerr, std::stoi, std::invalid_argument, std::out_of_range, std::move, std::regex, std::ifstream,
        std::ofstream, std::string, std::getline, std::smatch;

bool SubtitleShifter::parseArguments(int argc, char *argv[]) {
    if (argc < 3) {
        printUsage(argv[0]);
        return mAreArgumentsValid = false;
    }

    int i;
    for (i = 1; i < argc; ++i) {
        if (argv[i][0] != '-' || isdigit(argv[i][1]))
            break;

        for (int j = 1, len = (int) strlen(argv[i]); j < len; ++j) {
            auto flag = argv[i][j];

            if (flag == 'm') {
                if (mIsDestinationPathSpecified) {
                    cerr << "Switches 'm' and 'd' are incongruous\n";
                    return mAreArgumentsValid = false;
                }

                mDoModify = true;

            } else if (flag == 'd') {
                if (mDoModify) {
                    cerr << "Switches 'm' and 'd' are incongruous\n";
                    return mAreArgumentsValid = false;
                }

                if (mIsDestinationPathSpecified)
                    continue;

                if (i == argc - 1 && j == len - 1) {
                    cerr << "No destination path specified after 'd' switch\n";
                    return mAreArgumentsValid = false;
                }

                fs::path destinationPath((j == len - 1) ? argv[++i] : argv[i] + j + 1);

                if (!fs::is_directory(destinationPath)) {
                    cerr << "Invalid directory " << destinationPath << '\n';
                    return mAreArgumentsValid = false;
                }

                mDestinationPath = destinationPath;
                mIsDestinationPathSpecified = true;

                break;

            } else if (flag == 'i') {
                mIgnoreInvalidFiles = true;

            } else if (flag == 'r') {
                mDoRecurse = true;

            } else {
                cerr << "Unknown switch '" << flag << "'\n";
                return mAreArgumentsValid = false;
            }
        }
    }

    if (i > argc - 2) {
        printUsage(argv[0]);
        return mAreArgumentsValid = false;
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

        fs::path path(argv[i]);
        if (path.filename() == "*")
            path.remove_filename();

        if (fs::is_regular_file(path)) {

            if (!isFileValid(path)) {
                if (mIgnoreInvalidFiles)
                    continue;
                return mAreArgumentsValid = false;
            }

            mPaths.push_back(move(path));

        } else if (fs::is_directory(path)) {

            if (mDoRecurse) {
                for (auto const &directoryEntry: fs::recursive_directory_iterator(path)) {
                    if (!directoryEntry.is_regular_file())
                        continue;

                    if (!isFileValid(directoryEntry.path())) {
                        if (mIgnoreInvalidFiles)
                            continue;
                        return mAreArgumentsValid = false;
                    }

                    mPaths.push_back(directoryEntry.path());
                }

            } else {
                for (auto const &directoryEntry: fs::directory_iterator(path)) {
                    if (!directoryEntry.is_regular_file())
                        continue;

                    if (!isFileValid(directoryEntry.path())) {
                        if (mIgnoreInvalidFiles)
                            continue;
                        return mAreArgumentsValid = false;
                    }

                    mPaths.push_back(directoryEntry.path());
                }
            }

        } else {
            cerr << "Invalid file/directory " << path << '\n';
            return mAreArgumentsValid = false;
        }
    }

    if (mPaths.empty()) {
        cerr << "No valid files provided\n";
        return mAreArgumentsValid = false;
    }

    return mAreArgumentsValid = true;
}

void SubtitleShifter::printUsage(char *programName) {
    cout << "Usage: " << programName << " [-m | -d <destination-path>] [-r] [-i] <offset-ms> <path>...\n";
}

void SubtitleShifter::shift() {
    if (!mAreArgumentsValid || mPaths.empty())
        return;

    // https://regex101.com/r/w2aGaG/1
    const regex srtTimeStampRegex(
            R"(^(0\d|[1-9]\d+):([0-5]\d):([0-5]\d),(\d{3}) --> (0\d|[1-9]\d+):([0-5]\d):([0-5]\d),(\d{3})$)");

    const TimeStamp offset(mMillisecondsOffset);

    for (const auto &path: mPaths) {

        ifstream inStream(path);
        if (!inStream.is_open()) {
            cerr << "Cannot open input file " << path << "\n";
            return;
        }

        fs::path outputPath(path.stem().string() + "_shifted" + path.extension().string());
        if (mIsDestinationPathSpecified)
            outputPath = mDestinationPath / outputPath;

        ofstream outStream(outputPath);
        if (!outStream.is_open()) {
            cerr << "Cannot open output file " << outputPath << "\n";
            return;
        }

        int lineNumber = 1;

        string line;
        while (getline(inStream, line)) {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            smatch timeStampMatches;
            if (regex_match(line, timeStampMatches, srtTimeStampRegex)) {
                TimeStamp from(stoi(timeStampMatches[1]), stoi(timeStampMatches[2]),
                               stoi(timeStampMatches[3]), stoi(timeStampMatches[4]));
                TimeStamp to(stoi(timeStampMatches[5]), stoi(timeStampMatches[6]),
                             stoi(timeStampMatches[7]), stoi(timeStampMatches[8]));

                from += offset;
                to += offset;

                if (from.isNegative() || to.isNegative()) {
                    cout << "Warning: Shifting has produced negative timestamp in file "
                         << (mDoModify ? path : outputPath) << ", line #" << lineNumber << '\n';
                }

                outStream << from << " --> " << to;

            } else if (!line.empty()) {
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

bool SubtitleShifter::isFileValid(const std::filesystem::path &path) const {
    if (path.extension() != ".srt") {
        if (!mIgnoreInvalidFiles) {
            cerr << "File type " << path.extension() << " (" << path << ") is not supported.\n"
                                                        "Supported file types are: .srt\n";
        }
        return false;
    }

    auto status = fs::status(path);

    if ((status.permissions() & fs::perms::group_read) == fs::perms::none) {
        if (!mIgnoreInvalidFiles)
            cerr << "There are insufficient permissions to read file " << path << '\n';
        return false;
    }

    if (mDoModify && (status.permissions() & fs::perms::group_write) == fs::perms::none) {
        if (!mIgnoreInvalidFiles)
            cerr << "There are insufficient permissions to modify file " << path << '\n';
        return false;
    }

    return true;
}
