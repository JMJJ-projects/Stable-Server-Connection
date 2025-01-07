/*John-Michael JENY JEYARAJ 12013787
Je déclare qu'il s'agit de mon propre travail.
Ce travail a été réalisé intégralement par un être humain. */
/* fichiers de la bibliothèque standard */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
/* bibliothèque standard unix */
#include <unistd.h> /* close, read, write */
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dirent.h>
#include <errno.h>
/* spécifique à internet */
#include <arpa/inet.h> /* inet_pton */
/* spécifique aux comptines */
#include "comptine_utils.h"
/*Approfondissmen 2 threads*/
#include <pthread.h>
#include <signal.h>
#define PORT_WCP 4321

void usage(char *nom_prog)
{
	fprintf(stderr, "Usage: %s repertoire_comptines\n"
			"serveur pour WCP (Wikicomptine Protocol)\n"
			"Exemple: %s comptines\n", nom_prog, nom_prog);
}
/** Retourne en cas de succès le descripteur de fichier d'une socket d'écoute
 *  attachée au port port et à toutes les adresses locales. */
int creer_configurer_sock_ecoute(uint16_t port);

/** Écrit dans le fichier de desripteur fd la liste des comptines présents dans
 *  le catalogue c comme spécifié par le protocole WCP, c'est-à-dire sous la
 *  forme de plusieurs lignes terminées par '\n' :
 *  chaque ligne commence par le numéro de la comptine (son indice dans le
 *  catalogue) commençant à 0, écrit en décimal, sur 6 caractères
 *  suivi d'un espace
 *  puis du titre de la comptine
 *  une ligne vide termine le message */
void envoyer_liste(int fd, struct catalogue *c);

/** Lit dans fd un entier sur 2 octets écrit en network byte order
 *  retourne : cet entier en boutisme machine. */
uint16_t recevoir_num_comptine(int fd, int max);

/** Écrit dans fd la comptine numéro ic du catalogue c dont le fichier est situé
 *  dans le répertoire dirname comme spécifié par le protocole WCP, c'est-à-dire :
 *  chaque ligne du fichier de comptine est écrite avec son '\n' final, y
 *  compris son titre, deux lignes vides terminent le message */
void envoyer_comptine(int fd, const char *dirname, struct catalogue *c, uint16_t ic);

struct work { //on utilise se struct dans le thread
	int socket;
	struct catalogue *log;
	char addr[1024];
	char *dir_name;
};
int recevoir_comptine(int fd, char* dirname);

int recevoir_newcomptine(int fd, char* dirname);

void *worker(void *work); //fonction pour le thread

pthread_mutex_t lock;

int main(int argc, char *argv[]){
	if (argc != 2) {
		usage(argv[0]);
		return 1;
	}
	signal(SIGPIPE, SIG_IGN);
	int sock_l = creer_configurer_sock_ecoute(PORT_WCP); //socket d'écoute
	
	
	while(1){ //boucle infinie pour le serveur.
	
		struct sockaddr_in sa_clt;
		socklen_t sl = sizeof(sa_clt);
		int s = accept(sock_l, (struct sockaddr *) &sa_clt, &sl); //accept est bloquante, attend pour un client
		if (s < 0) {
			perror("Échec accept");
			continue;
		}
		
		struct work* working = (struct work*) malloc(sizeof(struct work)); //on rempli working pour l'argument du thread
		working->socket = s;
		inet_ntop(AF_INET, &sa_clt.sin_addr, working->addr, sl);
		working->dir_name = argv[1];
		
		pthread_t t;
		pthread_create(&t, NULL, worker, working); //thread avec la fonction worker avec struct working pour argument
		pthread_detach(t); //pour quitter sans attendre le join
	}
	return 0;
}

