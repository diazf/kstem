/********************************************************************/
/*                     Copyright 1990,1991 by the                   */
/*                  Information Retrieval Laboratory,               */
/*                     University of Massachusetts,                 */
/*                          Amherst MA. 01003                       */
/*                         All Rights Reserved.                     */
/*            Do not distribute without written permission.         */
/********************************************************************/
/*
    This is a stemmer that handles inflectional morphology and the
    most common forms of derivational morphology.  It first checks a 
    word against the dictionary, and if it is found it leaves it alone.
    If not, it handles inflectional endings (plurals into singular form,
    and past tense and "ing" endings into present tense), and then
    conflates the most common derivational variants.

    Author: Bob Krovetz

    Kstem is free software; you can redistribute it and/or modify it under 
    the terms of the GNU Library General Public License as published by the 
    Free Software Foundation; either version 2 of the  License, or (at your 
    option) any later version.

    Kstem is distributed in the hope that it will be useful, but WITHOUT ANY 
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS 
    FOR A PARTICULAR PURPOSE.  See the GNU Library General Public License for 
    more details.

    The GNU Library General Public License is available at part of the Linux
    system, or by writing to the Free Software Foundation, Inc., 59 Temple Place,
    Suite 330, Boston, MA 02111-1307, USA. 

    Version 0.8 - First public release (beta version).  The basic
                 lexicon (using the Longman dictionary) was replaced
                 by a modified /usr/dict/words (a standard file on
                 Unix systems).  A new hash table routine is used
                 that is taken from the "lispdebug" routine, which
                 is also under the GPL.  

                 I added a few constraints to rules for the -ble, 
                  -nce, and -ncy endings.


   To-Do: 1) Change the code to explicitly malloc the strings rather than 
             have them allocated in an array.

*/


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "hash.h"             /* hash tables */

#define vowel(i) (!consonant(i))

#define MAX_WORD_LENGTH 25
#define MAX_FILENAME_LENGTH 125  /* including the full directory path */
#define MAX_ROOTS 35000           /* maximum number of root forms in the lexicon */
#define HASH_DICT_SIZE 40000
#define TRUE 1
#define FALSE 0


/* These macros expand to expressions which evaluate to the following: */

#define wordlength (k + 1)     /* the length of word (not an lvalue) */
#define stemlength (j + 1)     /* length of stem within word (not an lvalue) */
#define final_c    (word[k])   /* the last character of word */
#define penult_c   (word[k-1]) /* the penultimate character of word */

#define ends_in(s) ends(s, sizeof(s)-1)      /* s must be a string constant */
#define setsuffix(s) setsuff(s, sizeof(s)-1) /* s must be a string constant */



/* -----------------------------  Declarations ------------------------------*/
typedef int boolean;

typedef struct
    {
    boolean e_exception;  /* is the word an exception for words ending in "e" */
    char *root;           /* used for direct lookup (e.g. irregular variants) */
   } dictentry;

/* ------------------------- Function Declarations --------------------------*/



/* ------------------------------ Definitions -------------------------------*/

char *word;

void *lookup_value;


int j;    /* INDEX of final letter in stem (within word) */
int k;    /* INDEX of final letter in word.
	     You must add 1 to k to get the current length of word.  
	     When you want the length of word, use the macro wordlength,
	     which is #defined as (k+1).  Note that wordlength is only
             used for its value (never assigned to), so this is ok. */


boolean dict_initialized_flag = FALSE;  /* ensure we load it before using it */

HASH *dict_ht;                          /* the hashtable used to store the dictionary */

dictentry *dep;                         /* for general use with dictionary entries    */

char headword[MAX_ROOTS][MAX_WORD_LENGTH];  /* use an array (instead of char*) because 
                                                  of the need for separate storage of the 
                                                  root form in the hash table */

int headword_cnt = 0;                  /* for moving through the array of headwords */





/* -------------------------- Function Definitions --------------------------*/


/* read_dict_info() reads the words from the dictionary and puts them into a
                    hash table.  It also stores the other lexicon information
                    required by the stemmer (proper noun information, supplemental
                    dictionary files, direct mappings for irregular variants, etc.)
*/

