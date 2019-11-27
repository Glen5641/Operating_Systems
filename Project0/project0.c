#include <stdio.h>

/*
 * Program for counting each character in a file.
 * @Author Glen5641
 */
int main(int argc, char** argv){

  //Declare high or low to catch console args and a counter.
  unsigned char high = 125;
  unsigned char low = 32;
  int i = 0;
  
  //Read the console args and either accept 1 argument for a "Low"
  //or 2 arguments for "Low" and "High".
  if(argc == 2){
    low = (unsigned char)argv[1][0];
  }
  if(argc == 3){
    low = (unsigned char)argv[1][0];
    high = (unsigned char)argv[2][0];
  }

  //Print the range of the character span we are counting.
  printf("Range: %c-%c\n", low, high);

  //Initiallize the array for the characters with a
  //starting value of zero to avoid nullity.
  int characters[high - low + 1];
  for(i = 0; i <= (high - low + 1); ++i){
    characters[i] = 0;
  }

  //Read piped file or keyboard to count each character
  // and increment its parallel array. Source: C-stdio.h
  // library functions (https://fresh2refresh.com/c-prog
  // ramming/c-function/stdio-h-libary-functions)
  int ch;
  while(!feof(stdin)){
    ch = getc(stdin) - low;
    if(ch >= 0  && ch < (high - low + 1)){
      characters[ch] = characters[ch] + 1;
    }
  }

  //Iterate through each character and show the character of discription
  //of the invisible character and amount of times the character was counted.
  //Source: Extended ASCII table (https://theasciicode.com.ar/)
  for(i = 0; i <= (high - low); ++i){
    printf("%c: %4d ",    (unsigned char)(low + i), characters[i]);

    //Iterate through the amount of characters to show count using '#'
    int j;
    for(j = 0; j < characters[i]; ++j){
      printf("#");
    }
    printf("\n");
  }

  //Return with a system exit
  return 0;
}
