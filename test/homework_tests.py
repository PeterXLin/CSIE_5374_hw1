import tester

GENERAL_ERROR = "[eE]rror:[\s\S]*"

t = tester.HomeworkTester()

"""Modify this file with your tests.

The test is already filled out with some basic tests.

Basically, your main usage is:

    t.add_test("command to execute 1", "expected output as a regex string")
    t.add_test("command to execute 2", "expected output as a regex string")
    ...
    t.add_test("command to execute 3", "expected output as a regex string")
    t.run()
    t.print_results()
    t.reset()
"""

##################### Basic Executables #########################
# ls should not be found
t.add_test("ls", GENERAL_ERROR)

# But /bin/echo should work
t.add_test("/bin/echo hello world", "hello world")
t.run()
t.print_results()
t.reset()

############################# Builtins ##########################
# Test that cd works
t.add_test("cd /tmp", "")
t.add_test("/bin/pwd", "/tmp")
t.add_test("cd /var", "")
t.add_test("/bin/pwd", "/var")
t.add_test("cd", GENERAL_ERROR)
t.run()
t.print_results()
t.reset()

############################# Custom ##########################
t.add_test("ls|grep shell", GENERAL_ERROR)
t.add_test("history", "1  ls|grep shell\n    2  history")
t.add_test("/bin/ls | /bin/grep cs53", "cs5374_sh")
t.add_test("history -c 3", GENERAL_ERROR)
t.add_test("history -80", GENERAL_ERROR)
t.add_test("122323 -80 GADG", GENERAL_ERROR)
t.add_test("123EAEE -80 D DE", GENERAL_ERROR)
t.add_test("history | /bin/grep test", "8  history | /bin/grep test")
t.add_test("echo hello", GENERAL_ERROR)
t.add_test("history 2|/bin/grep hello|/bin/grep hello|/bin/grep hello|/bin/grep hello", "9  echo hello\n   10  history 2|/bin/grep hello|/bin/grep hello|/bin/grep hello|/bin/grep hello")
t.add_test("cd jejejej jeje jekej jejk ekje kej kjekk", GENERAL_ERROR)
t.add_test("history 3j3kj33kj3k3jk", GENERAL_ERROR)
t.add_test("123 DSFAD-80", GENERAL_ERROR)
t.add_test("exit 1", GENERAL_ERROR)
t.run()
t.print_results()
t.reset()

# for i in range(20):
#     t.add_test("echo {i}".format(i), "i")

# t.add_test("history 1", "11  history")
# t.run()
# t.print_results()
# t.reset()