int creer_configurer_sock_ecoute(uint16_t port) {
	int sock_l = socket(AF_INET, SOCK_STREAM, 0); //crée une socket TCP
	if (sock_l < 0) {
		perror("Échec socket");
		exit(2);
	}
	
	struct sockaddr_in sa = {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr.s_addr = htonl(INADDR_ANY)
	};
	socklen_t sl = sizeof(sa);
	int opt = 1;
	setsockopt(sock_l, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));
	if (bind(sock_l, (struct sockaddr *) &sa, sl) < 0) { //attache le socket à un port local
		perror("Échec bind");
		exit(3);
	}
	
	if (listen(sock_l, 128) < 0){ //pour un socket d'écoute
		perror("Échec listen"); 
		exit(2);
	}
	return sock_l;
}
void envoyer_liste(int fd, struct catalogue *c)
{
	for (int i = 0; (i < c->nb) && (i < 65535); i++){ //car on le protocole WCP restraint la taille du catalogue: le nombre de comptines sur 2 octet 
		dprintf(fd, "%6d %s", i, c->tab[i]->titre); //6d pour montrer printf jusqu'a 6 chiffre et la liste des comptine reste toute droite sur le terminale.
	}
	dprintf(fd, "\n"); //la ligne vide termine le message
}

uint16_t recevoir_num_comptine(int fd, int max)
{
	uint16_t n;
	int test = 0;
	test = read(fd, &n, 2); //read un entier en network byte order et de taille 2 octet
	if(test <0)
	{
		perror("read");
		return max;
	}
	n = ntohs(n); //le change en un entier.	
	
	return n;
}

void envoyer_comptine(int fd, const char *dirname, struct catalogue *c, uint16_t ic)
{
	char *path = malloc(sizeof(char)*(strlen(c->tab[ic]->nom_fichier)+strlen(dirname)+10)); //alloue de la mémoire pour le path
	if(path == NULL){
		perror("malloc");
		free(path);
		return;
	}
	strcpy(path, dirname);
	strcat(path, "/");
	strcat(path, c->tab[ic]->nom_fichier);
	int file = open(path, O_RDONLY);//exemple comptines/cerf.cpt
	if(file < 0)
	{
		perror("open");
		return;
	}
	int allocdynami = 1024;
	char *buf = malloc(sizeof(char)*allocdynami);
	if(buf == NULL){
		perror("malloc");
		free(buf);
		return;
	}
	int total = 0;
	while ((total = read(file, buf, 100)) > 0) { //on lit le contenu de la comptine
			if(total>allocdynami-200){
				allocdynami= 2*allocdynami;
				char* res = realloc(buf,sizeof(char)*allocdynami);
				if(res != NULL){
					buf = res;
				}
				else{
					return;
				}
			}

		write(fd, buf, total); //on l'envoie
	}
	if(total < 0){
		perror("read");
		return;
	}
	write(fd, "\n\n",2); //2 \n signifie fin de l'envoie
	free(buf);
	close(file);
	free(path);
}
void *worker(void *working){
	pthread_mutex_init(&lock, NULL); //on utilise mutex quand on écrit dans log.txt car c'est une action critique
	struct work* client = (struct work*) working;
	
	client->log = creer_catalogue(client->dir_name); //crée le catalogue
	if(client->log == NULL){
		fprintf(stderr, "Échec log");
		return NULL;
	}
	int fdlogging = open("log.txt", O_WRONLY | O_CREAT | O_APPEND, 0666); //on ouvre le log avec append pour y ajouter les action du client.
	if(fdlogging < 0){
		perror("Échec open");
		return NULL;
	}
	pthread_mutex_lock(&lock);// on lock puis on écrit dans le log et après on unlock
	dprintf(fdlogging, "Client IP: %s est connecté.\n", client->addr);
	pthread_mutex_unlock(&lock);
	envoyer_liste(client->socket, client->log); // le serveur envoie la liste des comptines au debut de la connexion. 
	uint16_t n;
	int test = 0;
	char command[1024];
	while ((test = read(client->socket, &command, 1)) > 0){ //boucle infini pour le chaque client
		if(command[0] =='C'){
			n = recevoir_num_comptine(client->socket, client->log->nb); 
			if(n < client->log->nb && n >= 0){ //on teste si le nombre de comptine recu est dans le catalogue
				envoyer_comptine(client->socket, client->dir_name, client->log, n); //on envoie le contenu de la comptine
				pthread_mutex_lock(&lock);
				dprintf(fdlogging, "Client IP: %s a demandé comptine: %d.\n", client->addr, n);
				pthread_mutex_unlock(&lock);
			}
		}
		if(command[0] == 'T'){ //quand le client téléverse une comptine
			n = recevoir_comptine(client->socket, client->dir_name);
			if(n==0){//téléverse succès
				pthread_mutex_lock(&lock);
				dprintf(fdlogging, "Client IP: %s a téléverser une comptine.\n", client->addr);
				pthread_mutex_unlock(&lock);
			}
			
		}
		if(command[0] == 'E'){ //quand le client écrit un comptine
			n = recevoir_newcomptine(client->socket, client->dir_name);
			if(n==0){ //écriture succès
				pthread_mutex_lock(&lock);
				dprintf(fdlogging, "Client IP: %s a téléverser comptine.\n", client->addr);
				pthread_mutex_unlock(&lock);
			}
		}
		if(command[0] == 'L'){ //le client demande à nouveau la liste
			liberer_catalogue(client->log);
			client->log = creer_catalogue(client->dir_name);
			envoyer_liste(client->socket, client->log);
		}
		if(command[0] == 'Q'){//si le client quitte, on sort de la boucle infini
			break;
			}
		liberer_catalogue(client->log);
		client->log = creer_catalogue(client->dir_name);
		if(client->log == NULL){
			fprintf(stderr, "Échec log");
			exit(1);
		}
	}
	if(test < 0){ //on n'arrive jamais içi
		perror("read");
	}
	close(fdlogging);
	liberer_catalogue(client->log);
	return NULL;
}

