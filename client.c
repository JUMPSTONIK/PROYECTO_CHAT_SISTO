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


void catch_ctrl_c_and_exit(){
	flag = 1;
} 

void recv_msg_handler(){
	char message[BUFFER_SZ] = {};
	while(1){
		int receive = recv(sockfd, message, BUFFER_SZ, 0);
		if(receive > 0){
			printf("%s ", message);
			str_overwrite_stdout();
		}else if(receive == 0){
			break;
		}
		bzero(message, BUFFER_SZ);
	}
}

void send_msg_handler(){
	char buffer[BUFFER_SZ] = {};
	char message[BUFFER_SZ + NAME_LEN] = {};

	while(1){
		str_overwrite_stdout();
		fgets(buffer, BUFFER_SZ, stdin);
		str_trim_lf(buffer, BUFFER_SZ);

		if (strcmp(buffer, "exit") == 0)
		{
			break;
		}else {
			sprintf(message, "%s: %s\n", name, buffer);
			send(sockfd, message, strlen(message), 0);
		}

	bzero(buffer, BUFFER_SZ);
	bzero(message, BUFFER_SZ + NAME_LEN);
	}
	catch_ctrl_c_and_exit(2);
}

int main(int argc, char **argv){
	if(argc != 2){
		printf("Usage: %s <port>\n", argv[0]);
		return EXIT_FAILURE;
	}

	char *ip = "127.0.0.1";
	int port = atoi(argv[1]);

	signal(SIGINT, catch_ctrl_c_and_exit);

	printf("Ingrese un nombre: ");
	fgets(name, NAME_LEN, stdin);
	str_trim_lf(name, strlen(name));

	if(strlen(name) > NAME_LEN - 1 ||  strlen(name) < 2){
		printf("Ingrese un nombre correctamente\n");
		return EXIT_FAILURE;
	}

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
    send(sockfd, name, NAME_LEN,0);

    printf("///WELCOME TO THE CHAT///\n");

    pthread_t send_msg_thread;
    if(pthread_create(&send_msg_thread, NULL, (void*)send_msg_handler, NULL) != 0){
    	printf("ERROR: pthread\n");
    	return EXIT_FAILURE;
    }

    pthread_t recv_msg_thread;
    if(pthread_create(&recv_msg_thread, NULL, (void*)recv_msg_handler, NULL) != 0){
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







