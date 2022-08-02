#include "subtitle_shifter.h"

int main(int argc, char *argv[]) {

    SubtitleShifter shifter;

    if (auto status = shifter.parseArguments(argc, argv); status != ParseStatus::Continue)
        return (status == ParseStatus::Error) ? 1 : 0;
    shifter.shift();

    return 0;
}
