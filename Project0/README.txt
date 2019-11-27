TITLE: Project 0

NAME: Clayton Glenn
EMAIL: Glen5641@ou.edu

DATE: 9/11/2018

Description
Reading in arguments from Ubuntu ::- Reading argc to see ammount of
	argumens argv had. Once done, we know, if 1, then argv will not
	contain any needed characters. If arrgc is 2, then argv will contain
	the Lower unsigned character, if 3, then argv will contain both the
	high and low unsigned characters. Once high and low are found, the
	range is printed to the console.(Unsigned Char)

Creating Counting Array ::- Use an integer array to save the count of each
	 character for example, if the set of elements is a-g, then the size
	 of the array is a,b,c,d,e,f,g, which is size 7, where a=0 and g=6
	 and the whole array is initiallized to 0 to rid NULL values.

Reading file::- The file is read until end of file one character at a time.
	Each character is subtracted by the low constraint and will convert a
	character range of 70-100 to 0-30 and these indices are used for the
	array, and that specific character is incremented by 1.(STDIO)

Output::- The program loops through each character in the range specified and
	  prints the count and loops through a j-count for loop to print each
	  #. The end of the program prints a new line to break the output
	  from command line.

References
STDIO Library Functions - https://fresh2refresh.com/c-programming/c-function/stdio-h-library-functions(STDIO)
Extended ASCII table - https://theasciicode.com.ar/(ASCII)
Use of Unsigned Characters - https://canvas.ou.edu/courses/88749/discussion_topics/416585(Unsigned Char)
