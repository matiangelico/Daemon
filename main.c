#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#define LOCK_FILE "/home/matete/Documentos/Facu/SoftwareLibre/MyDaemon/daemon.pid"
#define PATH_TRASH "/home/matete/.local/share/Trash/files/*.txt"

void eliminarTxtPapelera ();
int obtenerPidArchivo();
int buscaArchivo();
void salirError();
static void skeleton_daemon();
int main();
void setearPidArchivo();

void eliminarTxtPapelera (){
    char cmd[1024];
    strcpy(cmd,"rm ");
    strcat(cmd,PATH_TRASH);
    system(cmd);
}

int buscaArchivo() {

    FILE *fd;
    fd = fopen(LOCK_FILE, "r");
    if ( fd != NULL) {          //hay archivo = 1
        fclose(fd);
        return 1;
    }
    return 0;
}

void setearPidArchivo(int pid){

    FILE *fd;

    fd = fopen(LOCK_FILE,"a+");

    fprintf(fd,"%d \n",pid);       // Escribir el PID del proceso en el archivo de bloqueo

    fclose(fd);

}

int obtenerPidArchivo(){

    char pid[5];

    FILE *fd;

    fd = fopen(LOCK_FILE,"r");

    fread(pid,sizeof(pid),1,fd);       // Escribir el PID del proceso en el archivo de bloqueo

    fclose(fd);

    return atoi(pid);
}

void salirError(){
    remove(LOCK_FILE);
    exit(EXIT_FAILURE);
}

static void skeleton_daemon(){

    pid_t pid = fork(); /* Primer fork */

    if (pid < 0)
        salirError();

    if (pid > 0){    /* Termina proceso Padre*/
        //printf("Primer deamon iniciado con PID %d \n",pid);
        exit(EXIT_SUCCESS); //sale del proceso padre
    }

    if (setsid() < 0){
        //printf("Error al establecer el proceso hijo como lider de sesion.\n");
        salirError();
    }

    pid = fork();       //Segundo fork

    if (pid < 0)
        salirError();

    if (pid > 0){     //Termina con el proceso Padre
        //printf("Segundo Deamon iniciado con PID %d \n",pid);
        exit(EXIT_SUCCESS); //sale del proceso padre
    }

    umask(0);     /* setea umask para que deamon tenga permisos */

    if (chdir("/")<0)    //Cambia el directorio de trabajo actual al raiz
        salirError();

    for (int x = sysconf(_SC_OPEN_MAX); x>=0; x--) close (x);      // Cierra todos los descriptores

}

int main(int argc,char *argv[])
{
    int hayArchivo = buscaArchivo();

    if (argc <= 1 || argc>2 || (strcmp(argv[1],"start")!=0 && strcmp(argv[1],"stop")!=0) && strcmp(argv[1],"info")!=0){
        printf("Error ./mydeamon start or ./mydeamon stop or ./mydeamon info\n");
        exit(EXIT_FAILURE);
    }
    if (strcmp(argv[1],"info")==0){
        printf("Daemon info\n");
        printf("./daemon start -> inicia el daemon\n");
        printf("./daemon stop  -> detiene el daemon\n");
        printf("./daemon info  -> informacion de parametros\n");
        exit(EXIT_SUCCESS);
    }
    if (strcmp(argv[1],"start")==0 && hayArchivo) {
        printf("El daemon ya está en ejecución.\n");
        exit(EXIT_FAILURE);
    }
    if (strcmp(argv[1],"stop")==0 && hayArchivo) {
        kill(obtenerPidArchivo(),SIGTERM);
        remove(LOCK_FILE);
        exit(EXIT_SUCCESS);
    }
    if (strcmp(argv[1],"stop")==0 && !hayArchivo) {
        printf("El deamon no esta en ejecución.\n");
        exit(EXIT_SUCCESS);
    }

    skeleton_daemon();

    setearPidArchivo(getpid());

    while (1)
    {
        eliminarTxtPapelera();
        sleep(20);
    }
}
