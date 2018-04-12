#include <stdlib.h>
#include <stdio.h>
void stem(char *term, char *stem);
void read_dict_info() ;

int main (int argc, char *argv[]) {

   char word[80];
   char thestem[80];

   FILE *fd;

   fd = fopen(argv[1], "r");
   if (!fd) {
     printf("Couldn't open the input file: %s", argv[1]);
     exit(1);
    }

   read_dict_info();
   
   fscanf(fd, "%s", word);
   while (!feof(fd)) {
     stem(word, thestem);
     printf("%s\n", thestem);
     fscanf(fd, "%s", word);
   }

   fclose(fd);
}
