#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>


//Variables globales
int pathLen = 0;
char **path;
static char error_message[25] = "An error has occurred\n";
//Firma de los métodos
int execute(char *comand);
int executeFileOrComand(char *comand, int paralel);
int selectComand(char *comand, int paralel);

int main(int argc, char const *argv[]){
    //path inicial
    pathLen = 0;
    path = (char **)realloc(path, (pathLen + 1) * sizeof(char *));
    path[pathLen] = strdup("/bin");
    pathLen++;
    //Flag para salir de la shell
    short exitBash = 0;
    //Validar los argumentos para entrar en el modo bash
    if (argc < 2){
        char *comand;
        size_t bytesNumber = 0;
        int readBytes;
            //Ciclo para generar el efecto de consola
            do{
                printf("wish>  ");
                readBytes = getline(&comand, &bytesNumber, stdin);
                char realComand[readBytes];
                strncpy(realComand, comand, readBytes - 1);
                realComand[readBytes - 1] = '\0';

                //Caso en el que se produzca un error o no se ingrese un comando bien
                if (readBytes == -1){
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
                else if (readBytes > 1){
                    exitBash = execute(realComand);
                }
            } while (exitBash == 0);
        
    }
    //En caso que se ingrese un script con las instrucciones
    else if (argc == 2){

        //Manejo del archivo ingresado
        FILE *file = fopen(argv[1], "r");
        if (file != NULL){
            char *line;
            size_t len = 0;
            ssize_t read;
            char *spacePoint;
            int spaceIndex;
            int spaces = 0;
            while (((read = getline(&line, &len, file)) != -1) && (exitBash == 0)){
                //se omite el simbolo # que es usado como comentario
                if (strchr(line, '#') != NULL){
                    continue;
                }

                //Buscamos primer espacio en la linea leída
                spacePoint = strchr(line, ' ');
                spaceIndex = (spacePoint == NULL ? -1 : spacePoint - line);

                if (spaceIndex == 0){
                    spaces = consecutiveSpaces(line, spaceIndex, 0);
                }

                char realLine[read - spaces];
                if (read - spaces < 1){
                    continue;
                }
                if (strchr(line, '\n') != NULL){
                    subString(line, spaces, read - spaces - 1, realLine);
                    realLine[read - spaces - 1] = '\0';
                }
                else{
                    strncpy(realLine, line, read);
                }
                exitBash = execute(realLine);
            }
            fclose(file);
        }
        else{
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }
    }
    else{
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }
    exit(0);
}

int execute(char *comand){
    int exitBash = 0;
    //Separamos los comandos ingresados
    char *paralelPoint = strchr(comand, '&'); 
    int paralelIndex = (paralelPoint == NULL ? -1 : paralelPoint - comand); 
    if (paralelIndex == -1){
        exitBash = executeFileOrComand(comand, 0);
    }
    else if (paralelIndex > 1){
        int acomulatedLen = strlen(comand);
        char acomulated[acomulatedLen];
        char temp[acomulatedLen];
        strcpy(acomulated, comand);
        int comandLen;
        int spaces;
        int pSize = 0;
        for (size_t i = 0; i < acomulatedLen; i++){
            if (acomulated[i] == '&'){
                pSize++;
            }
        }
        int pids[pSize + 1];
        int pNumber = 0;
        do{
            paralelPoint = strchr(acomulated, '&');
            paralelIndex = (paralelPoint == NULL ? -1 : paralelPoint - acomulated);
            if (paralelIndex != -1){
                spaces = consecutiveSpaces(acomulated, paralelIndex - 1, 1);
                comandLen = paralelIndex - spaces;
            }
            else{
                comandLen = acomulatedLen;
            }
            char paralelComand[comandLen];
            subString(acomulated, 0, comandLen, paralelComand);
            if (paralelIndex != -1){
                strcpy(temp, acomulated);
                spaces = consecutiveSpaces(acomulated, paralelIndex + 1, 0);
                acomulatedLen = acomulatedLen - paralelIndex - spaces - 1;
                subString(temp, paralelIndex + spaces + 1, acomulatedLen, acomulated);
            }
            int rc = fork();

            if (rc == -1){
                write(STDERR_FILENO, error_message, strlen(error_message));
            }
            else if (rc == 0){
                executeFileOrComand(paralelComand, 1);
            }
            else{
                pids[pNumber] = rc;
                pNumber++;
            }
         } while (paralelIndex != -1);

        int status;
        for (size_t i = 0; i < pNumber; i++){
            waitpid(pids[i], &status, 0);
        }
    }
    return exitBash;
}

//Método para validar si se ingreso un comando o un Script y verificamos si hay redirección
int executeFileOrComand(char *comand, int paralel){
    int exitConsole = 0;

    char *initCommand;
    char *redirectFile;

    //busco el signo de redireccion para saber si es necesario realizarla
    char *redicetionPint = strchr(comand, '>');
    int redirectIndex = (redicetionPint == NULL ? -1 : redicetionPint - comand);
    int console = dup(1);
    if (redirectIndex == -1){
        initCommand = (char *)malloc(strlen(comand));
        strcpy(initCommand, comand);
    }
    else if (redirectIndex == 0){
        write(STDERR_FILENO, error_message, strlen(error_message));
        return exitConsole;
    }
    else{
        //si hay redireccion separo el comando del archivo destino
        int spaces = consecutiveSpaces(comand, redirectIndex - 1, 1);
        //error si no hay comando
        if (redirectIndex - spaces == 0){
            write(STDERR_FILENO, error_message, strlen(error_message));
            return exitConsole;
        }
        initCommand = (char *)malloc(redirectIndex - spaces);
        subString(comand, 0, redirectIndex - spaces, initCommand);
        spaces = consecutiveSpaces(comand, redirectIndex + 1, 0);
        int fileNameLen = strlen(comand) - redirectIndex - spaces - 1;
        //error si no hay archivo destino
        if (fileNameLen == 0){
            write(STDERR_FILENO, error_message, strlen(error_message));
            return exitConsole;
        }
        redirectFile = (char *)malloc(fileNameLen);
        subString(comand, redirectIndex + spaces + 1, fileNameLen, redirectFile);
        char *error = strchr(redirectFile, ' ');
        //error si existe otro argumento despues de la redirección
        if (error != NULL){
            write(STDERR_FILENO, error_message, strlen(error_message));
            return exitConsole;
        }
        //error si hay mas de una redirección 
        error = strchr(redirectFile, '>');
        if (error != NULL){
            write(STDERR_FILENO, error_message, strlen(error_message));
            return exitConsole;
        }
    }
    FILE *file = fopen(initCommand, "r");

    if (file == NULL){
        //si hay redirección se desvia a la salida al archivo destino
        if (redirectIndex != -1){
            int fd = open(redirectFile, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            dup2(fd, 1);
            close(fd);
        }
        exitConsole = selectComand(initCommand, paralel);
        
        //despues de imprimir el comando se reestablece la salida a la consola
        if (redirectIndex != -1){
            dup2(console, 1);
            close(console);
        }
    }
    else{
        char *line;
        size_t len = 0;
        ssize_t read;
        //si hay redirección se desvia a la salida al archivo destino
        if (redirectIndex != 1){
            int fd = open(redirectFile, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

            dup2(fd, 1); 

            close(fd);
        }
        while ((read = getline(&line, &len, file)) != -1){
            if (strchr(line, '#') != NULL){
                continue;
            }
            char realLine[read];
            if (strchr(line, '\n') != NULL){
                strncpy(realLine, line, read - 1);
                realLine[read - 1] = '\0';
            }
            else{
                strncpy(realLine, line, read);
            }
            exitConsole = selectComand(realLine, 0);
        }
        fclose(file);
        //despues de imprimir el comando se reestablece la salida a la consola
        if (redirectIndex != -1){
            dup2(console, 1);
            close(console);
        }
        if (paralel != 0){
            exit(0);
        }
    }
    return exitConsole;
}

//Segun el comando redireccionamos al método correspondiente, si es integrado o a la ejecucion general si no lo es
int selectComand(char *comand, int paralel){
    int exit = 0;
    if (strncmp(comand, "exit", 4) == 0){
        if (strlen(comand) < 5){
            exit = 1;
        }else{
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
    }else if (strncmp(comand, "cd", 2) == 0){
        comandCD(comand);
    }else if (strncmp(comand, "path", 4) == 0){
        comandPath(comand);
    }else{
        executeComand(comand, paralel);
    }
    return exit;
}

