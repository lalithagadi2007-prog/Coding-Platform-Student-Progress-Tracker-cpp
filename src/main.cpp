// ============================================================
//  CODING PLATFORM / STUDENT PROGRESS TRACKER  -  ADVANCED EDITION
//  OOPS Through C++ capstone
//
//  Advanced OOP features in this version:
//   1. Abstract base class + interface (Person, Serializable)
//   2. Real runtime polymorphism (Admin / Faculty / Student as Person*)
//   3. Class templates (Repository<T>) - one generic file store
//   4. Custom exception hierarchy (AppException -> Validation / NotFound)
//   5. Operator overloading (operator<, operator==, operator<<)
//   6. Singleton design pattern (Logger -> activity.log audit trail)
//   7. Multiple inheritance (Student = Person + Serializable)
//   8. Function overloading, friend function, static members
//   9. Salted password hashing, per-user passwords, login lockout
//  10. Points + leaderboard, late penalty, topic analytics,
//      problem recommendations, deadline reminders, CSV export, backup
//
//  Written in C++98-compatible style (works in Dev-C++ / any g++).
//  Old data files (students.txt etc.) from the earlier version still load.
// ============================================================

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <iomanip>
#include <cstdlib>
#include <cctype>
#include <cmath>
#include <ctime>

using namespace std;

// ============================================================
// UTILITY HELPERS
// ============================================================

string intToString(int value)
{
    ostringstream oss;
    oss << value;
    return oss.str();
}

string toLowerStr(string s)
{
    for (size_t i = 0; i < s.size(); i++)
        s[i] = (char)tolower((unsigned char)s[i]);
    return s;
}

string toUpperStr(string s)
{
    for (size_t i = 0; i < s.size(); i++)
        s[i] = (char)toupper((unsigned char)s[i]);
    return s;
}

string trim(const string &s)
{
    size_t start = 0;
    while (start < s.size() && isspace((unsigned char)s[start])) start++;
    size_t end = s.size();
    while (end > start && isspace((unsigned char)s[end - 1])) end--;
    return s.substr(start, end - start);
}

bool startsWith(const string &text, const string &prefix)
{
    return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}

bool containsIgnoreCase(const string &text, const string &keyword)
{
    return toLowerStr(text).find(toLowerStr(keyword)) != string::npos;
}

string shorten(const string &s, size_t maxLen)
{
    if (s.size() <= maxLen) return s;
    return s.substr(0, maxLen - 2) + "..";
}

bool isValidEmail(const string &e)
{
    size_t at = e.find('@');
    if (at == string::npos || at == 0) return false;
    size_t dot = e.find('.', at);
    if (dot == string::npos || dot == at + 1 || dot == e.size() - 1) return false;
    return e.find(' ') == string::npos;
}

bool isValidURL(const string &u)
{
    return (startsWith(u, "http://") || startsWith(u, "https://"))
           && u.size() > 10 && u.find(' ') == string::npos;
}

// ---------- dates (format YYYY-MM-DD) ----------

