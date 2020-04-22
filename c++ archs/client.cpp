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

volatile sig_atomic_t flag = 0;
int sockfd = 0;
char name[NAME_LEN];

//no estoy seguro de esta parte
//segun entiendo, sirve para limpiar la salida de un buffer y mover la data del buffer a la consola
void str_overwrite_stdout(){
    printf("\r%s", " > ");
    fflush(stdout);
}
//esta es una funcion para limpiar el \n del mesnaje cuando se manda y cambiarlo por un valor vacio
void str_trim_lf(char* arr, int length){
    int i= 0;
    for(i=0; i<length; i++){
        if(arr[i] == '\n'){
            arr[i] = '\0';
            break;
        }
    }
}


void catch_ctrl_c_and_exit(int var){
	flag = 1;
}

void help(){
	printf("A continuacion esta la lista de todos los comandos, para que funcionan y como usarlos\n~exit - use este comando para salir del chat o presione CTRL left + C \n~help - use este comando para desplegar esta ventana de nuevo\n~clients - use este comando para desplegar la lista de todos los clientes conectados al chat\n~dm - use este comnado para mandar un mensaje a un usuario en específico. Le pedirá información que puede encontrar en \"~clients\"\n");
} 

void *recv_msg_handler(void *){
	char message[BUFFER_SZ] = {};
	while(1){
		int receive = recv(sockfd, message, BUFFER_SZ, 0);
		if(receive > 0){
			string mensaje(message);
			ServerMessage sm;
			sm.ParseFromString(mensaje);
			if(sm.option()==1){
				if (sm.broadcast().has_username()){
					cout<<"[Broadcast] "<<sm.broadcast().username()<<": "<<sm.broadcast().message()<<endl;
					str_overwrite_stdout();
				}else{
					cout<<sm.broadcast().userid()<<": "<<sm.broadcast().message()<<endl;
					str_overwrite_stdout();
				}
			}else if(sm.option()==2){
				if(sm.message().has_username()){
					cout<<"[Private] "<<sm.message().username()<<": "<<sm.message().message();
					str_overwrite_stdout();
				}
				else{
					cout<<"[Private] "<<sm.message().userid()<<": "<<sm.message().message();
					str_overwrite_stdout();
				}
			}else if(sm.option()==3){
				cout<<"Error: "<<sm.error().errormessage()<<endl;
			}else if(sm.option()==5){
				int a=sm.connecteduserresponse().connectedusers_size();
				int i;
				for(i=0;i<a-1;i++){
					ConnectedUser cu=sm.connecteduserresponse().connectedusers()[i];
					cout<<"____________"<<endl;
					str_overwrite_stdout();
					cout<<"[Server] "<<"Username: "<<cu.username()<<endl;
					str_overwrite_stdout();
					if (cu.has_userid()){
						cout<<"[Server] "<<"UserId: "<<cu.userid()<<endl;
						str_overwrite_stdout();
					}
					cout<<"____________"<<endl;
					str_overwrite_stdout();
				}
			}else if (sm.option()==8){
				cout<<"[Server] "<<sm.directmessageresponse().messagestatus();
				str_overwrite_stdout();
			}
			
		}else if(receive == 0){
			break;
		}
		bzero(message, BUFFER_SZ);
	}
}

void *send_msg_handler(void *){
	char message[BUFFER_SZ] = {};
	char buffer[BUFFER_SZ + NAME_LEN] = {};

	while(1){
		str_overwrite_stdout();
		fgets(buffer, BUFFER_SZ, stdin);
		str_trim_lf(buffer, BUFFER_SZ);

		if (strcmp(buffer, "~exit") == 0)
		{
			break;
		}else if(
			strcmp(buffer, "~help")==0 ||
			strcmp(buffer, "~clients")==0||
			strcmp(buffer, "~exit")==0||
			strcmp(buffer, "~dm")==0||
			(buffer[0] == '~' && buffer[1] == 'i' && buffer[2] == 'n' && buffer[3] == 'f' && buffer[4] == 'o' )){
			
		    if (strcmp(buffer, "~help") == 0){
		    	help();
		        }
		    else if (strcmp(buffer, "~clients") == 0){
		    	ClientMessage cm;
		    	cm.set_option(2);
		    	connectedUserRequest *cur(new connectedUserRequest);
		    	cm.set_allocated_connectedusers(cur);
    			string binary;
    			cm.SerializeToString(&binary);
    			char bus[binary.size()+1];
    			strcpy(bus,binary.c_str());
    			send(sockfd, bus, strlen(bus),0);
		    	}
		    else if (buffer[0] == '~' && buffer[1] == 'i' && buffer[2] == 'n' && buffer[3] == 'f' && buffer[4] == 'o' ){
				printf("En caso no aparezca informacion despues este mensaje, puede ser a que no escribio bien el comando o el nombre de la persona.\n");
		    	}
		    else if(!strcmp(buffer,"~dm")){
		    	ClientMessage cm;
		    	DirectMessageRequest *dmr(new DirectMessageRequest);
				bzero(buffer, BUFFER_SZ + NAME_LEN);
		    	cout<<"Ingrese el nombre de usuario del destinatario (si no lo tiene ingrese \"-\")"<<endl;
				str_overwrite_stdout();
				fgets(buffer, BUFFER_SZ, stdin);
				if(!(strcmp(buffer,"-")==0||strcmp(buffer,"")==0)){
					string a(buffer);
					dmr->set_username(a);
				}
				bzero(buffer, BUFFER_SZ + NAME_LEN);
		    	cout<<"Ingrese el UserId del destinatario (si no lo tiene ingrese \"-\")"<<endl;
				str_overwrite_stdout();
				fgets(buffer, BUFFER_SZ, stdin);
				if(strcmp(buffer,"-") && strcmp(buffer,"")){
					string a(buffer);
					int b=stoi(a);
					dmr->set_userid(b);
				}
				cout<<"Ingrese el mensaje que desea enviar"<<endl;
				str_overwrite_stdout();
				fgets(buffer, BUFFER_SZ, stdin);
				string a(buffer);
				dmr->set_message(a);
				cm.set_option(5);
				cm.set_allocated_directmessage(dmr);
				string o;
				cm.SerializeToString(&o);
				char bus[o.size()+1];
				strcpy(bus,o.c_str());
				send(sockfd, bus, strlen(bus),0);
				
		    }
		    else {
		        printf("No valido :(");
		    }

		}else{
			ClientMessage cm;
			cm.set_option(4);
			string msg(buffer);
			BroadcastRequest *br(new BroadcastRequest);
			br->set_message(msg);
			cm.set_allocated_broadcast(br);
    		string binary;
    		cm.SerializeToString(&binary);
    		char bus[binary.size()+1];
    		strcpy(bus,binary.c_str());
    		send(sockfd, bus, strlen(bus),0);
		}

	bzero(message, BUFFER_SZ);
	bzero(buffer, BUFFER_SZ + NAME_LEN);
	}
	catch_ctrl_c_and_exit(2);
}

