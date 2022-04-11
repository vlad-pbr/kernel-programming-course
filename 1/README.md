# Assigment 1

Write a C program which accepts a path to a directory as an argument and for each file in that directory and subdirectories uses `wc` to count the amount of words in it. The program then displays the average amount of words and their variance. It also displays the name of the file and the difference between the amount of words in it and the average amount of words overall.

- For each directory a new process should be created
- For each file a new process should be created

Communication between the directory process and the file process should be achieved using pipes. Communication between the directory process and the main process should be achieved using message queue (System V)
