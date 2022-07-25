#include "subtitle_shifter.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <regex>
#include <fstream>

#include "time_stamp.h"

#ifdef _WIN32
#   define PATH_DIVIDER '\\'
#else
#   define PATH_DIVIDER '/'
#endif

namespace fs = std::filesystem;
using std::cout, std::cerr, std::stoi, std::invalid_argument, std::out_of_range, std::move, std::regex, std::ifstream,
        std::ofstream, std::string, std::getline, std::smatch;

ParseStatus SubtitleShifter::parseArguments(int argc, char *argv[]) {

    // Extract executable filename from executable path
    mExecutableName = argv[0];
    mExecutableName = mExecutableName.substr(mExecutableName.find_last_of(PATH_DIVIDER) + 1);

    // Index of current argument in argv
    int i;

    // Parse any potential options
    for (i = 1; i < argc && argv[i][0] == '-' && !isdigit(argv[i][1]); ++i) {

        for (int j = 1, len = (int) strlen(argv[i]); j < len; ++j) {
            bool doBreak = false;

            auto flag = argv[i][j];
            switch (flag) {
                case 'h':
                    printUsage();
                    return ParseStatus::Exit;

                case 'm':
                    if (mIsDestinationPathSpecified) {
                        cerr << "Switches 'm' and 'd' are incongruous\n";
                        return ParseStatus::Error;
                    }

                    mDoModify = true;
                    break;

                case 'd':
                    if (mDoModify) {
                        cerr << "Switches 'm' and 'd' are incongruous\n";
                        return ParseStatus::Error;
                    }

                    if (mIsDestinationPathSpecified)
                        continue;

                    if (i >= argc - 1 && j >= len - 1) {
                        cerr << "No destination path specified after 'd' switch\n";
                        return ParseStatus::Error;
                    }

                    if (fs::path destinationPath((j < len - 1) ? &argv[i][j + 1] : argv[++i]);
                            !fs::is_directory(destinationPath)) {
                        cerr << "Invalid directory " << destinationPath << '\n';
                        return ParseStatus::Error;

                    } else {
                        mDestinationPath = destinationPath;
                    }

                    mIsDestinationPathSpecified = true;

                    doBreak = true;
                    break;

                case 'i':
                    mIgnoreInvalidFiles = true;
                    break;

                case 'r':
                    mDoRecurse = true;
                    break;

                default:
                    cerr << "Unknown switch '" << flag << "'\n";
                    return ParseStatus::Error;
            }

            if (doBreak)
                break;
        }
    }

    // There must be at least two arguments after options
    if (i > argc - 2) {
        printUsage();
        return ParseStatus::Error;
    }

    // Parse offset-ms
    try {
        mMillisecondsOffset = stoi(argv[i]);

    } catch (invalid_argument &exception) {
        cerr << "Invalid number \"" << argv[i] << "\"\n";
        return ParseStatus::Error;

    } catch (out_of_range &exception) {
        cerr << "Number is out of range: " << argv[i] << '\n';
        return ParseStatus::Error;
    }

    // Parse paths
    for (++i; i < argc; ++i) {

        fs::path path(argv[i]);
        if (path.filename() == "*")
            path.remove_filename();

        if (fs::is_regular_file(path)) {

            if (!isFileValid(path)) {
                if (mIgnoreInvalidFiles)
                    continue;
                return ParseStatus::Error;
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
                        return ParseStatus::Error;
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
                        return ParseStatus::Error;
                    }

                    mPaths.push_back(directoryEntry.path());
                }
            }

        } else {
            cerr << "Invalid file/directory " << path << '\n';
            return ParseStatus::Error;
        }
    }

    if (mPaths.empty()) {
        cerr << "No valid files provided\n";
        return ParseStatus::Error;
    }

    mAreArgumentsValid = true;
    return ParseStatus::Continue;
}

void SubtitleShifter::printUsage() {
    cout << "Usage: " << mExecutableName << R"( [option]... <offset-ms> <path>...

    -h                      Print this message and exit
    -m                      Modify the file(s) instead of creating new file(s) with the shifted subtitles
    -d <destination-path>   Set destination directory for output files
    -r                      Recursively add files in a directory and its subdirectories
    -i                      Ignore invalid files provided with <path>

)";
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
