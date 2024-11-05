#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <time.h>
#include <string.h>
#include <sys/select.h>
#include <pthread.h>
#include <ncurses.h>

#define BUFLEN 80
#define DEFAULT_SERVICEPORT 2000
#define MAX_CLIENT 20


#define NUM_SUBWINDOWS 20
#define COLS_COUNT 5
#define ROWS_COUNT 4

const char* paroleImpiccato[20] = {
    "computer", 
    "gattino",  
    "bicicletta",
    "elefante", 
    "occhiali", 
    "zucchero", 
    "lampione", 
    "pianoforte",
    "calciatore",
    "farfalla", 
    "razzo",     
    "libreria", 
    "canguro",  
    "dinosauro", 
    "montagna", 
    "telefono", 
    "campanile", 
    "motocicletta", 
    "violino",  
    "chitarra"  
};

int permits[MAX_CLIENT];
WINDOW *subwindows[NUM_SUBWINDOWS];
int num_thread = 0;


void create_subwindows(WINDOW *subwindows[]) {
    
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    // Calcola dimensioni delle sottofinestre
    int subwin_height = max_y / ROWS_COUNT;   // Altezza di ogni riga
    int subwin_width = max_x / COLS_COUNT;    // Larghezza di ogni colonna

    // Crea le 20 sottofinestre in una griglia di 5 colonne e 4 righe
    for (int i = 0; i < NUM_SUBWINDOWS; i++) {
        int start_y = (i / COLS_COUNT) * subwin_height;  // Riga attuale
        int start_x = (i % COLS_COUNT) * subwin_width;   // Colonna attuale
        subwindows[i] = newwin(subwin_height - 1, subwin_width - 1, start_y, start_x);
        wbkgd(subwindows[i], COLOR_PAIR(2));  // Imposta lo sfondo rosso per ogni sottofinestra            // Aggiungi bordo
        wprintw(subwindows[i], "Thread %d\n\n", i + 1);  // Testo all'interno della finestra
        wrefresh(subwindows[i]);  // Aggiorna ogni sottofinestra
        scrollok(subwindows[i], TRUE);
    }
}

void *handle_client(void* arg){

    int scrive_client = 3;
    int scrive_server = 4;

    init_pair(scrive_client, COLOR_WHITE, COLOR_GREEN);
    init_pair(scrive_server, COLOR_BLACK, COLOR_GREEN);

    struct sockaddr_in local, remote;

    int new_port = *((int*)arg);
    free(arg);
    int thread_number = new_port;

    new_port = DEFAULT_SERVICEPORT + 1000 + new_port;

    unsigned int rm_len = sizeof(remote);
    int s = socket(AF_INET, SOCK_STREAM, 0);

    if(s < 0){
        permits[thread_number] = 0;
        num_thread--;
        return NULL;
    }

    local.sin_family = AF_INET;
    local.sin_port = htons(new_port);
    local.sin_addr.s_addr = htonl(INADDR_ANY);

    if( bind(s, (struct sockaddr*)&local, sizeof(local)) < 0){
        permits[thread_number] = 0;
        num_thread--;
        return NULL;
    }

    if(listen(s, 5) < 0) {
        permits[thread_number] = 0;
        num_thread--;
        return NULL;
    }

    int sc = accept(s, (struct sockaddr*)&remote, &rm_len);
    
    if(sc < 0){
        permits[new_port - 1000 - DEFAULT_SERVICEPORT] = 0;
        num_thread--;
        return NULL;
    }

    char snd_buf[BUFLEN] = {0};
    char rcv_buf[BUFLEN] = {0};
    int rcvlen;

    char parola[10];

    strcpy(parola, paroleImpiccato[rand()%20]);
    
    wprintw(subwindows[thread_number], "client accettato su porta %d\n", new_port);
    wprintw(subwindows[thread_number], "Parola estratta %s\n\n", parola);
    wbkgd(subwindows[thread_number], COLOR_PAIR(scrive_server));

    wrefresh(subwindows[thread_number]);

    int lunghezza_parola = strlen(parola);

    char *parola_to_send = (char*)malloc(lunghezza_parola * sizeof(char));

    for(int i = 0 ; i<lunghezza_parola ; i++){
        parola_to_send[i] = '*';
    }

    int correct_guesses = 0;
    int client_playing = 1;

    while(client_playing){

        if( (rcvlen = recv(sc, rcv_buf, BUFLEN, 0) ) < 0){
            permits[thread_number] = 0;
            num_thread--;
            close(s);
            close(sc);
            return NULL;
        }

        rcv_buf[rcvlen] = '\0';

        int found = 0; 
        char guess = rcv_buf[0];

        wprintw(subwindows[thread_number], "client guessed = %c\n", guess);
        wrefresh(subwindows[thread_number]);

        if(guess == '0'){

            sprintf(snd_buf, "F%s hai deciso di non gicare", parola);
            client_playing = 0;
            
            wprintw(subwindows[thread_number], "%s\n", snd_buf);
            wrefresh(subwindows[thread_number]);

            send(sc, snd_buf, strlen(snd_buf), 0);
            permits[thread_number] = 0;


        }else{
            for (int i = 0; i < lunghezza_parola; i++) {
                if (parola[i] == guess && parola_to_send[i] == '*') {
                    parola_to_send[i] = guess;
                    found = 1;
                    correct_guesses++;
                }
            }

            if (found && correct_guesses == lunghezza_parola) {
                sprintf(snd_buf, "K%s (Parola completata!) indovina un altra parola\n", parola_to_send);

                wbkgd(subwindows[thread_number], COLOR_PAIR(scrive_client));                    
                wprintw(subwindows[thread_number], "%s\n", snd_buf);
                wrefresh(subwindows[thread_number]);

                strcpy(parola, paroleImpiccato[rand()%20]);
                lunghezza_parola = strlen(parola);
                free(parola_to_send);
                parola_to_send = (char*)malloc(lunghezza_parola * sizeof(char));

                for (int i = 0; i < lunghezza_parola; i++) {
                    parola_to_send[i] = '*';
                }

                parola_to_send[lunghezza_parola] = '\0'; 
                correct_guesses = 0;
                found = 0;

                wbkgd(subwindows[thread_number], COLOR_PAIR(scrive_client));
                wprintw(subwindows[thread_number], "nuova parola = %s\n\n", parola);
                wrefresh(subwindows[thread_number]);

            } else if (found) {
                sprintf(snd_buf, "Y%s (Lettera trovata)\n", parola_to_send);

                wbkgd(subwindows[thread_number], COLOR_PAIR(scrive_client));
                wprintw(subwindows[thread_number], "%s\n", snd_buf);
                wrefresh(subwindows[thread_number]);

            } else {
                sprintf(snd_buf, "N%s (Lettera non trovata)\n", parola_to_send);

                wbkgd(subwindows[thread_number], COLOR_PAIR(scrive_client));
                wprintw(subwindows[thread_number], "%s\n", snd_buf);
                wrefresh(subwindows[thread_number]);

            }
        }

        send( sc, snd_buf, strlen(snd_buf), 0);
        
    } 

    

    wclear(subwindows[thread_number]);
    wprintw(subwindows[thread_number], "Thread %d", thread_number+1);
    wbkgd(subwindows[thread_number], COLOR_PAIR(2));
    wrefresh(subwindows[thread_number]);


    permits[thread_number] = 0;
    num_thread--;
    close(sc);
    close(s);

    return NULL;
}