void read_dict_info() 
{

   FILE *dict_file;                      /* main list of words in dictionary */
   FILE *e_exception_file;               /* exceptions to "e" ending (suited/suit, suites/suite) */
   FILE *dict_supplement_file;           /* words not found in the basic dictionary list */
   FILE *proper_noun_file;               /* so we don't have Paris->Pari or Inverness->Inver */
   FILE *country_nationality_file;       /* Italian->Italy, French->France, etc. */
   FILE *direct_conflation_file;         /* variants that can be directly conflated */


   char *stemdir;                         /* the directory where all these files reside */
   char currentfile[MAX_FILENAME_LENGTH]; /* the current lexicon file */
 
   char *variant;
   char *root;
   
   dict_ht = create_hash(HASH_DICT_SIZE);


   /* the hash table package allows a pointer to be associated with each word stored in
      the hash table.  The pointer is pointing to a structure  with two fields, one that 
      tells whether the word is an exception to words ending in "e" 
      (e.g., `automating'->`automate', but `doing'->`do').  The other field is 
      used for a direct conflation (e.g., irregular variants, and mapping between 
      nationalites and countries (`Italian'->`Italy')), and for caching the results
      of stemming. */

    
   /* get the directory name from an environment variable, and then build
      the name of the file (including the path) in the variable "currentfile" */

   stemdir = getenv("STEM_DIR");
   if (!stemdir)  {
      fprintf(stderr, "Error!  The environment variable STEM_DIR is not defined.\nIt must be set to the directory that contains files used by the stemmer.\n");
      exit(0);
      }   
   if (strlen(stemdir) > 100) {
      fprintf(stderr, "Error!  The directory path in the environment variable STEM_DIR is                       too long. \nThe limit is 100 characters.\n");
      exit(0);
      }


   /*  read in a list of words from the general dictionary and store it 
       in the table */

   strcpy((char *)&currentfile[0], (char *)&stemdir[0]);
   strcat((char *)&currentfile[0], "/head_word_list.txt");
   dict_file = fopen(currentfile, "r");
   if (!dict_file)  {
      fprintf(stderr, "Error!  Couldn't open dictionary headword file.\n");
      exit(0);
      }

   headword_cnt++;
   root= headword[headword_cnt];
   fscanf(dict_file, "%s", root);
   while (!feof(dict_file))  {
      lookup_value = search_hash(dict_ht, root);
      /* if the word isn't already there, insert it */
      if (lookup_value != NULL) { 
           fprintf(stderr, "Error!  %s (from the general dictionary file) appears to have                    a duplicate entry.\n", root);
            exit(0);}
      dep = (dictentry *)malloc(sizeof(dictentry));
      dep->e_exception = FALSE;
      dep->root = "";
      insert_hash(dict_ht, root, (void *)dep);
      headword_cnt++;
      root= headword[headword_cnt];
      fscanf(dict_file, "%s", root);
      }


   fclose(dict_file);


   /* now store words that are not found in the main dictionary.  I make
      a distinction between a "main" dictionary and a supplemental one
      because this makes it easier to maintain the dictionary and to 
      trace down differences in performance. */

   strcpy((char *)&currentfile[0], (char *)&stemdir[0]);
   strcat((char *)&currentfile[0], "/dict_supplement.txt");
   dict_supplement_file = fopen(currentfile, "r");
   if (!dict_supplement_file)  {
       fprintf(stderr, "Error!  Couldn't open file of supplemental words to the dictionary.\n");
       exit(0);
       }

   headword_cnt++;
   root= headword[headword_cnt];
   fscanf(dict_supplement_file, "%s", root);
   while (!feof(dict_supplement_file))  {
      lookup_value = search_hash(dict_ht, root);
      if (lookup_value != NULL) {
         fprintf(stderr, "Error!  Word %s (from the supplemental dictionary) appears to have                          a duplicate entry.\n", root);
         exit(0);
         }
      dep = (dictentry *)malloc(sizeof(dictentry));
      dep->e_exception = FALSE;
      dep->root = "";
      insert_hash(dict_ht, root, (void *)dep);
      headword_cnt++;
      root= headword[headword_cnt];
      fscanf(dict_supplement_file, "%s", root);
      }

   fclose(dict_supplement_file);


   /* read in a list of words that are exceptions to the stemming rule I use 
      (i.e., if a word can end with an `e', it does).  So, `automating' -> `automate' 
      according to the normal application of the rule, but `doing' shouldn't become 
      `doe'.   These particular exceptions *only* apply to words that might end
      in the letter "e".
    */

   strcpy((char *)&currentfile[0], (char *)&stemdir[0]);
   strcat((char *)&currentfile[0], "/e_exception_words.txt");
   e_exception_file = fopen(currentfile,  "r");
   if (!e_exception_file)  {
       fprintf(stderr, "Error!  Couldn't open file of words that are exceptions with 'e' ending.\n");
       exit(0);
       }

   headword_cnt++;
   root= headword[headword_cnt];
   fscanf(e_exception_file, "%s", root);
   while (!feof(e_exception_file))  {
       lookup_value = search_hash(dict_ht, root);
       if (lookup_value == NULL)  {
           fprintf(stderr, "Error!  %s (from the 'e' ending exception file) was not                   found in the main or supplemental dictioanry.\n", root);
           exit(0);
           }
       dep = (dictentry *)malloc(sizeof(dictentry));
       dep->e_exception = TRUE;
       dep->root = "";
       insert_hash(dict_ht, headword[headword_cnt], (void *)dep);        
       headword_cnt++;
       root= headword[headword_cnt];       
       fscanf(e_exception_file, "%s", root);
       }

   fclose(e_exception_file);



   /*  store words for which we have a direct conflation.  These are cases that
       would not go through due to length restrictions (`owing'->`owe'), or in which
       the user wishes to over-ride the normal operation of the stemmer */

   strcpy((char *)&currentfile[0], (char *)&stemdir[0]);
   strcat((char *)&currentfile[0], "/direct_conflations.txt");
   direct_conflation_file = fopen(currentfile, "r");
   if (!direct_conflation_file)  {
      fprintf(stderr, "Error!  Couldn't open file of conflation words for the dictionary.\n");
      exit(0);
      }
     
   headword_cnt++;
   variant = headword[headword_cnt];
   headword_cnt++;
   root = headword[headword_cnt];
   fscanf(direct_conflation_file, "%s %s", variant, root);
   while (!feof(direct_conflation_file))  {
       lookup_value = search_hash(dict_ht, variant);
       if (lookup_value != NULL)  {
           fprintf(stderr, "Error!  %s (from the direct conflation file) appears to have                    a duplicate entry.\n", variant);
           exit(0);
           }         
       dep = (dictentry *)malloc(sizeof(dictentry));
       dep->e_exception = FALSE;
       dep->root = root;
       insert_hash(dict_ht, variant, (void *)dep);
       headword_cnt++;
       variant = headword[headword_cnt];
       headword_cnt++;
       root = headword[headword_cnt];
       fscanf(direct_conflation_file, "%s %s", variant, root);
       }

   fclose(direct_conflation_file);




   /* store the mappings between countries and nationalities (e.g., British/Britain), 
      and morphology associated with continents (european/europe).  */

   strcpy((char *)&currentfile[0], (char *)&stemdir[0]);
   strcat((char *)&currentfile[0], "/country_nationality.txt");
   country_nationality_file = fopen(currentfile, "r");
   if (!country_nationality_file)  {
      fprintf(stderr, "Error!  Couldn't open file of variants associated with the names              of countries.\n");
      exit(0);
      }


   /* Just as with the previous direct-conflation file, we want to create a direct
      mapping from a variant to a root form.  In this case the mapping is between
      country nationalities and the country name (e.g., British->Britain).  They 
      are kept in separate files for ease of maintenance */
        
   headword_cnt++;
   variant = headword[headword_cnt];
   headword_cnt++;
   root = headword[headword_cnt];
   fscanf(country_nationality_file, "%s %s", variant, root);
   while (!feof(country_nationality_file))  {
      lookup_value = search_hash(dict_ht, variant);
      if (lookup_value != NULL) {
         fprintf(stderr, "Error!  Word %s (from the country/nationality file) appears                         to have a duplicate entry.\n", variant);
         exit(0);}
      dep = (dictentry *)malloc(sizeof(dictentry));
      dep->e_exception = FALSE;
      dep->root = root;
      insert_hash(dict_ht, variant, (void *)dep);
      headword_cnt++;
      variant = headword[headword_cnt];
      headword_cnt++;
      root = headword[headword_cnt];
      fscanf(country_nationality_file, "%s %s", variant, root);
      }

   fclose(country_nationality_file);



   /* finally, store proper nouns that would otherwise be altered by
      the stemmer (e.g. `Inverness') */


   strcpy((char *)&currentfile[0], (char *)&stemdir[0]);
   strcat((char *)&currentfile[0], "/proper_nouns.txt");
   proper_noun_file = fopen(currentfile, "r");
   if (!proper_noun_file)  {
      fprintf(stderr, "Error!  Couldn't open file of proper nouns.\n");
      exit(0);
      }

   headword_cnt++;
   root = headword[headword_cnt];
   fscanf(proper_noun_file, "%s", root);
   while (!feof(proper_noun_file))  {
      lookup_value = search_hash(dict_ht, root);
      if (lookup_value != NULL) {
         fprintf(stderr, "Error!  %s (from the proper noun file) appears to have                    a duplicate entry\n", root);
           exit(0);}
      dep = (dictentry *)malloc(sizeof(dictentry));
      dep->e_exception = FALSE;
      dep->root = "";
      insert_hash(dict_ht, root, (void *)dep);
      headword_cnt++;
      root = headword[headword_cnt];
      fscanf(proper_noun_file, "%s", root);
      }

   fclose(proper_noun_file);


   dict_initialized_flag = TRUE;
}




