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

#define MAX_CLIENTS 100
#define BUFFER_SZ 2048
#define NAME_LEN 32
//variable estatica para tener un conteo de los clientes conectados
static unsigned int cli_count = 0;
//numero base para asignar un id a cada cliente que se une
static int uid = 10;

//Estructura de los clientes
typedef struct{
    //se debe agregar la variable de estatus para el cliente
    char status[10];
    struct sockaddr_in address;
    int sockfd;
    int uid;
    char name[32];
} client_t;

//funcion para imprimir tu direccion del cliente
void print_ip_addr(struct sockaddr_in addr){
    printf("%d.%d.%d.%d", 
            addr.sin_addr.s_addr & 0xff,
            (addr.sin_addr.s_addr & 0xff00) >> 8,
            (addr.sin_addr.s_addr & 0xff0000) >> 16,
            (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

//se inicializa queue de los clientes en el chat conectados
client_t *clients[MAX_CLIENTS];
//se inicializa el mutex para coordinar los mensajes
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;


// set status del cliente
void status_change(client_t *cl, char *status){
    pthread_mutex_lock(&clients_mutex);
    //char nombreKK[100];
    char *msg;
    //ACTIVO, OCUPADO e INACTIVO
    //recorre todos los cleintes en que se busca
    if (strcmp(status, "~1") == 0){
        msg = (char*)"ACTIVO";
    }
    else if (strcmp(status, "~2") == 0){
        msg = (char*)"OCUPADO";
        }
    else if (strcmp(status, "~3") == 0){
        msg = (char*)"INACTIVO";
        }
    else {
        printf("No valido :(");
        return;
    }

    int i= 0;
    for(i=0; i < MAX_CLIENTS; ++i){
        if(clients[i] == cl){
            strcpy(clients[i] -> status, msg);
            //sprintf(nombreKK, "esta ahora: %s \n", clients[i] -> status);
            //printf("%s\n", nombreKK);
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void client_info(client_t *cl, char *msg){
    pthread_mutex_lock(&clients_mutex);
    char buff[BUFFER_SZ];
    char nam[NAME_LEN];
    int i = 0;
    for (i = 6; i < strlen(msg); ++i)
    {
        if (msg[i] != '#')
        {
            nam[i-6] = msg[i];
        }else{
            break;
        }
        
    }

    for(i=0; i < MAX_CLIENTS; ++i){
        if(clients[i]){
            if (strcmp(clients[i] -> name, nam)==0)
            {
                sprintf(buff, "el nombre de quien ha solicitado informacion es:\nnombre: %s\nstatus: %s\nuid: %d\nsocket: %d\n", clients[i] -> name,clients[i] -> status,clients[i] -> uid,clients[i] -> sockfd);
                printf("%s\n", buff);
                if(write(cl -> sockfd, buff, strlen(buff)) < 0){
                printf("ERROR: escrito al descriptor fallo\n");
                break;
            }
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

//no estoy seguro de esta parte
//segun entiendo, sirve para limpiar la salida de un buffer y mover la data del buffer a la consola
void str_overwrite_stdout(){
    printf("\r%s", " > ");
    fflush(stdout);
}
//esta es una funcion para limpiar el \n del mesnaje cuando se manda y cambiarlo por un valor vacio
void str_trim_lf(char* arr, int length){
    int i = 0;
    for(i=0; i<length; i++){
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
    int i= 0;
    for(i=0; i < MAX_CLIENTS; ++i){
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
    int i= 0;
    for(i = 0; i< MAX_CLIENTS; i++){
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
    int i= 0;
    for(i = 0; i< MAX_CLIENTS; i++){
        if(clients[i]){
            if(clients[i] -> uid != uid){
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

void connected_clients(client_t *cl){
	char userName[200];
	int i = 0;

    for(i=0; i < MAX_CLIENTS; ++i){
    	if(clients[i]){
	    	sprintf(userName, "%s esta conectado, \n", clients[i] -> name);
	        printf("%s", userName);
         	if(write(cl -> sockfd, userName, strlen(userName)) < 0){
                printf("ERROR: escrito al descriptor fallo\n");
                break;
            }
	    	//resest name and frees memeory
	        //bzero(userName, 100);	
    	}
    }
}

void *handle_client(void *arg){
    //se inicializan variables como el buffer para mensaje, el nombre, señal de salida y cantidad de clientes
    char buffer[BUFFER_SZ];
    char name[NAME_LEN];
    int leave_flag = 0;
    cli_count++;
    
    //se crea un cliente con la estructua echa al principio con sus valores reservados
    client_t *cli = (client_t*)arg;
    
    if(recv(cli-> sockfd, buffer, BUFFER_SZ, 0)<=0){
    	leave_flag =1;
    }else{
    	string IDKS(buffer);
    	ClientMessage m2;
    	m2.ParseFromString(IDKS);
    	if(m2.option()==1){
    		MyInfoResponse *mir(new MyInfoResponse);
    		mir->set_userid(cli->uid);
    		ServerMessage sm;
    		sm.set_option(4);
    		sm.set_allocated_myinforesponse(mir);
    		string a;
    		sm.SerializeToString(&a);
    		char bus[a.size()+1];
    		strcpy(bus,a.c_str());
    		write(cli->sockfd, bus,strlen(bus));
    	}else{
    		ErrorResponse *er(new ErrorResponse);
    		er->set_errormessage("Mensaje de tipo no admitido para conexion. Se debe enviar mensaje de tipo synchronize\n");
    		ServerMessage sm;
    		sm.set_option(3);
    		sm.set_allocated_error(er);
    		string a;
    		sm.SerializeToString(&a);
    		char bus[a.size()+1];
    		strcpy(bus,a.c_str());
    		write(cli->sockfd, bus,strlen(bus));
    	}
    	bzero(buffer, BUFFER_SZ);
    	if(recv(cli-> sockfd, buffer, BUFFER_SZ, 0)<=0){
    		leave_flag =1;
    	}else{
    		string s_res(buffer);
    		ClientMessage cm;
    		cm.ParseFromString(s_res);
    		if(cm.option()==6){
    		strcpy(name,m2.synchronize().username().c_str());
    		}else{
    		ErrorResponse *er(new ErrorResponse);
    		er->set_errormessage("Mensaje de tipo no admitido para conexion. Se debe enviar mensaje de tipo acknowledge\n");
    		ServerMessage sm;
    		sm.set_option(3);
    		sm.set_allocated_error(er);
    		string a;
    		sm.SerializeToString(&a);
    		char bus[a.size()+1];
    		strcpy(bus,a.c_str());
    		write(cli->sockfd, bus,strlen(bus));
    		}
    	}
	}
    if(leave_flag){
    	cli_count=cli_count;
    }
    else if(strlen(name) < 2 || strlen(name) >= NAME_LEN -1){
        printf("ingrese un nombre aceptable");
        leave_flag =1;
    }else{
        strcpy(cli -> name, name);
        sprintf(buffer, "%s has joined\n", cli -> name);
        printf("%s", buffer);
        //send_message(buffer, cli -> uid);
    }
    //vacia el buffer dejando en ceros la data dentro de el
    bzero(buffer, BUFFER_SZ);
    while(1){
        
        if(leave_flag){
            break;
        }
        
        //aqui se recive el mensaje del cliente si es que se ha logeado con su nombre correctamente.
        //se recive el mensaje del cliente al socket asociado. se manda el mensaje a almacenar en el buffer
        int receive = recv(cli -> sockfd, buffer, BUFFER_SZ, 0);
        //se revisa que no sea cero para ver que no este vacio ni receive, ni el buffer.
        if(receive > 0){
            string mensaje(buffer);
			ClientMessage cm;
			cm.ParseFromString(mensaje);
			if(cm.option()==1){
			}else if(cm.option()==2){
				ServerMessage sm;
				sm.set_option(5);
				ConnectedUserResponse *cur(new ConnectedUserResponse);
				int i;
				for(i=0;i<MAX_CLIENTS;i++){
					ConnectedUser *cu=cur->add_connectedusers();
					if (clients[i]){
						cu->set_username(clients[i]->name);
						cu->set_userid(clients[i]->uid);
						cu->set_status(clients[i]->status);
					}
					
				}
				sm.set_allocated_connecteduserresponse(cur);
				string bin;
    			sm.SerializeToString(&bin);
    			char buss[bin.size()+1];
    			strcpy(buss,bin.c_str());
    			send(cli->sockfd, buss, strlen(buss),0);
			}else if(cm.option()==3){
			//IDK
			}else if(cm.option()==4){
				ServerMessage sm;
				sm.set_option(1);
				BroadcastMessage *bm(new BroadcastMessage);
				bm->set_message(cm.broadcast().message());
				bm->set_userid(cli->uid);
				bm->set_username(name);
				sm.set_allocated_broadcast(bm);
				string bin;
    			sm.SerializeToString(&bin);
    			char buss[bin.size()+1];
    			strcpy(buss,bin.c_str());
    			int i;
    			for (i=0;i<MAX_CLIENTS;i++){
    				if(clients[i]){
		        		if(clients[i] -> uid != cli -> uid){
		            //se verifica que no sea un mensaje vacio el cual se manda por el socket
		            		if(write(clients[i] -> sockfd, buss, strlen(buss)) < 0){
		            	   		printf("ERROR: escrito al descriptor fallo\n");
		            	    	break;
		            		}
		        		}
            		}
    			}
    			
			}else if(cm.option()==5){
				DirectMessageRequest dmr=cm.directmessage();
				if (dmr.has_userid()){
					int i;
					for (i=0;i<MAX_CLIENTS;i++){
						if (clients[i]){
							if(clients[i]->uid==dmr.userid()){
								ServerMessage sm;
								sm.set_option(2);
								DirectMessage *dm(new DirectMessage);
								dm->set_message(dmr.message());
								dm->set_userid(cli->uid);
								dm->set_username(cli->name);
								sm.set_allocated_message(dm);
								string bin;
    							sm.SerializeToString(&bin);
    							char buss[bin.size()+1];
    							strcpy(buss,bin.c_str());
								write(clients[i]->sockfd,buss,strlen(buss));
							}
						}
					}
				}else if (dmr.has_username()){
					int i;
					for (i=0;i<MAX_CLIENTS;i++){
						if (clients[i]){
							if(clients[i]->name==dmr.username()){
								ServerMessage sm;
								sm.set_option(2);
								DirectMessage *dm(new DirectMessage);
								dm->set_message(dmr.message());
								dm->set_userid(clients[i]->uid);
								dm->set_username(clients[i]->name);
								sm.set_allocated_message(dm);
								string bin;
    							sm.SerializeToString(&bin);
    							char buss[bin.size()+1];
    							strcpy(buss,bin.c_str());
								write(clients[i]->sockfd,buss,strlen(buss));
							}
						}
					}
				}else{
					ServerMessage sm;
					sm.set_option(8);
					DirectMessageResponse *dmr (new DirectMessageResponse);
					string a="Error, mensaje no contenía nombre o userid";
					dmr->set_messagestatus(a);
					sm.set_allocated_directmessageresponse(dmr);
				}
			}
        }else if(receive == 0 || strcmp(buffer, "exit")==0){ //esta parte es para deja la sala de chat
            sprintf(buffer, "%s has left\n", cli -> name); //se muestra el mensaje de que tal usario dejo el chat
            printf("%s" , buffer); //muestra mensaje del buffer
            send_message(buffer, cli -> uid);//manda el mensaje al chat general para todos
            leave_flag = 1;
        }else{
            printf("ERROR: -1\n");//condicion en caso de que hubiese error
            leave_flag =1;
        }
        //vacia el buffer
        bzero(buffer, BUFFER_SZ);
    }
    close(cli -> sockfd);//se cierra el socket, por el cual el cliente se esta comunicando
    queue_remove(cli -> uid);//se manda el uid para remover al user que se desconecto
    free(cli); //se limpia la variable que esta usando el cliente con su info
    cli_count--; //se baja el indicador de la cantidad de clientes
    pthread_detach(pthread_self());//no se aun que es esto.
    
    return NULL;
}

int main(int argc, char **argv)
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