int main(int argc, char **argv){

	if(argc != 4){
        printf("Debe ingresar de las siguiente manera los parametros correspondientes al correr el programa\n");
        printf("<nombredelcliente> <nombredeusuario> <IPdelservidor> <puertodelservidor>\n");
        printf("por ejemplo: ./client jose 127.0.0.1 4444\n");
        //printf("Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

	strcpy(name, argv[1]);
	char *ip = argv[2];
    	int port = atoi(argv[3]);

	signal(SIGINT, catch_ctrl_c_and_exit);
	
	printf("%s\n",argv[1]);
	str_trim_lf(argv[1], strlen(argv[1]));

	if(strlen(argv[1]) > NAME_LEN - 1 ||  strlen(argv[1]) < 2){
		printf("Ingrese un nombre correctamente\n");
		return EXIT_FAILURE;
	}

/*
	printf("Ingrese un nombre: ");
	fgets(name, NAME_LEN, stdin);
	str_trim_lf(name, strlen(name));

	if(strlen(name) > NAME_LEN - 1 ||  strlen(name) < 2){
		printf("Ingrese un nombre correctamente\n");
		return EXIT_FAILURE;
	}
*/
	struct sockaddr_in server_addr;
	//configuracion de los sockets
    sockfd = socket(AF_INET, SOCK_STREAM,0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    //conectar al servidor
    int err = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(err == -1){
    	printf("ERROR: conectado\n");
    	return EXIT_FAILURE;
    }

    //Enviar el nombre
    
    
    MyInfoSynchronize * miInfo(new MyInfoSynchronize);
    miInfo->set_username(argv[1]);
    miInfo->set_ip(argv[2]);

    ClientMessage m;
    m.set_option(1);
    m.set_allocated_synchronize(miInfo);

    string binary;
    m.SerializeToString(&binary);
    char bus[binary.size()+1];
    strcpy(bus,binary.c_str());
    send(sockfd, bus, strlen(bus),0);
    
    //recibe el acknowledge
    char message[BUFFER_SZ];
	int recieve = recv(sockfd, message, BUFFER_SZ, 0);
	if (recieve){
		string acknol(message);
		ServerMessage sm;
		sm.ParseFromString(acknol);
		if (sm.option()==4){
			
			MyInfoAcknowledge * ack(new MyInfoAcknowledge);
    		ack->set_userid(sm.myinforesponse().userid());
    		ClientMessage m;
    		m.set_option(6);
    		m.set_allocated_acknowledge(ack);
    		
   		 	string bin;
    		m.SerializeToString(&bin);
    		char buss[bin.size()+1];
    		strcpy(buss,bin.c_str());
    		send(sockfd, buss, strlen(buss),0);

 
		}else if(sm.option()==3){
			cout<<sm.error().errormessage()<<endl;
			return EXIT_FAILURE;
		}
		else{
			cout<<"Error en respuesta del servidor 1"<<endl;
			return EXIT_FAILURE;
		}
    }else{
		cout<<"Error en respuesta del servidor 2"<<endl;
		return EXIT_FAILURE;
    }
	
    printf("///WELCOME TO THE CHAT///\nPara saber todos los comando del chat ponga el comando ~help y presione ENTER\n");

    pthread_t send_msg_thread;
    if(pthread_create(&send_msg_thread, NULL, send_msg_handler, NULL) != 0){
    	printf("ERROR: pthread\n");
    	return EXIT_FAILURE;
    }

    pthread_t recv_msg_thread;
    if(pthread_create(&recv_msg_thread, NULL, recv_msg_handler, NULL) != 0){
    	printf("ERROR: pthread\n");
    	return EXIT_FAILURE;
    }

    while(1){
    	if(flag){
    		printf("\nVuelva pronto\n");
    		break;
    	}
    }

    close(sockfd);

	return EXIT_SUCCESS;
}