int find_free_port(){
    for(int i = 0 ; i<MAX_CLIENT ; i++){
        if(permits[i] == 0){
            return i;
        }
    }

    return -1;
}

int main(){

    srand(time(NULL));

    initscr();
    cbreak();
    noecho();
    curs_set(0);


    if (!has_colors()) {
        endwin();
        printf("Terminal does not support colors\n");
        return 1;
    }

        start_color();

    // Definisci le coppie di colori
    init_pair(1, COLOR_WHITE, COLOR_BLACK);  // Sfondo nero per lo schermo principale
    init_pair(2, COLOR_WHITE, COLOR_RED);    // Sfondo rosso per le sottofinestre

    bkgd(COLOR_PAIR(1));
    clear();
    refresh();

    create_subwindows(subwindows);
    
    pthread_t thread_client[MAX_CLIENT];
    
    struct sockaddr_in local, remote;
    unsigned int remote_len;

    int s, sc;



    memset(permits, 0, sizeof(permits) );

    char snd_buf[BUFLEN];

    local.sin_family = AF_INET;
    local.sin_port = htons(DEFAULT_SERVICEPORT);
    local.sin_addr.s_addr = htonl(INADDR_ANY);

    if( ( s = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        exit(-1);
    }

    if( bind(s, (struct sockaddr*)&local, sizeof(local)) < 0){
        close(s);
        exit(-1);
    }

    listen(s,MAX_CLIENT);

    while(1){

        remote_len = sizeof(remote);
        if(num_thread < MAX_CLIENT){ 
            sc = accept(s, (struct sockaddr*)&remote, &remote_len);
            if(sc < 0){
                close(s);
                exit(-1);
            }

            int new_client_port = find_free_port();

            if(new_client_port == -1){
                sprintf(snd_buf, "server pieno");
            }else{                

                int* new_client_port_ptr = malloc(sizeof(int));
                *new_client_port_ptr = new_client_port;

                if(  pthread_create(&thread_client[new_client_port], NULL, handle_client, new_client_port_ptr) ){
                    sprintf(snd_buf, "errore nella creazione del thread\n");
                }else{
                    num_thread++;
                    permits[new_client_port] = 1;
                    new_client_port = DEFAULT_SERVICEPORT + 1000 + new_client_port;
                    sprintf(snd_buf, "%d", new_client_port);
                }

            }

            if( send(sc, snd_buf, BUFLEN, 0) < 0 ){
                close(s);
                close(sc);
                exit(-1);
            }


        }

    }

    close(s);
    close(sc);

    for(int i = 0 ; i<MAX_CLIENT ; i++){
        if(permits[i] == 1){
            if(  pthread_join(thread_client[i], NULL) != 0){
                return -1;                
            }
        }
    }


    for (int i = 0; i < NUM_SUBWINDOWS; i++) {
        delwin(subwindows[i]);
    }
    endwin();

    return 0;

}
