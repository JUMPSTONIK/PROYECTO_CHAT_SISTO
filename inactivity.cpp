// set inanctive testing
//used as an easier input out put
#include <iostream>
#include <future>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstring>

using namespace std;
using namespace std::chrono_literals;

#define MAX_CLIENTS 100

client_t *clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;


//client structure 
typedef struct{
    char const *status;
    char const *name;
} client_t;

//change status of user taking its name as prameter
void auto_Status (client_t *cl){
    //thread
    pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

    //inactive status 
    char *statusInactive;
    statusInactive = (char*)"INACTIVO";
    
    int i= 0;
    for(i=0; i < MAX_CLIENTS; ++i){
        if(clients[i] == cl){
             //timeout time 
            std::this_thread::sleep_for(3s); //change value here to less than 1 second to see Success
    
            //swap status
            strcpy(clients[i] -> status, statusInactive);

            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    //flag
    cout << "user is now inactive";

}

void emulated_msg(client_t *cl){
    cout << "user msg";
    auto_Status(cl);
    cout << "funciotn called";
}

int main() {
    //new client
    char *msg;
    client_t *cli = (client_t *)malloc(sizeof(client_t));
    cli -> status = "ACTIVO";
    cli -> name = "Julio";

    //simulando chat con while tru
    while(1){


        //simlutates message sending
        std::cout << "Ingrese mensaje \n";

        emulated_msg(cli);


        //std::cout << (cli -> status) << std::endl;
        break;
    }

    return 0;
}