#include "subtitle_shifter.h"

int main(int argc, char *argv[]) {

    SubtitleShifter shifter;

    if (auto status = shifter.parseArguments(argc, argv); status != ParseStatus::Continue) {
        if (status == ParseStatus::Exit)
            return 0;
        return 1;
    }

    shifter.shift();

    return 0;
}
