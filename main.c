#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <syslog.h>
#include <sys/statvfs.h>
#include <dirent.h>

#define LOCK_FILE "/home/matete/Documentos/Facu/SoftwareLibre/MyDaemon/daemon.pid"
#define TEXT "El porcentaje utilizado de la carpeta es de "

float espAnt = 0.0;

int obtenerPidArchivo();
int buscaArchivo();
void salirError();
static void skeleton_daemon();
int main();
void setearPidArchivo();
void handler(int signum);

unsigned long long obtener_espacio_utilizado(char *carpeta) {
    unsigned long long espacio_utilizado = 0;
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;

    dir = opendir(carpeta) ;

    chdir(carpeta);
    // Itera sobre los archivos en el directorio
    while ((entry = readdir(dir)) != NULL) {
        // Ignora las entradas "." y ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

       // Obtiene la información del archivo
        if (stat(entry->d_name, &statbuf) == -1) {
            perror("stat");
            continue;
        }

        // Si es un directorio, recursivamente calcula su tamaño
        if (S_ISDIR(statbuf.st_mode)) {
            espacio_utilizado += obtener_espacio_utilizado(entry->d_name);
        } else {
            // Suma el tamaño del archivo
            espacio_utilizado += statbuf.st_size;
        }
    }

    // Cierra el directorio
    closedir(dir);

    // Regresa al directorio padre
    chdir("..");

    return espacio_utilizado;
}

void notificacionEnvio(char* text,char* carpeta){

    char instruccion[300]= "echo \"";
    if (strcmp(text,"Comenzo el monitoreo de la carpeta ")==0 || strcmp(text,"Finalizo el monitoreo de la carpeta ")==0)
        strcat(instruccion,text);
    else{
        strcat(instruccion,TEXT);
        strcat(instruccion,text);
        strcat(instruccion,"%");
    }
    strcat(instruccion,"\" | mail -s \"Estado de Carpeta\"");
    strcat(instruccion,carpeta);
    strcat(instruccion," matete@matete");
    system(instruccion);
}

void verificar(char* carpeta, int limite){
    unsigned int espacioUtilizadoKb = obtener_espacio_utilizado(carpeta)/1024;
    float espAct = (float)espacioUtilizadoKb*100/limite;
    char espacio[10];
    snprintf(espacio, sizeof(espacio), "%.2f", espAct);
    if ((espAnt == 0.0)||(espAct>25 && espAnt<25)||(espAct>50 && espAnt<50)||(espAct>75 && espAnt<75)||(espAct>90 && espAnt<90)) {
        if (espAct > 90 && espAnt < 90) {
            syslog(LOG_NOTICE,"La capacidad de la carpeta ha superado el 90 %%");
        }
        espAnt = espAct;
        notificacionEnvio(espacio,carpeta);
     }

    if ((espAct<25 && espAnt>25)||(espAct>25 && espAct<50 && espAnt>50)||(espAct>50 && espAct<75 && espAnt>75)||(espAct>75 && espAct<90 && espAnt>90)) {
        espAnt = espAct;
        notificacionEnvio(espacio,carpeta);
     }

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

void handler(int signum){
    if (signum == SIGTERM){
        syslog (LOG_NOTICE," mydeamon finished.");
        closelog();
        remove(LOCK_FILE);
        exit(EXIT_SUCCESS);
    }
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

    /* Open the log file */
    openlog ("mydaemon",LOG_PID, LOG_DAEMON);
    syslog(LOG_NOTICE,"mydeamon started.");
}

int main(int argc,char *argv[])
{
    int hayArchivo = buscaArchivo();

    if (argc == 2 && strcmp(argv[1],"info")==0){
        printf("Daemon info\n");
        printf("./daemon start [path] [limit]-> inicia el daemon\n");
        printf("./daemon stop  -> detiene el daemon\n");

        exit(EXIT_SUCCESS);
    }

    if (argc == 2 && strcmp(argv[1],"stop")==0 && hayArchivo) {
        kill(obtenerPidArchivo(),SIGTERM);
        notificacionEnvio("Finalizo el monitoreo de la carpeta "," ");
        exit(EXIT_SUCCESS);
    }
    if (argc == 2 && strcmp(argv[1],"stop")==0 && !hayArchivo) {
        printf("ERROR: El deamon no esta en ejecución.\n");
        exit(EXIT_SUCCESS);
    }

    if (argc <= 3 || argc>4 || (strcmp(argv[1],"start")!=0 && strcmp(argv[1],"stop")!=0) && strcmp(argv[1],"info")!=0){
        printf("Error ./mydeamon info para mayor informacion\n");
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1],"start")==0 && hayArchivo) {
        printf("ERROR: El daemon ya está en ejecución.\n");
        exit(EXIT_FAILURE);
    }

    if (opendir(argv[2]) == NULL){
        printf("ERROR: No se pudo abrir el directorio %s\n", argv[2]);
        exit(EXIT_FAILURE);
    }
    char *endptr;
    int numero = strtol(argv[3], &endptr, 10);
    if (*endptr != '\0') {
        printf("ERROR: El limite no es un número válido.\n");
        exit(EXIT_FAILURE);
    }

    skeleton_daemon();

    setearPidArchivo(getpid());

    signal(SIGTERM, handler);

    notificacionEnvio("Comenzo el monitoreo de la carpeta ",argv[2]);

    while (1)
    {
        verificar(argv[2],numero);
        sleep(20);
    }
}
