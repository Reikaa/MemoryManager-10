// This is a simple test program

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>

#include "vmms.h"

struct student
{
  int	ID;
  char	name[20];
  struct student *left;
  struct student *right;
};

int main(int argc, char** argv)
{
  int rc = 0;
  char *student_ptr;
  char *list;
  char *list1;
  char *list2;
  char *list3;
  char *list4;
  char *rnd;
  char *rnd1;
  char *rnd2;
  char *rnd3;
  char *rnd4;


  list = (char*) vmms_malloc(50, &rc);
  list1 = (char *)vmms_malloc(19, &rc);
  list2 = (char *)vmms_malloc(90, &rc);
  list3 = (char *)vmms_malloc(10, &rc);
  list4 = (char *)vmms_malloc(78, &rc);

  if (list == NULL)
    return 1;
  strcpy (list, "dummy1");
  strcpy (list+10, "911");
  printf("list address = %08p; Name = %s; ID = %s\n", list, list, (char*)list+10);

  if (list1 == NULL)
	  return 1;
  strcpy(list1, "dummy2");
  strcpy(list1 + 10, "912");
  printf("list address = %08p; Name = %s; ID = %s\n", list1, list1, (char*)list1 + 10);

  rnd = vmms_malloc(234, &rc);
  if (rnd == NULL)
	  return 1;
  strcpy(rnd, "Random1");
  strcpy(rnd + 10, "375");
  printf("list address = %08p; Name = %s; ID = %s\n", rnd, rnd, (char*)rnd + 10);
   
  if (list2 == NULL)
	  return 1;
  strcpy(list2, "dummy3");
  strcpy(list2 + 10, "913");
  printf("list address = %08p; Name = %s; ID = %s\n", list2, list2, (char*)list2 + 10);
  
  if (list3 == NULL)
	  return 1;
  strcpy(list3, "dummy4");
  strcpy(list3 + 10, "914");
  printf("list address = %08p; Name = %s; ID = %s\n", list3, list3, (char*)list3 + 10);

  if (list4 == NULL)
	  return 1;
  strcpy(list4, "dummy5");
  strcpy(list4 + 10, "915");
  printf("list address = %08p; Name = %s; ID = %s\n", list4, list4, (char*)list4 + 10);

  system("pause");


  rc = vmms_free((char*)list);
  rc = vmms_free((char*)list1);
  rc = vmms_free((char*)list2);

  rnd1 = vmms_malloc(33, &rc);
  if (rnd1 == NULL)
	  return 1;
  strcpy(rnd1, "Random2");
  strcpy(rnd1 + 10, "937");
  printf("list address = %08p; Name = %s; ID = %s\n", rnd1, rnd1, (char*)rnd1 + 10);

  rc = vmms_free((char*)list3);
  rc = vmms_free((char*)list4);

  system("pause");

  char *list5;
  char *list6;
  char *list7;
  char *list8;
  char *list9;


  list5 = (char*)vmms_malloc(34, &rc);
  list6 = (char *)vmms_malloc(250, &rc);
  list7 = (char *)vmms_malloc(63, &rc);
  list8 = (char *)vmms_malloc(100, &rc);
  list9 = (char *)vmms_malloc(78, &rc);

  if (list5 == NULL)
	  return 1;
  strcpy(list5, "dummy6");
  strcpy(list5 + 10, "916");
  printf("list address = %08p; Name = %s; ID = %s\n", list5, list5, (char*)list5 + 10);

  if (list6 == NULL)
	  return 1;
  strcpy(list6, "dummy7");
  strcpy(list6 + 10, "917");
  printf("list address = %08p; Name = %s; ID = %s\n", list6, list6, (char*)list6 + 10);

  if (list7 == NULL)
	  return 1;
  strcpy(list7, "dummy8");
  strcpy(list7 + 10, "918");
  printf("list address = %08p; Name = %s; ID = %s\n", list7, list7, (char*)list7 + 10);

  if (list8 == NULL)
	  return 1;
  strcpy(list8, "dummy9");
  strcpy(list8 + 10, "919");
  printf("list address = %08p; Name = %s; ID = %s\n", list8, list8, (char*)list8 + 10);

  if (list9 == NULL)
	  return 1;
  strcpy(list9, "dummy10");
  strcpy(list9 + 10, "9110");
  printf("list address = %08p; Name = %s; ID = %s\n", list9, list9, (char*)list9 + 10);

  system("pause");


  rnd2 = vmms_malloc(33, &rc);
  if (rnd2 == NULL)
	  return 1;
  strcpy(rnd2, "Random3");
  strcpy(rnd2 + 10, "527");
  printf("list address = %08p; Name = %s; ID = %s\n", rnd2, rnd2, (char*)rnd2 + 10);

  rnd3 = vmms_malloc(33, &rc);
  if (rnd3 == NULL)
	  return 1;
  strcpy(rnd3, "Random4");
  strcpy(rnd3 + 10, "345");
  printf("list address = %08p; Name = %s; ID = %s\n", rnd3, rnd3, (char*)rnd3 + 10);

  rnd4 = vmms_malloc(33, &rc);
  if (rnd4 == NULL)
	  return 1;
  strcpy(rnd4, "Random5");
  strcpy(rnd4 + 10, "591");
  printf("list address = %08p; Name = %s; ID = %s\n", rnd4, rnd4, (char*)rnd4 + 10);
  
  
  system("pause");

  rc = vmms_free((char*)rnd1);
  rc = vmms_free((char*)rnd3);
  rc = vmms_free((char*)rnd2);
  
  system("pause");

  rc = vmms_free((char*)rnd4);
  rc = vmms_free((char*)rnd);

  rc = vmms_free((char*)list5);
  rc = vmms_free((char*)list6);
  rc = vmms_free((char*)list7);
  rc = vmms_free((char*)list8);
  rc = vmms_free((char*)list9);




  system("pause");
  return rc;
}



	


