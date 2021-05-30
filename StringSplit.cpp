#include "stringSplit.h"
#include <WString.h>

#define RESULT_SIZE 10

String* splitString(String s, char delimiter) {
   static String result[RESULT_SIZE];
   char deli[] = { delimiter, 0 };
   char buffer[255];        
   strncpy(buffer, s.c_str(), sizeof(buffer));
   for (int i = 0; i < RESULT_SIZE; i++) 
      result[i] = "";
   int index = 0;
   char* token = strtok(buffer, deli);
   while (token != NULL) {
      result[index++] = token;
      if (index >= RESULT_SIZE)
         break;
      token = strtok(NULL, deli);
   }
   return result;
}
