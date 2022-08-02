#pragma once

#include <ostream>

struct TimeStamp {
    int milliseconds;

    explicit TimeStamp(int milliseconds) : milliseconds(milliseconds) {
    }

    TimeStamp(int hours, int minutes, int seconds, int milliseconds) : milliseconds(milliseconds) {
        this->milliseconds += seconds * 1000 +
                              minutes * 1000 * 60 +
                              hours * 1000 * 60 * 60;
    }

    [[nodiscard]] bool isNegative() const {
        return milliseconds < 0;
    }

    TimeStamp operator+(const TimeStamp &rhs) const {
        return TimeStamp(milliseconds + rhs.milliseconds);
    }

    TimeStamp &operator+=(const TimeStamp &rhs) {
        milliseconds += rhs.milliseconds;
        return *this;
    }

    friend std::ostream &operator<<(std::ostream &outputStream, const TimeStamp &timeStamp) {
        int absoluteMilliseconds = std::abs(timeStamp.milliseconds);

        int millisecondsOutput = absoluteMilliseconds % 1000;

        int secondsOutput = absoluteMilliseconds / 1000;

        int minutesOutput = secondsOutput / 60;
        secondsOutput = secondsOutput % 60;

        int hoursOutput = minutesOutput / 60;
        minutesOutput = minutesOutput % 60;

        if (timeStamp.isNegative())
            outputStream << '-';

        if (hoursOutput < 10)
            outputStream << '0';
        outputStream << hoursOutput << ':';

        if (minutesOutput < 10)
            outputStream << '0';
        outputStream << minutesOutput << ':';

        if (secondsOutput < 10)
            outputStream << '0';
        outputStream << secondsOutput << ',';

        if (millisecondsOutput < 100) {
            outputStream << '0';
            if (millisecondsOutput < 10)
                outputStream << '0';
        }
        outputStream << millisecondsOutput;

        return outputStream;
    }
};
