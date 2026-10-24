# Coding Platform / Student Progress Tracker

A menu-driven C++ console application that helps an academic department
manage coding practice: faculty assign problems, students record
submissions, and everyone can see progress.

> OOPS Through C++ Cornerstone Project, B.Tech AIML, Aditya University

## Why this project?
Students practise on LeetCode, CodeChef, HackerRank, GeeksforGeeks and
Codeforces, so assigned problems and progress end up scattered. This app
keeps everything in one place.

## Features
- Three roles: **Admin**, **Faculty** and **Student**, each with its own menu
- Add, search, sort and delete students, ranked by CGPA
- Faculty add problems, assign them with deadlines (to one student or a whole department) and review submissions
- Students link platform accounts, record submissions and view their progress
- Progress report written to `student_report.txt`
- Input validation for emails, dates, links and CGPA range

## OOP concepts used
Classes and objects, encapsulation, inheritance (`Person` → `Student` / `Faculty`),
abstraction (pure virtual `display()`), function overloading, friend function,
exception handling, file handling, STL vectors and `sort()`, static members.

## How to run
Compile and run from the project folder:

    g++ -std=c++11 src/main.cpp -o tracker
    ./tracker

On Windows, run `tracker.exe` instead. Sample data is created automatically
on the first run.

**Demo logins:** `admin` / `admin123`, faculty ID `F101` / `faculty123`,
any sample student ID / `student123`.

## Limitations and future scope
- Fixed demo passwords and plain-text file storage
- Does not connect to the coding platforms
- Planned: database storage, individual accounts, web interface

## Team
- Gadi V V Lalitha Jahnavi
- Nujella Padmini
- Patnala Meenamrutha Sri Nukambika
- Allaka Jyothsna

Guide: Dr. Maganti Venkatesh, Associate Professor, Department of AIML

## Documentation
- [Project report (PDF)](Documentation.pdf)
- [Presentation (PPTX)](Presentation.pptx)