int recevoir_comptine(int fd, char* dirname) //affiche le contenu du comptine lu
{	
	pthread_mutex_init(&lock, NULL); //on initialise mutex car écrire dans un fichier est critique
	int allocnomfich = 1024;
	char *bufnomfich = malloc(sizeof(char)*allocnomfich);
	if(bufnomfich == NULL){
		perror("malloc");
		free(bufnomfich);
		return -1;
	}
	int allocbuffer = 1024;
	char *buffer = malloc(sizeof(char)*allocbuffer);
	if(buffer == NULL){
		perror("malloc");
		free(buffer);
		return -1;
	}
	int allocpath = 1024;
	char *path = malloc(sizeof(char)*allocpath);
	if(path == NULL){
		perror("malloc");
		free(path);
		return -1;
	}
	int nomfich = read_until_nl(fd, bufnomfich); //on lit le nom du fichier
	if(nomfich <= 0)
	{
		perror("read");
		return -1;
	}
										
	bufnomfich[nomfich] ='\0';
	strcpy(path, dirname);
	strcat(path,"/");
	strcat(path,bufnomfich);
	int newcomptine = open(path, O_WRONLY | O_CREAT| O_TRUNC, 0666); //on crée une comptine ou sinon on vide une comptine qui existe déjà
	if(newcomptine < 0){
		perror("open");
		return -1;
	}
	int total = 0;
	int newline = 0;
	for(;;){
		total = read_until_nl(fd, buffer); //on lit ligne par ligne
		if(total==0){//teste si il y a 2 ligne vide pour quitter, mais on veut garder les ligne vide
			if(newline == 1){//si on lit une deuxième ligne vide a la suite on arrête de lire 
				close(newcomptine);
				pthread_mutex_destroy(&lock);
				return 0;
			}
			pthread_mutex_lock(&lock); //avant de sauter à la prochaine boucle on écrit un \n dans le fichier qui est critique
			dprintf(newcomptine, "\n");
			pthread_mutex_unlock(&lock);
			newline++;
			continue;
		}
		if(newline==1)
			newline--;
		buffer[total] = '\n';
		buffer[total+1] = '\0';
		pthread_mutex_lock(&lock); //on écrit dans la comptine avec \n
		write(newcomptine, buffer, total+1);
		pthread_mutex_unlock(&lock);
	}
	close(newcomptine);
	free(buffer);
	free(bufnomfich);
	pthread_mutex_destroy(&lock);
	return -1;
}

