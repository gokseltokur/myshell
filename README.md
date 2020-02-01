# MyShell

The main() function of the program presents the command line prompt “myshell: ” and waits for the user to enter a command. The setup function (given in
the outline program of your textbook) reads the user’s next command and parses it into separate
tokens that are used to fill the argument vector for the command to be executed. It can also detect
background processes. This program is terminated when the user enters ^D (<CONTROL><D>);
and setup function then invokes exit. The contents of the command entered by the user is loaded
into the args array. A line of input will contain no more than 128 characters or
more than 32 distinct arguments.
