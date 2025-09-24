##For user

Introduction
  - This is an application for a simple automatic grading system.

System Overview
  - This application consist of 2 software, the command line tool for read_answer and a software with ui for an end users.

Installation
  - Ensure c, make, gtk4 and mongo-tools are installed.
  - Clone the repository (https://github.com/napookooo/read_answer) and run `make`.
  - Use the binary file ui to start using the program.

Getting Started
  - Run the binary file 'ui' to start using, the user will require a password to continue forward with the program, after the right password is entered user can now use the program.

Features and Functions
  - Grading 1 selected file at a time.
  - Grading 1 selected directory at a time.
  - Push the result in output directory into a database.

FAQ
  - If there are any questions, they can be ask in https://github.com/napookooo/read_answer.

'
'
'

##For dev

Introduction
  - This is an application for a simple automatic grading system.
  - Using C language with the help of gtk4 for ui and mongodb for databae.

System Architecture
  - Consist of 3 modules, the database to store the results, the grading application, and the ui for end users.
  - The database and grading application work independently, while the ui is the brdige for end users to simplify the usage of both database and grading application.

Installation
  - Ensure c, make, gtk4, mongodb and mongo-tools are installed.
  - Clone the repository (https://github.com/napookooo/read_answer) and run `make`.
  - Setup database using instruction from Database_setup.txt.

Code Structure
  - All of the source code are in the src/ directory inside the root directory of the project, editing the source code are encourage for a better experience (using `make` to build your newly edited code).

Database Design
  - Using mongodb, the default configuration are in the Database_setup.txt.
  - Developer are allow to design database that better suited the different usage.

Deployment Guide
  - See database setup from Database_setup.txt in manuals directory of the project.

Testing
  - Can try the database after setup by following Database_setup.txt.
  - The application and ui can be tested by using them with the answer sheet of the same type from the school admins.

Contribution Guidelines
  - Make a pull request in the https://github.com/napookooo/read_answer repository.
  - How to make a pull request -> https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/proposing-changes-to-your-work-with-pull-requests/creating-a-pull-request

Maintenance
  - Only the mongodb database is the main point of maintenance in this stage (https://www.mongodb.com/docs/manual/tutorial/perform-maintence-on-replica-set-members/).