/* consonant() returns TRUE if word[i] is a consonant.  (The recursion is safe.) */

static boolean consonant(int i)
{
    char ch;

    ch = word[i];
    if (ch == 'a' || ch == 'e' || ch == 'i' || ch == 'o' || ch == 'u')
	return(FALSE);

    if (ch != 'y' || i == 0)
	return(TRUE);
    else
	return (!consonant(i - 1));
}




/* This routine is useful for ensuring that we don't stem acronyms */

static boolean vowelinstem()
{
    int i;

    for (i = 0; i < stemlength; i++) 
	if (vowel(i)) return(TRUE);         /* vowel is a macro */
    return(FALSE);
}




/* return TRUE if word ends with a double consonant */

static boolean doublec (int i)
{
    if (i < 1)
	return(FALSE);

    if (word[i] != word[i - 1])
	return(FALSE);

    return(consonant(i));
}




/* Passing the length of str is awkward, but important for performance.  Since
   str is always a string constant, we can define a macro ends_in (see the macro
   section of this module) which takes str and determines its length at compile
   time.  Note that str must therefore no longer be padded with spaces in the calls 
   to ends_in (as it was in the original version of this code).
*/

static boolean ends(char *str, int sufflength)
{
    int r = wordlength - sufflength;    /* length of word before this suffix */
    boolean match;

    if (sufflength > k)
	return(FALSE);
    
    match = (strcmp((char *)word+r, (char *)str) == 0);
    j = (match ? r-1 : k);             /* use r-1 since j is an index rather than length */
    return(match);
}




/* replace old suffix with str */

static void setsuff(char *str, int length)
{
    strcpy((char *)word+j+1, (char *)str);
    k = j + length;
    word[k+1] = '\0';
}



/* convert plurals to singular form, and `-ies' to `y' */

