#include "subtitle_shifter.h"

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <regex>
#include <fstream>
#include <sstream>

#include <boost/program_options.hpp>

#include "time_stamp.h"

#ifdef _WIN32
#   define PATH_DIVIDER '\\'
#else
#   define PATH_DIVIDER '/'
#endif

namespace po = boost::program_options;
namespace fs = std::filesystem;
using std::cout, std::cerr, std::vector, std::regex, std::ifstream, std::ostringstream, std::string, std::ofstream,
    std::getline, std::smatch;

ParseStatus SubtitleShifter::parseArguments(int argc, const char *const argv[]) {

    // Extract executable filename from executable path
    mExecutableName = argv[0];
    mExecutableName = mExecutableName.substr(mExecutableName.find_last_of(PATH_DIVIDER) + 1);

    po::options_description description("Options");
    description.add_options()
        ("help,h", "Show this message and exit")
        ("modify-files,m", "Modify the file(s) instead of creating new file(s) with the shifted subtitles")
        ("destination-path,d", po::value<string>()->default_value("."), "Set destination directory for output files")
        ("recurse,r", "Recursively add files in a directory and its subdirectories")
        ("ignore,i", "Ignore invalid files provided with input-path")
        ("offset-ms", po::value<int>(), "Milliseconds by which subtitles will be offset")
        ("input-path", po::value<vector<string>>(), "Input subtitle files");

    po::positional_options_description positionalDescription;
    positionalDescription.add("offset-ms", 1).add("input-path", -1);

    po::variables_map map;

    try {
        po::store(
            po::command_line_parser(argc, argv)
                .options(description).positional(positionalDescription)
                .run(),
            map
        );
        po::notify(map);

    } catch (po::error &error) {
        cerr << mExecutableName << ": " << error.what() << '\n';
        return ParseStatus::Error;
    }

    if (map.count("help")) {
        cout << "Usage: " << mExecutableName << " [option]... <offset-ms> <input-path>...\n\n"
             << description << '\n';
        return ParseStatus::Exit;
    }

    // Offset and input path must be provided
    if (!map.count("offset-ms") || !map.count("input-path")) {
        cout << "Usage: " << mExecutableName << " [option]... <offset-ms> <input-path>...\n\n"
             << description << '\n';
        return ParseStatus::Error;
    }

    if (fs::path destinationPath(map["destination-path"].as<string>()); destinationPath != ".") {
        if (!fs::is_directory(destinationPath)) {
            cerr << mExecutableName << ": invalid directory " << destinationPath << '\n';
            return ParseStatus::Error;
        }
        setDestinationPath(destinationPath);
    }

    if (map.count("modify-files")) {
        if (mDestinationPath != ".") {
            cerr << mExecutableName << ": options '--modify-files' and '--destination-path' are incongruous\n";
            return ParseStatus::Error;
        }
        setDoModify(true);
    }

    if (map.count("recurse"))
        setDoRecurse(true);

    if (map.count("ignore"))
        setIgnoreInvalidFiles(true);

    setMillisecondsOffset(map["offset-ms"].as<int>());

    // Parse paths
    for (const auto &inputPath: map["input-path"].as<vector<string>>()) {

        fs::path path(inputPath);
        if (path.filename() == "*")
            path.remove_filename();

        if (fs::is_regular_file(path)) {

            if (!isFileValid(path)) {
                if (mIgnoreInvalidFiles)
                    continue;
                return ParseStatus::Error;
            }

            addFilePath(path);

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

                    addFilePath(directoryEntry.path());
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

                    addFilePath(directoryEntry.path());
                }
            }

        } else {
            cerr << mExecutableName << ": invalid file/directory " << path << '\n';
            return ParseStatus::Error;
        }
    }

    if (mPaths.empty()) {
        cerr << mExecutableName << ": no valid files provided\n";
        return ParseStatus::Error;
    }

    return ParseStatus::Continue;
}

void SubtitleShifter::addFilePath(const fs::path &path) {
    mPaths.push_back(path);
}

void SubtitleShifter::shift() {
    if (mPaths.empty())
        return;

    // https://regex101.com/r/w2aGaG/1
    const regex srtTimeStampRegex(
        R"(^(0\d|[1-9]\d+):([0-5]\d):([0-5]\d),(\d{3}) --> (0\d|[1-9]\d+):([0-5]\d):([0-5]\d),(\d{3})$)"
    );

    const TimeStamp offset(mMillisecondsOffset);

    for (const auto &path: mPaths) {

        ifstream inStream(path);
        if (!inStream) {
            cerr << mExecutableName << ": cannot open input file " << path << '\n';
            return;
        }

        fs::path outputPath(
            mDoModify ? path.string() : path.stem().string() + "_shifted" + path.extension().string()
        );
        if (!mDoModify)
            outputPath = mDestinationPath / outputPath;

        ostringstream outBuffer;
        if (!outBuffer) {
            cerr << mExecutableName << ": cannot create output buffer\n";
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
                         << outputPath << ", line #" << lineNumber << '\n';
                }

                outBuffer << from << " --> " << to;

            } else if (!line.empty()) {
                outBuffer << line;
            }

            outBuffer << '\n';
            ++lineNumber;
        }

        inStream.close();

        ofstream outStream(outputPath);
        if (!outStream) {
            cerr << mExecutableName << ": cannot open output file " << outputPath << '\n';
            return;
        }

        outStream << outBuffer.str();
    }
}

bool SubtitleShifter::isFileValid(const std::filesystem::path &path) const {
    if (path.extension() != ".srt") {
        if (!mIgnoreInvalidFiles) {
            cerr << mExecutableName << ": file type " << path.extension() << " (" << path << ") is not supported.\n"
                 << "Supported file types are: .srt\n";
        }
        return false;
    }

    auto status = fs::status(path);

    if ((status.permissions() & fs::perms::group_read) == fs::perms::none) {
        if (!mIgnoreInvalidFiles)
            cerr << mExecutableName << ": there are insufficient permissions to read file " << path << '\n';
        return false;
    }

    if (mDoModify && (status.permissions() & fs::perms::group_write) == fs::perms::none) {
        if (!mIgnoreInvalidFiles)
            cerr << mExecutableName << ": there are insufficient permissions to modify file " << path << '\n';
        return false;
    }

    return true;
}
