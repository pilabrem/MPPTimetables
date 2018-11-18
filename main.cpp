#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include "Lecture.h"

#include "Room.h"
#include "Course.h"
#include "distribution.h"
#include "rapid/rapidxml.hpp"
#include "rapid/rapidxml_print.hpp"
#include "DistributionRequired.h"
#include "DistributionPenalty.h"
#include "Limits.h"
#include "WithLimit.h"
#include "Instance.h"
#include "ILPExecuter.h"


Instance *readInputXML(std::string filename);

void readOutputXML(std::string filename, Instance *instance);

void writeOutputXML(std::string filename, Instance *instance, double time);

using namespace rapidxml;

int main() {
    clock_t tStart = clock();
    Instance *instance = readInputXML("/Volumes/MAC/ClionProjects/timetabler/data/input/wbg-fal10.xml");
    readOutputXML("/Volumes/MAC/ClionProjects/timetabler/data/output/wbg-fal10.xml", instance);

    ILPExecuter *runner = new ILPExecuter();
    runner->setInstance(instance);
    runner->createSol();
    runner->loadOutput();
    runner->definedRoomLecture();
    runner->definedLectureTime();
    runner->oneLectureSlot();
    runner->oneLectureRoom();
    //runner->slackStudent();
    runner->studentConflict();
    //  runner->oneLectureRoomConflict();
    //  runner->optimizeSeatedStudents();


    double v = runner->run();
    int **sol = runner->getSolutionRoom();
    writeOutputXML("/Volumes/MAC/ClionProjects/timetabler/data/output/wbg-fal10Out.xml", instance,
                   (double) (clock() - tStart) / CLOCKS_PER_SEC);
    runner = new ILPExecuter();
    runner->setInstance(instance);
    runner->definedRoomLecture();
    runner->definedLectureTime();
    runner->oneLectureSlot();
    runner->oneLectureRoom();
    //runner->slackStudent();
    runner->studentConflict();
    //runner->constraintSeatedStudents(v);
    runner->distanceToSolution(sol, nullptr, false);
    runner->run();


    printf("Time taken: %.2fs\n", (double) (clock() - tStart) / CLOCKS_PER_SEC);


    return 0;
}


void writeOutputXML(std::string filename, Instance *instance, double time) {
    xml_document<> doc;
    xml_node<> *decl = doc.allocate_node(node_declaration);
    decl->append_attribute(doc.allocate_attribute("version", "1.0"));
    decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
    doc.append_node(decl);

    xml_node<> *root = doc.allocate_node(node_element, "solution");
    root->append_attribute(doc.allocate_attribute("name", instance->getName().c_str()));
    root->append_attribute(doc.allocate_attribute("runtime", std::to_string(time).c_str()));
    root->append_attribute(doc.allocate_attribute("cores", "4"));
    root->append_attribute(doc.allocate_attribute("technique", "ILP"));
    root->append_attribute(doc.allocate_attribute("author", "Alexandre Lemos"));
    root->append_attribute(doc.allocate_attribute("institution", "INESC-ID"));
    root->append_attribute(doc.allocate_attribute("country", "Portugal"));
    doc.append_node(root);
    xml_node<> *child;
    for (int c = 0; c < instance->getClasses().size(); c++) {
        child = doc.allocate_node(node_element, "class");
        child->append_attribute(doc.allocate_attribute("id", doc.allocate_string(
                std::to_string(instance->getClasses()[c]->getId()).c_str())));
        child->append_attribute(
                doc.allocate_attribute("days", doc.allocate_string(instance->getClasses()[c]->getSolDays())));
        child->append_attribute(doc.allocate_attribute("start", doc.allocate_string(
                std::to_string(instance->getClasses()[c]->getSolStart()).c_str())));
        child->append_attribute(
                doc.allocate_attribute("weeks", doc.allocate_string(instance->getClasses()[c]->getSolWeek())));
        child->append_attribute(doc.allocate_attribute("room", doc.allocate_string(
                std::to_string(instance->getClasses()[c]->getSolRoom()).c_str())));
        for (int s = 0; s < instance->getClasses()[c]->getStudent().size(); ++s) {
            xml_node<> *student = doc.allocate_node(node_element, "student");
            student->append_attribute(doc.allocate_attribute("id", doc.allocate_string(
                    std::to_string(instance->getClasses()[c]->getStudent()[s]).c_str())));
            child->append_node(student);
        }
        root->append_node(child);

    }

    std::ofstream file_stored(filename);
    //print(std::cout, doc, 0);
    file_stored << doc;

    file_stored.close();
    doc.clear();

}

