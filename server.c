#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

#define MAX_CLIENTS 100
#define BUFFER_SZ 2048
#define NAME_LEN 32
//variable estatica para tener un conteo de los clientes conectados
static _Atomic unsigned int cli_count = 0;
//numero base para asignar un id a cada cliente que se une
static int uid = 10;

//Estructura de los clientes
typedef struct{
    //se debe agregar la variable de estatus para el cliente
    //char status
    struct sockaddr_in address;
    int sockfd;
    int uid;
    char name[NAME_LEN];
}

//funcion para imprimir tu direccion del cliente
void print_ip_addr(struct sockaddr_in addr){
    printf("%d.%d.%d.%d", 
            addr.sin_addr.s_addr & 0xff,
            (addr.sin_addr.s_addr & 0xff00) >> 8,
            (addr.sin_addr.s_addr & 0xff0000) >> 16,
            (addr.sin_addr.s_addr & 0xff000000) >> 24);
} client_t;

//se inicializa queue de los clientes en el chat conectados
client_t *clients[MAX_CLIENTS];
//se inicializa el mutex para coordinar los mensajes
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

//no estoy seguro de esta parte

void str overwrite_stdout(){
    printf("\r%s", " > ");
    fflush(stdout);
}
//ni esta parte
void str_trim_lf(char* arr, int length){
    for(int i=0; i<length; i++){
        if(arr[i] == '\n'){
            arr[i] = '\0';
            break;
        }
    }
}
//funcion para agregar clientes al queue
//se debe de enviar como parametro el cliente nuevo con su info como esta en la estructura
void queue_add(client_t *cl){
    pthread_mutex_lock(&clients_mutex);
    //se revisa que el cliente no existe dentro del queue de clientes para que no se repita
    for(int i= 0; i<MAX_CLIENTS; i++){
        if(!clients[i]){
            clients[i] = cl;
            break;
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

//funcion para remover los clientes
void queue_remove(int uid){
    pthread_mutex_lock(&clients_mutex);
    //se busca a dicho cliente por su uid y se remueve del queue
    for(int i = 0; i< MAX_CLIENTS; i++){
        if(clients[i]){
            if(clients[i] -> uid == uid){
                clients[i] = NULL;
                break;
            }
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
}
//esta funcion sirve para enviar el mensaje que se ha escrito por un usuario
//se envia de parametro el mensaje y el uid del usario que lo envio
void send_message(char *s, int uid){
    pthread_mutex_lock(&clients_mutex);
    //se verifica si existien clientes para luego revisar si existe el que tiene dicho iud
    for(int i = 0; i< MAX_CLIENTS; i++){
        if(clients[i]){
            if(clients[i] -> uid == uid){
                //se verifica que no sea un mensaje vacio el cual se manda por el socket
                if(write(clients[i] -> sockfd, s, strlen(s)) < 0){
                    printf("ERROR: escrito al descriptor fallo\n");
                    break;
                }
            }
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg){
    //se inicializan variables como el buffer para mensaje, el nombre, señal de salida y cantidad de clientes
    char buffer[BUFFER_SZ];
    char name[NAME_LEN];
    int leave_flag = 0;
    cli_count++;
    
    //se crea un cliente con la estructua echa al principio con sus valores reservados
    client_t *cli = (client_t*)arg;
    
    //nombre
    //aqui se trata de obtener el nombre del cliente al registrarse y se verifica si esta corretamente hecho
    if(recv(cli-> sockfd, name, NAME_LEN, 0) <= 0 || strlen(name) < 2 || strlen(name) >= NAME_LEN -1){
        printf("ingrese un nombre aceptable");
        leave_flag =1;
    }else{
        strcpy(cli -> name, name);
        printf(buffer, "%s has joined\n", cli -> name);
        printf("%s", buffer);
        send_message(buffer, cli -> uid);
    }
    
    bzero(buffer< BUFFER_SZ);
    
    while(1){
        if(leave_flag){
            break;
        }
        
        
        int receive = recv(cli -> sockfd, buffer, BUFFER_SZ, 0);
        
        if(receive > 0){
            if(strlen(buffer) > 0){
                send_message(buffer, cli->uid);
                str_trim_lf(buffer, strlen(buffer));
                printf("%s -> %s", buffer, cli-> name)
            }
        }else if(receive == 0 || strcmp(buffer, "exit")==0){
            sprintf(buffer, "%s has left\n", cli -> name);
            printf("%s" , buffer);
            send_message(buffer, cli -> uid);
            leave_flag = 1;
        }else{
            printf("ERROR: -1\n");
            leave_flag =1;
        }
        
        bzero(buffer, BUFFER_SZ);
    }
    close(cli -> sockfd);
    queue_remove(cli -> uid);
    free(cli);
    cli_count--;
    pthread_detach(pthread_self());
    
    return NULL;
}

int main(int argc, char **argv)
{
    if(argc != 2){
        printf("Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    char *ip = "127.0.0.1";
    int port = atoi(argv[1]);
    
    int option = 1;
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    pthread_t tid;
    
    //configuracion de los sockets
    listenfd = socket(AF_INET, SOCK_STREAM,0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);
    
    //señales
    signal(SIGPIPE, SIG_IGN);
    
    if(setsockopt(listenfd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char*)&option, sizeof(option)) < 0){
        printf("ERROR: setsockopt\n");
        return EXIT_FAILURE;
    }
    
    //el bind
    if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))<0){
        printf("ERROR: bind\n");
        return EXIT_FAILURE;
    }
    
    //haciendo al server escuchar
    if(listen(listenfd,10)<0){
        printf("ERROR: listen\n");
        return EXIT_FAILURE;
    }
    
    printf("///WELCOME TO THE CHAT///\n");
    
    while(1){
        socklen_t clilen = sizeof(cli_addr);
        connfd = accept(listenfd, (struct sockaddr*)&cli_addr, clilen);
        
        //comprobar el maximo de clientes conectados
        if((cli_count + 1) == MAX_CLIENTS){
            printf("Maxima cantidad de clientes conectados. Coneccion cortada\n");
            print_ip_addr(cli_addr);
            close(connfd);
            continue;
        }
        
        //configuraciones del clientes
        client_t *cli = (client_t *)malloc(sizeof(client_t));
        cli -> address = cli_addr;
        cli -> sockfd = connfd;
        cli -> uid = uid++;
        
        //agregar cliente al queue
        queue_add(cli);
        pthread_create(&tid, NULL, &handle_client, (void*)cli);
        
        //reducimos el uso del cpu
        sleep(1);
    }
    
    return EXIT_SUCCESS;
}