static void plural ()
{

   if (search_hash(dict_ht, word) != NULL)
      return;
  
   if (final_c == 's')  {
      if (ends_in("ies")) {
         word[j+3] = '\0';
         k--;
         if (search_hash(dict_ht, word) != NULL)        /* ensure calories -> calorie */
            return;
         k++;
         word[j+3] = 's';             
         setsuffix("y"); 
         }
      else 
        if (ends_in("es")) {
           /* try just removing the "s" */
           word[j+2] = '\0';
           k--;

           /* note: don't check for exceptions here.  So, `aides' -> `aide',
              but `aided' -> `aid'.  The exception for double s is used to prevent
              crosses -> crosse.  This is actually correct if crosses is a plural
              noun (a type of racket used in lacrosse), but the verb is much more
              common */

           if ((search_hash(dict_ht, word) != NULL)  && !((word[j] == 's') && (word[j-1] == 's')))
              return;

           /* try removing the "es" */

           word[j+1] = '\0';
           k--;
           if (search_hash(dict_ht, word) != NULL)
              return;

           /* the default is to retain the "e" */
           word[j+1] = 'e';
           word[j+2] = '\0';
           k++;
           return;
           }
        else 
          if (!ends_in("ous") && penult_c != 's' && wordlength > 3) {
             /* unless the word ends in "ous" or a double "s", remove the final "s" */
             word[k] = '\0';
             k--; 
             }
        }   
}



/* convert past tense (-ed) to present, and `-ied' to `y' */

static void past_tense ()
{

  if (search_hash(dict_ht, word) != NULL)
     return;

  /* Handle words less than 5 letters with a direct mapping  
     This prevents (fled -> fl).  */

  if (wordlength <= 4)
     return;

  if (ends_in("ied"))  {
     word[j+3] = '\0';
     k--;
     if (search_hash(dict_ht, word) != NULL) /* we almost always want to convert -ied to -y, but */
        return;                            /* this isn't true for short words (died->die)      */
     k++;                                  /* I don't know any long words that this applies to, */
     word[j+3] = 'd';                      /* but just in case...                              */
     setsuffix("y");
     return;
     }

   /* the vowelinstem() is necessary so we don't stem acronyms */
   if (ends_in("ed") && vowelinstem())  {
      /* see if the root ends in `e' */
      word[j+2] = '\0'; 
      k = j + 1;              

      lookup_value = search_hash(dict_ht, word);
      if (lookup_value != NULL)
         dep = (dictentry *)lookup_value;
      if ((lookup_value != NULL) && !(dep->e_exception))    /* if it's in the dictionary and not an exception */
         return;

      /* try removing the "ed" */
      word[j+1] = '\0';
      k = j;
      if (search_hash(dict_ht, word) != NULL)
         return;


      /* try removing a doubled consonant.  if the root isn't found in
         the dictionary, the default is to leave it doubled.  This will
         correctly capture `backfilled' -> `backfill' instead of
         `backfill' -> `backfille', and seems correct most of the time  */

      if (doublec(k))  {
         word[k] = '\0';
         k--;
         if (search_hash(dict_ht, word) != NULL)
             return;
         word[k+1] = word[k];
         k++;
         return; 
         }



      /* if we have a `un-' prefix, then leave the word alone  */
      /* (this will sometimes screw up with `under-', but we   */
      /*  will take care of that later)                        */

      if ((word[0] == 'u') && (word[1] == 'n'))  {
         word[k+1] = 'e';                            
         word[k+2] = 'd';                            
         k = k+2;
         return;
         }


      /* it wasn't found by just removing the `d' or the `ed', so prefer to
         end with an `e' (e.g., `microcoded' -> `microcode'). */

      word[j+1] = 'e';
      word[j+2] = '\0';
      k = j + 1;
      return;
      }
}





/* handle `-ing' endings */

static void aspect ()
{

  if (search_hash(dict_ht, word) != NULL)
     return;

  /* handle short words (aging -> age) via a direct mapping.  This
     prevents (thing -> the) in the version of this routine that
     ignores inflectional variants that are mentioned in the dictionary
     (when the root is also present) */

  if (wordlength <= 5)                           
     return;

  /* the vowelinstem() is necessary so we don't stem acronyms */
  if (ends_in("ing") && vowelinstem())  {

     /* try adding an `e' to the stem and check against the dictionary */
     word[j+1] = 'e';
     word[j+2] = '\0';
     k = j+1;          

     lookup_value = search_hash(dict_ht, word);
     if (lookup_value != NULL)
     dep = (dictentry *)lookup_value;

     /* if it's in the dictionary and not an exception */
     if ((lookup_value != NULL) && !(dep->e_exception)) 
        return;

     /* adding on the `e' didn't work, so remove it */
     word[k] = '\0';
     k--;                                      /* note that `ing' has also been removed */

     if (search_hash(dict_ht, word) != NULL)
        return;

     /* if I can remove a doubled consonant and get a word, then do so */
     if (doublec(k))  {
        k--;
        word[k+1] = '\0';
        if (search_hash(dict_ht, word) != NULL)
           return;
        word[k+1] = word[k];       /* restore the doubled consonant */

        /* the default is to leave the consonant doubled            */
        /*  (e.g.,`fingerspelling' -> `fingerspell').  Unfortunately */
        /*  `bookselling' -> `booksell' and `mislabelling' -> `mislabell'). */
        /*  Without making the algorithm significantly more complicated, this */
        /*  is the best I can do */
        k++;
        return;
        }

      /* the word wasn't in the dictionary after removing the stem, and then
         checking with and without a final `e'.  The default is to add an `e'
         unless the word ends in two consonants, so `microcoding' -> `microcode'.
         The two consonants restriction wouldn't normally be necessary, but is
         needed because we don't try to deal with prefixes and compounds, and
         most of the time it is correct (e.g., footstamping -> footstamp, not
         footstampe; however, decoupled -> decoupl).  We can prevent almost all
         of the incorrect stems if we try to do some prefix analysis first */
            
      if (consonant(j) && consonant(j-1)) {
         k = j;
         word[k+1] = '\0';
         return;
         }
   
      word[j+1] = 'e';
      word[j+2] = '\0';
      k = j+1;
      return;
      }
}



