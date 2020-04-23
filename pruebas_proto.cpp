#include <iostream>
using namespace std;
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

//protocol
#include "mensaje.pb.h"
using namespace chat;

int main()
{
    //verifica si existe el port de 4 digitos que se solicita en el formato <nombredelservidor> <puertodelservidor>
    if(argc != 2){
        printf("Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    //establecemos la ip a la que se conecta y se parcea a int y guarda en la variable port el port ingresado por el que inicio el server
    char *ip = (char*)"127.0.0.1";
    int port = atoi(argv[1]);
    
    //otras variables que necesitamos, pero no estoy seguro para que sirven
    int option = 1;
    int listenfd = 0, connfd = 0;
    char stat[8] = "ACTIVO";
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    //varaible para crear los treads
    pthread_t tid;
    
    //configuracion de los sockets
    listenfd = socket(AF_INET, SOCK_STREAM,0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);
    
    //señales
    signal(SIGPIPE, SIG_IGN);
    //basicamente crea la conexion al socket que estaran conectados para recibir los mensajes
    if(setsockopt(listenfd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char*)&option, sizeof(option)) < 0){
        printf("ERROR: setsockopt\n");
        return EXIT_FAILURE;
    }
    
    //el bind
    //basicamente enlazamos nombre al socket con el que vamos a trabajar y se pasan los parametros requeirdos para hacer dicho enlace
    if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))<0){
        printf("ERROR: bind\n");
        return EXIT_FAILURE;
    }
    
    //haciendo al server escuchar
    if(listen(listenfd,10)<0){
        printf("ERROR: listen\n");
        return EXIT_FAILURE;
    }
    //si todo salio bien y no hay errores, pues recibiremos este mensaje del parte del server. quiere decir que no hubo problemas con el socket
    //tambien que no hubo error en la configuraciond e los sockets, por lo que ya se puede chatear
    printf("///WELCOME TO THE CHAT///\n");
    
    while(1){

        socklen_t clilen = sizeof(cli_addr);
        //aqui basicamente creamso la aceptacion al socket que se ha conectado y que hemos configurado.
        connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);
        
        //comprobar el maximo de clientes conectados
        //solo es para comprobar que no hay mas clientes de los que el server puede soportar. automaticamente saca quien sobrepasa el limite
        if((cli_count + 1) == MAX_CLIENTS){
            printf("Maxima cantidad de clientes conectados. Coneccion cortada\n");
            //se imprime la direccion de usuario
            print_ip_addr(cli_addr);
            //se cierra su coneccion al socket
            close(connfd);
            continue;
        }
        
        //configuraciones del clientes
        //aqui se asigna la informacion a las variables de la estuructura de un cliente
        client_t *cli = (client_t *)malloc(sizeof(client_t));
        cli -> address = cli_addr;
        cli -> sockfd = connfd;
        cli -> uid = uid++;
        strcpy(cli -> status, stat);
        //strcpy(cli -> name, "ACTIVO");
        
        //agregar cliente al queue
        queue_add(cli);
        //se crea el thread para enviar mensajes del cliente
        pthread_create(&tid, NULL, &handle_client, (void*)cli);
        
        //reducimos el uso del cpu
        sleep(1);
    }
    
    return EXIT_SUCCESS;
}
