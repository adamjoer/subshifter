#include "subtitle_shifter.h"

int main(int argc, char *argv[]) {

    SubtitleShifter shifter;

    if (!shifter.parseArguments(argc, argv))
        return 1;

    shifter.shift();

    return 0;
}