bool isLeapYear(int y)
{
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

int daysInMonth(int y, int m)
{
    static const int days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (m == 2 && isLeapYear(y)) return 29;
    return days[m - 1];
}

// Checks the real calendar: 2026-02-31 is rejected, leap years handled.
bool isValidDate(const string &d)
{
    if (d.size() != 10 || d[4] != '-' || d[7] != '-') return false;
    for (int i = 0; i < 10; i++)
    {
        if (i == 4 || i == 7) continue;
        if (!isdigit((unsigned char)d[i])) return false;
    }
    int year = atoi(d.substr(0, 4).c_str());
    int month = atoi(d.substr(5, 2).c_str());
    int day = atoi(d.substr(8, 2).c_str());
    if (year < 2000 || year > 2100) return false;
    if (month < 1 || month > 12) return false;
    return day >= 1 && day <= daysInMonth(year, month);
}

string todayDate()
{
    time_t now = time(0);
    struct tm *t = localtime(&now);
    char buffer[16];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", t);
    return string(buffer);
}

string nowTime()
{
    time_t now = time(0);
    struct tm *t = localtime(&now);
    char buffer[16];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", t);
    return string(buffer);
}

time_t dateToTime(const string &d)
{
    struct tm t;
    t.tm_year = atoi(d.substr(0, 4).c_str()) - 1900;
    t.tm_mon = atoi(d.substr(5, 2).c_str()) - 1;
    t.tm_mday = atoi(d.substr(8, 2).c_str());
    t.tm_hour = 12;
    t.tm_min = 0;
    t.tm_sec = 0;
    t.tm_wday = 0;
    t.tm_yday = 0;
    t.tm_isdst = -1;
    return mktime(&t);
}

// Number of days from 'from' to 'to' (negative if 'to' is earlier).
long daysBetween(const string &from, const string &to)
{
    double diff = difftime(dateToTime(to), dateToTime(from));
    return (long)floor(diff / 86400.0 + 0.5);
}

vector<string> splitLine(const string &line, char delimiter)
{
    vector<string> parts;
    string current;
    for (size_t i = 0; i < line.size(); i++)
    {
        if (line[i] == delimiter) { parts.push_back(current); current = ""; }
        else if (line[i] != '\r') current += line[i];
    }
    parts.push_back(current);
    return parts;
}

bool copyFile(const string &from, const string &to)
{
    ifstream in(from.c_str(), ios::binary);
    if (!in) return false;
    ofstream out(to.c_str(), ios::binary);
    if (!out) return false;
    out << in.rdbuf();
    return true;
}

bool vectorContains(const vector<string> &v, const string &item)
{
    for (size_t i = 0; i < v.size(); i++)
        if (v[i] == item) return true;
    return false;
}


// ============================================================
// CUSTOM EXCEPTION HIERARCHY
// ============================================================

class AppException : public runtime_error
{
public:
    AppException(const string &msg) : runtime_error(msg) {}
};

class ValidationException : public AppException
{
public:
    ValidationException(const string &msg) : AppException("Validation failed: " + msg) {}
};

class NotFoundException : public AppException
{
public:
    NotFoundException(const string &msg) : AppException("Not found: " + msg) {}
};


// ============================================================
// LOGGER  (Singleton design pattern - one audit trail for the app)
// ============================================================

class Logger
{
private:
    string actor;

    Logger() : actor("anonymous") {}
    Logger(const Logger &);              // copying disabled
    Logger &operator=(const Logger &);   // assignment disabled

public:
    static Logger &instance()
    {
        static Logger theLogger;
        return theLogger;
    }

    void setActor(const string &who) { actor = who; }

    void log(const string &action) const
    {
        ofstream file("activity.log", ios::app);
        if (file)
            file << "[" << todayDate() << " " << nowTime() << "] "
                 << actor << ": " << action << "\n";
    }
};


// ============================================================
// INPUT HELPERS (validation)
// ============================================================

int getInteger(string message)
{
    int value;
    while (true)
    {
        cout << message;
        if (cin >> value)
        {
            cin.ignore(1000, '\n');
            return value;
        }
        if (cin.eof()) { cout << "\nInput closed. Exiting." << endl; exit(0); }
        cout << "Invalid input. Enter a number." << endl;
        cin.clear();
        cin.ignore(1000, '\n');
    }
}

int getIntegerInRange(string message, int lo, int hi)
{
    while (true)
    {
        int value = getInteger(message);
        if (value >= lo && value <= hi) return value;
        cout << "Please enter a value between " << lo << " and " << hi << "." << endl;
    }
}

double getDoubleInRange(string message, double lo, double hi)
{
    double value;
    while (true)
    {
        cout << message;
        if (cin >> value)
        {
            cin.ignore(1000, '\n');
            if (value >= lo && value <= hi) return value;
            cout << "Please enter a value between " << lo << " and " << hi << "." << endl;
        }
        else
        {
            if (cin.eof()) { cout << "\nInput closed. Exiting." << endl; exit(0); }
            cout << "Invalid input. Enter a number." << endl;
            cin.clear();
            cin.ignore(1000, '\n');
        }
    }
}

// Reads a line, trims it and replaces '|' (our file separator) with '/'.
string getString(string message)
{
    string value;
    cout << message;
    if (!getline(cin, value))
    {
        cout << "\nInput closed. Exiting." << endl;
        exit(0);
    }
    value = trim(value);
    for (size_t i = 0; i < value.size(); i++)
        if (value[i] == '|') value[i] = '/';
    return value;
}

string getNonEmptyString(string message)
{
    while (true)
    {
        string value = getString(message);
        if (!value.empty()) return value;
        cout << "This field cannot be empty." << endl;
    }
}

string getValidEmail(string message)
{
    while (true)
    {
        string value = getString(message);
        if (isValidEmail(value)) return value;
        cout << "Invalid email format (example: name@gmail.com)." << endl;
    }
}

string getValidDate(string message, bool notInPast)
{
    while (true)
    {
        string value = getString(message);
        if (!isValidDate(value))
        {
            cout << "Invalid date. Use a real date in the format YYYY-MM-DD." << endl;
            continue;
        }
        if (notInPast && value < todayDate())
        {
            cout << "Date cannot be in the past (today is " << todayDate() << ")." << endl;
            continue;
        }
        return value;
    }
}

// Returns "" if the user types 'cancel'.
string getValidURL(string message, const string &requiredPrefix)
{
    while (true)
    {
        string url = getString(message);
        if (toLowerStr(url) == "cancel") return "";
        if (!isValidURL(url))
        {
            cout << "Invalid link. It must start with http:// or https:// (or type 'cancel')." << endl;
            continue;
        }
        if (!requiredPrefix.empty() && !startsWith(url, requiredPrefix))
        {
            cout << "Link must belong to " << requiredPrefix << " (or type 'cancel')." << endl;
            continue;
        }
        return url;
    }
}

bool confirmAction(string message)
{
    string answer = toLowerStr(getString(message + " (y/n): "));
    return answer == "y" || answer == "yes";
}


// ============================================================
// SERIALIZABLE  (interface: anything that can be stored in a file)
// ============================================================

class Serializable
{
public:
    virtual string serialize() const = 0;
    virtual bool deserialize(const string &line) = 0;
    virtual ~Serializable() {}
};


// ============================================================
// PERSON - ABSTRACT BASE CLASS  (Admin, Student, Faculty derive from it)
// ============================================================

class Person
{
protected:
    string name;
    string email;

public:
    Person(string n = "", string e = "")
    {
        name = n;
        email = e;
    }

    virtual void display() const = 0;
    virtual string role() const = 0;

    string getName() const { return name; }
    string getEmail() const { return email; }
    void setEmail(string e) { email = e; }

    virtual ~Person() {}
};


// ============================================================
// ADMIN
// ============================================================

class Admin : public Person
{
public:
    Admin(string n = "System Administrator", string e = "admin@university.edu")
        : Person(n, e) {}

    virtual void display() const
    {
        cout << "\n--- Administrator ---" << endl;
        cout << "Name  : " << name << endl;
        cout << "Email : " << email << endl;
        cout << "Role  : " << role() << endl;
    }

    virtual string role() const { return "Admin"; }
};


// ============================================================
// TIMETABLE  (the university runs 9 timetables, numbered 1 - 9)
// ============================================================

class Timetable
{
public:
    enum { COUNT = 9 };

    static bool isValid(int no) { return no >= 1 && no <= COUNT; }

    static string timings(int no)
    {
        static const char *slots[COUNT] = {
            "8:30 AM - 3:20 PM",  "8:30 AM - 3:20 PM",  "9:00 AM - 4:00 PM",
            "9:00 AM - 4:00 PM",  "9:30 AM - 4:20 PM",  "9:30 AM - 4:20 PM",
            "10:00 AM - 5:00 PM", "10:00 AM - 5:00 PM", "11:00 AM - 6:00 PM"
        };
        if (!isValid(no)) return "Not assigned";
        return slots[no - 1];
    }

    static string labDay(int no)
    {
        static const char *days[COUNT] = {
            "Monday", "Tuesday", "Wednesday", "Thursday", "Friday",
            "Monday", "Tuesday", "Wednesday", "Thursday"
        };
        if (!isValid(no)) return "-";
        return days[no - 1];
    }

    static string describe(int no)
    {
        if (!isValid(no)) return "Not assigned";
        return "Timetable " + intToString(no) + " (" + timings(no) +
               ", Lab: " + labDay(no) + ")";
    }

    static void printAll()
    {
        cout << "\n--- University Timetables ---" << endl;
        cout << setw(6) << "No." << setw(22) << "Timings" << "Lab Day" << endl;
        cout << string(42, '-') << endl;
        for (int i = 1; i <= COUNT; i++)
            cout << setw(6) << i << setw(22) << timings(i) << labDay(i) << endl;
    }
};


// ============================================================
// STUDENT CLASS  (multiple inheritance: Person + Serializable)
// ============================================================

class Student : public Person, public Serializable
{
protected:
    string studentID;
    int year;
    string department;
    int timetableNo;     // 1 - 9
    double cgpa;         // 0.00 - 10.00

public:
    Student(string n = "", string id = "", string e = "",
            int y = 1, string dpt = "", int tt = 1, double g = 0.0)
        : Person(n, e)
    {
        studentID = id;
        year = y;
        department = dpt;
        timetableNo = tt;
        cgpa = g;
    }

    virtual void display() const
    {
        cout << "\n--- Student Details ---" << endl;
        cout << "Name       : " << name << endl;
        cout << "Student ID : " << studentID << endl;
        cout << "Email      : " << email << endl;
        cout << "Year       : " << year << endl;
        cout << "Department : " << department << endl;
        cout << "CGPA       : " << cgpa << endl;
        cout << "Timetable  : " << Timetable::describe(timetableNo) << endl;
    }

    virtual string role() const { return "Student"; }

    // ----- Serializable -----
    virtual string serialize() const
    {
        ostringstream o;
        o << fixed << setprecision(2);
        o << studentID << "|" << name << "|" << email << "|" << year << "|"
          << department << "|" << timetableNo << "|" << cgpa;
        return o.str();
    }

    virtual bool deserialize(const string &line)
    {
        vector<string> f = splitLine(line, '|');
        if (f.size() < 7) return false;      // skips old-format / damaged lines

        int y = atoi(f[3].c_str());
        int tt = atoi(f[5].c_str());
        double g = atof(f[6].c_str());
        if (y < 1 || y > 4) y = 1;
        if (!Timetable::isValid(tt)) tt = 1;
        if (g < 0.0 || g > 10.0) g = 0.0;

        studentID = f[0];
        name = f[1];
        email = f[2];
        year = y;
        department = f[4];
        timetableNo = tt;
        cgpa = g;
        return true;
    }

    // ----- validated updates (throw custom exceptions) -----
    void updateYear(int newYear)
    {
        if (newYear < 1 || newYear > 4)
            throw ValidationException("Year must be between 1 and 4.");
        year = newYear;
    }

    void updateEmail(string newEmail)
    {
        if (!isValidEmail(newEmail))
            throw ValidationException("Invalid email.");
        email = newEmail;
    }

    void updateTimetable(int newNo)
    {
        if (!Timetable::isValid(newNo))
            throw ValidationException("Timetable number must be between 1 and 9.");
        timetableNo = newNo;
    }

    void updateCGPA(double newCGPA)
    {
        if (newCGPA < 0.0 || newCGPA > 10.0)
            throw ValidationException("CGPA must be between 0.00 and 10.00.");
        cgpa = newCGPA;
    }

    string getStudentID() const { return studentID; }
    int getYear() const { return year; }
    string getDepartment() const { return department; }
    int getTimetableNo() const { return timetableNo; }
    double getCGPA() const { return cgpa; }

    // ----- operator overloading -----
    // Natural ordering = ranking: highest CGPA first, ties alphabetical.
    bool operator<(const Student &other) const
    {
        if (fabs(cgpa - other.cgpa) > 0.0001) return cgpa > other.cgpa;
        return name < other.name;
    }

    bool operator==(const Student &other) const
    {
        return studentID == other.studentID;
    }

    friend ostream &operator<<(ostream &os, const Student &s)
    {
        os << s.studentID << " | " << s.name << " | " << s.department
           << " | Year " << s.year << " | CGPA " << s.cgpa;
        return os;
    }

    friend void studentSummary(const Student &s);
};

void studentSummary(const Student &s)
{
    cout << "\nStudent Summary";
    cout << "\nName       : " << s.name;
    cout << "\nStudent ID : " << s.studentID;
    cout << "\nDepartment : " << s.department;
    cout << "\nYear       : " << s.year;
    cout << "\nCGPA       : " << s.cgpa;
    cout << "\nTimetable  : " << s.timetableNo << endl;
}


// ============================================================
// FACULTY CLASS
// ============================================================

class Faculty : public Person, public Serializable
{
protected:
    string facultyID;
    string department;
    string subject;
    string courseCode;

public:
    Faculty(string n = "", string id = "", string e = "",
            string dpt = "", string sub = "", string code = "")
        : Person(n, e)
    {
        facultyID = id;
        department = dpt;
        subject = sub;
        courseCode = code;
    }

    virtual void display() const
    {
        cout << "\n--- Faculty Details ---" << endl;
        cout << "Name        : " << name << endl;
        cout << "Faculty ID  : " << facultyID << endl;
        cout << "Email       : " << email << endl;
        cout << "Department  : " << department << endl;
        cout << "Subject     : " << subject << endl;
        cout << "Course Code : " << courseCode << endl;
    }

    virtual string role() const { return "Faculty"; }

    virtual string serialize() const
    {
        return facultyID + "|" + name + "|" + email + "|" + department + "|" +
               subject + "|" + courseCode;
    }

    virtual bool deserialize(const string &line)
    {
        vector<string> f = splitLine(line, '|');
        if (f.size() < 6) return false;
        facultyID = f[0];
        name = f[1];
        email = f[2];
        department = f[3];
        subject = f[4];
        courseCode = f[5];
        return true;
    }

    string getFacultyID() const { return facultyID; }
    string getDepartment() const { return department; }
    string getSubject() const { return subject; }
    string getCourseCode() const { return courseCode; }
};


// ============================================================
// PLATFORM  (with website, so problem links can be validated)
// ============================================================

class Platform : public Serializable
{
private:
    int platformID;
    string platformName;
    string website;

public:
    Platform(int id = 0, string name = "", string site = "")
    {
        platformID = id;
        platformName = name;
        website = site;
    }

    void display() const
    {
        cout << setw(5) << platformID << setw(16) << platformName << website << endl;
    }

    virtual string serialize() const
    {
        return intToString(platformID) + "|" + platformName + "|" + website;
    }

    virtual bool deserialize(const string &line)
    {
        vector<string> f = splitLine(line, '|');
        if (f.size() < 3) return false;
        platformID = atoi(f[0].c_str());
        platformName = f[1];
        website = f[2];
        return true;
    }

    int getID() const { return platformID; }
    string getName() const { return platformName; }
    string getWebsite() const { return website; }
};

int findPlatformIndex(const vector<Platform> &platforms, int id)
{
    for (int i = 0; i < (int)platforms.size(); i++)
        if (platforms[i].getID() == id) return i;
    return -1;
}

string getPlatformName(const vector<Platform> &platforms, int id)
{
    int idx = findPlatformIndex(platforms, id);
    if (idx == -1) return "Unknown";
    return platforms[idx].getName();
}

string getPlatformWebsite(const vector<Platform> &platforms, int id)
{
    int idx = findPlatformIndex(platforms, id);
    if (idx == -1) return "";
    return platforms[idx].getWebsite();
}

int nextPlatformID(const vector<Platform> &platforms)
{
    int maxID = 0;
    for (int i = 0; i < (int)platforms.size(); i++)
        if (platforms[i].getID() > maxID) maxID = platforms[i].getID();
    return maxID + 1;
}

bool platformNameExists(const vector<Platform> &platforms, const string &name)
{
    for (int i = 0; i < (int)platforms.size(); i++)
        if (toLowerStr(platforms[i].getName()) == toLowerStr(name)) return true;
    return false;
}


// ============================================================
// PROBLEM CLASS  (each problem carries its own direct link)
// ============================================================

class Problem : public Serializable
{
private:
    int problemID;
    string title;
    string difficulty;
    int platformID;
    string topic;
    string url;

public:
    Problem(int id = 0, string t = "", string d = "",
            int pid = 0, string top = "", string u = "")
    {
        problemID = id;
        title = t;
        difficulty = d;
        platformID = pid;
        topic = top;
        url = u;
    }

    void display(const vector<Platform> &platforms) const
    {
        cout << "\nProblem ID : " << problemID;
        cout << "\nTitle      : " << title;
        cout << "\nDifficulty : " << difficulty;
        cout << "\nPlatform   : " << getPlatformName(platforms, platformID);
        cout << "\nTopic      : " << topic;
        cout << "\nLink       : " << url << endl;
    }

    virtual string serialize() const
    {
        return intToString(problemID) + "|" + title + "|" + difficulty + "|" +
               intToString(platformID) + "|" + topic + "|" + url;
    }

    virtual bool deserialize(const string &line)
    {
        vector<string> f = splitLine(line, '|');
        if (f.size() < 6) return false;
        problemID = atoi(f[0].c_str());
        title = f[1];
        difficulty = f[2];
        platformID = atoi(f[3].c_str());
        topic = f[4];
        url = f[5];
        return true;
    }

    int getID() const { return problemID; }
    string getTitle() const { return title; }
    string getDifficulty() const { return difficulty; }
    int getPlatformID() const { return platformID; }
    string getTopic() const { return topic; }
    string getURL() const { return url; }
};

int findProblemIndex(const vector<Problem> &problems, int id)
{
    for (int i = 0; i < (int)problems.size(); i++)
        if (problems[i].getID() == id) return i;
    return -1;
}

int nextProblemID(const vector<Problem> &problems)
{
    int maxID = 100;   // problem IDs start at 101
    for (int i = 0; i < (int)problems.size(); i++)
        if (problems[i].getID() > maxID) maxID = problems[i].getID();
    return maxID + 1;
}

bool problemURLExists(const vector<Problem> &problems, const string &url)
{
    for (int i = 0; i < (int)problems.size(); i++)
        if (problems[i].getURL() == url) return true;
    return false;
}


// ============================================================
// STUDENT PLATFORM (relationship: one student -> many platforms)
// ============================================================

class StudentPlatform : public Serializable
{
private:
    string studentID;
    int platformID;
    string profileURL;

public:
    StudentPlatform(string sid = "", int pid = 0, string url = "")
    {
        studentID = sid;
        platformID = pid;
        profileURL = url;
    }

    void display(const vector<Platform> &platforms) const
    {
        cout << "\nStudent ID  : " << studentID;
        cout << "\nPlatform    : " << getPlatformName(platforms, platformID);
        cout << "\nProfile URL : " << profileURL << endl;
    }

    virtual string serialize() const
    {
        return studentID + "|" + intToString(platformID) + "|" + profileURL;
    }

    virtual bool deserialize(const string &line)
    {
        vector<string> f = splitLine(line, '|');
        if (f.size() < 3) return false;
        studentID = f[0];
        platformID = atoi(f[1].c_str());
        profileURL = f[2];
        return true;
    }

    string getStudentID() const { return studentID; }
    int getPlatformID() const { return platformID; }
    string getProfileURL() const { return profileURL; }
};

bool isStudentPlatformUnique(const vector<StudentPlatform> &links,
                              const string &studentID, int platformID)
{
    for (int i = 0; i < (int)links.size(); i++)
    {
        if (links[i].getStudentID() == studentID &&
            links[i].getPlatformID() == platformID)
            return false;
    }
    return true;
}


// ============================================================
// ASSIGNMENT CLASS  (Student + Problem + Faculty + Deadline)
// ============================================================

class Assignment : public Serializable
{
private:
    int assignmentID;
    string studentID;
    int problemID;
    string facultyID;
    string deadline;

public:
    Assignment(int aid = 0, string sid = "", int pid = 0,
               string fid = "", string dl = "")
    {
        assignmentID = aid;
        studentID = sid;
        problemID = pid;
        facultyID = fid;
        deadline = dl;
    }

    virtual string serialize() const
    {
        return intToString(assignmentID) + "|" + studentID + "|" +
               intToString(problemID) + "|" + facultyID + "|" + deadline;
    }

    virtual bool deserialize(const string &line)
    {
        vector<string> f = splitLine(line, '|');
        if (f.size() < 5) return false;
        assignmentID = atoi(f[0].c_str());
        studentID = f[1];
        problemID = atoi(f[2].c_str());
        facultyID = f[3];
        deadline = f[4];
        return true;
    }

    int getAssignmentID() const { return assignmentID; }
    string getStudentID() const { return studentID; }
    int getProblemID() const { return problemID; }
    string getFacultyID() const { return facultyID; }
    string getDeadline() const { return deadline; }
};

int nextAssignmentID(const vector<Assignment> &assignments)
{
    int maxID = 0;
    for (int i = 0; i < (int)assignments.size(); i++)
        if (assignments[i].getAssignmentID() > maxID) maxID = assignments[i].getAssignmentID();
    return maxID + 1;
}

int findAssignmentIndex(const vector<Assignment> &assignments,
                         const string &studentID, int problemID)
{
    for (int i = 0; i < (int)assignments.size(); i++)
        if (assignments[i].getStudentID() == studentID &&
            assignments[i].getProblemID() == problemID)
            return i;
    return -1;
}

bool hasAssignment(const vector<Assignment> &assignments,
                    const string &studentID, int problemID)
{
    return findAssignmentIndex(assignments, studentID, problemID) != -1;
}

bool facultyAssignedProblem(const vector<Assignment> &assignments, const string &facultyID,
                             const string &studentID, int problemID)
{
    for (int i = 0; i < (int)assignments.size(); i++)
        if (assignments[i].getFacultyID() == facultyID &&
            assignments[i].getStudentID() == studentID &&
            assignments[i].getProblemID() == problemID)
            return true;
    return false;
}


// ============================================================
// SUBMISSION CLASS  (the evidence of a student's work)
// ============================================================

class Submission : public Serializable
{
private:
    int submissionID;
    string studentID;
    int problemID;
    int platformID;
    string submissionURL;
    string submissionDate;
    string status; // Pending, Accepted, Rejected, In Progress

public:
    Submission(int sub = 0, string sid = "", int pid = 0, int plid = 0,
               string url = "", string date = "", string st = "Pending")
    {
        submissionID = sub;
        studentID = sid;
        problemID = pid;
        platformID = plid;
        submissionURL = url;
        submissionDate = date;
        status = st;
    }

    void display(const vector<Platform> &platforms) const
    {
        cout << "\nSubmission ID : " << submissionID;
        cout << "\nStudent ID    : " << studentID;
        cout << "\nProblem ID    : " << problemID;
        cout << "\nPlatform      : " << getPlatformName(platforms, platformID);
        cout << "\nSubmission URL: " << submissionURL;
        cout << "\nDate          : " << submissionDate;
        cout << "\nStatus        : " << status << endl;
    }

    virtual string serialize() const
    {
        return intToString(submissionID) + "|" + studentID + "|" +
               intToString(problemID) + "|" + intToString(platformID) + "|" +
               submissionURL + "|" + submissionDate + "|" + status;
    }

    virtual bool deserialize(const string &line)
    {
        vector<string> f = splitLine(line, '|');
        if (f.size() < 7) return false;
        submissionID = atoi(f[0].c_str());
        studentID = f[1];
        problemID = atoi(f[2].c_str());
        platformID = atoi(f[3].c_str());
        submissionURL = f[4];
        submissionDate = f[5];
        status = f[6];
        return true;
    }

    int getSubmissionID() const { return submissionID; }
    string getStudentID() const { return studentID; }
    int getProblemID() const { return problemID; }
    int getPlatformID() const { return platformID; }
    string getSubmissionURL() const { return submissionURL; }
    string getDate() const { return submissionDate; }
    string getStatus() const { return status; }

    void updateStatus(string newStatus)
    {
        if (newStatus != "Pending" && newStatus != "Accepted" &&
            newStatus != "Rejected" && newStatus != "In Progress")
            throw ValidationException("Invalid submission status.");
        status = newStatus;
    }
};

int findSubmissionIndex(const vector<Submission> &submissions, int id)
{
    for (int i = 0; i < (int)submissions.size(); i++)
        if (submissions[i].getSubmissionID() == id) return i;
    return -1;
}

int nextSubmissionID(const vector<Submission> &submissions)
{
    int maxID = 0;
    for (int i = 0; i < (int)submissions.size(); i++)
        if (submissions[i].getSubmissionID() > maxID) maxID = submissions[i].getSubmissionID();
    return maxID + 1;
}

bool hasAcceptedSubmission(const vector<Submission> &submissions,
                            const string &studentID, int problemID)
{
    for (int i = 0; i < (int)submissions.size(); i++)
    {
        if (submissions[i].getStudentID() == studentID &&
            submissions[i].getProblemID() == problemID &&
            submissions[i].getStatus() == "Accepted")
            return true;
    }
    return false;
}

bool hasUnderReviewSubmission(const vector<Submission> &submissions,
                               const string &studentID, int problemID)
{
    for (int i = 0; i < (int)submissions.size(); i++)
    {
        if (submissions[i].getStudentID() == studentID &&
            submissions[i].getProblemID() == problemID &&
            (submissions[i].getStatus() == "Pending" ||
             submissions[i].getStatus() == "In Progress"))
            return true;
    }
    return false;
}


// ============================================================
// CREDENTIAL  (salted + hashed passwords, one record per user)
// ============================================================

string defaultPasswordFor(const string &role)
{
    if (role == "Admin") return "admin123";
    if (role == "Faculty") return "faculty123";
    return "student123";
}

// Educational salted hash (32-bit, 1000 rounds). Fine for a course project;
// a real system would use bcrypt / Argon2.
string hashPassword(const string &password, const string &salt)
{
    string s = salt + password;
    unsigned long h = 5381;
    for (int round = 0; round < 1000; round++)
        for (size_t i = 0; i < s.size(); i++)
            h = (h * 33 + (unsigned char)s[i]) & 0xFFFFFFFFUL;
    ostringstream o;
    o << hex << h;
    return o.str();
}

string makeSalt()
{
    ostringstream o;
    o << hex << (rand() % 0x7FFF) << (rand() % 0x7FFF);
    return o.str();
}

class Credential : public Serializable
{
private:
    string role;
    string userID;
    string salt;
    string hash;
    bool mustChange;

public:
    Credential(string r = "", string id = "")
    {
        role = r;
        userID = id;
        salt = "";
        hash = "";
        mustChange = true;
    }

    void setPassword(const string &password, bool forceChange)
    {
        salt = makeSalt();
        hash = hashPassword(password, salt);
        mustChange = forceChange;
    }

    bool matches(const string &password) const
    {
        return hash == hashPassword(password, salt);
    }

    virtual string serialize() const
    {
        return role + "|" + userID + "|" + salt + "|" + hash + "|" +
               (mustChange ? "1" : "0");
    }

    virtual bool deserialize(const string &line)
    {
        vector<string> f = splitLine(line, '|');
        if (f.size() < 5) return false;
        role = f[0];
        userID = f[1];
        salt = f[2];
        hash = f[3];
        mustChange = (f[4] == "1");
        return true;
    }

    string getRole() const { return role; }
    string getUserID() const { return userID; }
    bool getMustChange() const { return mustChange; }
};

int findCredentialIndex(const vector<Credential> &creds,
                         const string &role, const string &id)
{
    for (int i = 0; i < (int)creds.size(); i++)
        if (creds[i].getRole() == role && creds[i].getUserID() == id) return i;
    return -1;
}

void removeCredential(vector<Credential> &creds, const string &role, const string &id)
{
    int idx = findCredentialIndex(creds, role, id);
    if (idx != -1) creds.erase(creds.begin() + idx);
}

// A user with no credential record yet logs in with the role's default password
// and is forced to choose a new one.
bool authenticate(vector<Credential> &creds, const string &role, const string &id,
                   const string &password, bool &needsChange)
{
    int idx = findCredentialIndex(creds, role, id);
    if (idx == -1)
    {
        if (password != defaultPasswordFor(role)) return false;
        Credential c(role, id);
        c.setPassword(defaultPasswordFor(role), true);
        creds.push_back(c);
        needsChange = true;
        return true;
    }
    if (!creds[idx].matches(password)) return false;
    needsChange = creds[idx].getMustChange();
    return true;
}

void setNewPassword(vector<Credential> &creds, const string &role, const string &id)
{
    int idx = findCredentialIndex(creds, role, id);
    if (idx == -1)
    {
        creds.push_back(Credential(role, id));
        idx = (int)creds.size() - 1;
    }

    while (true)
    {
        string p1 = getString("Enter new password (min 6 characters): ");
        if (p1.size() < 6) { cout << "Password is too short." << endl; continue; }
        if (p1 == defaultPasswordFor(role))
        {
            cout << "Choose a password different from the default one." << endl;
            continue;
        }
        string p2 = getString("Confirm new password: ");
        if (p1 != p2) { cout << "Passwords do not match." << endl; continue; }

        creds[idx].setPassword(p1, false);
        cout << "\nPassword updated successfully." << endl;
        Logger::instance().log("changed password");
        return;
    }
}

void changePasswordMenu(vector<Credential> &creds, const string &role, const string &id)
{
    string oldPw = getString("Enter current password: ");
    bool needsChange = false;
    if (!authenticate(creds, role, id, oldPw, needsChange))
    {
        cout << "\nIncorrect current password." << endl;
        return;
    }
    setNewPassword(creds, role, id);
}


// ============================================================
// REPOSITORY<T>  (class template: one generic file store for every entity)
// ============================================================

template <typename T>
class Repository
{
private:
    vector<T> items;
    string filename;

public:
    Repository(const string &file) : filename(file) {}

    vector<T> &all() { return items; }
    const vector<T> &all() const { return items; }
    size_t size() const { return items.size(); }
    const string &file() const { return filename; }

    void load()
    {
        items.clear();
        ifstream in(filename.c_str());
        if (!in) return;
        string line;
        while (getline(in, line))
        {
            T item;
            if (item.deserialize(line)) items.push_back(item);
        }
    }

    void save() const
    {
        ofstream out(filename.c_str());
        for (size_t i = 0; i < items.size(); i++)
            out << items[i].serialize() << "\n";
    }
};


// ============================================================
// DATA STORE  (groups every repository; saves after every action)
// ============================================================

struct DataStore
{
    Repository<Student> students;
    Repository<Faculty> faculty;
    Repository<Platform> platforms;
    Repository<Problem> problems;
    Repository<Assignment> assignments;
    Repository<Submission> submissions;
    Repository<StudentPlatform> links;
    Repository<Credential> credentials;

    DataStore()
        : students("students.txt"), faculty("faculty.txt"),
          platforms("platforms.txt"), problems("problems.txt"),
          assignments("assignments.txt"), submissions("submissions.txt"),
          links("studentplatforms.txt"), credentials("credentials.txt") {}

    void loadAll()
    {
        platforms.load();
        students.load();
        faculty.load();
        problems.load();
        assignments.load();
        submissions.load();
        links.load();
        credentials.load();
    }

    void saveAll() const
    {
        students.save();
        faculty.save();
        platforms.save();
        problems.save();
        assignments.save();
        submissions.save();
        links.save();
        credentials.save();
    }

    // Copies every data file to <name>.bak
    int backupAll() const
    {
        int copied = 0;
        if (copyFile(students.file(), students.file() + ".bak")) copied++;
        if (copyFile(faculty.file(), faculty.file() + ".bak")) copied++;
        if (copyFile(platforms.file(), platforms.file() + ".bak")) copied++;
        if (copyFile(problems.file(), problems.file() + ".bak")) copied++;
        if (copyFile(assignments.file(), assignments.file() + ".bak")) copied++;
        if (copyFile(submissions.file(), submissions.file() + ".bak")) copied++;
        if (copyFile(links.file(), links.file() + ".bak")) copied++;
        if (copyFile(credentials.file(), credentials.file() + ".bak")) copied++;
        return copied;
    }
};


// ============================================================
// ASSIGNMENT STATE + PROGRESS CALCULATION
// ============================================================

// Completed / Under Review / OVERDUE / Not Completed
string assignmentState(const vector<Submission> &submissions, const Assignment &a)
{
    if (hasAcceptedSubmission(submissions, a.getStudentID(), a.getProblemID()))
        return "Completed";
    if (hasUnderReviewSubmission(submissions, a.getStudentID(), a.getProblemID()))
        return "Under Review";
    if (a.getDeadline() < todayDate())
        return "OVERDUE";
    return "Not Completed";
}

struct ProgressStats
{
    int total;
    int completed;
    int underReview;
    int overdue;
};

ProgressStats calculateProgress(const vector<Assignment> &assignments,
                                 const vector<Submission> &submissions,
                                 const string &studentID)
{
    ProgressStats s;
    s.total = 0;
    s.completed = 0;
    s.underReview = 0;
    s.overdue = 0;

    for (int i = 0; i < (int)assignments.size(); i++)
    {
        if (assignments[i].getStudentID() != studentID) continue;

        s.total++;
        string state = assignmentState(submissions, assignments[i]);
        if (state == "Completed") s.completed++;
        else if (state == "Under Review") s.underReview++;
        else if (state == "OVERDUE") s.overdue++;
    }
    return s;
}


// ============================================================
// POINTS SYSTEM  (Easy 10 / Medium 20 / Hard 30, half if late)
// ============================================================

int pointsFor(const string &difficulty)
{
    if (difficulty == "Easy") return 10;
    if (difficulty == "Medium") return 20;
    return 30;
}

int studentScore(const vector<Assignment> &assignments,
                  const vector<Submission> &submissions,
                  const vector<Problem> &problems, const string &studentID)
{
    int score = 0;
    for (int i = 0; i < (int)assignments.size(); i++)
    {
        if (assignments[i].getStudentID() != studentID) continue;

        int p = findProblemIndex(problems, assignments[i].getProblemID());
        if (p == -1) continue;

        for (int j = 0; j < (int)submissions.size(); j++)
        {
            if (submissions[j].getStudentID() == studentID &&
                submissions[j].getProblemID() == assignments[i].getProblemID() &&
                submissions[j].getStatus() == "Accepted")
            {
                int pts = pointsFor(problems[p].getDifficulty());
                if (submissions[j].getDate() > assignments[i].getDeadline())
                    pts /= 2;                         // late penalty
                score += pts;
                break;
            }
        }
    }
    return score;
}

struct ScoreEntry
{
    string id;
    string name;
    string dept;
    int score;
    int solved;
};

bool compareScoreEntry(const ScoreEntry &a, const ScoreEntry &b)
{
    if (a.score != b.score) return a.score > b.score;
    if (a.solved != b.solved) return a.solved > b.solved;
    return a.name < b.name;
}

void displayLeaderboard(const vector<Student> &students,
                         const vector<Assignment> &assignments,
                         const vector<Submission> &submissions,
                         const vector<Problem> &problems)
{
    if (students.empty()) { cout << "\nNo students available." << endl; return; }

    vector<ScoreEntry> board;
    for (int i = 0; i < (int)students.size(); i++)
    {
        ScoreEntry e;
        e.id = students[i].getStudentID();
        e.name = students[i].getName();
        e.dept = students[i].getDepartment();
        e.score = studentScore(assignments, submissions, problems, e.id);
        e.solved = calculateProgress(assignments, submissions, e.id).completed;
        board.push_back(e);
    }
    sort(board.begin(), board.end(), compareScoreEntry);

    cout << "\n===== CODING LEADERBOARD =====" << endl;
    cout << "(Easy 10 pts, Medium 20 pts, Hard 30 pts; late solutions earn half)\n" << endl;
    cout << setw(6) << "Rank" << setw(13) << "Student ID" << setw(20) << "Name"
         << setw(7) << "Dept" << setw(8) << "Solved" << "Points" << endl;
    cout << string(62, '-') << endl;

    int rank = 1;
    for (int i = 0; i < (int)board.size(); i++)
    {
        if (i > 0 && (board[i].score != board[i - 1].score ||
                      board[i].solved != board[i - 1].solved))
            rank = i + 1;
        cout << setw(6) << rank << setw(13) << board[i].id
             << setw(20) << shorten(board[i].name, 18)
             << setw(7) << board[i].dept << setw(8) << board[i].solved
             << board[i].score << endl;
    }
}


// ============================================================
// ANALYTICS  (topic mastery, recommendations, deadlines, departments)
// ============================================================

void topicAnalytics(const vector<Assignment> &assignments,
                     const vector<Submission> &submissions,
                     const vector<Problem> &problems, const string &studentID)
{
    vector<string> topics;
    vector<int> assigned;
    vector<int> done;

    for (int i = 0; i < (int)assignments.size(); i++)
    {
        if (assignments[i].getStudentID() != studentID) continue;
        int p = findProblemIndex(problems, assignments[i].getProblemID());
        if (p == -1) continue;

        string topic = problems[p].getTopic();
        int t = -1;
        for (int k = 0; k < (int)topics.size(); k++)
            if (topics[k] == topic) { t = k; break; }
        if (t == -1)
        {
            topics.push_back(topic);
            assigned.push_back(0);
            done.push_back(0);
            t = (int)topics.size() - 1;
        }

        assigned[t]++;
        if (hasAcceptedSubmission(submissions, studentID, assignments[i].getProblemID()))
            done[t]++;
    }

    if (topics.empty()) { cout << "\nNo assigned problems to analyse yet." << endl; return; }

    cout << "\n===== TOPIC ANALYTICS =====" << endl;
    cout << setw(28) << "Topic" << setw(10) << "Assigned" << setw(10) << "Accepted"
         << "Mastery" << endl;
    cout << string(66, '-') << endl;

    int weakest = -1;
    double weakestRatio = 2.0;
    for (int i = 0; i < (int)topics.size(); i++)
    {
        int filled = (done[i] * 10) / assigned[i];
        string bar = string(filled, '#') + string(10 - filled, '-');
        cout << setw(28) << shorten(topics[i], 26) << setw(10) << assigned[i]
             << setw(10) << done[i] << "[" << bar << "]" << endl;

        double ratio = (double)done[i] / assigned[i];
        if (ratio < weakestRatio) { weakestRatio = ratio; weakest = i; }
    }
    if (weakest != -1 && weakestRatio < 1.0)
        cout << "\nFocus area: " << topics[weakest] << endl;
    else
        cout << "\nGreat work - every assigned topic is mastered!" << endl;
}

void recommendProblems(const vector<Problem> &problems,
                        const vector<Platform> &platforms,
                        const vector<Assignment> &assignments,
                        const vector<Submission> &submissions,
                        const string &studentID)
{
    // Topics the student still has unfinished work in.
    vector<string> weakTopics;
    for (int i = 0; i < (int)assignments.size(); i++)
    {
        if (assignments[i].getStudentID() != studentID) continue;
        if (hasAcceptedSubmission(submissions, studentID, assignments[i].getProblemID())) continue;
        int p = findProblemIndex(problems, assignments[i].getProblemID());
        if (p != -1 && !vectorContains(weakTopics, problems[p].getTopic()))
            weakTopics.push_back(problems[p].getTopic());
    }

    vector<int> picks;
    // Pass 1: unassigned problems on weak topics.
    for (int i = 0; i < (int)problems.size() && picks.size() < 5; i++)
        if (!hasAssignment(assignments, studentID, problems[i].getID()) &&
            vectorContains(weakTopics, problems[i].getTopic()))
            picks.push_back(i);

    // Pass 2: fill with unassigned Easy problems.
    for (int i = 0; i < (int)problems.size() && picks.size() < 5; i++)
        if (!hasAssignment(assignments, studentID, problems[i].getID()) &&
            problems[i].getDifficulty() == "Easy" &&
            find(picks.begin(), picks.end(), i) == picks.end())
            picks.push_back(i);

    if (picks.empty())
    {
        cout << "\nNo new problems to recommend right now." << endl;
        return;
    }

    cout << "\n===== RECOMMENDED PRACTICE PROBLEMS =====";
    for (int i = 0; i < (int)picks.size(); i++)
        problems[picks[i]].display(platforms);
}

void deadlineReminders(const vector<Assignment> &assignments,
                        const vector<Submission> &submissions,
                        const vector<Problem> &problems, const string &studentID)
{
    bool any = false;
    for (int i = 0; i < (int)assignments.size(); i++)
    {
        if (assignments[i].getStudentID() != studentID) continue;

        string state = assignmentState(submissions, assignments[i]);
        if (state == "Completed" || state == "Under Review") continue;

        long left = daysBetween(todayDate(), assignments[i].getDeadline());
        if (left > 3) continue;

        if (!any) { cout << "\n===== DEADLINE REMINDERS =====" << endl; any = true; }

        int p = findProblemIndex(problems, assignments[i].getProblemID());
        string title = (p != -1) ? problems[p].getTitle()
                                 : "Problem " + intToString(assignments[i].getProblemID());
        if (left < 0)
            cout << "[OVERDUE by " << -left << " day(s)] " << title << endl;
        else if (left == 0)
            cout << "[DUE TODAY] " << title << endl;
        else
            cout << "[Due in " << left << " day(s)] " << title
                 << " (" << assignments[i].getDeadline() << ")" << endl;
    }
    if (!any) cout << "\nNo urgent deadlines. You are on track!" << endl;
}

void departmentAnalytics(const vector<Student> &students,
                          const vector<Assignment> &assignments,
                          const vector<Submission> &submissions,
                          const vector<Problem> &problems)
{
    if (students.empty()) { cout << "\nNo students available." << endl; return; }

    vector<string> depts;
    vector<int> count, totalAssigned, totalDone, scoreSum;
    vector<double> cgpaSum;

    for (int i = 0; i < (int)students.size(); i++)
    {
        string d = students[i].getDepartment();
        int k = -1;
        for (int j = 0; j < (int)depts.size(); j++)
            if (depts[j] == d) { k = j; break; }
        if (k == -1)
        {
            depts.push_back(d);
            count.push_back(0);
            totalAssigned.push_back(0);
            totalDone.push_back(0);
            scoreSum.push_back(0);
            cgpaSum.push_back(0.0);
            k = (int)depts.size() - 1;
        }

        ProgressStats s = calculateProgress(assignments, submissions, students[i].getStudentID());
        count[k]++;
        cgpaSum[k] += students[i].getCGPA();
        totalAssigned[k] += s.total;
        totalDone[k] += s.completed;
        scoreSum[k] += studentScore(assignments, submissions, problems, students[i].getStudentID());
    }

    cout << "\n===== DEPARTMENT ANALYTICS =====" << endl;
    cout << setw(8) << "Dept" << setw(10) << "Students" << setw(10) << "Avg CGPA"
         << setw(12) << "Avg Points" << "Completion" << endl;
    cout << string(54, '-') << endl;
    for (int i = 0; i < (int)depts.size(); i++)
    {
        double completion = totalAssigned[i] > 0
                            ? (double)totalDone[i] / totalAssigned[i] * 100 : 0.0;
        cout << setw(8) << depts[i] << setw(10) << count[i]
             << setw(10) << cgpaSum[i] / count[i]
             << setw(12) << (double)scoreSum[i] / count[i]
             << completion << "%" << endl;
    }
}


// ============================================================
// PLATFORM OPERATIONS
// ============================================================

void displayPlatforms(const vector<Platform> &platforms)
{
    if (platforms.empty()) { cout << "\nNo platforms available." << endl; return; }
    cout << "\n--- Platforms ---" << endl;
    cout << setw(5) << "ID" << setw(16) << "Name" << "Website" << endl;
    cout << string(50, '-') << endl;
    for (int i = 0; i < (int)platforms.size(); i++) platforms[i].display();
}

void addPlatform(vector<Platform> &platforms)
{
    string name = getNonEmptyString("Enter platform name: ");
    if (platformNameExists(platforms, name))
    {
        cout << "\nError: Platform '" << name << "' already exists." << endl;
        return;
    }

    string website = getValidURL("Enter platform website (e.g. https://leetcode.com): ", "");
    if (website.empty()) { cout << "\nCancelled." << endl; return; }
    while (!website.empty() && website[website.size() - 1] == '/')
        website.erase(website.size() - 1);

    int id = nextPlatformID(platforms);
    platforms.push_back(Platform(id, name, website));
    Logger::instance().log("added platform " + name);
    cout << "\nPlatform added successfully with ID " << id << "." << endl;
}

void deletePlatform(vector<Platform> &platforms, const vector<Problem> &problems,
                     const vector<StudentPlatform> &links)
{
    int id = getInteger("Enter platform ID: ");
    int idx = findPlatformIndex(platforms, id);
    if (idx == -1) throw NotFoundException("platform " + intToString(id));

    for (int i = 0; i < (int)problems.size(); i++)
    {
        if (problems[i].getPlatformID() == id)
        {
            cout << "\nCannot delete: problems still belong to this platform." << endl;
            return;
        }
    }
    for (int i = 0; i < (int)links.size(); i++)
    {
        if (links[i].getPlatformID() == id)
        {
            cout << "\nCannot delete: students have linked accounts on this platform." << endl;
            return;
        }
    }

    Logger::instance().log("deleted platform " + platforms[idx].getName());
    platforms.erase(platforms.begin() + idx);
    cout << "\nPlatform deleted successfully." << endl;
}


// ============================================================
// STUDENT OPERATIONS  (CGPA ranking, timetable, CRUD)
// ============================================================

int findStudentIndex(const vector<Student> &students, const string &id)
{
    for (int i = 0; i < (int)students.size(); i++)
        if (students[i].getStudentID() == id) return i;
    return -1;
}

bool isStudentIDUnique(const vector<Student> &students, const string &id)
{
    return findStudentIndex(students, id) == -1;
}

// Uses the overloaded operator< of Student (highest CGPA first).
bool compareStudentByCGPA(const Student &a, const Student &b)
{
    return a < b;
}

bool compareStudentByName(const Student &a, const Student &b)
{
    return a.getName() < b.getName();
}

bool compareStudentByDepartment(const Student &a, const Student &b)
{
    if (a.getDepartment() != b.getDepartment()) return a.getDepartment() < b.getDepartment();
    return a < b;
}

// Students with the same CGPA share the same rank.
int getStudentRank(const vector<Student> &students, const string &id)
{
    int idx = findStudentIndex(students, id);
    if (idx == -1) return -1;

    int rank = 1;
    for (int i = 0; i < (int)students.size(); i++)
        if (students[i].getCGPA() > students[idx].getCGPA() + 0.0001) rank++;
    return rank;
}

void printStudentTable(const vector<Student> &list, bool showRank)
{
    if (list.empty()) { cout << "\nNo students to show." << endl; return; }

    cout << "\n";
    if (showRank) cout << setw(6) << "Rank";
    cout << setw(13) << "Student ID" << setw(20) << "Name" << setw(7) << "Dept"
         << setw(6) << "Year" << setw(7) << "CGPA" << "Timetable" << endl;
    cout << string(72, '-') << endl;

    int rank = 1;
    for (int i = 0; i < (int)list.size(); i++)
    {
        if (showRank)
        {
            if (i > 0 && fabs(list[i].getCGPA() - list[i - 1].getCGPA()) > 0.0001)
                rank = i + 1;
            cout << setw(6) << rank;
        }
        cout << setw(13) << list[i].getStudentID()
             << setw(20) << shorten(list[i].getName(), 18)
             << setw(7) << list[i].getDepartment()
             << setw(6) << list[i].getYear()
             << setw(7) << list[i].getCGPA()
             << list[i].getTimetableNo() << endl;
    }
}

void addStudent(vector<Student> &students)
{
    string id = toUpperStr(getNonEmptyString("Enter student ID: "));

    if (!isStudentIDUnique(students, id))
    {
        cout << "\nError: Student ID " << id << " already exists." << endl;
        return;
    }

    string name = getNonEmptyString("Enter student name: ");
    string email = getValidEmail("Enter email: ");
    int year = getIntegerInRange("Enter year (1-4): ", 1, 4);
    string department = toUpperStr(getNonEmptyString("Enter department (e.g. CSE, AIML): "));

    Timetable::printAll();
    int timetableNo = getIntegerInRange("Enter timetable number (1-9): ", 1, Timetable::COUNT);
    double cgpa = getDoubleInRange("Enter CGPA (0.00 - 10.00): ", 0.0, 10.0);

    students.push_back(Student(name, id, email, year, department, timetableNo, cgpa));
    sort(students.begin(), students.end());
    Logger::instance().log("added student " + id);
    cout << "\nStudent added successfully." << endl;
}

// Always shows students ranked by CGPA (highest first).
void displayStudents(const vector<Student> &students)
{
    if (students.empty()) { cout << "\nNo students available." << endl; return; }

    vector<Student> ranked = students;
    sort(ranked.begin(), ranked.end());

    cout << "\n===== STUDENT RANKING (by CGPA) =====";
    printStudentTable(ranked, true);
}

void displayStudentsByDepartment(const vector<Student> &students)
{
    string dept = toUpperStr(getNonEmptyString("Enter department: "));

    vector<Student> filtered;
    for (int i = 0; i < (int)students.size(); i++)
        if (students[i].getDepartment() == dept) filtered.push_back(students[i]);

    if (filtered.empty()) { cout << "\nNo students found in " << dept << "." << endl; return; }

    sort(filtered.begin(), filtered.end());
    cout << "\n===== " << dept << " STUDENTS (by CGPA) =====";
    printStudentTable(filtered, true);
}

void displayStudentsByTimetable(const vector<Student> &students)
{
    Timetable::printAll();
    int no = getIntegerInRange("Enter timetable number (1-9): ", 1, Timetable::COUNT);

    vector<Student> filtered;
    for (int i = 0; i < (int)students.size(); i++)
        if (students[i].getTimetableNo() == no) filtered.push_back(students[i]);

    if (filtered.empty()) { cout << "\nNo students follow timetable " << no << "." << endl; return; }

    sort(filtered.begin(), filtered.end());
    cout << "\n===== " << Timetable::describe(no) << " =====";
    printStudentTable(filtered, true);
}

void updateStudent(vector<Student> &students)
{
    string id = toUpperStr(getNonEmptyString("Enter student ID: "));
    int idx = findStudentIndex(students, id);
    if (idx == -1) throw NotFoundException("student " + id);

    cout << "\n1. Update Year";
    cout << "\n2. Update Email";
    cout << "\n3. Update Timetable";
    cout << "\n4. Update CGPA";
    int choice = getInteger("\nEnter choice: ");

    if (choice == 1)
        students[idx].updateYear(getIntegerInRange("Enter new year (1-4): ", 1, 4));
    else if (choice == 2)
        students[idx].updateEmail(getValidEmail("Enter new email: "));
    else if (choice == 3)
    {
        Timetable::printAll();
        students[idx].updateTimetable(getIntegerInRange("Enter new timetable number (1-9): ", 1, Timetable::COUNT));
    }
    else if (choice == 4)
        students[idx].updateCGPA(getDoubleInRange("Enter new CGPA (0.00 - 10.00): ", 0.0, 10.0));
    else { cout << "Invalid choice." << endl; return; }

    sort(students.begin(), students.end());
    Logger::instance().log("updated student " + id);
    cout << "\nStudent updated successfully." << endl;
}

// Deleting a student also removes everything that belonged to them.
void deleteStudent(vector<Student> &students, vector<Assignment> &assignments,
                    vector<Submission> &submissions, vector<StudentPlatform> &links,
                    vector<Credential> &creds)
{
    string id = toUpperStr(getNonEmptyString("Enter student ID: "));
    int idx = findStudentIndex(students, id);
    if (idx == -1) throw NotFoundException("student " + id);

    if (!confirmAction("Delete " + students[idx].getName() + " and all their assignments/submissions?"))
    {
        cout << "\nCancelled." << endl;
        return;
    }

    for (int i = (int)assignments.size() - 1; i >= 0; i--)
        if (assignments[i].getStudentID() == id) assignments.erase(assignments.begin() + i);
    for (int i = (int)submissions.size() - 1; i >= 0; i--)
        if (submissions[i].getStudentID() == id) submissions.erase(submissions.begin() + i);
    for (int i = (int)links.size() - 1; i >= 0; i--)
        if (links[i].getStudentID() == id) links.erase(links.begin() + i);
    removeCredential(creds, "Student", id);

    students.erase(students.begin() + idx);
    Logger::instance().log("deleted student " + id);
    cout << "\nStudent deleted successfully." << endl;
}

void searchStudent(const vector<Student> &students)
{
    cout << "\n1. Search by Student ID";
    cout << "\n2. Search by Name keyword";
    int choice = getIntegerInRange("\nEnter choice: ", 1, 2);

    if (choice == 1)
    {
        string id = toUpperStr(getNonEmptyString("Enter student ID: "));
        int idx = findStudentIndex(students, id);
        if (idx == -1) throw NotFoundException("student " + id);

        const Person &p = students[idx];      // polymorphic call through Person
        p.display();
        cout << "CGPA Rank  : " << getStudentRank(students, id) << " of " << students.size() << endl;
    }
    else
    {
        string keyword = getNonEmptyString("Enter name keyword: ");
        bool found = false;
        cout << endl;
        for (int i = 0; i < (int)students.size(); i++)
        {
            if (containsIgnoreCase(students[i].getName(), keyword))
            {
                cout << students[i] << endl;   // overloaded operator<<
                found = true;
            }
        }
        if (!found) cout << "No student name contains '" << keyword << "'." << endl;
    }
}

void sortStudents(vector<Student> &students)
{
    if (students.empty()) { cout << "\nNo students available." << endl; return; }

    cout << "\n1. Sort by CGPA (highest first)";
    cout << "\n2. Sort by Name (A-Z)";
    cout << "\n3. Sort by Department (then CGPA)";
    int choice = getIntegerInRange("\nEnter choice: ", 1, 3);

    if (choice == 1)
    {
        sort(students.begin(), students.end());
        cout << "\nStudents sorted by CGPA.";
        printStudentTable(students, true);
    }
    else if (choice == 2)
    {
        sort(students.begin(), students.end(), compareStudentByName);
        cout << "\nStudents sorted by name.";
        printStudentTable(students, false);
    }
    else
    {
        sort(students.begin(), students.end(), compareStudentByDepartment);
        cout << "\nStudents sorted by department.";
        printStudentTable(students, false);
    }
}


// ============================================================
// FACULTY OPERATIONS
// ============================================================

int findFacultyIndex(const vector<Faculty> &faculty, const string &id)
{
    for (int i = 0; i < (int)faculty.size(); i++)
        if (faculty[i].getFacultyID() == id) return i;
    return -1;
}

bool isFacultyIDUnique(const vector<Faculty> &faculty, const string &id)
{
    return findFacultyIndex(faculty, id) == -1;
}

void addFaculty(vector<Faculty> &faculty)
{
    string id = toUpperStr(getNonEmptyString("Enter faculty ID: "));

    if (!isFacultyIDUnique(faculty, id))
    {
        cout << "\nError: Faculty ID " << id << " already exists." << endl;
        return;
    }

    string name = getNonEmptyString("Enter faculty name: ");
    string email = getValidEmail("Enter email: ");
    string department = toUpperStr(getNonEmptyString("Enter department: "));
    string subject = getNonEmptyString("Enter subject taught: ");
    string courseCode = toUpperStr(getNonEmptyString("Enter course code (e.g. CS201): "));

    faculty.push_back(Faculty(name, id, email, department, subject, courseCode));
    Logger::instance().log("added faculty " + id);
    cout << "\nFaculty added successfully." << endl;
}

void displayFaculty(const vector<Faculty> &faculty)
{
    if (faculty.empty()) { cout << "\nNo faculty available." << endl; return; }

    cout << "\n===== FACULTY DIRECTORY =====\n" << endl;
    cout << setw(8) << "ID" << setw(28) << "Name" << setw(7) << "Dept"
         << setw(36) << "Subject" << "Course" << endl;
    cout << string(87, '-') << endl;
    for (int i = 0; i < (int)faculty.size(); i++)
    {
        cout << setw(8) << faculty[i].getFacultyID()
             << setw(28) << shorten(faculty[i].getName(), 26)
             << setw(7) << faculty[i].getDepartment()
             << setw(36) << shorten(faculty[i].getSubject(), 34)
             << faculty[i].getCourseCode() << endl;
    }
}

void deleteFaculty(vector<Faculty> &faculty, const vector<Assignment> &assignments,
                    vector<Credential> &creds)
{
    string id = toUpperStr(getNonEmptyString("Enter faculty ID: "));
    int idx = findFacultyIndex(faculty, id);
    if (idx == -1) throw NotFoundException("faculty " + id);

    for (int i = 0; i < (int)assignments.size(); i++)
    {
        if (assignments[i].getFacultyID() == id)
        {
            cout << "\nCannot delete: this faculty member still has assignments on record." << endl;
            return;
        }
    }

    removeCredential(creds, "Faculty", id);
    faculty.erase(faculty.begin() + idx);
    Logger::instance().log("deleted faculty " + id);
    cout << "\nFaculty deleted successfully." << endl;
}


// ============================================================
// PROBLEM OPERATIONS
// ============================================================

void printProblemRow(const Problem &p, const vector<Platform> &platforms)
{
    cout << setw(6) << p.getID()
         << setw(40) << shorten(p.getTitle(), 38)
         << setw(9) << p.getDifficulty()
         << setw(15) << getPlatformName(platforms, p.getPlatformID())
         << p.getTopic() << endl;
    cout << "      Link: " << p.getURL() << endl;
}

void printProblemHeader()
{
    cout << "\n" << setw(6) << "ID" << setw(40) << "Title" << setw(9) << "Level"
         << setw(15) << "Platform" << "Topic" << endl;
    cout << string(90, '-') << endl;
}

void addProblem(vector<Problem> &problems, const vector<Platform> &platforms)
{
    if (platforms.empty()) { cout << "\nAdd a platform first." << endl; return; }

    string title = getNonEmptyString("Enter problem title: ");

    cout << "\n1. Easy  2. Medium  3. Hard";
    int level = getIntegerInRange("\nEnter difficulty: ", 1, 3);
    string difficulty = (level == 1) ? "Easy" : (level == 2) ? "Medium" : "Hard";

    displayPlatforms(platforms);
    int platformID = getInteger("Enter platform ID from the list above: ");
    if (findPlatformIndex(platforms, platformID) == -1)
    {
        cout << "\nError: Platform ID " << platformID << " does not exist." << endl;
        return;
    }

    string topic = getNonEmptyString("Enter topic: ");

    string site = getPlatformWebsite(platforms, platformID);
    cout << "\nPaste the direct link to this problem";
    if (!site.empty()) cout << " (must start with " << site << ")";
    cout << ".";
    string url = getValidURL("\nProblem link (or 'cancel'): ", site);
    if (url.empty()) { cout << "\nCancelled." << endl; return; }

    if (problemURLExists(problems, url))
    {
        cout << "\nError: a problem with this link already exists." << endl;
        return;
    }

    int id = nextProblemID(problems);
    problems.push_back(Problem(id, title, difficulty, platformID, topic, url));
    Logger::instance().log("added problem " + intToString(id) + " (" + title + ")");
    cout << "\nProblem added successfully with ID " << id << "." << endl;
}

void displayProblems(const vector<Problem> &problems, const vector<Platform> &platforms)
{
    if (problems.empty()) { cout << "\nNo problems available." << endl; return; }
    printProblemHeader();
    for (int i = 0; i < (int)problems.size(); i++) printProblemRow(problems[i], platforms);
    cout << "\nTotal problems: " << problems.size() << endl;
}

void displayProblemsByPlatform(const vector<Problem> &problems, const vector<Platform> &platforms)
{
    displayPlatforms(platforms);
    int platformID = getInteger("Enter platform ID: ");
    if (findPlatformIndex(platforms, platformID) == -1)
        throw NotFoundException("platform " + intToString(platformID));

    bool found = false;
    for (int i = 0; i < (int)problems.size(); i++)
    {
        if (problems[i].getPlatformID() == platformID)
        {
            if (!found) printProblemHeader();
            printProblemRow(problems[i], platforms);
            found = true;
        }
    }
    if (!found) cout << "\nNo problems on this platform yet." << endl;
}

void deleteProblem(vector<Problem> &problems, const vector<Assignment> &assignments)
{
    int id = getInteger("Enter problem ID: ");
    int idx = findProblemIndex(problems, id);
    if (idx == -1) throw NotFoundException("problem " + intToString(id));

    for (int i = 0; i < (int)assignments.size(); i++)
    {
        if (assignments[i].getProblemID() == id)
        {
            cout << "\nCannot delete: this problem is already assigned to students." << endl;
            return;
        }
    }

    Logger::instance().log("deleted problem " + intToString(id));
    problems.erase(problems.begin() + idx);
    cout << "\nProblem deleted successfully." << endl;
}


// ============================================================
// FUNCTION OVERLOADING  (search by keyword vs by ID)
// ============================================================

void searchProblem(const vector<Problem> &problems,
                    const vector<Platform> &platforms, string keyword)
{
    bool found = false;
    for (int i = 0; i < (int)problems.size(); i++)
    {
        if (containsIgnoreCase(problems[i].getTitle(), keyword) ||
            containsIgnoreCase(problems[i].getTopic(), keyword))
        {
            if (!found) printProblemHeader();
            printProblemRow(problems[i], platforms);
            found = true;
        }
    }
    if (!found) cout << "\nNo problem matches '" << keyword << "'." << endl;
}

void searchProblem(const vector<Problem> &problems,
                    const vector<Platform> &platforms, int problemID)
{
    int idx = findProblemIndex(problems, problemID);
    if (idx == -1) throw NotFoundException("problem " + intToString(problemID));
    problems[idx].display(platforms);
}

void searchProblemMenu(const vector<Problem> &problems, const vector<Platform> &platforms)
{
    cout << "\n1. Search by Problem ID";
    cout << "\n2. Search by Title / Topic keyword";
    int choice = getIntegerInRange("\nEnter choice: ", 1, 2);

    if (choice == 1)
    {
        int id = getInteger("Enter problem ID: ");
        searchProblem(problems, platforms, id);       // int overload
    }
    else
    {
        string keyword = getNonEmptyString("Enter keyword: ");
        searchProblem(problems, platforms, keyword);  // string overload
    }
}


// ============================================================
// STUDENT PLATFORM OPERATIONS
// ============================================================

void addStudentPlatform(vector<StudentPlatform> &links, const string &studentID,
                         const vector<Platform> &platforms)
{
    displayPlatforms(platforms);
    int platformID = getInteger("Enter platform ID from the list above: ");
    if (findPlatformIndex(platforms, platformID) == -1)
        throw NotFoundException("platform " + intToString(platformID));

    if (!isStudentPlatformUnique(links, studentID, platformID))
    {
        cout << "\nYou already have an account linked for this platform." << endl;
        return;
    }

    string site = getPlatformWebsite(platforms, platformID);
    string url = getValidURL("Enter your profile URL on this platform (or 'cancel'): ", site);
    if (url.empty()) { cout << "\nCancelled." << endl; return; }

    links.push_back(StudentPlatform(studentID, platformID, url));
    Logger::instance().log("linked platform " + intToString(platformID));
    cout << "\nPlatform account linked successfully." << endl;
}

void displayStudentPlatforms(const vector<StudentPlatform> &links,
                              const vector<Platform> &platforms,
                              const string &studentID)
{
    bool found = false;
    for (int i = 0; i < (int)links.size(); i++)
    {
        if (links[i].getStudentID() == studentID)
        {
            links[i].display(platforms);
            found = true;
        }
    }
    if (!found) cout << "\nNo linked platform accounts found." << endl;
}


// ============================================================
// ASSIGNMENT OPERATIONS  (Faculty assigns Problem to Student)
// ============================================================

void printAssignment(const Assignment &a, const vector<Problem> &problems,
                      const vector<Platform> &platforms,
                      const vector<Submission> &submissions)
{
    int pIdx = findProblemIndex(problems, a.getProblemID());

    cout << "\nAssignment ID : " << a.getAssignmentID();
    cout << "\nStudent ID    : " << a.getStudentID();
    cout << "\nAssigned By   : " << a.getFacultyID();
    if (pIdx != -1)
    {
        const Problem &p = problems[pIdx];
        cout << "\nProblem       : " << p.getID() << " - " << p.getTitle()
             << " [" << p.getDifficulty() << ", " << p.getTopic() << "]";
        cout << "\nPlatform      : " << getPlatformName(platforms, p.getPlatformID());
        cout << "\nProblem Link  : " << p.getURL();
    }
    else
    {
        cout << "\nProblem       : " << a.getProblemID() << " (no longer available)";
    }
    cout << "\nDeadline      : " << a.getDeadline();
    string state = assignmentState(submissions, a);
    cout << "\nState         : " << state;
    if (state == "Not Completed")
        cout << "  (" << daysBetween(todayDate(), a.getDeadline()) << " day(s) left)";
    cout << endl;
}

// Returns true if a new assignment was created (false if it already existed).
bool createAssignment(vector<Assignment> &assignments, const string &studentID,
                       int problemID, const string &facultyID, const string &deadline)
{
    if (hasAssignment(assignments, studentID, problemID)) return false;
    assignments.push_back(Assignment(nextAssignmentID(assignments), studentID,
                                      problemID, facultyID, deadline));
    return true;
}

void assignProblem(vector<Assignment> &assignments, const vector<Student> &students,
                    const vector<Problem> &problems, const string &facultyID)
{
    string studentID = toUpperStr(getNonEmptyString("Enter student ID: "));
    if (findStudentIndex(students, studentID) == -1)
        throw NotFoundException("student " + studentID);

    int problemID = getInteger("Enter problem ID: ");
    if (findProblemIndex(problems, problemID) == -1)
        throw NotFoundException("problem " + intToString(problemID));

    if (hasAssignment(assignments, studentID, problemID))
    {
        cout << "\nThis problem is already assigned to this student." << endl;
        return;
    }

    string deadline = getValidDate("Enter deadline (YYYY-MM-DD): ", true);
    createAssignment(assignments, studentID, problemID, facultyID, deadline);
    Logger::instance().log("assigned problem " + intToString(problemID) + " to " + studentID);
    cout << "\nProblem assigned successfully." << endl;
}

void assignProblemToDepartment(vector<Assignment> &assignments, const vector<Student> &students,
                                const vector<Problem> &problems, const string &facultyID)
{
    int problemID = getInteger("Enter problem ID: ");
    if (findProblemIndex(problems, problemID) == -1)
        throw NotFoundException("problem " + intToString(problemID));

    string dept = toUpperStr(getNonEmptyString("Enter department (e.g. CSE): "));
    string deadline = getValidDate("Enter deadline (YYYY-MM-DD): ", true);

    int added = 0;
    int skipped = 0;
    for (int i = 0; i < (int)students.size(); i++)
    {
        if (students[i].getDepartment() != dept) continue;
        if (createAssignment(assignments, students[i].getStudentID(), problemID, facultyID, deadline))
            added++;
        else
            skipped++;
    }

    if (added == 0 && skipped == 0)
        cout << "\nNo students found in department " << dept << "." << endl;
    else
    {
        Logger::instance().log("assigned problem " + intToString(problemID) +
                               " to department " + dept + " (" + intToString(added) + " students)");
        cout << "\nAssigned to " << added << " student(s); " << skipped
             << " already had this problem." << endl;
    }
}

void displayAssignmentsForStudent(const vector<Assignment> &assignments,
                                   const vector<Problem> &problems,
                                   const vector<Platform> &platforms,
                                   const vector<Submission> &submissions,
                                   const string &studentID)
{
    bool found = false;
    for (int i = 0; i < (int)assignments.size(); i++)
    {
        if (assignments[i].getStudentID() == studentID)
        {
            printAssignment(assignments[i], problems, platforms, submissions);
            found = true;
        }
    }
    if (!found) cout << "\nNo assignments found for this student." << endl;
}

void displayAssignmentsForFaculty(const vector<Assignment> &assignments,
                                   const vector<Problem> &problems,
                                   const vector<Platform> &platforms,
                                   const vector<Submission> &submissions,
                                   const string &facultyID)
{
    bool found = false;
    for (int i = 0; i < (int)assignments.size(); i++)
    {
        if (assignments[i].getFacultyID() == facultyID)
        {
            printAssignment(assignments[i], problems, platforms, submissions);
            found = true;
        }
    }
    if (!found) cout << "\nYou have not assigned any problems yet." << endl;
}


// ============================================================
// SUBMISSION OPERATIONS
// ============================================================

void recordSubmission(vector<Submission> &submissions,
                       const vector<Assignment> &assignments,
                       const vector<Problem> &problems,
                       const vector<Platform> &platforms,
                       const string &studentID)
{
    int problemID = getInteger("Enter the problem ID you solved: ");

    // A submission must correspond to a real assignment.
    int aIdx = findAssignmentIndex(assignments, studentID, problemID);
    if (aIdx == -1)
    {
        cout << "\nThis problem was not assigned to you. Cannot record submission." << endl;
        return;
    }

    int pIdx = findProblemIndex(problems, problemID);
    if (pIdx == -1) throw NotFoundException("problem " + intToString(problemID));

    if (hasAcceptedSubmission(submissions, studentID, problemID))
    {
        cout << "\nThis problem is already marked Accepted. Nothing more to submit." << endl;
        return;
    }

    int platformID = problems[pIdx].getPlatformID();
    string site = getPlatformWebsite(platforms, platformID);
    cout << "\nProblem  : " << problems[pIdx].getTitle();
    cout << "\nPlatform : " << getPlatformName(platforms, platformID) << endl;

    if (todayDate() > assignments[aIdx].getDeadline())
        cout << "Note: the deadline (" << assignments[aIdx].getDeadline()
             << ") has passed - an accepted late solution earns half points." << endl;

    string url = getValidURL("Enter your submission URL (or 'cancel'): ", site);
    if (url.empty()) { cout << "\nCancelled." << endl; return; }

    // Students cannot mark their own work as Accepted - it starts as Pending
    // and only the faculty who assigned the problem can review it.
    int id = nextSubmissionID(submissions);
    submissions.push_back(Submission(id, studentID, problemID, platformID,
                                      url, todayDate(), "Pending"));
    Logger::instance().log("recorded submission " + intToString(id) +
                           " for problem " + intToString(problemID));
    cout << "\nSubmission recorded (ID " << id << "). Status: Pending faculty review." << endl;
}

void displaySubmissionsForStudent(const vector<Submission> &submissions,
                                   const vector<Platform> &platforms,
                                   const string &studentID)
{
    bool found = false;
    for (int i = 0; i < (int)submissions.size(); i++)
    {
        if (submissions[i].getStudentID() == studentID)
        {
            submissions[i].display(platforms);
            found = true;
        }
    }
    if (!found) cout << "\nNo submissions found for this student." << endl;
}

// Shows submissions for problems this faculty assigned.
void displaySubmissionsForFaculty(const vector<Submission> &submissions,
                                   const vector<Assignment> &assignments,
                                   const vector<Platform> &platforms,
                                   const string &facultyID, bool onlyPending)
{
    bool found = false;
    for (int i = 0; i < (int)submissions.size(); i++)
    {
        if (!facultyAssignedProblem(assignments, facultyID,
                                     submissions[i].getStudentID(), submissions[i].getProblemID()))
            continue;

        if (onlyPending && submissions[i].getStatus() != "Pending" &&
            submissions[i].getStatus() != "In Progress")
            continue;

        submissions[i].display(platforms);
        found = true;
    }
    if (!found)
        cout << (onlyPending ? "\nNo submissions are waiting for your review."
                             : "\nNo submissions found for your assignments.") << endl;
}

void reviewSubmission(vector<Submission> &submissions,
                       const vector<Assignment> &assignments,
                       const vector<Platform> &platforms,
                       const string &facultyID)
{
    int id = getInteger("Enter submission ID to review: ");
    int idx = findSubmissionIndex(submissions, id);
    if (idx == -1) throw NotFoundException("submission " + intToString(id));

    if (!facultyAssignedProblem(assignments, facultyID,
                                 submissions[idx].getStudentID(), submissions[idx].getProblemID()))
    {
        cout << "\nYou did not assign this problem to this student, so you cannot review it." << endl;
        return;
    }

    submissions[idx].display(platforms);

    cout << "\n1. Pending  2. In Progress  3. Accepted  4. Rejected";
    int choice = getInteger("\nEnter new status: ");

    string newStatus;
    if (choice == 1) newStatus = "Pending";
    else if (choice == 2) newStatus = "In Progress";
    else if (choice == 3) newStatus = "Accepted";
    else if (choice == 4) newStatus = "Rejected";
    else { cout << "Invalid choice." << endl; return; }

    submissions[idx].updateStatus(newStatus);    // may throw ValidationException
    Logger::instance().log("set submission " + intToString(id) + " to " + newStatus);
    cout << "\nSubmission status updated." << endl;
}


// ============================================================
// PROGRESS + REPORT + EXPORT
// ============================================================

void studentProgress(const vector<Assignment> &assignments,
                      const vector<Submission> &submissions,
                      const vector<Problem> &problems,
                      const string &studentID)
{
    ProgressStats s = calculateProgress(assignments, submissions, studentID);

    if (s.total == 0)
    {
        cout << "\nNo assignments found for this student." << endl;
        return;
    }

    double percentage = (double)s.completed / s.total * 100;

    cout << "\n===== STUDENT PROGRESS =====" << endl;
    cout << "Student ID   : " << studentID << endl;
    cout << "Assigned     : " << s.total << endl;
    cout << "Accepted     : " << s.completed << endl;
    cout << "Under Review : " << s.underReview << endl;
    cout << "Overdue      : " << s.overdue << endl;
    cout << "Progress     : " << percentage << "%" << endl;
    cout << "Points       : " << studentScore(assignments, submissions, problems, studentID) << endl;
}

void generateReport(const vector<Student> &students,
                     const vector<Assignment> &assignments,
                     const vector<Submission> &submissions,
                     const vector<Problem> &problems)
{
    ofstream file("student_report.txt");
    if (!file) { cout << "\nUnable to create report." << endl; return; }

    vector<Student> ranked = students;
    sort(ranked.begin(), ranked.end());

    file << fixed << setprecision(2);
    file << "CODING PLATFORM - STUDENT REPORT (ranked by CGPA)\n";
    file << "Generated on: " << todayDate() << "\n";
    file << "==================================================\n\n";

    for (int i = 0; i < (int)ranked.size(); i++)
    {
        ProgressStats s = calculateProgress(assignments, submissions, ranked[i].getStudentID());

        file << "Rank        : " << getStudentRank(ranked, ranked[i].getStudentID()) << "\n";
        file << "Student     : " << ranked[i].getName() << "\n";
        file << "ID          : " << ranked[i].getStudentID() << "\n";
        file << "Department  : " << ranked[i].getDepartment()
             << " (Year " << ranked[i].getYear() << ")\n";
        file << "CGPA        : " << ranked[i].getCGPA() << "\n";
        file << "Timetable   : " << Timetable::describe(ranked[i].getTimetableNo()) << "\n";
        file << "Assigned    : " << s.total << "\n";
        file << "Accepted    : " << s.completed << "\n";
        file << "Under Review: " << s.underReview << "\n";
        file << "Overdue     : " << s.overdue << "\n";
        file << "Points      : " << studentScore(assignments, submissions, problems,
                                                 ranked[i].getStudentID()) << "\n";

        if (s.total > 0)
            file << "Progress    : " << ((double)s.completed / s.total * 100) << "%\n\n";
        else
            file << "Progress    : N/A (nothing assigned)\n\n";
    }

    file.close();
    Logger::instance().log("generated student_report.txt");
    cout << "\nReport generated as student_report.txt" << endl;
}

void exportCSV(const vector<Student> &students,
                const vector<Assignment> &assignments,
                const vector<Submission> &submissions,
                const vector<Problem> &problems)
{
    ofstream file("students_export.csv");
    if (!file) { cout << "\nUnable to create CSV file." << endl; return; }

    vector<Student> ranked = students;
    sort(ranked.begin(), ranked.end());

    file << fixed << setprecision(2);
    file << "Rank,StudentID,Name,Department,Year,CGPA,Assigned,Accepted,Overdue,Points\n";
    for (int i = 0; i < (int)ranked.size(); i++)
    {
        string id = ranked[i].getStudentID();
        ProgressStats s = calculateProgress(assignments, submissions, id);
        file << getStudentRank(ranked, id) << "," << id << ",\"" << ranked[i].getName()
             << "\"," << ranked[i].getDepartment() << "," << ranked[i].getYear() << ","
             << ranked[i].getCGPA() << "," << s.total << "," << s.completed << ","
             << s.overdue << ","
             << studentScore(assignments, submissions, problems, id) << "\n";
    }
    file.close();
    Logger::instance().log("exported students_export.csv");
    cout << "\nData exported to students_export.csv (opens in Excel)." << endl;
}

void viewActivityLog()
{
    ifstream file("activity.log");
    if (!file) { cout << "\nNo activity recorded yet." << endl; return; }

    vector<string> lines;
    string line;
    while (getline(file, line)) lines.push_back(line);

    if (lines.empty()) { cout << "\nNo activity recorded yet." << endl; return; }

    size_t start = lines.size() > 30 ? lines.size() - 30 : 0;
    cout << "\n===== ACTIVITY LOG (latest " << (lines.size() - start) << " entries) =====" << endl;
    for (size_t i = start; i < lines.size(); i++) cout << lines[i] << endl;
}

// Admin: removes a user's credential so they log in with the default password again.
void resetUserPassword(vector<Credential> &creds, const vector<Student> &students,
                        const vector<Faculty> &faculty)
{
    cout << "\n1. Student  2. Faculty";
    int type = getIntegerInRange("\nEnter user type: ", 1, 2);
    string id = toUpperStr(getNonEmptyString("Enter user ID: "));

    string role = (type == 1) ? "Student" : "Faculty";
    bool exists = (type == 1) ? (findStudentIndex(students, id) != -1)
                              : (findFacultyIndex(faculty, id) != -1);
    if (!exists) throw NotFoundException(role + " " + id);

    removeCredential(creds, role, id);
    Logger::instance().log("reset password of " + role + " " + id);
    cout << "\nPassword reset. The user must log in with the default password "
         << "and choose a new one." << endl;
}


// ============================================================
// DEFAULT (SEED) DATA - used on the very first run
// ============================================================

void seedPlatforms(vector<Platform> &platforms)
{
    platforms.push_back(Platform(1, "LeetCode",      "https://leetcode.com"));
    platforms.push_back(Platform(2, "CodeChef",      "https://www.codechef.com"));
    platforms.push_back(Platform(3, "HackerRank",    "https://www.hackerrank.com"));
    platforms.push_back(Platform(4, "GeeksforGeeks", "https://www.geeksforgeeks.org"));
    platforms.push_back(Platform(5, "Codeforces",    "https://codeforces.com"));
}

void seedStudents(vector<Student> &s)
{
    //                  name              ID            email                        yr  dept    TT  CGPA
    s.push_back(Student("Jahnavi",        "25B11AI586", "jahnavi@gmail.com",          2, "AIML", 5, 9.12));
    s.push_back(Student("Rahul Sharma",   "25B11CS501", "rahul.sharma@gmail.com",     2, "CSE",  6, 8.45));
    s.push_back(Student("Priya Reddy",    "25B11CS502", "priya.reddy@gmail.com",      2, "CSE",  5, 9.35));
    s.push_back(Student("Arjun Verma",    "25B11CS503", "arjun.verma@gmail.com",      3, "CSE",  7, 7.80));
    s.push_back(Student("Sneha Iyer",     "25B11AI587", "sneha.iyer@gmail.com",       2, "AIML", 5, 8.90));
    s.push_back(Student("Karthik Rao",    "25B11AI588", "karthik.rao@gmail.com",      1, "AIML", 1, 8.15));
    s.push_back(Student("Ananya Gupta",   "25B11EC601", "ananya.gupta@gmail.com",     2, "ECE",  3, 8.72));
    s.push_back(Student("Vikram Singh",   "25B11EC602", "vikram.singh@gmail.com",     3, "ECE",  8, 7.35));
    s.push_back(Student("Divya Nair",     "25B11IT701", "divya.nair@gmail.com",       2, "IT",   6, 9.05));
    s.push_back(Student("Rohan Mehta",    "25B11IT702", "rohan.mehta@gmail.com",      1, "IT",   2, 7.60));
    s.push_back(Student("Kavya Pillai",   "25B11ME801", "kavya.pillai@gmail.com",     4, "ME",   9, 8.30));
    s.push_back(Student("Aditya Kumar",   "25B11ME802", "aditya.kumar@gmail.com",     3, "ME",   7, 6.95));
    s.push_back(Student("Meera Joshi",    "25B11CS504", "meera.joshi@gmail.com",      1, "CSE",  1, 9.60));
    s.push_back(Student("Siddharth Rao",  "25B11CS505", "siddharth.rao@gmail.com",    4, "CSE",  9, 8.05));
    s.push_back(Student("Pooja Desai",    "25B11AI589", "pooja.desai@gmail.com",      3, "AIML", 8, 8.60));
    s.push_back(Student("Naveen Chandra", "25B11EC603", "naveen.chandra@gmail.com",   1, "ECE",  2, 7.10));
    s.push_back(Student("Ishita Bose",    "25B11IT703", "ishita.bose@gmail.com",      4, "IT",   9, 9.25));
    s.push_back(Student("Manoj Pillai",   "25B11ME803", "manoj.pillai@gmail.com",     2, "ME",   4, 6.70));
    s.push_back(Student("Ritika Agarwal", "25B11CS506", "ritika.agarwal@gmail.com",   3, "CSE",  8, 8.85));
    s.push_back(Student("Suresh Babu",    "25B11AI590", "suresh.babu@gmail.com",      1, "AIML", 3, 7.45));

    sort(s.begin(), s.end());
}

void seedFaculty(vector<Faculty> &f)
{
    //                  name                       ID      email                       dept    subject                          course
    f.push_back(Faculty("Abhilash",                "F101", "abhilash@gmail.com",       "AIML", "Data Structures",               "CS201"));
    f.push_back(Faculty("Dr. Lakshmi Narayan",     "F102", "lakshmi.narayan@gmail.com","CSE",  "Design & Analysis of Algorithms","CS301"));
    f.push_back(Faculty("Prof. Ramesh Kulkarni",   "F103", "ramesh.kulkarni@gmail.com","CSE",  "Database Management Systems",   "CS305"));
    f.push_back(Faculty("Dr. Sunita Menon",        "F104", "sunita.menon@gmail.com",   "AIML", "Machine Learning",              "AI302"));
    f.push_back(Faculty("Prof. Harish Patel",      "F105", "harish.patel@gmail.com",   "IT",   "Web Technologies",              "IT210"));
    f.push_back(Faculty("Dr. Meenakshi Sundaram",  "F106", "meenakshi.s@gmail.com",    "IT",   "Computer Networks",             "IT305"));
    f.push_back(Faculty("Prof. Anil Deshmukh",     "F107", "anil.deshmukh@gmail.com",  "ECE",  "Digital Signal Processing",     "EC303"));
    f.push_back(Faculty("Dr. Farah Khan",          "F108", "farah.khan@gmail.com",     "ECE",  "Embedded Systems",              "EC401"));
    f.push_back(Faculty("Prof. Venkat Subramanian","F109", "venkat.s@gmail.com",       "ME",   "Numerical Methods",             "ME205"));
    f.push_back(Faculty("Dr. Rekha Balan",         "F110", "rekha.balan@gmail.com",    "ME",   "Engineering Mathematics",       "MA201"));
    f.push_back(Faculty("Prof. Sandeep Joshi",     "F111", "sandeep.joshi@gmail.com",  "CSE",  "Operating Systems",             "CS303"));
    f.push_back(Faculty("Dr. Pallavi Rao",         "F112", "pallavi.rao@gmail.com",    "AIML", "Deep Learning",                 "AI401"));
}

void seedProblems(vector<Problem> &p)
{
    const string LC = "https://leetcode.com/problems/";
    const string GG = "https://www.geeksforgeeks.org/problems/";
    const string HR = "https://www.hackerrank.com/challenges/";
    const string CC = "https://www.codechef.com/problems/";
    const string CF = "https://codeforces.com/problemset/problem/";

    //                  ID   title                                           level     plat topic                        link
    p.push_back(Problem(101, "Two Sum",                                      "Easy",   1, "Arrays",                     LC + "two-sum/"));
    p.push_back(Problem(102, "Binary Search",                                "Medium", 4, "Searching",                  GG + "binary-search-1587115620/1"));
    p.push_back(Problem(103, "Valid Parentheses",                            "Easy",   1, "Stacks",                     LC + "valid-parentheses/"));
    p.push_back(Problem(104, "Reverse a Linked List",                        "Easy",   4, "Linked Lists",               GG + "reverse-a-linked-list/1"));
    p.push_back(Problem(105, "Maximum Depth of Binary Tree",                 "Easy",   1, "Binary Trees",               LC + "maximum-depth-of-binary-tree/"));
    p.push_back(Problem(106, "Climbing Stairs",                              "Easy",   1, "Dynamic Programming",        LC + "climbing-stairs/"));
    p.push_back(Problem(107, "Number of Islands",                            "Medium", 1, "Graphs",                     LC + "number-of-islands/"));
    p.push_back(Problem(108, "Rat in a Maze Problem - I",                    "Medium", 4, "Backtracking",               GG + "rat-in-a-maze-problem/1"));
    p.push_back(Problem(109, "Longest Substring Without Repeating Characters","Medium",1, "Sliding Window",            LC + "longest-substring-without-repeating-characters/"));
    p.push_back(Problem(110, "Solve Me First",                               "Easy",   3, "Input/Output Basics",        HR + "solve-me-first/problem"));
    p.push_back(Problem(111, "Diagonal Difference",                          "Easy",   3, "Matrix Traversal",           HR + "diagonal-difference/problem"));
    p.push_back(Problem(112, "Container With Most Water",                    "Medium", 1, "Two Pointers",               LC + "container-with-most-water/"));
    p.push_back(Problem(113, "Kth Largest Element in an Array",              "Medium", 1, "Heaps",                      LC + "kth-largest-element-in-an-array/"));
    p.push_back(Problem(114, "Implement Trie (Prefix Tree)",                 "Medium", 1, "Tries",                      LC + "implement-trie-prefix-tree/"));
    p.push_back(Problem(115, "Implement Queue using Stacks",                 "Easy",   1, "Queues",                     LC + "implement-queue-using-stacks/"));
    p.push_back(Problem(116, "Detect Loop in Linked List",                   "Medium", 4, "Floyd's Cycle Detection",    GG + "detect-loop-in-linked-list/1"));
    p.push_back(Problem(117, "Kadane's Algorithm",                           "Medium", 4, "Subarray Optimization",      GG + "kadanes-algorithm-1587115620/1"));
    p.push_back(Problem(118, "Lonely Integer",                               "Easy",   3, "Bit Manipulation",           HR + "lonely-integer/problem"));
    p.push_back(Problem(119, "Sparse Arrays",                                "Medium", 3, "Hashing",                    HR + "sparse-arrays/problem"));
    p.push_back(Problem(120, "Mark and Toys",                                "Easy",   3, "Greedy",                     HR + "mark-and-toys/problem"));
    p.push_back(Problem(121, "Add Two Numbers",                              "Easy",   2, "Basic Math",                 CC + "FLOW001"));
    p.push_back(Problem(122, "Prime Generator",                              "Medium", 2, "Number Theory",              CC + "PRIME1"));
    p.push_back(Problem(123, "Turbo Sort",                                   "Medium", 2, "Sorting",                    CC + "TSORT"));
    p.push_back(Problem(124, "Reverse The Number",                           "Easy",   2, "Digit Manipulation",         CC + "FLOW007"));
    p.push_back(Problem(125, "GCD and LCM",                                  "Easy",   2, "Euclidean Algorithm",        CC + "FLOW016"));
    p.push_back(Problem(126, "ATM",                                          "Easy",   2, "Conditional Logic",          CC + "HS08TEST"));
    p.push_back(Problem(127, "Watermelon",                                   "Easy",   5, "Math & Parity",              CF + "4/A"));
    p.push_back(Problem(128, "Way Too Long Words",                           "Easy",   5, "String Manipulation",        CF + "71/A"));
    p.push_back(Problem(129, "Bit++",                                        "Easy",   5, "Implementation",             CF + "282/A"));
    p.push_back(Problem(130, "Domino piling",                                "Easy",   5, "Tiling & Constructive Logic",CF + "50/A"));
    p.push_back(Problem(131, "Beautiful Matrix",                             "Easy",   5, "Grid Distance",              CF + "263/A"));
}

void seedAssignments(vector<Assignment> &a)
{
    //                     id  student        problem faculty  deadline
    a.push_back(Assignment(1,  "25B11AI586", 101, "F101", "2026-12-31"));
    a.push_back(Assignment(2,  "25B11AI586", 103, "F101", "2026-12-31"));
    a.push_back(Assignment(3,  "25B11AI587", 101, "F101", "2026-12-31"));
    a.push_back(Assignment(4,  "25B11AI588", 104, "F101", "2026-11-30"));
    a.push_back(Assignment(5,  "25B11CS501", 106, "F102", "2026-11-30"));
    a.push_back(Assignment(6,  "25B11CS502", 107, "F102", "2026-09-10"));
    a.push_back(Assignment(7,  "25B11CS503", 109, "F102", "2026-12-15"));
    a.push_back(Assignment(8,  "25B11CS504", 114, "F102", "2026-12-15"));
    a.push_back(Assignment(9,  "25B11AI589", 119, "F104", "2026-12-10"));
    a.push_back(Assignment(10, "25B11IT701", 110, "F105", "2026-10-31"));
    a.push_back(Assignment(11, "25B11IT702", 111, "F106", "2026-11-05"));
    a.push_back(Assignment(12, "25B11EC601", 121, "F107", "2026-10-31"));
    a.push_back(Assignment(13, "25B11EC602", 127, "F108", "2026-11-20"));
    a.push_back(Assignment(14, "25B11ME801", 122, "F109", "2026-11-15"));
    a.push_back(Assignment(15, "25B11ME802", 125, "F110", "2026-11-15"));
}

void seedSubmissions(vector<Submission> &s)
{
    // Sample data only - the submission links are placeholders.
    s.push_back(Submission(1, "25B11AI586", 101, 1, "https://leetcode.com/submissions/detail/1001/", "2026-09-12", "Accepted"));
    s.push_back(Submission(2, "25B11AI586", 103, 1, "https://leetcode.com/submissions/detail/1002/", "2026-09-18", "Pending"));
    s.push_back(Submission(3, "25B11AI587", 101, 1, "https://leetcode.com/submissions/detail/1003/", "2026-09-14", "Accepted"));
    s.push_back(Submission(4, "25B11CS501", 106, 1, "https://leetcode.com/submissions/detail/1004/", "2026-09-16", "Accepted"));
    s.push_back(Submission(5, "25B11CS502", 107, 1, "https://leetcode.com/submissions/detail/1005/", "2026-09-08", "Rejected"));
    s.push_back(Submission(6, "25B11IT701", 110, 3, "https://www.hackerrank.com/challenges/solve-me-first/submissions/code/1006", "2026-09-15", "Accepted"));
    s.push_back(Submission(7, "25B11EC601", 121, 2, "https://www.codechef.com/viewsolution/1007", "2026-09-17", "In Progress"));
}

void seedStudentPlatforms(vector<StudentPlatform> &l)
{
    l.push_back(StudentPlatform("25B11AI586", 1, "https://leetcode.com/u/jahnavi/"));
    l.push_back(StudentPlatform("25B11AI586", 5, "https://codeforces.com/profile/jahnavi"));
    l.push_back(StudentPlatform("25B11CS502", 1, "https://leetcode.com/u/priya_reddy/"));
}

void seedDefaultData(DataStore &db)
{
    seedPlatforms(db.platforms.all());
    seedStudents(db.students.all());
    seedFaculty(db.faculty.all());
    seedProblems(db.problems.all());
    seedAssignments(db.assignments.all());
    seedSubmissions(db.submissions.all());
    seedStudentPlatforms(db.links.all());
}


// ============================================================
// LOGIN  (hashed passwords, forced first-login change, 3 attempts)
// ============================================================

// Returns true on success and stores the validated ID in idOut.
bool loginFlow(DataStore &db, const string &role, string &idOut)
{
    const int MAX_ATTEMPTS = 3;

    for (int attempt = 1; attempt <= MAX_ATTEMPTS; attempt++)
    {
        string id;
        if (role == "Admin") id = toLowerStr(getString("Username: "));
        else id = toUpperStr(getString(role + " ID: "));
        string password = getString("Password: ");

        bool exists;
        if (role == "Admin") exists = (id == "admin");
        else if (role == "Faculty") exists = (findFacultyIndex(db.faculty.all(), id) != -1);
        else exists = (findStudentIndex(db.students.all(), id) != -1);

        bool needsChange = false;
        if (exists && authenticate(db.credentials.all(), role, id, password, needsChange))
        {
            Logger::instance().setActor(role + " " + id);
            Logger::instance().log("logged in");
            if (needsChange)
            {
                cout << "\nFirst login (or password reset): please choose a new password." << endl;
                setNewPassword(db.credentials.all(), role, id);
            }
            db.credentials.save();
            idOut = id;
            return true;
        }

        Logger::instance().setActor("anonymous");
        Logger::instance().log("failed " + role + " login for '" + id + "'");
        cout << "\nInvalid " << toLowerStr(role) << " login. Attempts left: "
             << (MAX_ATTEMPTS - attempt) << endl;
    }

    cout << "\nToo many failed attempts. Returning to the main menu." << endl;
    return false;
}


// ============================================================
// STUDENT MENU
// ============================================================

void studentMenu(DataStore &db, const string &studentID)
{
    vector<Student> &students = db.students.all();
    int idx = findStudentIndex(students, studentID);

    // Runtime polymorphism: the same Person pointer type serves every role.
    Person *user = &students[idx];
    cout << "\nWelcome, " << user->getName() << " [" << user->role() << "]!" << endl;

    deadlineReminders(db.assignments.all(), db.submissions.all(), db.problems.all(), studentID);

    while (true)
    {
        cout << "\n\n===== STUDENT MENU =====";
        cout << "\n1.  View My Profile";
        cout << "\n2.  View My Linked Platforms";
        cout << "\n3.  Link a New Platform Account";
        cout << "\n4.  View My Assigned Problems";
        cout << "\n5.  Search Problems";
        cout << "\n6.  Record a Submission";
        cout << "\n7.  View My Submissions";
        cout << "\n8.  View My Progress";
        cout << "\n9.  My Topic Analytics";
        cout << "\n10. Recommended Practice Problems";
        cout << "\n11. Leaderboard";
        cout << "\n12. Deadline Reminders";
        cout << "\n13. Change Password";
        cout << "\n14. Logout";

        int choice = getInteger("\nEnter choice: ");

        try
        {
            if (choice == 1)
            {
                user->display();
                cout << "CGPA Rank  : " << getStudentRank(students, studentID)
                     << " of " << students.size() << endl;
            }
            else if (choice == 2) displayStudentPlatforms(db.links.all(), db.platforms.all(), studentID);
            else if (choice == 3) addStudentPlatform(db.links.all(), studentID, db.platforms.all());
            else if (choice == 4)
                displayAssignmentsForStudent(db.assignments.all(), db.problems.all(),
                                             db.platforms.all(), db.submissions.all(), studentID);
            else if (choice == 5) searchProblemMenu(db.problems.all(), db.platforms.all());
            else if (choice == 6)
            {
                displayAssignmentsForStudent(db.assignments.all(), db.problems.all(),
                                             db.platforms.all(), db.submissions.all(), studentID);
                recordSubmission(db.submissions.all(), db.assignments.all(), db.problems.all(),
                                 db.platforms.all(), studentID);
            }
            else if (choice == 7) displaySubmissionsForStudent(db.submissions.all(), db.platforms.all(), studentID);
            else if (choice == 8)
            {
                studentSummary(students[idx]);
                studentProgress(db.assignments.all(), db.submissions.all(), db.problems.all(), studentID);
            }
            else if (choice == 9)
                topicAnalytics(db.assignments.all(), db.submissions.all(), db.problems.all(), studentID);
            else if (choice == 10)
                recommendProblems(db.problems.all(), db.platforms.all(), db.assignments.all(),
                                  db.submissions.all(), studentID);
            else if (choice == 11)
                displayLeaderboard(students, db.assignments.all(), db.submissions.all(), db.problems.all());
            else if (choice == 12)
                deadlineReminders(db.assignments.all(), db.submissions.all(), db.problems.all(), studentID);
            else if (choice == 13) changePasswordMenu(db.credentials.all(), "Student", studentID);
            else if (choice == 14)
            {
                Logger::instance().log("logged out");
                cout << "\nLogged out." << endl;
                db.saveAll();
                break;
            }
            else cout << "\nInvalid choice." << endl;
        }
        catch (const AppException &e)
        {
            cout << "\nError: " << e.what() << endl;
        }
        catch (const exception &e)
        {
            cout << "\nUnexpected error: " << e.what() << endl;
        }

        db.saveAll();    // auto-save after every action
    }
}


// ============================================================
// FACULTY MENU
// ============================================================

void facultyMenu(DataStore &db, const string &facultyID)
{
    vector<Faculty> &faculty = db.faculty.all();
    int fIdx = findFacultyIndex(faculty, facultyID);

    Person *user = &faculty[fIdx];
    cout << "\nWelcome, " << user->getName() << " [" << user->role() << ", "
         << faculty[fIdx].getSubject() << ", " << faculty[fIdx].getDepartment() << "]" << endl;

    while (true)
    {
        cout << "\n\n===== FACULTY MENU =====";
        cout << "\n1.  Add Coding Problem (with link)";
        cout << "\n2.  View All Problems";
        cout << "\n3.  View Problems by Platform";
        cout << "\n4.  Search Problems";
        cout << "\n5.  Delete Problem";
        cout << "\n6.  View Platforms";
        cout << "\n7.  Add Platform";
        cout << "\n8.  Assign Problem to a Student";
        cout << "\n9.  Assign Problem to a Whole Department";
        cout << "\n10. View My Assignments";
        cout << "\n11. View Submissions Waiting for Review";
        cout << "\n12. View All Submissions for My Assignments";
        cout << "\n13. Review a Submission (Accept / Reject)";
        cout << "\n14. View Student Progress";
        cout << "\n15. Generate Report";
        cout << "\n16. Leaderboard";
        cout << "\n17. Department Analytics";
        cout << "\n18. Export Data to CSV";
        cout << "\n19. Change Password";
        cout << "\n20. Logout";

        int choice = getInteger("\nEnter choice: ");

        try
        {
            if (choice == 1) addProblem(db.problems.all(), db.platforms.all());
            else if (choice == 2) displayProblems(db.problems.all(), db.platforms.all());
            else if (choice == 3) displayProblemsByPlatform(db.problems.all(), db.platforms.all());
            else if (choice == 4) searchProblemMenu(db.problems.all(), db.platforms.all());
            else if (choice == 5) deleteProblem(db.problems.all(), db.assignments.all());
            else if (choice == 6) displayPlatforms(db.platforms.all());
            else if (choice == 7) addPlatform(db.platforms.all());
            else if (choice == 8)
                assignProblem(db.assignments.all(), db.students.all(), db.problems.all(), facultyID);
            else if (choice == 9)
                assignProblemToDepartment(db.assignments.all(), db.students.all(), db.problems.all(), facultyID);
            else if (choice == 10)
                displayAssignmentsForFaculty(db.assignments.all(), db.problems.all(), db.platforms.all(),
                                             db.submissions.all(), facultyID);
            else if (choice == 11)
                displaySubmissionsForFaculty(db.submissions.all(), db.assignments.all(),
                                             db.platforms.all(), facultyID, true);
            else if (choice == 12)
                displaySubmissionsForFaculty(db.submissions.all(), db.assignments.all(),
                                             db.platforms.all(), facultyID, false);
            else if (choice == 13)
                reviewSubmission(db.submissions.all(), db.assignments.all(), db.platforms.all(), facultyID);
            else if (choice == 14)
            {
                string id = toUpperStr(getNonEmptyString("Enter student ID: "));
                if (findStudentIndex(db.students.all(), id) == -1)
                    throw NotFoundException("student " + id);
                studentProgress(db.assignments.all(), db.submissions.all(), db.problems.all(), id);
            }
            else if (choice == 15)
                generateReport(db.students.all(), db.assignments.all(), db.submissions.all(), db.problems.all());
            else if (choice == 16)
                displayLeaderboard(db.students.all(), db.assignments.all(), db.submissions.all(), db.problems.all());
            else if (choice == 17)
                departmentAnalytics(db.students.all(), db.assignments.all(), db.submissions.all(), db.problems.all());
            else if (choice == 18)
                exportCSV(db.students.all(), db.assignments.all(), db.submissions.all(), db.problems.all());
            else if (choice == 19) changePasswordMenu(db.credentials.all(), "Faculty", facultyID);
            else if (choice == 20)
            {
                Logger::instance().log("logged out");
                cout << "\nLogged out." << endl;
                db.saveAll();
                break;
            }
            else cout << "\nInvalid choice." << endl;
        }
        catch (const AppException &e)
        {
            cout << "\nError: " << e.what() << endl;
        }
        catch (const exception &e)
        {
            cout << "\nUnexpected error: " << e.what() << endl;
        }

        db.saveAll();
    }
}


// ============================================================
// ADMIN MENU
// ============================================================

void adminMenu(DataStore &db)
{
    Admin adminUser;
    Person *user = &adminUser;
    cout << "\nWelcome, " << user->getName() << " [" << user->role() << "]" << endl;

    while (true)
    {
        cout << "\n\n===== ADMIN MENU =====";
        cout << "\n1.  Add Student";
        cout << "\n2.  View Students (ranked by CGPA)";
        cout << "\n3.  View Students by Department";
        cout << "\n4.  View Students by Timetable";
        cout << "\n5.  Update Student";
        cout << "\n6.  Delete Student";
        cout << "\n7.  Search Student (ID / Name)";
        cout << "\n8.  Sort Students (CGPA / Name / Department)";
        cout << "\n9.  Add Faculty";
        cout << "\n10. View Faculty";
        cout << "\n11. Delete Faculty";
        cout << "\n12. View Platforms";
        cout << "\n13. Delete Platform";
        cout << "\n14. View Problems (oversight)";
        cout << "\n15. View University Timetables";
        cout << "\n16. Leaderboard";
        cout << "\n17. Department Analytics";
        cout << "\n18. View Activity Log";
        cout << "\n19. Reset a User's Password";
        cout << "\n20. Backup All Data";
        cout << "\n21. Export Data to CSV";
        cout << "\n22. Change My Password";
        cout << "\n23. Logout";

        int choice = getInteger("\nEnter choice: ");

        try
        {
            if (choice == 1) addStudent(db.students.all());
            else if (choice == 2) displayStudents(db.students.all());
            else if (choice == 3) displayStudentsByDepartment(db.students.all());
            else if (choice == 4) displayStudentsByTimetable(db.students.all());
            else if (choice == 5) updateStudent(db.students.all());
            else if (choice == 6)
                deleteStudent(db.students.all(), db.assignments.all(), db.submissions.all(),
                              db.links.all(), db.credentials.all());
            else if (choice == 7) searchStudent(db.students.all());
            else if (choice == 8) sortStudents(db.students.all());
            else if (choice == 9) addFaculty(db.faculty.all());
            else if (choice == 10) displayFaculty(db.faculty.all());
            else if (choice == 11) deleteFaculty(db.faculty.all(), db.assignments.all(), db.credentials.all());
            else if (choice == 12) displayPlatforms(db.platforms.all());
            else if (choice == 13) deletePlatform(db.platforms.all(), db.problems.all(), db.links.all());
            else if (choice == 14) displayProblems(db.problems.all(), db.platforms.all());
            else if (choice == 15) Timetable::printAll();
            else if (choice == 16)
                displayLeaderboard(db.students.all(), db.assignments.all(), db.submissions.all(), db.problems.all());
            else if (choice == 17)
                departmentAnalytics(db.students.all(), db.assignments.all(), db.submissions.all(), db.problems.all());
            else if (choice == 18) viewActivityLog();
            else if (choice == 19)
                resetUserPassword(db.credentials.all(), db.students.all(), db.faculty.all());
            else if (choice == 20)
            {
                db.saveAll();
                int n = db.backupAll();
                Logger::instance().log("created backup (" + intToString(n) + " files)");
                cout << "\nBackup complete: " << n << " file(s) copied as *.bak" << endl;
            }
            else if (choice == 21)
                exportCSV(db.students.all(), db.assignments.all(), db.submissions.all(), db.problems.all());
            else if (choice == 22) changePasswordMenu(db.credentials.all(), "Admin", "admin");
            else if (choice == 23)
            {
                Logger::instance().log("logged out");
                cout << "\nLogged out." << endl;
                db.saveAll();
                break;
            }
            else cout << "\nInvalid choice." << endl;
        }
        catch (const AppException &e)
        {
            cout << "\nError: " << e.what() << endl;
        }
        catch (const exception &e)
        {
            cout << "\nUnexpected error: " << e.what() << endl;
        }

        db.saveAll();
    }
}


// ============================================================
// MAIN
// ============================================================

int main()
{
#ifdef _WIN32
    system("color f0");
#endif
    srand((unsigned)time(0));
    cout << fixed << setprecision(2) << left;

    DataStore db;
    db.loadAll();

    // First-ever run: nothing on disk yet, so load the sample data.
    bool firstRun = db.students.size() == 0 && db.faculty.size() == 0 &&
                    db.problems.size() == 0 && db.platforms.size() == 0;
    if (firstRun)
    {
        seedDefaultData(db);
    }
    else
    {
        if (db.faculty.size() == 0) seedFaculty(db.faculty.all());
        sort(db.students.all().begin(), db.students.all().end());
    }
    db.saveAll();

    while (true)
    {
        Logger::instance().setActor("anonymous");

        cout << "\n\n==========================================";
        cout << "\n CODING PLATFORM / STUDENT PROGRESS";
        cout << "\n==========================================";
        cout << "\n1. Admin Login";
        cout << "\n2. Faculty Login";
        cout << "\n3. Student Login";
        cout << "\n4. View Coding Platforms";
        cout << "\n5. View Leaderboard";
        cout << "\n6. Exit";

        int choice = getInteger("\nEnter choice: ");

        try
        {
            string id;
            if (choice == 1)
            {
                if (loginFlow(db, "Admin", id)) adminMenu(db);
            }
            else if (choice == 2)
            {
                if (loginFlow(db, "Faculty", id)) facultyMenu(db, id);
            }
            else if (choice == 3)
            {
                if (loginFlow(db, "Student", id)) studentMenu(db, id);
            }
            else if (choice == 4)
            {
                displayPlatforms(db.platforms.all());
            }
            else if (choice == 5)
            {
                displayLeaderboard(db.students.all(), db.assignments.all(),
                                   db.submissions.all(), db.problems.all());
            }
            else if (choice == 6)
            {
                db.saveAll();
                cout << "\nData saved successfully.";
                cout << "\nThank you for using the system!" << endl;
                break;
            }
            else
            {
                cout << "\nInvalid choice." << endl;
            }
        }
        catch (const exception &e)
        {
            cout << "\nUnexpected error: " << e.what() << endl;
        }
        db.saveAll();
    }

    return 0;
}
