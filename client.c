#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <time.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <string.h>
#include <ctype.h>

#define BUFLEN 80
#define DEFAULT_PORT 2000
#define MAX_CONNECTION_TRY 20

const char SERVICENAME[] = "127.0.0.1";

int play(int s){

    char snd_buf[BUFLEN] = {0};
    char rcv_buf[BUFLEN] = {0};
    int playing = 1;
    char player_guess;
    int rcvlen;

    while(playing){

        do{
            printf("Inserisci un carattere (lettera o '0' per uscire): ");
            scanf("%c", &player_guess);
            player_guess = tolower(player_guess);
        }while( !isalpha(player_guess) && player_guess != '0' );

        if(player_guess == '0'){
            playing = 0;
        }

        sprintf(snd_buf, "%c", player_guess);

        if( send(s, snd_buf, strlen(snd_buf), 0) < 0){
            printf("Errore nella send");
            close(s);
            exit(2);
        }

        if ((rcvlen = recv (s, rcv_buf, BUFLEN, 0)) < 0) {
            perror ("errore receive ");
            close(s);
            exit (2);
        }

        rcv_buf[rcvlen] = '\0';
        printf("messaggio ricevuto dal server: %s\n", rcv_buf);

    }   
    
}

int main(){
    struct sockaddr_in local, remote;
    int s;

    ssize_t rcvlen;
    char rcvbuf [BUFLEN];

    int connected_to_new_port = 0;
    int connection_try = 0;

    int new_port = 0;


    struct hostent *h;

    local.sin_family = AF_INET;
    local.sin_port = htons(INADDR_ANY);
    local.sin_addr.s_addr = htons(INADDR_ANY);

    remote.sin_family = AF_INET;
    h = gethostbyname(SERVICENAME);
    remote.sin_addr.s_addr = *((int*)h->h_addr_list[0]);


    while(!connected_to_new_port && (connection_try < MAX_CONNECTION_TRY) ){
        connection_try++;
        remote.sin_port = htons(DEFAULT_PORT);

        if( (s = socket(AF_INET, SOCK_STREAM, 0)) < 0){
            printf("Errore nella creazione del socekt: ");
            printf("%d %d\n", connection_try, new_port);
            exit(-1);
        }else{
            printf("Socket creato con successo\n");
        }

        if( bind(s, (struct sockaddr*)&local, sizeof(local)) < 0){
            printf("Errore nella bind ");
            printf("%d %d\n", connection_try, new_port);
            close(s);
            exit(1);
        }else{
            printf("Bind eseguita con successo\n");
        }



        if( connect(s, (struct sockaddr*)&remote, sizeof(remote)) < 0 ){
            printf("la connect è andata in errore: ");
            printf("%d %d\n", connection_try, new_port);
            close(s);
            exit(-1);
        }else{
            printf("Connessione alla porta default eseguita con successo\n");
        }

        rcvlen = recv(s, rcvbuf, BUFLEN, 0);
        
        if(rcvlen < 0){
            printf("Errore nella receive della nuova porta: ");
            printf("%d %d\n", connection_try, new_port);
            exit(-1);
        }else{
            printf("ricevuto risposta con successo\n");
        }

        rcvbuf[rcvlen] = 0;

        new_port = 0;

        printf("\n\n nuova porta ricevuta dal server: %s \n", rcvbuf);

        for(int i = 0 ; rcvbuf[i] != '\0' ; i++){
            new_port = (new_port * 10) + (rcvbuf[i] - '0');
        }
        printf("new port = %d\n", new_port);
        close(s);

        remote.sin_port = htons(new_port);


        if( (s = socket(AF_INET, SOCK_STREAM, 0)) < 0){
            printf("Errore nella creazione del socekt: ");
            printf("%d %d\n", connection_try, new_port);
            exit(-1);
        }else{
            printf("secondo socket creato successo\n");
        }

        if( bind(s, (struct sockaddr*)&local, sizeof(local)) < 0){
            printf("Errore nella bind ");
            printf("%d %d\n", connection_try, new_port);
            close(s);
            exit(1);
        }else{
            printf("seconda bind successo\n");
        }

        sleep(3);

        if( connect(s, (struct sockaddr*)&remote, sizeof(remote)) < 0){
            perror("errore sulla connect di new port, rimando la connessione a defaultport");
            printf("%d %d\n\n", connection_try, new_port);
            close(s);
        }else{
            play(s);
            connected_to_new_port = 1;
        }
    }


    close(s);

    return 0;
}