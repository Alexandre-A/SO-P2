/**
 *  \file semSharedMemSmoker.c (implementation file)
 *
 *  \brief Problem name: SoccerGame
 *
 *  Synchronization based on semaphores and shared memory.
 *  Implementation with SVIPC.
 *
 *  Definition of the operations carried out by the players:
 *     \li arrive
 *     \li playerConstituteTeam
 *     \li waitReferee
 *     \li playUntilEnd
 *
 *  \author Nuno Lau - December 2024
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <string.h>
#include <math.h>

#include "probConst.h"
#include "probDataStruct.h"
#include "logging.h"
#include "sharedDataSync.h"
#include "semaphore.h"
#include "sharedMemory.h"

/** \brief logging file name */
static char nFic[51];

/** \brief shared memory block access identifier */
static int shmid;

/** \brief semaphore set access identifier */
static int semgid;

/** \brief pointer to shared memory region */
static SHARED_DATA *sh;

/** \brief player takes some time to arrive */
static void arrive (int id);

/** \brief player constitutes team */
static int playerConstituteTeam (int id);

/** \brief player waits for referee to start match */
static void waitReferee(int id, int team);

/** \brief player waits for referee to end match */
static void playUntilEnd(int id, int team);

/**
 *  \brief Main program.
 *
 *  Its role is to generate the life cycle of one of intervening entities in the problem: the player.
 */
int main (int argc, char *argv[])
{
    int key;                                            /*access key to shared memory and semaphore set */
    char *tinp;                                                       /* numerical parameters test flag */
    int n, team;

    /* validation of command line parameters */
    if (argc != 4) { 
        freopen ("error_PL", "a", stderr);
        fprintf (stderr, "Number of parameters is incorrect!\n");
        return EXIT_FAILURE;
    }
    

    /* get goalie id - argv[1]*/
    n = (unsigned int) strtol (argv[1], &tinp, 0);
    if ((*tinp != '\0') || (n >= NUMPLAYERS )) { 
        fprintf (stderr, "Player process identification is wrong!\n");
        return EXIT_FAILURE;
    }

    /* get logfile name - argv[2]*/
    strcpy (nFic, argv[2]);

    /* redirect stderr to error file  - argv[3]*/
    freopen (argv[3], "w", stderr);
    setbuf(stderr,NULL);


    /* getting key value */
    if ((key = ftok (".", 'a')) == -1) {
        perror ("error on generating the key");
        exit (EXIT_FAILURE);
    }

    /* connection to the semaphore set and the shared memory region and mapping the shared region onto the
       process address space */
    if ((semgid = semConnect (key)) == -1) { 
        perror ("error on connecting to the semaphore set");
        return EXIT_FAILURE;
    }
    if ((shmid = shmemConnect (key)) == -1) { 
        perror ("error on connecting to the shared memory region");
        return EXIT_FAILURE;
    }
    if (shmemAttach (shmid, (void **) &sh) == -1) { 
        perror ("error on mapping the shared region on the process address space");
        return EXIT_FAILURE;
    }

    /* initialize random generator */
    srandom ((unsigned int) getpid ());                                                 


    /* simulation of the life cycle of the player */
    arrive(n);
    if((team = playerConstituteTeam(n))!=0) {
        waitReferee(n, team);
        playUntilEnd(n, team);
    }

    /* unmapping the shared region off the process address space */
    if (shmemDettach (sh) == -1) {
        perror ("error on unmapping the shared region off the process address space");
        return EXIT_FAILURE;;
    }

    return EXIT_SUCCESS;
}

/**
 *  \brief player takes some time to arrive
 *
 *  Player updates state and takes some time to arrive
 *  The internal state should be saved.
 *
 */
static void arrive(int id)
{    
    if (semDown (semgid, sh->mutex) == -1)  {                                                     /* enter critical region */
        perror ("error on the up operation for semaphore access (PL)");
        exit (EXIT_FAILURE);
    }

    /* TODO: insert your code here */

    // É atribuído o estado de arriving
    (sh->fSt).st.playerStat[id] = ARRIVING; //A
    saveState(nFic,&sh->fSt);
    
    if (semUp (semgid, sh->mutex) == -1) {                                                         /* exit critical region */
        perror ("error on the down operation for semaphore access (PL)");
        exit (EXIT_FAILURE);
    }

    usleep((200.0*random())/(RAND_MAX+1.0)+50.0);
}

