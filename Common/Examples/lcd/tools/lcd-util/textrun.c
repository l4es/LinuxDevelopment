#include <stdlib.h>
#include <string.h>

/*
 * wrap around a string *text by step characters
 * repeated calls give a "running text" effect
 */

void textrun(char *text, unsigned int step)
{
char *tmp;

  if (text!=NULL)
    tmp=(char *)malloc(strlen(text)*sizeof(char));
  else
    return;
  if (strlen(text)>step) {
    strcpy(tmp,&text[step]);
    strncat(tmp,text,step);
    strcpy(text,tmp);
  }
}
