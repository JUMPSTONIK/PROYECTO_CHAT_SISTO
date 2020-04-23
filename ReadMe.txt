Instrucciones para compilación y ejecución

------Compilación------

Para compilar los archivos requiere que se tenga g++ 7 (Está presente en la version de ubuntu 18.04). 
Se debe instalar protobuf c++ de acuerdo a las intrucciones de acuerdo con las instrucciones presentes en "https://github.com/protocolbuffers/protobuf/blob/master/src/README.md"
Luego se ejecuta la funcion make de linux en la carpeta donde se incluye el Makefile y se compilará resultando en los archivos "client" y "server"

Si no se puede realizar el proceso previo a la ejecución, ya se incluyen los compilados de "client" y "server"

------Ejecución------

PAra ejecutar un servidor se escribe el comando para ejecutar ("./server") seguido del numero de socket a utilizar.
Ej. $ ./server 4444

Para iniciar un cliente, debe de estarse ejecutando el servidor y se inicializa con el comando de ejecutar, seguido de un argumento de una palabra para indicar username, luego un argumento escribiendo la dirección ip del cliente y finalizando con el numero de socket que utiliza el servidor.
Ej. $ ./client user1 127.0.0.1 4444


luego el cliente obtendrá la pantalla de comando. Para saber los comandos que puede utilizar. Estos comandos consisten de una virgulilla (~) seguido de un identificador. SI se agrega algo más allá del comando se tomará como un mensaje en lugar de un comando.
los comandos son ~help, ~dm, ~exit y ~clients. Si se escriben por su cuenta haran una de las proximas opciones
	~help: help mostrará todos los comandos que se pueden realizar y sus efectos especificos
	~clients: mostrará todos los clientes conectados, su username y userId.
	~dm: iniciara el proceso de enviarle un mensaje privado a una persona. lo cual pedirá username, userid y el mensaje. Estos datos se pueden obtener con ~clients.
	~exit: saldrá del programa, funciona de la misma manera que ingresar Ctrl + C durante la ejecución del programa.
El no ingresar un comando o escribir algo más luego del mismo(ej. "> ~help texto extra" o "Este es un mensaje") lanzará un mensaje global el cual será visto por todos los usuarios conectados.

Al llegar un mensaje se puede saber su origen por medio del "[*tag]" agregado al inicio
Ej. > [Server] Mensaje de error
Los tags que se pueden ver son [Server] mensajes automatizados del server. [Broadcast] mensajes generales a todos los usuario
y [Private] denotando un mensaje privado.
[Broadcast] y [Private] van seguidos por el username del usuario que envió el mensaje
Ej. > [Private] Andreé: Hola. Este es un mensaje privado.
      [Broadcast] Julio: Hola todos! Este es un mensaje publico.