int recevoir_newcomptine(int fd, char* dirname) //affiche le contenu du comptine lu
{	
	pthread_mutex_init(&lock, NULL); //on initialise mutex car écrire dans un fichier est critique
	int allocnomfich = 512;
	char *bufnomfich = malloc(sizeof(char)*allocnomfich);
	int allocbuffer = 1024;
	char *buffer = malloc(sizeof(char)*allocbuffer);
	int allocpath = 1024;
	char *path = malloc(sizeof(char)*allocpath);
	
	int nomfich = read_until_nl(fd, bufnomfich);	//on lit le nom du fichier
	if(nomfich < 0){
		perror("read");
		return -1;
	}
	while(nomfich>allocpath){
		allocpath+= nomfich;
		char* test1 = realloc(path, sizeof(char)*allocpath+nomfich);
		if(test1 != NULL)
		{
			path = test1;
		}
	}
	while(nomfich>allocbuffer){
		allocbuffer+= nomfich;
		char* test2 = realloc(buffer, sizeof(char)*allocbuffer);
		if(test2 != NULL)
		{
			buffer = test2;
		}
	}
	bufnomfich[nomfich] = '\0';
	strcpy(path, dirname);
	strcat(path,"/");
	strcat(path,bufnomfich);
	int newcomptine = open(path, O_WRONLY | O_CREAT| O_TRUNC, 0666); //on crée une comptine ou sinon on vide une comptine qui existe déjà
	if(newcomptine < 0){
		perror("open");
		return -1;
	}
	int titre = read(fd,buffer, allocbuffer);	 //on lit le titre
	while(titre>allocbuffer){	
		allocbuffer+= titre;
		char* res = realloc(buffer, sizeof(char)*allocbuffer);
		if(res == NULL){
			free(buffer);;
			return -1;
		}
		buffer = res;
	}
	buffer[titre+1] ='\0';
	
	pthread_mutex_lock(&lock); //on écrit le nom du titre (qui contient un \n) dans la comptine qui est critique
	write(newcomptine,buffer, titre);
	pthread_mutex_unlock(&lock);
	
	int total = 0;
	int newline = 0;
	for(;;){
		total = read_until_nl(fd, buffer); //on lit les entrées du client ligne par ligne
		if(total==0){//teste si il y a 2 ligne vide pour quitter, mais on veut garder les ligne vide
			if(newline == 1){//si on lit une deuxième ligne vide a la suite on arrête de lire
				close(newcomptine);
				free(buffer);
				free(bufnomfich);
				pthread_mutex_destroy(&lock);
				return 0;
			}
			pthread_mutex_lock(&lock); //on écrit dans la comptine qui est critique
			dprintf(newcomptine, "\n");
			pthread_mutex_unlock(&lock);
			newline++;
			continue;
		}
		if(newline==1)
			newline--;
		if(total>allocbuffer-100){
			allocbuffer=2 *allocbuffer;
			buffer = realloc(buffer,sizeof(char)*allocbuffer);
		}
		
		buffer[total] = '\n';
		buffer[total+1] = '\0';
		
		pthread_mutex_lock(&lock); //on écrit dans la comptine qui est critique
		dprintf(newcomptine,"%s", buffer);
		pthread_mutex_unlock(&lock);
	}
	
	close(newcomptine);
	free(buffer);
	free(bufnomfich);
	pthread_mutex_destroy(&lock);
	return -1;
}