/**
 *  \brief player constitutes team
 *
 *  If player is late, it updates state and leaves.
 *  If there are enough free players and free goalies to form a team, player forms team allowing 
 *  team members to proceed and waiting for them to acknowledge registration.
 *  Otherwise it updates state, waits for the forming teammate to "call" him, saves its team
 *  and acknowledges registration.
 *  The internal state should be saved.
 *
 *  \param id player id
 * 
 *  \return id of player team (0 for late goalies; 1 for team 1; 2 for team 2)
 *
 */
static int playerConstituteTeam (int id)
{
    int ret = 0;
    bool waitFormation; // Variável usada para indicar se o player se encontra em espera


    if (semDown (semgid, sh->mutex) == -1)  {                                                     /* enter critical region */
        perror ("error on the up operation for semaphore access (PL)");
        exit (EXIT_FAILURE);
    }


    /* TODO: insert your code here */

    waitFormation = false;
    (sh->fSt).playersArrived += 1;
    (sh->fSt).playersFree +=1;
    if ((sh->fSt).playersArrived>2*NUMTEAMPLAYERS){
        // É atribuído o estado de Late
        (sh->fSt).st.playerStat[id] = LATE; //L
        saveState(nFic,&sh->fSt);
        ret = 0;
    } 
    else{
        if (((sh->fSt).playersFree<NUMTEAMPLAYERS)||((sh->fSt).goaliesFree<NUMTEAMGOALIES)){
            // É atribuído o estado de Waiting
            (sh->fSt).st.playerStat[id] = WAITING_TEAM; // W 
            saveState(nFic,&sh->fSt);
            waitFormation = true;
        }
        else{                    
            // É atribuído o estado de Forming Team
            (sh->fSt).st.playerStat[id] = FORMING_TEAM; // F
            saveState(nFic,&sh->fSt);

            //Remoção do número de elementos constituintes de uma equipa
            (sh->fSt).playersFree = (sh->fSt).playersFree + -NUMTEAMPLAYERS;
            (sh->fSt).goaliesFree = (sh->fSt).goaliesFree + -NUMTEAMGOALIES;

            // Registo dos players
            for (int i = 0; i < NUMTEAMPLAYERS-1; i += 1) {
                if (semUp(semgid,sh->playersWaitTeam)==-1){ // Dá um up ao playersWaitTeam, para que os restantes players
                    perror ("error on the up operation for semaphore access (PL)"); //possam se registar na equipa (ver ultimo if no caso do goalie, semelhante)
                    exit (EXIT_FAILURE);
                }  
                
                if (semDown(semgid,sh->playerRegistered)==-1){
                    perror ("error on the up operation for semaphore access (PL)");
                    exit (EXIT_FAILURE);
                } //Bloqueia o processo do player até o jogador sinalizado (do semUp acima) registar-se na equipa
            }

            // Registo do goalie
            for (int i =0;i<NUMTEAMGOALIES;i+=1){
                if (semUp(semgid,sh->goaliesWaitTeam)==-1){ // Dá um up ao goaliesWaitTeam, para que o(s) goalie(s) possa(m) se registar na equipa 
                    perror ("error on the up operation for semaphore access (PL)");
                    exit (EXIT_FAILURE);
                } 
                if (semDown(semgid,sh->playerRegistered)==-1){
                    perror ("error on the up operation for semaphore access (PL)");
                    exit (EXIT_FAILURE);
                } //Bloqueia o processo do player até o jogador sinalizado (do semUp acima) registar-se na equipa
            }

            ret = (sh->fSt).teamId; //Guarda o id da equipa onde o guarda-redes que forma a equipa vai ficar
            (sh->fSt).teamId = (sh->fSt).teamId + 1; //Atualiza o id da equipa seguinte

            if (semUp(semgid,sh->refereeWaitTeams)==-1){ //Liberta o semáforo que bloqueia o referee, após a formação de cada equipa
                perror ("error on the up operation for semaphore access (PL)");
                exit (EXIT_FAILURE);
            } 
        }
    }

    
    if (semUp (semgid, sh->mutex) == -1) {                                                         /* exit critical region */
        perror ("error on the down operation for semaphore access (PL)");
        exit (EXIT_FAILURE);
    }

    /* TODO: insert your code here */

    // Se ele estiver à espera, espera que um dos players/goalie (com estado Forming) dê up ao semáforo
    // playersWaitTeam (o semDown bloqueia até ser dado esse up), e depois
    // liberta o playerRegistered para o jogador que forma a equipa poder continuar o seu algoritmo (ver acima)
    if (waitFormation){
        if (semDown(semgid,sh->playersWaitTeam)==-1){
            perror ("error on the up operation for semaphore access (PL)");
            exit (EXIT_FAILURE);
        }
        ret = (sh->fSt).teamId;
        if (semUp(semgid,sh->playerRegistered)==-1){
            perror ("error on the up operation for semaphore access (PL)");
            exit (EXIT_FAILURE);
        }
    }

    return ret;
}

