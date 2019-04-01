//
// Created by Alexandre Lemos on 05/11/2018.
//

#ifndef TIMETABLER_LECTURE_H
#define TIMETABLER_LECTURE_H


#include <ostream>

class Lecture {
    std::string days;
    int start;
    int lenght;
    std::string weeks;
    int penalty;
    bool doubleL = false;
 public:
    Lecture(int lenght, int start, std::string weeks, std::string days, int penalty) : days(days), start(start),
                                                                                       lenght(lenght),
                                                                                       weeks(weeks),
                                                                                       penalty(penalty) {}

    Lecture(int lenght) : lenght(lenght) {}

    const std::string &getDays() const {
        return days;
    }

    void setDays(const std::string &days) {
        Lecture::days = days;
    }

    int getNumbWeeks() {
        int value = 0;
        for (char &c : weeks) {
            if (c == '1')
                value++;

        }
        return value;
    }

    int getStart() const {
        return start;
    }

    void setStart(int start) {
        Lecture::start = start;
    }

    int getLenght() {
        return lenght;
    }

    const std::string &getWeeks() const {
        return weeks;
    }

    void setWeeks(const std::string &weeks) {
        Lecture::weeks = weeks;
    }

    void setLenght(int lenght) {
        Lecture::lenght = lenght;
    }

    friend std::ostream &operator<<(std::ostream &os, const Lecture &lecture) {
        os << "days: " << lecture.days << " start: " << lecture.start << " lenght: " << lecture.lenght << " weeks: "
           << lecture.weeks << " penalty: " << lecture.penalty;
        return os;
    }

    bool isActive(int w) {
        if (weeks[w] == '1')
            return true;
        return false;
    }

    Lecture(int id, char *doubleLec) {
        if (strcmp(doubleLec, "yes") == 0)
            doubleL = true;
        lenght = 1;
    }
};


#endif //TIMETABLER_LECTURE_H
