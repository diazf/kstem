These are the lexicon files for Kstem.  They are described in
detail in the doc directory for the source code. 

  1. The most important thing to note is that the environment 
     variable, STEM_DIR, must be set to point to this directory 
     before the stemmer will work.

  2. PLEASE do not change the file permissions for head_word_list.txt,
     proper_nouns.txt, e_exception_words.txt, and country_nationality.txt.  
     I have intentionally set them to be read-only and there should be 
     no need to change them.  All changes can be made through 
     dict_supplement.txt, and direct_conflations.txt.  I made the
     other files read-only in order to make it easier to keep track
     of any differences that are due to changes in the lexicon.

