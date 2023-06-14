//Zeynep Turkmen - 29541
#include <list>
#include <pthread.h>
#include <iostream>
#include <unistd.h>

using namespace std;

class HeapManager {

    public:
        HeapManager() { //initialize the lock
            cool_lock = PTHREAD_MUTEX_INITIALIZER;
        }
        
        int initHeap(int size){ //initialize the heap with the given value
            memoryList.push_back(Node(-1, size, 0)); //add a free node with size "size" to the memory list
            cout << "Memory initialized." << endl;
            print();
            return 1;
        }
        
        int myMalloc(int threadId, int size) {
            pthread_mutex_lock(&cool_lock); //lock before doing modifications to the list
            
            //start iterating through the list to fins a node of sufficient size
               for (list<Node>::iterator n = memoryList.begin(); n != memoryList.end(); ++n) {
                   //if current iterated node is free and has enough size
                   if (n->tid == -1 && n->size >= size) {
                      
                       int newStart = n->start; //place new one to the beginning of this node
                       int newSize = n->size - size; //remaining size
                       
                       //create the new node
                       Node newNode = Node(threadId, size, newStart);
                       
                       //insert it before n.
                       memoryList.insert(n, newNode);
                       
                       //if we have enough space modify n's values
                       if (newSize > 0) {
                           n->start = newStart+size;
                           n->size = newSize;
                       }
                       else { //delete n completely
                           memoryList.erase(n);
                       }
                       //print the messages and the updated list.
                       cout << "Allocated for thread " << threadId << endl;
                       printMe();
                       pthread_mutex_unlock(&cool_lock);
                       return newStart;
                   }
               }
            //print the messages and the updated list.
            cout << "Can not allocate, requested size " << size << " for thread " << threadId << " is bigger than remaining size " << endl;
            printMe();
            pthread_mutex_unlock(&cool_lock);
            return -1;
        }

        int myFree(int threadId, int start) {

            pthread_mutex_lock(&cool_lock);
            for (list<Node>::iterator n = memoryList.begin(); n != memoryList.end(); ++n) {
                
                //if there is a matching thing to free with the tid
                    if (n->tid == threadId && n->start == start) {
                        
                        //check if its prev node is also free
                        if (n != memoryList.begin()) {
                            list<Node>::iterator prev = n;
                            --prev; //make prev "prev"
                               if (prev->tid == -1) { //if prev is free
                                   prev->size += n->size; //increment prevs size
                                   n = memoryList.erase(n);
                                   --n; //set n to be prev
                               }
                           }
                        n->tid = -1; //set is as unoccupied

                        list<Node>::iterator next = n;
                        ++next; //iterate to the next one
                        //if next node is empty, delete next and add its storage to n
                        if (next != memoryList.end() && next->tid == -1) {
                            n->size += next->size;
                            memoryList.erase(next);
                           }
                        
                        //print the messages
                        cout << "Freed for thread " << threadId << endl;
                        printMe();
                        pthread_mutex_unlock(&cool_lock);
                        return 1;
                    }
                }
              //print messages, release lock
              cout << "No matching node to free for the thread " << threadId << endl;
              printMe();
              pthread_mutex_unlock(&cool_lock);
              return -1;
        }
        
        void print() {
            pthread_mutex_lock(&cool_lock);
            for (Node n : memoryList) { //iterate through the list and print them
               cout << "[" << n.tid << "][" << n.size << "][" << n.start << "]---";
            }
            cout << endl;
            pthread_mutex_unlock(&cool_lock);
        }

    private:
        struct Node {
            int tid;
            int size;
            int start;
            Node(int t, int s, int st) : tid(t), size(s), start(st) {}
        };

        list<Node> memoryList;
        pthread_mutex_t cool_lock;
    
        void printMe() {
            for (Node n : memoryList) { //iterate through the list and print them
               cout << "[" << n.tid << "][" << n.size << "][" << n.start << "]---";
            }
            cout << endl;
        }
};

