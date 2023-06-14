#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <fstream>
#include <vector> //to store thread ids, for waiting
#include <sys/wait.h>
#include <sstream>

using namespace std;
pthread_mutex_t cool_lock = PTHREAD_MUTEX_INITIALIZER;

void * listener(void * arg){
    long int fdRead = (long int)(arg); //read end of the pipe
    bool flag = true;
    FILE * pipeFile;
    
    while(flag){ //keep listening (try to lock) until the contents of the pipe gets printed on the console
        pthread_mutex_lock(&cool_lock);
        pipeFile = fdopen(fdRead, "r"); //open the pipe in read mode
        char buff[100]; //to get the message
        
        if(pipeFile){ //if file was succesfully opened
            if((fgets(buff, sizeof(buff), pipeFile) != NULL)){ //check if the file has any content
                printf("------------- %ld\n", pthread_self());
                printf("%s", buff); //print the results
                fsync(STDOUT_FILENO);
                while (fgets(buff, sizeof(buff), pipeFile) != NULL){ //read it until EOF
                    printf("%s", buff);
                    fsync(STDOUT_FILENO);
                }
                printf("-------------- %ld\n", pthread_self());
                fsync(STDOUT_FILENO);
                flag = false;
            } //else keep trying to get the lock until it has content
        }
        pthread_mutex_unlock(&cool_lock);
    }
    fclose(pipeFile); // close the pipeFile
    return NULL;
}

int main(int argc, const char * argv[]) {
    ifstream file;
    file.open("commands.txt"); //input stream
    
    ofstream parse;
    parse.open("parse.txt"); //parsed commands output stream

    string line, word;
    vector<int> processIDs; //stores all the child process IDs
    vector<pthread_t> threadIDs; //stores all the threadIDs
    
    while(getline(file, line)){ //loop through commands.txt
        istringstream ss(line); //to read the line word by word
        ss >> word; //now the word IS the command to be executed
        
        char redirect = '-';
        string filename, input, options;
        
        char * command[4] = { NULL, NULL, NULL, NULL}; //there are at most 4 things in a command: command itself, input, option and the null for execvp
        command[0] = strdup(word.c_str()); //the commant to be executed
        int idx = 1;
        
        while(ss >> word){
            if (word == ">"){ //theres a redirection
                redirect = '>'; //set redirect
                ss >> word;
                filename = word; //set filename
            }
            else if (word == "<"){
                redirect = '<';
                ss >> word;
                filename = word;
            }
            else if (word != "&"){ //& is the last thing in the line so if the word is not that NOR the redirect things, means its a part of the command array.
                if (word.find("-") == 0){ //checks if its an option
                    options = word;
                }
                else{
                    input = word;
                }
                command[idx] = strdup(word.c_str()); //adds it to the array
                idx++;
            }
        }
        //the command was parsed, the array is ready
        parse << "----------" << endl;
        parse << "Command: " << command[0] << endl;
        parse << "Input: " << input << endl;
        parse << "Option: " << options << endl;
        parse << "Redirection: " << redirect << endl;
        parse << "Background: " << (word == "&" ? "y": "n") << endl;
        parse << "----------" << endl;
        parse.flush();
        
        if (word == "wait"){ //waiting for all the child processes and joining their corresponding threads. AKA making sure everything gets printed
            for (int i = 0; i < processIDs.size(); i++){
                waitpid(processIDs[i], NULL, NULL);
            }
            processIDs.clear(); //clearing the memory
            
            for(int i=0; i <  threadIDs.size(); i ++){
                pthread_join(threadIDs[i], NULL);
            }
            threadIDs.clear();
        }
        else{ //the command should be executed by execvp
            if(redirect == '>'){ //the only case where the command won't be printed to console!!!
                int pid_redirect = fork();
        
                if (pid_redirect < 0){
                    cout << "fork failed!!"<< endl;
                }
                else if (pid_redirect == 0){
                    int out = open(filename.c_str(), O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU); //open the output file
                    dup2(out, STDOUT_FILENO); //make STDOUT into the output file
                    close(out); //close the unused one
                    execvp(command[0], command);
                }
                else{
                    if (word != "&"){ //not a background
                        waitpid(pid_redirect, NULL, NULL); //wait before continuing to read more commands.
                    }
                    else{
                        processIDs.push_back(pid_redirect); //push it to keep track
                    }
                }
            }
            else{ //the output will be on console.
                int fd[2];
                pipe(fd);
                pthread_t thread1;
            
                int pid_sendParent = fork();
                
                if(pid_sendParent < 0){
                    cout << "Failed to fork!" << endl;
                }
                else if (pid_sendParent == 0){
                    if (redirect == '<'){
                        int in = open(filename.c_str(), O_RDONLY); //open file in read only mode
                        dup2(in, STDIN_FILENO); //swap the input to be that file
                        close(in); //close, not used
                        
                    }
                    close(fd[0]); //close the read end, not used
                    dup2(fd[1], STDOUT_FILENO); //makes the output as the anonymous pipe file
                    close(fd[1]);
                    execvp(command[0], command); //run the command
                }
                else{
                    close(fd[1]); //close the write end
                    pthread_create(&thread1, NULL, listener, (void*) fd[0]); //send the read end to the listener thread
                    if (word != "&"){ //not background
                        waitpid(pid_sendParent, NULL, NULL); // wait before continiuing the loop
                        pthread_join(thread1, NULL); //join the thread
                    }
                    else{ //push them to keep track
                        processIDs.push_back(pid_sendParent);
                        threadIDs.push_back(thread1);
                    }
                }
            }
        }
    }
    parse.close(); //close the files, work is done.
    file.close();
    
    //waiting, joining every thread to make sure everything is printed.
    for (int i=0; i < processIDs.size(); i++){
        waitpid(processIDs[i], NULL, NULL);
    }
    processIDs.clear();
    
    for(int i=0; i <  threadIDs.size(); i ++){
        pthread_join(threadIDs[i], NULL);
    }
    threadIDs.clear();
    return 0;
}