/**
 *  \brief player waits for referee to start match
 *
 *  The player updates its state and waits for referee to end match.  
 *  The internal state should be saved.
 *
 *  \param id   player id
 *  \param team player team
 */
static void waitReferee (int id, int team)
{
    if (semDown (semgid, sh->mutex) == -1)  {                                                     /* enter critical region */
        perror ("error on the up operation for semaphore access (PL)");
        exit (EXIT_FAILURE);
    }

    /* TODO: insert your code here */

    // É atribuído o estado de waiting start, dependendo da equipa a que pertence
    if (team ==1){
        (sh->fSt).st.playerStat[id] = WAITING_START_1; //s
        saveState(nFic,&sh->fSt);
    }
    else{
        (sh->fSt).st.playerStat[id] = WAITING_START_2; //S
        saveState(nFic,&sh->fSt);
    }

    if (semUp (semgid, sh->mutex) == -1) {                                                         /* exit critical region */
        perror ("error on the down operation for semaphore access (PL)");
        exit (EXIT_FAILURE);
    }

    /* TODO: insert your code here */

    //Bloqueia neste estado até o referee dar up (ou seja estar pronto)
    if (semDown(semgid,sh->playersWaitReferee)==-1){
        perror ("error on the up operation for semaphore access (PL)");
        exit (EXIT_FAILURE);
    }

    // Informa o referee que já está playing -> Talvez depois disto o referee atualize para refereeing
    if (semUp(semgid,sh->playing)==-1){
        perror ("error on the up operation for semaphore access (PL)");
        exit (EXIT_FAILURE);
    }

}

/**
 *  \brief player waits for referee to end match
 *
 *  The player updates its state and waits for referee to end match.  
 *  The internal state should be saved.
 *
 *  \param id   player id
 *  \param team player team
 */
static void playUntilEnd (int id, int team)
{
    if (semDown (semgid, sh->mutex) == -1)  {                                                     /* enter critical region */
        perror ("error on the up operation for semaphore access (PL)");
        exit (EXIT_FAILURE);
    }

    /* TODO: insert your code here */

    // É atribuído o estado de playing, dependendo da equipa a que pertence
    if (team ==1){
        (sh->fSt).st.playerStat[id] = PLAYING_1; //p
        saveState(nFic,&sh->fSt);
    }
    else{
        (sh->fSt).st.playerStat[id] = PLAYING_2; //P
        saveState(nFic,&sh->fSt);
    }

    if (semUp (semgid, sh->mutex) == -1) {                                                         /* exit critical region */
        perror ("error on the down operation for semaphore access (PL)");
        exit (EXIT_FAILURE);
    }

    /* TODO: insert your code here */

    // Fica bloqueado (espera) no estado playing até o referee indicar o end
    if(semDown(semgid,sh->playersWaitEnd)==-1){
        perror ("error on the up operation for semaphore access (PL)");
        exit (EXIT_FAILURE);
    }

}



