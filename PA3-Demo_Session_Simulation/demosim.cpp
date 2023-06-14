//  Created by Zeynep Türkmen on 7.05.2023.
#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <vector>

//first cool lock is for updating the eligible num of assistants and students for a demo
pthread_mutex_t cool_lock = PTHREAD_MUTEX_INITIALIZER;
//the other one is for keeping track of the ones that are currently performing a demo session -> awakeStu and awakeAss
pthread_mutex_t cool_lock2 = PTHREAD_MUTEX_INITIALIZER;

//semaphores for all conditions
sem_t enterLeave; //numStu <= 3*numAss
sem_t semStudent; //make sure 2 Stu
sem_t semAssistant; // and an Ass is in a demo session

int numStu = 0;
int numAss = 0;
int awakeStu = 0;
int awakeAss = 0;

using namespace std;
vector<pthread_barrier_t> cool_barriers; //a barrier for each demo session to make sure all members of that demo wait for each others participation

void * student(void * arg){
    
    int barrIdx; //the index of the barrier that belongs to this students demo session
    printf("Thread ID:%ld, Role:Student, I want to enter the classroom.\n", pthread_self());
   
    //if this doesnt hold, either an assistant needs to enter or a student needs to leave
    sem_wait(&enterLeave); //decrement enterLeave
    
    printf("Thread ID:%ld, Role:Student, I entered the classroom.\n", pthread_self());
    
    //checking the demo session conditions....
    pthread_mutex_lock(&cool_lock);
    numStu+=1; //increment avaiable students
    if (!(numStu >= 2 && numAss >= 1)){ //demo cannot be done
        pthread_mutex_unlock(&cool_lock);
        sem_wait(&semStudent);
    }
    else{ //this thread starts the demo
        //reduce the number of waiting students and assistants
        numStu -= 2;
        numAss -= 1;
        //wake a student and an assistant
        sem_post(&semAssistant);
        sem_post(&semStudent);
        pthread_mutex_unlock(&cool_lock);
    }

    pthread_mutex_lock(&cool_lock2);
    awakeStu ++; //number of current demo participating students
    //assign indexes accordingly to the wake up time
    if (awakeStu % 2 == 0) //2 consequtive students will get same index
        barrIdx = awakeStu / 2 - 1;
    else
        barrIdx = awakeStu / 2;
    pthread_mutex_unlock(&cool_lock2);
    
    printf("Thread ID:%ld, Role:Student, I am now participating.\n", pthread_self());
        
    //waiting for their own sessions barrier
    pthread_barrier_wait(&cool_barriers[barrIdx]);
    
    //after all session group participated, student can leave
    printf("Thread ID:%ld, Role:Student, I left the classroom.\n", pthread_self());
    sem_post(&enterLeave); //a student left, new one can come for: 3*numA condition
    return NULL;
}

void * assistant(void * arg){
    int barrIdx; //the index of the barrier that belongs to this students demo session
    
    printf("Thread ID:%ld, Role:Assistant, I entered the classroom.\n", pthread_self());
    //allow 3 students to enter
    sem_post(&enterLeave);
    sem_post(&enterLeave);
    sem_post(&enterLeave);
    
    //checking the demo session condition ----------------------
    pthread_mutex_lock(&cool_lock);
    numAss +=1;
    if (!(numStu >= 2 && numAss >= 1)){ //cannot start a demo session
        pthread_mutex_unlock(&cool_lock);
        sem_wait(&semAssistant);
    }
    else{ //this thread starts the demo
        //wake up 2 students
        sem_post(&semStudent);
        sem_post(&semStudent);
        numStu -= 2;
        numAss -= 1;
        pthread_mutex_unlock(&cool_lock);
    }
    
    pthread_mutex_lock(&cool_lock2);
    awakeAss ++;
    barrIdx = awakeAss-1;
    pthread_mutex_unlock(&cool_lock2);
    
    printf("Thread ID:%ld, Role:Assistant, I am now participating.\n", pthread_self());
    //reduce the waiting guys count
    pthread_barrier_wait(&cool_barriers[barrIdx]);
    //conclude the demo after barrier
    printf("Thread ID:%ld, Role:Assistant, demo is over.\n", pthread_self());

    //making sure 3*numA > stu condition holds before leaving
    sem_wait(&enterLeave);
    sem_wait(&enterLeave);
    sem_wait(&enterLeave);
    
    printf("Thread ID:%ld, Role:Assistant, I left the classroom.\n", pthread_self());
    return NULL;
}

int main(int argc, char **argv) {
    int numA, numS;
    vector<pthread_t> threads;
    
    printf("My program compiles with all the conditions.\n");
    
    //initialize all semaphores with zero
    sem_init(&semStudent, 0, 0);
    sem_init(&semAssistant, 0, 0);
    sem_init(&enterLeave, 0, 0);
    
    if (argc == 3){
        numA = atoi(argv[1]); // Convert the arguments to integer
        numS = atoi(argv[2]);

        if (numA <= 0)
            cout << "Number of assistants has to be positive!" << endl;
        else if (numA * 2 != numS)
            cout << "Number of students has to be double the assistants!" << endl;
        else{
            //initialize barriers of size 3 for all demo sessions
            for (int i = 0; i < numS/2; i++) {
                pthread_barrier_t barr;
                pthread_barrier_init(&barr, NULL, 3);
                cool_barriers.push_back(barr);
            }
            for (int i=0; i < numA; i++){ //create assistant threads
                pthread_t ass;
                pthread_create(&ass, NULL, assistant, NULL);
                threads.push_back(ass);
            }
            for (int i=0; i < numS; i++){  //create student threads
                pthread_t stu;
                pthread_create(&stu, NULL, student, NULL);
                threads.push_back(stu);
            }
        }

        //joining all threads
        for (int i=0; i < threads.size(); i++){
            pthread_join(threads[i], NULL);
        }
        //clearing the memory
        threads.clear();
        
        //destroy the barriers and the barrier vector
        for (int i=0; i < cool_barriers.size(); i++){
            pthread_barrier_destroy(&cool_barriers[i]);
        }
        cool_barriers.clear();
    }
    else{
        cout << "Only enter 2 arguments..." << endl;
    }
    sem_destroy(&enterLeave);
    sem_destroy(&semStudent);
    sem_destroy(&semAssistant);

    cout << "The main terminates..." << endl;
    return 0;
}

