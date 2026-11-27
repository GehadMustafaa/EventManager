#pragma once
#include <cctype>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

namespace DateUtil {

inline bool isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

inline int daysInMonth(int year, int month) {
    static constexpr int kDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && isLeapYear(year)) return 29;
    return kDays[month - 1];
}

inline bool isValidIsoDate(const std::string& date) {
    if (date.size() != 10 || date[4] != '-' || date[7] != '-') return false;
    for (std::size_t i = 0; i < date.size(); ++i) {
        if (i == 4 || i == 7) continue;
        if (!std::isdigit(static_cast<unsigned char>(date[i]))) return false;
    }
    const int year = std::stoi(date.substr(0, 4));
    const int month = std::stoi(date.substr(5, 2));
    const int day = std::stoi(date.substr(8, 2));
    if (year < 1900 || year > 9999) return false;
    if (month < 1 || month > 12) return false;
    return day >= 1 && day <= daysInMonth(year, month);
}

inline void requireValidDate(const std::string& date) {
    if (!isValidIsoDate(date)) throw std::invalid_argument("event date must be a valid YYYY-MM-DD date");
}

inline std::string formatTime12h(int hour12, int minute, bool isPm) {
    if (hour12 < 1 || hour12 > 12) throw std::invalid_argument("hour must be between 1 and 12");
    if (minute < 0 || minute > 59) throw std::invalid_argument("minute must be between 0 and 59");
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << hour12 << ':'
        << std::setw(2) << std::setfill('0') << minute << ' ' << (isPm ? "PM" : "AM");
    return oss.str();
}

inline bool parseTime(const std::string& raw, int& hour12, int& minute, bool& isPm) {
    std::string s;
    for (char c : raw) {
        if (!std::isspace(static_cast<unsigned char>(c)))
            s.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    }
    if (s.size() < 6) return false;
    const std::string suffix = s.substr(s.size() - 2);
    if (suffix != "AM" && suffix != "PM") return false;
    isPm = (suffix == "PM");
    const std::string hhmm = s.substr(0, s.size() - 2);
    const std::size_t colon = hhmm.find(':');
    if (colon == std::string::npos || colon == 0 || colon + 1 >= hhmm.size()) return false;
    const std::string hStr = hhmm.substr(0, colon);
    const std::string mStr = hhmm.substr(colon + 1);
    if (hStr.size() > 2 || mStr.size() != 2) return false;
    for (char c : hStr)
        if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    for (char c : mStr)
        if (!std::isdigit(static_cast<unsigned char>(c))) return false;

    const int h = std::stoi(hStr);
    const int m = std::stoi(mStr);
    if (h < 1 || h > 12 || m < 0 || m > 59) return false;
    hour12 = h;
    minute = m;
    return true;
}

inline std::string requireValidTime(const std::string& time) {
    int h = 0;
    int m = 0;
    bool pm = false;
    if (!parseTime(time, h, m, pm))
        throw std::invalid_argument("event time must be in hh:mm AM/PM format");
    return formatTime12h(h, m, pm);
}

inline int timeToMinutes(const std::string& time) {
    int h = 0;
    int m = 0;
    bool pm = false;
    if (!parseTime(time, h, m, pm)) return 0;
    const int h24 = (h % 12) + (pm ? 12 : 0);
    return h24 * 60 + m;
}

}  // namespace DateUtil