void readOutputXML(std::string filename, Instance *instance) {
    xml_document<> doc;
    std::ifstream file(filename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    std::__1::string content(buffer.str());
    doc.parse<0>(&content[0]);
    xml_node<> *pRoot = doc.first_node();
    for (const xml_attribute<> *a = pRoot->first_attribute(); a; a = a->next_attribute()) {
        if (strcmp(a->name(), "name") == 0 && strcmp(a->value(), instance->getName().c_str()) != 0) {
            std::cerr << "Instance and Solution do not match" << std::endl;
        }
    }
    int total = 0;
    for (const xml_node<> *n = pRoot->first_node(); n; n = n->next_sibling()) {
        char *weeks, *days;
        int id = -1, start = -1, room = -1;
        for (const xml_attribute<> *a = n->first_attribute(); a; a = a->next_attribute()) {
            if (strcmp(a->name(), "id") == 0) {
                id = atoi(a->value());
            } else if (strcmp(a->name(), "start") == 0) {
                start = atoi(a->value());
            } else if (strcmp(a->name(), "room") == 0) {
                room = atoi(a->value());
            } else if (strcmp(a->name(), "weeks") == 0) {
                weeks = a->value();
            } else if (strcmp(a->name(), "days") == 0) {
                days = a->value();
            }
        }
        Class *s = instance->getClass(id);
        s->setSolution(start, room, weeks, days);
        std::vector<int> student;
        for (const xml_node<> *stu = n->first_node(); stu; stu = stu->next_sibling()) {
            for (const xml_attribute<> *a = stu->first_attribute(); a; a = a->next_attribute()) {
                student.push_back(atoi(a->value()));
                instance->getStudent(atoi(a->value())).addClass(s);
            }
        }
        s->addStudents(student);
        total += s->getSteatedStudents();
    }
    instance->setTotalNumberSteatedStudent(total);


}

Instance *readInputXML(std::string filename) {//parent flag missing
    xml_document<> doc;
    std::ifstream file(filename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    std::__1::string content(buffer.str());
    doc.parse<0>(&content[0]);
    xml_node<> *pRoot = doc.first_node();
    Instance *instance;
    for (const xml_attribute<> *a = pRoot->first_attribute(); a; a = a->next_attribute()) {
        if (strcmp(a->name(), "name") == 0)
            instance= new Instance(a->value());
        else if (strcmp(a->name(), "nrDays") == 0)
            instance->setNdays(atoi(a->value()));
        else if (strcmp(a->name(), "slotsPerDay") == 0)
            instance->setSlotsperday(atoi(a->value()));
        else if (strcmp(a->name(), "nrWeeks") == 0)
            instance->setNweek(atoi(a->value()));
    }
    for (const xml_node<> *n = pRoot->first_node(); n; n = n->next_sibling()) {
        if (strcmp(n->name(), "optimization") == 0) {
            for (const xml_attribute<> *a = n->first_attribute(); a; a = a->next_attribute()) {
                if (strcmp(a->name(), "time") == 0) {
                    instance->setTimePen(atoi(a->value()));
                } else if (strcmp(a->name(), "room") == 0) {
                    instance->setRoomPen(atoi(a->value()));
                } else if (strcmp(a->name(), "distribution") == 0) {
                    instance->setDistributionPen(atoi(a->value()));
                } else if (strcmp(a->name(), "student") == 0) {
                    instance->setStudentPen(atoi(a->value()));
                }
            }
        } else if (strcmp(n->name(), "rooms") == 0) {
            for (const xml_node<> *s = n->first_node(); s; s = s->next_sibling()) {
                int id=-1, capacity=-1;
                for (const xml_attribute<> *a = s->first_attribute(); a; a = a->next_attribute()) {
                    if (strcmp(a->name(), "capacity") == 0) {
                        capacity=atoi(a->value());
                    } else if (strcmp(a->name(), "id") == 0) {
                        id=atoi(a->value());
                    }
                }
                std::map<int, int> travel;
                std::vector<Unavailability> una;
                for (const xml_node<> *rs = s->first_node(); rs; rs = rs->next_sibling()) {
                    if (strcmp(rs->name(), "travel") == 0) {
                        int value=-1,room=-1;
                        for (const xml_attribute<> *a = rs->first_attribute(); a; a = a->next_attribute()) {
                            if (strcmp(a->name(), "room") == 0) {
                                room=atoi(a->value());
                            } else if (strcmp(a->name(), "value") == 0) {
                                value=atoi(a->value());
                            }
                        }
                        travel.insert(std::pair<int, int>(room, value));


                    } else if (strcmp(rs->name(), "unavailable") == 0) {
                        char* days,*weeks;
                        int length=-1,start=-1;
                        for (const xml_attribute<> *a = rs->first_attribute(); a; a = a->next_attribute()) {
                            if (strcmp(a->name(), "days") == 0) {
                                days=a->value();
                            } else if (strcmp(a->name(), "start") == 0) {
                                start=atoi(a->value());
                            } else if (strcmp(a->name(), "length") == 0) {
                                length=atoi(a->value());
                            } else if (strcmp(a->name(), "weeks") == 0) {
                                weeks=a->value();
                            }
                        }
                        Unavailability *u = new Unavailability(days, start, length, weeks);
                        //std::cout<<*u<<std::endl;
                        una.push_back(*u);
                    }


                }
                Room *r = new Room(id, capacity, travel, una);
                //   std::cout<<*r<<std::endl;
                //      std::cout<<r->getSlots().size()<<std::endl;
                instance->addRoom(r);
                //std::cout<<instance->getRooms().size()<<std::endl;
            }

        } else if (strcmp(n->name(), "courses") == 0) {

            for (const xml_node<> *s = n->first_node(); s; s = s->next_sibling()) {
                char *id;
                std::__1::map<int, std::__1::vector<Subpart *>> config;
                for (const xml_attribute<> *a = s->first_attribute(); a; a = a->next_attribute()) {
                    id=a->value();
                }
                for (const xml_node<> *rs = s->first_node(); rs; rs = rs->next_sibling()) {
                    int idConf = -1;

                    std::__1::vector<Subpart *> subpartvec;
                    for (const xml_attribute<> *a = rs->first_attribute(); a; a = a->next_attribute()) {
                        idConf = atoi(a->value());
                    }
                    for (const xml_node<> *sub = rs->first_node(); sub; sub = sub->next_sibling()) {
                        std::__1::string idsub;
                        std::__1::vector<Class *> clasv;
                        for (const xml_attribute<> *a = sub->first_attribute(); a; a = a->next_attribute()) {
                            idsub = a->value();
                        }
                        for (const xml_node<> *cla = sub->first_node(); cla; cla = cla->next_sibling()) {
                            int idclass=-1,limit=-1;
                            std::map<Room, int> roomsv;
                            std::vector<Lecture *> lecv;
                            for (const xml_attribute<> *a = cla->first_attribute(); a; a = a->next_attribute()) {
                                if (strcmp(a->name(), "id") == 0)
                                    idclass=atoi(a->value());
                                else
                                    limit = atoi(a->value());
                            }
                            for (const xml_node<> *lec = cla->first_node(); lec; lec = lec->next_sibling()) {
                                if (strcmp(lec->name(), "room") == 0) {
                                    int idRoom = -1, penalty = -1;
                                    for (const xml_attribute<> *a = lec->first_attribute(); a; a = a->next_attribute()) {
                                        //std::cout<<a->name()<<std::endl;
                                        if (strcmp(a->name(), "id") == 0)
                                            idRoom = atoi(a->value());
                                        else if (strcmp(a->name(), "penalty") == 0)
                                            penalty = atoi(a->value());

                                    }
                                    roomsv.insert(std::pair<Room, int>(instance->getRoom(idRoom), penalty));
                                } else if (strcmp(lec->name(), "time") == 0) {
                                    int lenght = -1, start = -1, penalty = -1;
                                    std::string weeks, days;
                                    for (const xml_attribute<> *a = lec->first_attribute(); a; a = a->next_attribute()) {


                                        if (strcmp(a->name(), "length") == 0) {
                                            lenght = atoi(a->value());
                                        } else if (strcmp(a->name(), "start") == 0) {
                                            start = atoi(a->value());
                                        } else if (strcmp(a->name(), "weeks") == 0) {
                                            weeks = a->value();
                                        } else if (strcmp(a->name(), "days") == 0) {
                                            days = a->value();
                                        } else if (strcmp(a->name(), "penalty") == 0) {
                                            penalty = atoi(a->value());
                                        }


                                    }
                                    //std::cout<<idclass<<" "<<lenght<<" "<<start<<std::endl;
                                    Lecture *l = new Lecture(lenght, start, weeks, days, penalty);
                                    lecv.push_back(l);
                                }

                                //limit=atoi(a->value());


                            }
                            Class *c = new Class(idclass, limit, lecv, roomsv);
                            clasv.push_back(c);

                        }
                        Subpart *subpart = new Subpart(idsub, clasv);
                        subpartvec.push_back(subpart);

                    }
                    config.insert(std::pair<int, std::vector<Subpart *>>(idConf, subpartvec));

                }
                Course *course = new Course(id, config);
                instance->addCourse(course);

            }


        } else if (strcmp(n->name(), "distributions") == 0) {
            for (const xml_node<> *sub = n->first_node(); sub; sub = sub->next_sibling()) {
                bool isReq = false;
                std::string type;
                int penalty = -1;
                std::vector<int> c;
                int limit = -1, limit1 = -1;
                for (const xml_attribute<> *a = sub->first_attribute(); a; a = a->next_attribute()) {
                    if (strcmp(a->name(), "required") == 0) {
                        isReq = (atoi(a->value()) == 1);
                    } else if (strcmp(a->name(), "type") == 0) {
                        type = a->value();
                        long size;
                        std::string rest;
                        if ((size = type.find("(")) != std::string::npos) {
                            rest = type.substr(size + 1, std::string::npos);
                            type = type.substr(0, size);
                            if ((size = rest.find(",")) != std::string::npos) {
                                limit1 = atoi(rest.substr(size + 1, std::string::npos - 1).c_str());
                            }
                            limit = atoi(rest.substr(0, size).c_str());
                        }
                    }

                }
                for (const xml_node<> *course = sub->first_node(); course; course = course->next_sibling()) {
                    int idClassesDist = -1;
                    for (const xml_attribute<> *a = course->first_attribute(); a; a = a->next_attribute()) {
                        idClassesDist = atoi(a->value());
                    }
                    c.push_back(idClassesDist);
                    //std::cout<<course->name()<<std::endl;
                }
                Constraint *limite;
                distribution *req;
                if (isReq) {
                    if (limit1 != -1) {
                        limite = new Limits(limit, limit1);
                    } else if (limit != -1) {
                        limite = new WithLimit(limit);
                    } else {
                        limite = new NoLimit();
                    }
                    limite->setType(type);
                    req = new DistributionRequired(limite, c);
                } else {
                    if (limit1 != -1) {
                        limite = new Limits(limit, limit1);
                    } else if (limit != -1) {
                        limite = new WithLimit(limit);
                    } else {
                        limite = new NoLimit();
                    }
                    limite->setType(type);
                    req = new DistributionPenalty(limite, c, penalty);

                }
                if (req)
                    instance->addDistribution(req);

            }

        } else if (strcmp(n->name(), "students") == 0) {
            std::__1::map<int, Student> std;
            for (const xml_node<> *sub = n->first_node(); sub; sub = sub->next_sibling()) {
                int studentID = -1;
                for (const xml_attribute<> *a = sub->first_attribute(); a; a = a->next_attribute()) {
                    studentID = atoi(a->value());
                }
                std::__1::vector<Course *> c;
                for (const xml_node<> *course = sub->first_node(); course; course = course->next_sibling()) {
                    std::__1::string courseIDstd;
                    for (const xml_attribute<> *a = course->first_attribute(); a; a = a->next_attribute()) {
                        courseIDstd = a->value();
                    }
                    c.push_back(instance->getCourse(courseIDstd));

                }
                std.insert(std::pair<int, Student>(studentID, Student(studentID, c)));
            }
            instance->setStudent(std);

        }

    }
    return instance;
}