/* handle some derivational endings */


/* this routine deals with -ion, -ition, -ation, -ization, and -ication.  The 
   -ization ending is always converted to -ize */

static void ion_endings ()
{
  int old_k = k;

  if (search_hash(dict_ht, word) != NULL)
     return;

  if (ends_in("ization"))  {   /* the -ize ending is very productive, so simply accept it as the root */
     word[j+3] = 'e';
     word[j+4] = '\0';
     k = j+3;
     return; 
     }


  if (ends_in("ition")) {     
     word[j+1] = 'e';
     word[j+2] = '\0';
     k = j+1;

     /* remove -ition and add `e', and check against the dictionary */
     if (search_hash(dict_ht, word) != NULL)     
        return;                    /* (e.g., definition->define, opposition->oppose) */

     /* restore original values */
     word[j+1] = 'i';
     word[j+2] = 't';
     k = old_k;
     }


  if (ends_in("ation"))  {
     word[j+3] = 'e';
     word[j+4] = '\0';
     k = j+3;         
     
    /* remove -ion and add `e', and check against the dictionary */
     if (search_hash(dict_ht, word) != NULL)   
        return;                  /* (elmination -> eliminate)  */


     word[j+1] = 'e';            /* remove -ation and add `e', and check against the dictionary */
     word[j+2] = '\0';           /* (allegation -> allege) */
     k = j+1;
     if (search_hash(dict_ht, word) != NULL)
        return;

     word[j+1] = '\0';           /* just remove -ation (resignation->resign) and check dictionary */
     k = j;
     if (search_hash(dict_ht, word) != NULL)
        return;
     
     /* restore original values */
     word[j+1] = 'a';
     word[j+2] = 't';
     word[j+3] = 'i';
     word[j+4] = 'o';            /* no need to restore word[j+5] (n); it was never changed */
     k = old_k;
     }


  /* test -ication after -ation is attempted (e.g., `complication->complicate' 
     rather than `complication->comply') */

  if (ends_in("ication"))  {
     word[j+1] = 'y';
     word[j+2] = '\0';
     k = j+1;
     
     /* remove -ication and add `y', and check against the dictionary */
     if (search_hash(dict_ht, word) != NULL)  
        return;                 /* (e.g., amplification -> amplify) */

     /* restore original values */
     word[j+1] = 'i';
     word[j+2] = 'c';
     k = old_k;
     }


  if (ends_in("ion")) {
     word[j+1] = 'e';
     word[j+2] = '\0';
     k = j+1;

     /* remove -ion and add `e', and check against the dictionary */
     if (search_hash(dict_ht, word) != NULL)    
        return;

     word[j+1] = '\0';
     k = j;

     /* remove -ion, and if it's found, treat that as the root */
     if (search_hash(dict_ht, word) != NULL)    
        return;

     /* restore original values */
     word[j+1] = 'i';
     word[j+2] = 'o';
     k = old_k;
     }


  return;
}



/* this routine deals with -er, -or, -ier, and -eer.  The -izer ending is always converted to
   -ize */

static void er_and_or_endings ()
{
  int old_k = k;

  char word_char;                 /* so we can remember if it was -er or -or */

  if (search_hash(dict_ht, word) != NULL)
    return;

  if (ends_in("izer")) {          /* -ize is very productive, so accept it as the root */
     word[j+4] = '\0';
     k = j+3;
     return;
     }

  if (ends_in("er") || ends_in("or")) {
     word_char = word[j+1];
     if (doublec(j)) {
        word[j] = '\0';
        k = j - 1;
        if (search_hash(dict_ht, word) != NULL)
           return;
        word[j] = word[j-1];       /* restore the doubled consonant */
        }
    
     
     if (word[j] == 'i') {         /* do we have a -ier ending? */
        word[j] = 'y';
        word[j+1] = '\0';
        k = j;
        if (search_hash(dict_ht, word) != NULL)  /* yes, so check against the dictionary */
           return;
        word[j] = 'i';             /* restore the endings */ 
        word[j+1] = 'e';
        }   


     if (word[j] == 'e') {         /* handle -eer */
        word[j] = '\0';
        k = j - 1;
        if (search_hash(dict_ht, word) != NULL)
           return;
        word[j] = 'e';
        }
       
     word[j+2] = '\0';            /* remove the -r ending */
     k = j+1;
     if (search_hash(dict_ht, word) != NULL)
        return;
     word[j+1] = '\0';            /* try removing -er/-or */
     k = j;
     if (search_hash(dict_ht, word) != NULL)
        return;
     word[j+1] = 'e';             /* try removing -or and adding -e */
     word[j+2] = '\0';
     k = j+1;
     if (search_hash(dict_ht, word) != NULL)
        return;
      
     word[j+1] = word_char;       /* restore the word to the way it was */
     word[j+2] = 'r';
     k = old_k;                  
     }

}




