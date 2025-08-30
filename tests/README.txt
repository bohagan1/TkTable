Create Test Cases

1. Create the test case *.csv file with a good representative filename. You can
   use multiple files as needed. Generally it's a good idea to group test cases
   related to the same module, command, etc. in the same file.

2. Add test cases to *.csv files. Each test case is on a separate line. The
   column titles correspond to the tcltest command options. Leave a column
   blank if not used. Generally it is a good idea to group like test cases
   (i.e. for the same function) together with the same group name. See tcltest
   for more info on each column.

3. Define any common functions in a common.tcl file or in the *.csv file itself
   using the special "command" group.


Generate Test File

4. To create the test cases script, execute make_test_files.tcl. This will use
   the *.csv files to create the *.test files.


Execute Test Suite

5. To run the test suite, execute the all.tcl file.
