//Zeynep Turkmen - 29541
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    printf("This is SHELL process with PID: %d, COMMAND = $ man touch | grep -A 1 -m 1 -e '-t' > output.txt \n", getpid());
    int fd[2];
    pipe(fd);

    int pid1 = fork(); //pid of the first child
    
    if (pid1 < 0) {
        fprintf(stderr, "first fork failed\n");
        exit(1);
    }
    else if (pid1 == 0) {
    
        int pid2 = fork();
        
        if (pid2 < 0) {
            fprintf(stderr, "second fork failed\n");
            exit(1);
        }
        else if (pid2 == 0) { //the grandchild process
            printf("This is MAN process(grandchild) with PID: %d, COMMAND = $ man touch \n", getpid());
            
            close(fd[0]); //close the input => currently not used
            dup2(fd[1], STDOUT_FILENO); //makes the output as the anonymous pipe file
            close(fd[1]);
            
            char * command[3];
            command[0] = strdup("man"); // man command
            command[1] = strdup("touch"); // options
            command[2] = NULL;
            //execute the command, output will be written into fd[1]
            execvp(command[0], command); // running the command creates
                                      //another process basically
        }
        else { //child process
            wait(NULL); //waiting for the grandchild
            printf("This is GREP process(child) with PID: %d, COMMAND = $ grep -A 1 -m 1 -e '-t' > output.txt \n", getpid());
            dup2(fd[0], STDIN_FILENO); //swap input with anonymous files input
            close(fd[0]); //close the input file
            
            int output = open("output.txt", O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
            dup2(output, STDOUT_FILENO); //makes output.txt the standard output file
            close(fd[1]); //close the output file
            
            char * command2[6]; //my commands and their options
            command2[0] = strdup("grep");
            command2[1] = strdup("-A 1");
            command2[2] = strdup("-m 1");
            command2[3] = strdup("-e");
            command2[4] = strdup("-t");
            command2[5] = NULL;
            //this will take man touch command as its input and run grep command
            execvp(command2[0], command2);
        }
    }
    else {
        wait(NULL); //making the shell wait for the child process
        printf("This is SHELL process with PID: %d, execution completed see the results inside output.txt \n", getpid());
    }
    return 0;
}
