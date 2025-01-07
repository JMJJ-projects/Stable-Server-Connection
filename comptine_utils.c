/*John-Michael JENY JEYARAJ 12013787
Je déclare qu'il s'agit de mon propre travail.
Ce travail a été réalisé intégralement par un être humain. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "comptine_utils.h"

int read_until_nl(int fd, char *buf)
{
	int allocdynamique = 1024;
	int total = 0;
	int n;
	while((n = read(fd, buf+total, 1)) > 0){
		if(buf[total] == '\n'){
			break;
		}
		total++;
		while(total>allocdynamique-200){
			allocdynamique = reallocation(&buf,allocdynamique); //approfondissement non fini
								
		}	
	}
	if(n<0){
		perror("read");
		return -1;
	}
	return total;
}
int reallocation(char **buf, int max){
	max= 2*max;
	*buf = realloc(*buf, max); 
	return max;
}


int est_nom_fichier_comptine(char *nom_fich)
{
	int length = strlen(nom_fich);
	if(length > 4 && nom_fich[length-4] == '.' && nom_fich[length-3] == 'c' && nom_fich[length-2] == 'p' && nom_fich[length-1] == 't') //fichier qui termine par .cpt
		return 1;
	return 0;
}

struct comptine *init_cpt_depuis_fichier(const char *dir_name, const char *base_name)
{
	struct comptine *cpt = malloc(sizeof(struct comptine));
	if(cpt == NULL){
		perror("Échec cpt");
		liberer_comptine(cpt);
		return NULL;
	}
	
	char *path = malloc(sizeof(char)*1024);
	strcpy(path, dir_name);
	strcat(path, "/");
	strcat(path, base_name); //exemple: comptines/cerf.cpt
	int fd = open(path, O_RDONLY);
	if(fd < 0){
		perror("Échec open"); 
		liberer_comptine(cpt);
		return NULL;
	}
	
	char *buf = malloc(sizeof(char)*1024);
	int n = read_until_nl(fd, buf); //titre dans buf
	cpt->titre = malloc(sizeof(char)*n+1024);
	if(cpt->titre == NULL){
		perror("Échec titre");
		liberer_comptine(cpt);
		return NULL;
	}
	buf[n+1]='\0';
	strcpy(cpt->titre, buf);
	
	cpt->nom_fichier = malloc(sizeof(char)*(n+4));
	if(cpt->nom_fichier == NULL){
		perror("Échec nom fichier");
		liberer_comptine(cpt);
		return NULL;
	}
	strcpy(cpt->nom_fichier, base_name); //base_name est le nom du fichier
	if(!est_nom_fichier_comptine(cpt->nom_fichier)){
		return NULL;
		liberer_comptine(cpt);
	}
	close(fd);
	return cpt;
}

void liberer_comptine(struct comptine *cpt)
{
	free(cpt->nom_fichier);
	free(cpt->titre);
	free(cpt);
}

struct catalogue *creer_catalogue(const char *dir_name)
{
	DIR *dir = opendir(dir_name);
	if (dir == NULL) {
		perror("Échec opendir");
		return NULL;
	}
	struct catalogue *log = malloc(sizeof(struct catalogue));
	if(log == NULL){
		perror("Échec log");
		closedir(dir);
		return NULL;
	}
	log->nb=1;
	struct dirent* entity; //dirent.h
	entity = readdir(dir);
	while(entity != NULL && strcmp(entity->d_name, ".") !=0 && strcmp(entity->d_name, "..") !=0){ //pour compter le nombre de comptine
		log->nb++;
		entity = readdir(dir);
	}
	log->tab = malloc(log->nb*sizeof(struct comptine));
	if(log->tab == NULL){
		perror("Échec log tab");
		closedir(dir);
		return NULL;
	}
	log->nb = 0;
	rewinddir(dir);//on recommence, pour populer titre et nomfich
	while((entity = readdir(dir))){
		if(est_nom_fichier_comptine(entity->d_name) && entity != NULL && strcmp(entity->d_name, ".") !=0 && strcmp(entity->d_name, "..") !=0)
			log->tab[log->nb++] = init_cpt_depuis_fichier(dir_name, entity->d_name);
	}
	closedir(dir);
	return log;
}

void liberer_catalogue(struct catalogue *c)
{
	for(int i = 0; i < c->nb; i++)
		 liberer_comptine(c->tab[i]);
	free(c->tab);
	free(c);
}
