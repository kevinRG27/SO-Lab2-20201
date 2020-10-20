#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>


int main(int argc, char *argv[]){
    //Verificación de parametros
    if (argc == 1){
        printf("time: time [COMMAND]\n");
        return 1;
    }
    //Declaraciones para los timepos
    struct timeval final, inicial;
    //Obtenemos el tiempo inicial
    gettimeofday(&inicial, NULL);
    //Creamos el Fork
    int child = fork(), status;
    //Descartamos errores en el Fork
    if (child < 0){
        fprintf(stderr, "fork failed\n");
        return 1;
    }
    //El hijo ejecutara el comando
    if (child == 0){
        //Usamos 'exec' para correr el comando ingresado como argumento
        execlp(argv[1], argv[1], NULL);
        return 1;
    }else{
        //Invocación del wait para el padre
        wait(&status);
        //Si el proceso hijo terminó correctamente se toma el tiempo final y se imprime el tiempo total
        if (WIFEXITED(status) != 0){
            //Obtenemos tiempo final
            gettimeofday(&final, NULL);
            //Imprimimos y calculamos el tiempo gastado
            printf("Elapsed time: %ld.%ld\n", final.tv_sec - inicial.tv_sec, final.tv_usec - inicial.tv_usec);
        }
    }
    return 0;
}