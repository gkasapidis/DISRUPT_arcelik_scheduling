#include "string.h"
#include <vector>
#include <string>
#include <typeinfo>

using namespace std;

//STRSEP custom implementation
char* mystrsep(char** stringp, const char* delim)
{
	char* start = *stringp;
	char* p;

	p = (start != NULL) ? strpbrk(start, delim) : NULL;

	if (p == NULL)
	{
		*stringp = NULL;
	}
	else
	{
		*p = '\0';
		*stringp = p + 1;
	}

	return start;
}


//INSERT_STRING
/*	This function takes a string as an input, a delimiter, an input text and a position
	It splits the string based on the selected delimiter and then adds the text to the 
	corresponding position of the split vector
*/

char* insert_string(string in_string, const char* delim, string text, int pos){
	char* out_string = new char[400]();
	char temp[200] = {0};
	char* p_t = temp;
	char** pp_t = &p_t;

	//Copy input string to temp
	strcpy(temp, in_string.c_str());
	//Split string using delimiter

	char* point;
	//const char* delim = ".";
	point = mystrsep(&p_t, delim);

	vector<char*> split = vector<char*>();
	int split_step = 0;
	while (point != NULL) {
		if (strlen(point) != 0) split.push_back(point);

		if (split_step == pos) split.push_back((char*) text.c_str());

		split.push_back((char*) delim);
		point = mystrsep(&p_t, delim);
		split_step += 1;
	}

	for (size_t i = 0; i < split.size() - 1; i++) {
		strcat(out_string, split[i]);
	}
	
	//Cleanup
	split.clear();
	vec_del<char*>(split);
	
	return out_string;
};



