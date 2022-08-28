#include "tool.h"

const long MILLISECONDS_PER_SECOND = 1000;
const long MICROSECONDS_PER_MILLISECOND = 1000;
const long NANOSECONDS_PER_MICROSECOND = 1000;

const long MICROSECONDS_PER_SECOND =
        MICROSECONDS_PER_MILLISECOND * MILLISECONDS_PER_SECOND;
const long NANOSECONDS_PER_MILLISECOND =
        NANOSECONDS_PER_MICROSECOND * MICROSECONDS_PER_MILLISECOND;
const long NANOSECONDS_PER_SECOND =
        NANOSECONDS_PER_MILLISECOND * MILLISECONDS_PER_SECOND;

const int SECONDS_PER_MINUTE = 60;
const int MINUTES_PER_HOUR = 60;
const int HOURS_PER_DAY = 24;
const int SECONDS_PER_HOUR = SECONDS_PER_MINUTE * MINUTES_PER_HOUR;

const long MILLISECONDS_PER_MINUTE =
        MILLISECONDS_PER_SECOND * SECONDS_PER_MINUTE;
const long NANOSECONDS_PER_MINUTE = NANOSECONDS_PER_SECOND * SECONDS_PER_MINUTE;
const long NANOSECONDS_PER_HOUR = NANOSECONDS_PER_SECOND * SECONDS_PER_HOUR;
const long NANOSECONDS_PER_DAY = NANOSECONDS_PER_HOUR * HOURS_PER_DAY;

std::string ToSecondStr(long nano, const char* format) {
    if (nano <= 0)
        return std::string("NULL");
    nano /= NANOSECONDS_PER_SECOND;
    struct tm* dt = {0};
    char buffer[30];
    dt = gmtime(&nano);
    strftime(buffer, sizeof(buffer), format, dt);
    return std::string(buffer);
}


long NanoTime() {
    std::chrono::high_resolution_clock::time_point curtime = std::chrono::high_resolution_clock().now();
    long orin_nanosecs = std::chrono::duration_cast<std::chrono::nanoseconds>(curtime.time_since_epoch()).count();
    return orin_nanosecs;
//    return NanoTimer::getInstance()->getNano();
}

std::string NanoTimeStr() {
    long nano_time = NanoTime();
    string time_now = ToSecondStr(nano_time, "%Y-%m-%d %H:%M:%S");
    time_now += "." + std::to_string(nano_time % NANOSECONDS_PER_SECOND);
    return time_now;
}
