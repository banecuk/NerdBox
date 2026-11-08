#include "WeatherDayMath.h"

namespace WeatherDayMath {

time_t localMidnight(struct tm nowLocal) {
    nowLocal.tm_hour = 0;
    nowLocal.tm_min = 0;
    nowLocal.tm_sec = 0;
    nowLocal.tm_isdst = -1;
    return mktime(&nowLocal);
}

bool needsRollover(const struct tm& nowLocal, time_t firstDayStart) {
    return localMidnight(nowLocal) != firstDayStart;
}

}  // namespace WeatherDayMath
