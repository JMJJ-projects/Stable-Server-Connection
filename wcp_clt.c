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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
/* spécifique à internet */
#include <arpa/inet.h> /* inet_pton */
/* spécifique aux comptines */
#include "comptine_utils.h"

#include <pthread.h>
#define PORT_WCP 4321

void usage(char *nom_prog)
{
	fprintf(stderr, "Usage: %s addr_ipv4\n"
			"client pour WCP (Wikicomptine Protocol)\n"
			"Exemple: %s 208.97.177.124\n", nom_prog, nom_prog);
}

/** Retourne (en cas de succès) le descripteur de fichier d'une socket
 *  TCP/IPv4 connectée au processus écoutant sur port sur la machine d'adresse
 *  addr_ipv4 */
int creer_connecter_sock(char *addr_ipv4, uint16_t port);

/** Lit la liste numérotée des comptines dans le descripteur fd et les affiche
 *  sur le terminal.
 *  retourne : le nombre de comptines disponibles */
uint16_t recevoir_liste_comptines(int fd);

/** Demande à l'utilisateur un nombre entre 0 (compris) et nc (non compris)
 *  et retourne la valeur saisie. */
uint16_t saisir_num_comptine(uint16_t nb_comptines);

/** Écrit l'entier ic dans le fichier de descripteur fd en network byte order */
void envoyer_num_comptine(int fd, uint16_t nc);

/** Affiche la comptine arrivant dans fd sur le terminal */
void afficher_comptine(int fd);

void televerser_comptine(int fd);

void ecrire_comptine(int fd);
                        
int main(int argc, char *argv[])
{
	if (argc != 2) {
		usage(argv[0]);
		return 1;
	}

	uint16_t saisie;
	int sock = creer_connecter_sock(argv[1], PORT_WCP); //Initialisation avec l'ip du serveur et Port WCP
	printf("Bonjour, vous êtes connecté au serveur\n");
	uint16_t recv = recevoir_liste_comptines(sock); //recevoir la liste des comptines par le serveur
	do{
		char command[1024];
		printf("-------------------------\n");
		printf("L pour voir la liste des comptines\nC pour voir le contenu d'une comptine\nT pour téléverser une comptine\nE pour écrire une comptine\nQ pour quitter\n");
		int res = scanf("%s", command);//Approfondissement menu avec un petit language 
		if(res == 1){
			if(!strcmp(command,"C")){
				write(sock, "C", 1);
				saisie = saisir_num_comptine(recv); //saisie du numéro de comptine à envoyer
				if(saisie == 1024)
					continue;
				envoyer_num_comptine(sock, saisie); //envoyer le numéro de comptine a lire
				afficher_comptine(sock); //affiche le retour du serveur
			}
			else if(!strcmp(command,"T")){
				write(sock, "T", 1);
				televerser_comptine(sock);
			}
			else if(!strcmp(command,"E")){ //Écrire une nouvelle comptine pour le serveur
				write(sock, "E", 1);
				ecrire_comptine(sock);
			}
			else if(!strcmp(command,"L")){ //pour recevoir la liste
				write(sock, "L", 1);
				recv = recevoir_liste_comptines(sock);
			}
			else if(!strcmp(command,"Q")){ //pour quitter
				write(sock, "Q", 1);
				break;
			}
		}
		else if(res == EOF){//on teste si le client envoie des entrées non conforme.
			printf("Erreur Fatale\n");
			break;
		}			
		else{
			printf("Erreur Fatale\n");
			break;
		}
	}while(1);
	close(sock);
	return 0;
}

int creer_connecter_sock(char *addr_ipv4, uint16_t port) //créer une connection TCP et renvoie un int pour le socket
{
	int fd = socket(AF_INET, SOCK_STREAM, 0); //crée une socket
	if (fd < 0) {
		perror("Échec socket");
		exit(1);
	}
	struct sockaddr_in sa = {
		.sin_family = AF_INET,
		.sin_port = htons(port)
	};
	if (inet_pton(AF_INET, addr_ipv4, &sa.sin_addr) != 1){ //presentation to network pour l'adresse IP
		fprintf(stderr, "Échec adresse ipv4 non valable");
		exit(1);
	}
	socklen_t sl = sizeof(struct sockaddr_in);

	if(connect(fd, (struct sockaddr *) &sa, sl) < 0){ //demande de connexion
		perror("Échec connect");
		exit(1);
	}
	
	return fd;
}

uint16_t recevoir_liste_comptines(int fd) //recevoir quand le serveur envoie la liste des comptines
{
	printf("-------------------------\n"); //séparation de champs
	char *buf = malloc(sizeof(char)*1024);
	uint16_t n = 0;
	int m = 0;
	do{					
		m = read_until_nl(fd, buf);	//lire la liste des comptines ligne par ligne
		if(m<=0)
			break;
		buf[m+1] = '\0';
		printf("%s", buf);		//les affiche sur le terminale
		n++;
	}while(m>0);
	free(buf);
	return n; //on renvoie le nombre de comptine dans la liste
}

uint16_t saisir_num_comptine(uint16_t nb_comptines) 	//le client demande au serveur de lire un comptine
{							//boucle while jusqu'a le client envoie un entier entre 0 et le nombre de comptines
	uint16_t n = 0;
	int res = 0;
	do{	
		printf("Quelle comptine voulez-vous ? (entre 0 et %d): ", nb_comptines -1);
		if ((res = scanf("%"SCNu16, &n)) < 1) {//on teste si c'est un unsigned int de taille inférieur à 16
			fprintf(stderr, "Erreur fatale : un entier positif était attendu.\n"); //teste pour entrée non conforme
			return 1024;
		}
		else if(res == EOF){
			printf("Erreur Fatale");
			return 1024;
		}
		printf("\n");
	}while(n<0 ||n>=nb_comptines);
	return n;
}