/* this routine deals with -ly endings.  The -ally ending is always converted to -al 
   Sometimes this will temporarily leave us with a non-word (e.g., heuristically
   maps to heuristical), but then the -al is removed in the next step.  */

static void ly_endings ()
{
   int old_k = k;

   if (search_hash(dict_ht, word) != NULL)
      return;

   if (ends_in("ly")) {
      word[j+2] = 'e';             /* try converting -ly to -le */
      if (search_hash(dict_ht, word) != NULL)       
         return;
      word[j+2] = 'y';

      word[j+1] = '\0';            /* try just removing the -ly */
      k = j;
      if (search_hash(dict_ht, word) != NULL)
         return;
      if ((word[j-1] == 'a') && (word[j] == 'l'))    /* always convert -ally to -al */
         return;
      word[j+1] = 'l';
      k = old_k;

      if ((word[j-1] == 'a') && (word[j] == 'b')) {  /* always convert -ably to -able */
         word[j+2] = 'e';
         k = j+2;
         return;
         }

      if (word[j] == 'i') {        /* e.g., militarily -> military */
         word[j] = 'y';
         word[j+1] = '\0';
         k = j;
         if (search_hash(dict_ht, word) != NULL)
            return;
         word[j] = 'i';
         word[j+1] = 'l';
         k = old_k;
         }

      word[j+1] = '\0';           /* the default is to remove -ly */
      k = j;
      }
   return;
}



/* this routine deals with -al endings.  Some of the endings from the previous routine
   are finished up here.  */

static void al_endings() 
{
   int old_k = k;

   if (search_hash(dict_ht, word) != NULL)
      return;

   if (ends_in("al"))  {
      word[j+1] = '\0';
      k = j;
      if (search_hash(dict_ht, word) != NULL)     /* try just removing the -al */
         return;

      if (doublec(j))  {            /* allow for a doubled consonant */
        word[j] = '\0';
        k = j-1;
        if (search_hash(dict_ht, word) != NULL)
           return;
        word[j] = word[j-1];
        }

      word[j+1] = 'e';              /* try removing the -al and adding -e */
      word[j+2] = '\0';
      k = j+1;
      if (search_hash(dict_ht, word) != NULL)
         return;

      word[j+1] = 'u';              /* try converting -al to -um */
      word[j+2] = 'm';              /* (e.g., optimal - > optimum ) */
      k = j+2;
      if (search_hash(dict_ht, word) != NULL)
         return;

      word[j+1] = 'a';              /* restore the ending to the way it was */
      word[j+2] = 'l';
      word[j+3] = '\0';
      k = old_k;

      if ((word[j-1] == 'i') && (word[j] == 'c'))  {
         word[j-1] = '\0';          /* try removing -ical  */
         k = j-2;
         if (search_hash(dict_ht, word) != NULL)
            return;

         word[j-1] = 'y';           /* try turning -ical to -y (e.g., bibliographical) */
         word[j] = '\0';
         k = j-1;
         if (search_hash(dict_ht, word) != NULL)
            return;

         word[j-1] = 'i';
         word[j] = 'c';
         word[j+1] = '\0';          /* the default is to convert -ical to -ic */
         k = j;
         return;
         }

      if (word[j] == 'i') {        /* sometimes -ial endings should be removed */
         word[j] = '\0';           /* (sometimes it gets turned into -y, but we */
         k = j-1;                  /* aren't dealing with that case for now) */
         if (search_hash(dict_ht, word) != NULL)
            return;
         word[j] = 'i';
         k = old_k;
         }

      }
      return;
}




/* this routine deals with -ive endings.  It normalizes some of the
   -ative endings directly, and also maps some -ive endings to -ion. */

static void ive_endings() 
{
   int old_k = k;

   if (search_hash(dict_ht, word) != NULL)
      return;

   if (ends_in("ive"))  {
      word[j+1] = '\0';          /* try removing -ive entirely */
      k = j;
      if (search_hash(dict_ht, word) != NULL)
         return;

      word[j+1] = 'e';           /* try removing -ive and adding -e */
      word[j+2] = '\0';
      k = j+1;
      if (search_hash(dict_ht, word) != NULL)
         return;
      word[j+1] = 'i';
      word[j+2] = 'v';

      if ((word[j-1] == 'a') && (word[j] == 't'))  {
         word[j-1] = 'e';       /* try removing -ative and adding -e */
         word[j] = '\0';        /* (e.g., determinative -> determine) */
         k = j-1;
         if (search_hash(dict_ht, word) != NULL)
            return;
         word[j-1] = '\0';     /* try just removing -ative */
         if (search_hash(dict_ht, word) != NULL)
            return;
         word[j-1] = 'a';
         word[j] = 't';
         k = old_k;
         }

       /* try mapping -ive to -ion (e.g., injunctive/injunction) */
       word[j+2] = 'o';
       word[j+3] = 'n';
       if (search_hash(dict_ht, word) != NULL)
          return;

       word[j+2] = 'v';       /* restore the original values */
       word[j+3] = 'e';
       k = old_k;
       }
   return;
}


