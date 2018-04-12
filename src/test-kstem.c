#include <stdio.h>

/* Function prototypes */
void read_dict_info();
void stem(char *, char *);

int main () {

   char word[80];
   char thestem[80];

   read_dict_info();

   do  {
      printf("Please enter a word (<CR> to quit): ");
      gets(word);
      if (*word == '\0') break;

      stem(word, thestem);

      printf("\n\nThe stem was: %s\n", thestem);
   } while(1);

}