void envoyer_num_comptine(int fd, uint16_t nc)
{
	nc = htons(nc); //host to network short, pour transmettre un int
	
	int status = write(fd, &nc, 2); //envoyer le nombre de comptine à lire
	if (status == -1) {
		perror("Échec write");
		exit(1);
	}
}

void afficher_comptine(int fd) //affiche le contenu du comptine lu
{
	
	int allocbuf = 1024;
	char *buffer = malloc(sizeof(char)*allocbuf);
	int total = 0;
	int newline = 0;
	for(;;){
		if((total = read_until_nl(fd, buffer)) >= 0){ //on lit jusqu'a \n
			if(total==0){ //teste si on a 2 \n à la suite pour savoir si on est arrivé à la fin de la comptine
				if(newline == 1){
					return;
				}
				newline=1;
				continue;
			}
			if(newline==1){
				printf("\n");
				newline=0;
				}
			buffer[total] = '\n';
			buffer[total + 1] = '\0';
			printf("%s", buffer);
		}
		if(total < 0){
			perror("read");
		}
	}
	free(buffer);
}

void televerser_comptine(int fd){
	int allocnomfich = 1024;
	char *bufnomfich= malloc(sizeof(char)*allocnomfich);
	int res = 0;
	do{
	printf("Nom de fichier à envoyer (.cpt):"); //on teste si le client envoie des entrées non conforme.
		if((res = scanf("%s", bufnomfich)) < 1){
			printf("Erreur Fatale\n");
			return;
		}
		if(res == EOF){
			printf("Erreur Fatale\n");
			return;
		}
			
	}while(!est_nom_fichier_comptine(bufnomfich)&& strlen(bufnomfich) < 255 && (strchr(bufnomfich,'/') != NULL)); //taille max d'un nom de fichier est 255 et on n'a pas le droit d'utiliser /
	
	int file;
	if ((file = open(bufnomfich, O_RDONLY)) == -1) { //on ouvre la comptine à téléverser
		perror("open");
		dprintf(fd, "\n");
		return;
	}
	strcat(bufnomfich, "\n");
	dprintf(fd,"%s", bufnomfich); //on envoie le nom du fichier après l'avoir ouvert pour l'envoyer avec un \n pour read_until_nl
	int allocdynami = 1024;
	char *buf = malloc(sizeof(char)*allocdynami);
	int total = 0;
	int test = 0;
	while ((total = read(file, buf, allocdynami)) > 0) { //on envoie le contenu de la comptine 
		test+=total;
		while(test>allocdynami-100){
			allocdynami= 2*allocdynami;
			char* res = realloc(buf,sizeof(char)*allocdynami);
			if(res != NULL)
			{
				buf = res;
			}
		}
		printf("%s", buf);
		buf[total+1] = '\0';
		write(fd, buf, total);
	}
	if(total < 0){
		perror("read");
		return;
	}
	write(fd, "\n\n", 2); //2 \n signifie fin de l'envoie

	free(buf);
	close(file);
}

void ecrire_comptine(int fd){
	int allocnomfich = 1024;
	char *bufnomfich= malloc(sizeof(char)*allocnomfich);
	int alloctitre = 1024;
	char *buftitre= malloc(sizeof(char)*alloctitre);
	int allocbuf = 1024;
	char *buf = malloc(sizeof(char)*allocbuf);
	int ligne = 1;
	int res = 0;
	do{
	printf("Nom de fichier à envoyer (.cpt):"); 
		if((res = scanf("%s", bufnomfich)) < 1){ //on teste si le client envoie des entrées non conforme.
			printf("Erreur Fatale\n");
			return;
		}
		if(res == EOF){
			printf("Erreur Fatale\n");
			return;
		}
	}while(!est_nom_fichier_comptine(bufnomfich)&& strlen(bufnomfich) < 255 && (strchr(bufnomfich,'/') != NULL)); //taille max d'un nom de fichier est 255 et on n'a pas le droit d'utiliser /
	
	strcat(bufnomfich, "\n");
	dprintf(fd,"%s", bufnomfich); //on enovie le nom du fichier.
	
	fgets(bufnomfich, alloctitre, stdin); //bug avec scanf, on s'en débarrasse avant de lire avec fgets.
	
	do{
		printf("Titre de la comptine:");
	}while(!fgets(buftitre,alloctitre,stdin));
	strcat(buftitre, "\n");
	dprintf(fd, "%s", buftitre); //on envoie le titre avec /n
	
	printf("\nPour arrêter d'écrire faite 'entrée' sur une ligne vide\nPour écrire une ligne vide dans la comptine faite un '\\n'\nligne %d: ", ligne); //on envoie le contenu de la comptine ligne par ligne
	while(fgets(buf,allocbuf,stdin)){
		if(buf[0]== '\n') //si le client fait un retour à la ligne on arrête de lire
			break;
		if(buf[0] == '\\' && buf[1] == 'n'){
			dprintf(fd, "\n");
			ligne++;	
			printf("ligne %d: ", ligne);
			continue;
		}
		dprintf(fd,"%s", buf);
		ligne++;	
		printf("ligne %d: ", ligne);
	}
	dprintf(fd, "\n\n"); //2 \n signifie fin de l'envoie
	free(bufnomfich);
	free(buftitre);
	free(buf);
}