/* this routine deals with -ize endings. */

static void ize_endings() 
{
  int old_k = k;

  if (search_hash(dict_ht, word) != NULL)
     return;

   if (ends_in("ize"))  {
      word[j+1] = '\0';       /* try removing -ize entirely */
      k = j;
      if (search_hash(dict_ht, word) != NULL)
         return;
      word[j+1] = 'i';

      if (doublec(j))  {      /* allow for a doubled consonant */
         word[j] = '\0';
         k = j-1;
        if (search_hash(dict_ht, word) != NULL)
           return;
        word[j] = word[j-1];
        }

      word[j+1] = 'e';        /* try removing -ize and adding -e */
      word[j+2] = '\0';
      k = j+1;
      if (search_hash(dict_ht, word) != NULL)
         return;
      word[j+1] = 'i';
      word[j+2] = 'z';
      k = old_k;
      }
   return;
}



/* this routine deals with -ment endings. */

static void ment_endings() 
{
  int old_k = k;

  if (search_hash(dict_ht, word) != NULL)
      return;

  if (ends_in("ment"))  {
     word[j+1] = '\0';
     k = j;
     if (search_hash(dict_ht, word) != NULL)
        return;
     word[j+1] = 'm';
     k = old_k;
     }
  return;
}




/* this routine deals with -ity endings.  It accepts -ability, -ibility,
   and -ality, even without checking the dictionary because they are so 
   productive.  The first two are mapped to -ble, and the -ity is remove
   for the latter */

static void ity_endings() 
{
  int old_k = k;

  if (search_hash(dict_ht, word) != NULL)
      return;

  if (ends_in("ity"))  {
     word[j+1] = '\0';             /* try just removing -ity */
     k = j;
     if (search_hash(dict_ht, word) != NULL)
        return;
     word[j+1] = 'e';              /* try removing -ity and adding -e */
     word[j+2] = '\0';
     k = j+1;
     if (search_hash(dict_ht, word) != NULL)
        return;
     word[j+1] = 'i';
     word[j+2] = 't';
     k = old_k;

    /* the -ability and -ibility endings are highly productive, so just accept them */
    if ((word[j-1] == 'i') && (word[j] == 'l'))  {   
       word[j-1] = 'l';          /* convert to -ble */
       word[j] = 'e';
       word[j+1] = '\0';
       k = j;
       return;
       }


    /* ditto for -ivity */
    if ((word[j-1] == 'i') && (word[j] == 'v'))  {
       word[j+1] = 'e';         /* convert to -ive */
       word[j+2] = '\0';
       k = j+1;
       return;
       }

    /* ditto for -ality */
    if ((word[j-1] == 'a') && (word[j] == 'l'))  {
       word[j+1] = '\0';
       k = j;
       return;
       }

    /* if the root isn't in the dictionary, and the variant *is*
       there, then use the variant.  This allows `immunity'->`immune',
       but prevents `capacity'->`capac'.  If neither the variant nor
       the root form are in the dictionary, then remove the ending
       as a default */

    if (search_hash(dict_ht, word) != NULL)   
       return;

    /* the default is to remove -ity altogether */
    word[j+1] = '\0';
    k = j;
    return;
    }
}




/* handle -able and -ible */

static void ble_endings() 
{
  int old_k = k;
  char word_char;

  if (search_hash(dict_ht, word) != NULL)
     return;

  if (ends_in("ble"))  {

     if (!((word[j] == 'i') || (word[j] == 'a'))) return;

     word_char = word[j];
     word[j] = '\0';             /* try just removing the ending */
     k = j-1;
     if (search_hash(dict_ht, word) != NULL) 
        return;
     if (doublec(k))  {          /* allow for a doubled consonant */
        word[k] = '\0';
        k--;
        if (search_hash(dict_ht, word) != NULL)
           return;
        k++;
        word[k] = word[k-1];
        }
     word[j] = 'e';              /* try removing -a/ible and adding -e */
     word[j+1] = '\0';
     k = j;
     if (search_hash(dict_ht, word) != NULL)
        return;

     word[j] = 'a';              /* try removing -able and adding -ate */
     word[j+1] = 't';            /* (e.g., compensable/compensate)     */
     word[j+2] = 'e';
     word[j+3] = '\0';
     k = j+2;
     if (search_hash(dict_ht, word) != NULL)
        return;

     word[j] = word_char;        /* restore the original values */
     word[j+1] = 'b';
     word[j+2] = 'l';
     word[j+3] = 'e';
     k = old_k;
     }
    return;
}



/* handle -ness */

static void ness_endings() 
{

  if (search_hash(dict_ht, word) != NULL)
     return;

   if (ends_in("ness"))  {     /* this is a very productive endings, so just accept it */
      word[j+1] = '\0';
      k = j;
      if (word[j] == 'i')  
         word[j] = 'y';
      }
   return;
}



/* handle -ism */

static void ism_endings()
{

   if (search_hash(dict_ht, word) != NULL)
      return;

   if (ends_in("ism"))  {    /* this is a very productive ending, so just accept it */
      word[j+1] = '\0';
      k = j;
      }
   return;
}



