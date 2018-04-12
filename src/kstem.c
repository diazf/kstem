#include <stdio.h>
#include <string.h>
#define MAXLINE 500000
/* Function prototypes */
void read_dict_info();
void stem(char *, char *);

int main () {
    read_dict_info();
	char buffer[MAXLINE];
	while (fgets(buffer,MAXLINE,stdin)!=NULL){
		char *w = NULL;
		for (w=strtok(buffer,"\t\r\n ");w!=NULL;w=strtok(NULL,"\t\r\n ")){
			if (strlen(w)>0){
			    char thestem[80];
		        stem(w, thestem);
				fprintf(stdout,"%s ",thestem);
			}
		}
		fprintf(stdout,"\n");
	}
}
