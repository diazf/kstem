/*
 * lisp debug version 0.8 a source level debugger for lisp
 * Copyright (C) 1998 Marc Mertens
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 * You can reach me at : mmertens@akam.be
 */

/*
 * List and hash functions
 */


#include <stdlib.h>
#include <string.h>
#include "hash.h"
#include <math.h>


/* 
 * Conses an element and a list , memory for data pointed to by key and data is already created
 */

LIST *cons(char *key,void *data,LIST *lst)
{
  LIST *new_list;
  
  new_list=(LIST *) malloc(sizeof(LIST));
  new_list->key=key;
  new_list->data=data;
  new_list->next=lst;
  return new_list;
}


/*
 * Search in a list for a key and returns pointer to data
 */

void *search(char *key,LIST *lst)
{
  LIST *l;
  
  l=lst;
  while (l!=NULL)
    {
      if (strcmp(l->key,key)==0)
	{
	  return l->data;
	}
      else
	{
	  l=l->next;
	};
    };
  return NULL;
}


/*
 * Remove the first occurence of a element in a list
 */

LIST *delete_key(char *key,LIST *lst)
{
  LIST *l1,*l2;
  
  if (lst==NULL)
    return NULL;
  if (strcmp(lst->key,key)==0)
    {
      free(lst->data);
      free(lst->key);
      free(lst);
      return lst->next;
    }
  else
    {
      l1=lst;
      l2=lst->next;
      while (l2!=NULL)
	{
	  if (strcmp(l2->key,key)==0)
	    {
	      free(l2->data);
	      free(l2->key);
	      l1->next=l2->next;
	      free(l2);
	      return lst;
	    }
	  else
	    {
	      l1=l2;
	      l2=l1->next;
	    };
	};
    }
  return lst;
}

/*
 * Remove all the occurences of a element in a list
 */

LIST *delete_all(char *key,LIST *lst)
{
  LIST *l1,*l2;
  
  while (lst !=NULL && strcmp(lst->key,key)==0)
    {
      free(lst->key);
      free(lst->data);
      l1=lst;
      lst=lst->next;
      free(l1);
    };
  if (lst==NULL)
    {
      return NULL;
    }
  else
    {
      l1=lst;
      l2=lst->next;
      while (l2!=NULL)
	{
	  if (strcmp(l2->key,key)==0)
	    {
	      free(l2->key);
	      free(l2->data);
	      l1->next=l2->next;
	      free(l2);
	      l2=l1->next;
	    }
	  else
	    {
	      l1=l2;
	      l2=l1->next;
	    };
	};
      return lst;
    };
}

/*
 * Remove all the occurences of a element in a list based on a condition
 */

LIST *delete_condition(LIST *lst,int (condition) (char *key,void *data))
{
  LIST *l1,*l2;
  
  while (lst !=NULL && condition(lst->key,lst->data))
    {
      free(lst->key);
      free(lst->data);
      l1=lst;
      lst=lst->next;
      free(l1);
    };
  if (lst==NULL)
    {
      return NULL;
    }
  else
    {
      l1=lst;
      l2=lst->next;
      while (l2!=NULL)
	{
	  if (condition(l2->key,l2->data))
	    {
	      free(l2->key);
	      free(l2->data);
	      l1->next=l2->next;
	      free(l2);
	      l2=l1->next;
	    }
	  else
	    {
	      l1=l2;
	      l2=l1->next;
	    };
	};
      return lst;
    };
}

/*
 * Walk a list 
 */

void walk_list(LIST *lst,void (action) (char *key,void *data))
{
  LIST *l1,*l2;
  
  l1=lst;
  while (l1!=NULL)
    {
      l2=l1;
      l1=l1->next;
      action(l2->key,l2->data);
    };
}
      
/* 
 * Generate hash key of some key
 */

int hash(char key[],int m)
{
  double A=(sqrt(5)-1)/2;
  int k,i;

  k=0;
  for (i=0;key[i]!='\0';i++)
    k=k+key[i];
  return m*fmod(A*k,1);
}


/*
 * Create hash table with +/_ m elements 
 */

HASH *create_hash(int m)
{
  HASH *h;
  int i;

  h=(HASH *) malloc(sizeof(HASH));
  h->l=m;
  h->t=(LIST **) malloc(sizeof(LIST *)*(m+1));
  for (i=0;i<=m;i++)
    h->t[i]=(LIST *) NULL;
  return h;
}


/* 
 * Insert an element in a hash table
 */

void insert_hash(HASH *h,char *key,void *data)
{
  int k;
  
  k=hash(key,h->l);
  h->t[k]=cons(key,data,h->t[k]);
};

/*
 * Search a hash table 
 */


void *search_hash(HASH *h,char *key)
{
  int k;
  
  k=hash(key,h->l);
  return search(key,h->t[k]);
}

/* 
 * Delete a item from the hash table
 */

void delete_hash(HASH *h,char *key)
{
  int k;
  
  k=hash(key,h->l);
  h->t[k]=delete_key(key,h->t[k]);
}

/* 
 * Delete a item based on a conditions from the hash table
 */

void delete_hash_condition(HASH *h,int (condition) (char *key,void *data))
{
  int i;
  
  for (i=0;i<=h->l;i++)
    if (h->t[i]!=NULL)
      h->t[i]=delete_condition(h->t[i],condition);
}

/*
 * Walk a hashtable
 */

void walk_hash(HASH *h,void (action) (char *key,void *data))
{
  int i;
  for (i=0;i<=h->l;i++)
    {
      if (h->t!=NULL)
	{
	  walk_list(h->t[i],action);
	};
    };
}