/* handle -ic endings.   This is fairly straightforward, but this is
   also the only place we try *expanding* an ending, -ic -> -ical.
   This is to handle cases like `canonic' -> `canonical' */

static void ic_endings()
{

    if (search_hash(dict_ht, word) != NULL)
       return;

    if (ends_in("ic")) {
       word[j+3] = 'a';        /* try converting -ic to -ical */
       word[j+4] = 'l';
       word[j+5] = '\0';
       k = j+4;
       if (search_hash(dict_ht, word) != NULL)
          return;

       word[j+1] = 'y';        /* try converting -ic to -y */
       word[j+2] = '\0';
       k = j+1;
       if (search_hash(dict_ht, word) != NULL)
          return;
    
       word[j+1] = 'e';        /* try converting -ic to -e */
       if (search_hash(dict_ht, word) != NULL)
          return;

       word[j+1] = '\0';       /* try removing -ic altogether */
       k = j;
       if (search_hash(dict_ht, word) != NULL)
          return;

       word[j+1] = 'i';        /* restore the original ending */
       word[j+2] = 'c';
       word[j+3] = '\0';
       k = j+2;
       }
    return;
}



/* handle -ency and -ancy */

static void ncy_endings()
{
  if (search_hash(dict_ht, word) != NULL)
      return;

   if (ends_in("ncy"))  {

      if (!((word[j] == 'e') || (word[j] == 'a'))) return; 

      word[j+2] = 't';          /* try converting -ncy to -nt */
      word[j+3] = '\0';         /* (e.g., constituency -> constituent) */
      k = j+2;

      if (search_hash(dict_ht, word) != NULL)
         return;

      word[j+2] = 'c';          /* the default is to convert it to -nce */
      word[j+3] = 'e';
      k = j+3;
      }
   return;
}



/* handle -ence and -ance */

static void nce_endings()
{
   int old_k = k;

   char word_char;

   if (search_hash(dict_ht, word) != NULL)
      return;

   if (ends_in("nce"))  {

      if (!((word[j] == 'e') || (word[j] == 'a'))) return; 

      word_char = word[j];
      word[j] = 'e';        /* try converting -e/ance to -e (adherance/adhere) */
      word[j+1] = '\0';
      k = j;
      if (search_hash(dict_ht, word) != NULL)
         return;
      word[j] = '\0';       /* try removing -e/ance altogether (disappearance/disappear) */
      k = j-1;
      if (search_hash(dict_ht, word) != NULL)
         return;
      word[j] = word_char;  /* restore the original ending */
      word[j+1] = 'n';
      k = old_k;
      }
    return;
}




void stem(char *term, char *stem)
{
    int i;

    if (!dict_initialized_flag) {
      printf("Error!  Dictionary was not initialized.\n              A call to read_dict_info() must be made before calling the stemmer.\n");
      exit(1);
      }

    word = stem;

    k = strlen((char *)term) - 1;
    for (i=0; i<=k; i++)           /* lowercase the local copy */
      word[i] = tolower(term[i]);

    word[k+1] = '\0';



    /* if the string is not entirely alphabetic, then just return it
       as the stem */

    for (i=0; i<=k; i++)          
      if (!isalpha(word[i]))
         return;


    /* the basic algorithm is to check the dictionary, and leave the word as it
       is if the word is found.  Otherwise, recognize plurals, tense, etc. and
       normalize according to the rules for those affixes.  Check against the
       dictionary at each stage, so `longings' -> `longing' rather than `long'.
       Finally, deal with some derivational endings.  The -ion, -er, and -ly
       endings must be checked before -ize.  The -ity ending must come before
       -al, and -ness must come before -ly and -ive.  Finally, -ncy must come
       before -nce (because -ncy is converted to -nce for some instances). */



    /* try for a direct mapping  (this allows for cases like `Italian'->`Italy') */
    lookup_value = search_hash(dict_ht, word);
    if (lookup_value != NULL) {
       dep = (dictentry *)lookup_value;             /* if the root is "", then the result is */
       if (dep->root != "") {                       /* the word itself (which was simply shifted */
          strcpy((char *)stem, (char *)dep->root);  /* to lowercase at the beginning of the  */
          return;                                   /* routine). */
          } 
       }

    plural();
    past_tense();
    aspect();

   
    /* try again for a direct mapping (this allows cases like `Italians'->`Italy') */
   lookup_value = search_hash(dict_ht, word);
    if (lookup_value != NULL)  {
       dep = (dictentry *)lookup_value;             /* if the root is "", then the result is */
       if (dep->root != "")  {                      /* the word itself (which was simply shifted */
         strcpy((char *)stem, (char *)dep->root);   /* to lowercase at the beginning of the */
         return;                                    /* routine). */
          }
        }

    ity_endings();
    ness_endings();
    ion_endings();
    er_and_or_endings();
    ly_endings();
    al_endings();
    ive_endings();
    ize_endings();
    ment_endings();
    ble_endings();
    ism_endings();
    ic_endings();
    ncy_endings();
    nce_endings();
    
    /* for the last time, try for a direct mapping */
    lookup_value = search_hash(dict_ht, word);
    if (lookup_value != NULL)  {              /* if we now have a word in the dictionary, */
       dep = (dictentry *)lookup_value;       /* see if we can convert it to another form  */
       if (dep->root != "")
          strcpy((char *)stem, (char *)dep->root);
       }
}


