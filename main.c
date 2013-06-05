/*
   Album database reading program.
       This program provides a command-line interface to read, search, and
   modify a database of albums.

   FINAL: December 5th, 2011

   Written by Mike Perron
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_L 256

typedef struct _node {
	struct _node *next;
	char *raw;
	char *artist, *album, *year;
	int year_i;
} node, *list;

// Make strings friendly to users.
char *desanitize(char *buffer);
void desanitize_r(char *buffer);

// Read from an open file db.
list read_db(FILE *db);

// Sort by artist, album, or year.
list sort(list db, int mode);

// Make strings friendly to databases.
char *sanitize(char *buffer);

// Write list db to open file out.
void save_to(list db, FILE *out);

// Case insensitive search for target in source.
//(returns -1 on no match, else location of match in the string)
int search(char *source, char *target);

// Return a pointer to a string containing source in all lowercase letters.
char *mash_case(char *source);

// Remove the pos element from db.
list rm(list db, int pos);


int main(){
	char state=0, *buffer, *free_buffer=malloc(sizeof(char));
	list database=NULL, temp;
	FILE *in=NULL, *out=NULL;
	int i, u, counter=0;

	printf("\nMike Perron's Library Manager\nType help for command assistance.\n");
	while(!(state&1)){	// Loop is broken when state's 0x1 bit is set.
		free(free_buffer);
		free_buffer=buffer=calloc(MAX_L, sizeof(char));

		printf("\n> ");
		fgets(buffer, 256, stdin);
		for(i=0;(buffer[i]!='\n')&&(buffer[i]!='\0')&&(buffer[i]!='\r');i++);
		buffer[i]='\0';

		// Command time!
		if((!strcmp(buffer, "exit"))||(!strcmp(buffer, "quit"))||(!strcmp(buffer, "q")))
			state^=1;
		// Open a database (commands either open or load <filename>)
		else if((!strncmp(buffer, "open ", 5))||(!strncmp(buffer, "load ", 5))){
			buffer+=5;
			if(!(in=fopen(buffer, "r")))
				printf("Could not open file.\n");
			else {
				database=read_db(in);
				fclose(in);
			}
		}
		// List open database (commands either list or ls)
		else if((!strcmp(buffer, "list")||(!strcmp(buffer, "ls")))){
			if(!database){
				printf("Need a database first. Try open or help.\n");
				continue;
			}
			i=0;
			for(temp=database;temp!=NULL;temp=temp->next)
				printf("%4d: %s - %s (%d)\n", ++i, temp->artist, temp->album, temp->year_i);
		}
		// Sort the open database (command sort <artist|album|year>)
		else if(!strncmp(buffer, "sort ", 5)){
			if(!database){
				printf("Need a database first. Try open or help.\n");
				continue;
			}
			buffer+=5;
			if(!strcmp(buffer, "year")){
				database=sort(database, 1);
				printf("Sorted by year.\n");
			}
			else if(!strcmp(buffer, "artist")){
				database=sort(database, 2);
				printf("Sorted by artist.\n");
			}
			else if(!strcmp(buffer, "album")){
				database=sort(database, 3);
				printf("Sorted by album.\n");
			}
			else printf("Need a sorting method. Try artist, album, or year.\n");
		}
		// Add a new entry. (command add)
		else if(!strcmp(buffer, "add")){
			// Make a new entry and point its next to database
			temp=malloc(sizeof(node));
			temp->next=database;

			temp->artist=calloc(MAX_L, sizeof(char));
			printf("Artist: ");
			fgets(temp->artist, MAX_L, stdin);

			temp->album=calloc(MAX_L, sizeof(char));
			printf("Album: ");
			fgets(temp->album, MAX_L, stdin);

			temp->year=calloc(MAX_L, sizeof(char));
			printf("Year: ");
			fgets(temp->year, MAX_L, stdin);

			for(i=0;temp->artist[i]!='\n'&&temp->artist[i]!='\0'&&temp->artist[i]!='\r';i++);
			temp->artist[i]='\0';

			for(i=0;temp->album[i]!='\n'&&temp->album[i]!='\0'&&temp->album[i]!='\r';i++);
			temp->album[i]='\0';

			for(i=0;temp->year[i]!='\n'&&temp->year[i]!='\0'&&temp->year[i]!='\r';i++);
			temp->year[i]='\0';

			temp->year_i=atoi(temp->year);
			database=temp;
		}
		// Save active database to a file (command save <filename>)
		else if(!strncmp(buffer, "save ", 5)){
			buffer+=5;
			for(i=0;buffer[i]!='\n'&&buffer[i]!='\0'&&buffer[i]!='\r';i++);
			buffer[i]='\0';

			printf("Attempting to write to %s...", buffer);
			if(!(out=fopen(buffer, "w")))
				printf("Could not open file.\n");
			else save_to(database, out);
			fclose(out);
		}
		// Case insensitive search (command search <query>)
		else if(!strncmp(buffer, "search ", 7)){
			buffer+=7;
			counter=i=0;
			for(temp=database;temp!=NULL;temp=temp->next){
				// Search for matches in each category, including string year.
				// Print the entire album's information for each match as found.
				++i;
				if((search(temp->artist, buffer)!=-1)||(search(temp->album, buffer)!=-1)||(search(temp->year, buffer)!=-1)){
					printf("%4d: %s - %s (%d)\n", i, temp->artist, temp->album, temp->year_i);
					counter++;
				}
			}
			printf("%d result%s containing '%s.'\n", counter, (counter==1)?"":"s", buffer);
		}
		// Remove item from list (command remove <index>)
		else if(!strncmp(buffer, "remove ", 7)){
			buffer+=7;
			i=atoi(buffer);
			if(--i<0){
				printf("Index values are positive integers.\nType list or search to find them, or help if you're confused.\n");
				continue;
			}
			u=i;
			for(temp=database;i--&&temp!=NULL;temp=temp->next);
			if(temp){
				// Confirm removal (default YES if no response)
				printf("Remove \"%s - %s\" (Y/n): ", temp->artist, temp->album, temp->year_i);
				free(free_buffer);
				free_buffer=buffer=calloc(MAX_L, sizeof(char));
				fgets(buffer, MAX_L, stdin);
				buffer=mash_case(buffer);
				if(*buffer=='y'||*buffer=='\n'||*buffer=='\r'||*buffer=='\0')
					database=rm(database, u);
			} else printf("No such entry.\n");
		}
		// Help! (command help)
		else if((!strcmp(buffer, "help")||(!strcmp(buffer, "?")))){
			printf("-- Commands -- (all lowercase)\n");
			printf("quit, exit        |\tExit.\n");
			printf("open <filename>   |\tOpens and parses the given database.\n");
			printf("save <filename>   |\tSave the active database to the given filename.\n");
			printf("list, ls          |\tLists the raw data from the database.\n");
			printf("sort <mode>       |\tMode can be year, artist, or album.\n");
			printf("search <string>   |\tSearches the current data for the given string.\n");
			printf("add               |\tAllows you to add a new entry.\n");
			printf("remove <index>    |\tRemoves the given entry from the list. (Type list to see index values.)\n");
		}
		else printf("Command not recognized. Type help for command assistance.\n");
	}

	return 0;
}

// Desanitize protecting buffer
char *desanitize(char *buffer){
	char *buf=malloc(MAX_L*sizeof(char));
	int i;

	strcpy(buf, buffer);
	desanitize_r(buf);

	return buf;
}

// Desanitize destroying buffer
void desanitize_r(char *buffer){
	int i;

	for(i=0;buffer[i]!='\0';i++)switch(buffer[i]){
		case '_':
			buffer[i]=' ';
			break;
		case '\\':
			buffer[i]='_';
			break;
		case '\r':
		case '\n':
			buffer[i]='\0';
			break;
	}
}

list read_db(FILE *db){
	node *data, *root, *release;
	char *buffer, *free_buffer;
	int i, u, counter=0;

	data=root=malloc(sizeof(node));
	data->next=data;
	
	do{	free_buffer=buffer=calloc(MAX_L, sizeof(char));
		fgets(buffer, MAX_L, db);
		if(feof(db))break;
		if(*buffer=='\0'||*buffer=='\r'||*buffer=='\n'){ // Skip parsing empty lines
			free(free_buffer);
			continue;
		}
		data=data->next;

		data->raw=malloc(MAX_L*sizeof(char));
		strcpy(data->raw, desanitize(buffer));

		// Read the artist name
		for(i=0;(buffer[i]!=' ')&&(buffer[i]!='\0')&&(buffer[i]!='\r')&&(buffer[i]!='\n');i++);
		data->artist=malloc((i+1)*sizeof(char)); // allocate exactly enough space
		strncpy(data->artist, buffer, i);
		data->artist[i]='\0'; // strncpy doesn't null-terminate.
		buffer+=++i;
		desanitize_r(data->artist);

		// Read the album name
		for(i=0;(buffer[i]!=' ')&&(buffer[i]!='\0')&&(buffer[i]!='\r')&&(buffer[i]!='\n');i++);
		data->album=malloc((i+1)*sizeof(char));
		strncpy(data->album, buffer, i);
		data->album[i]='\0'; // strncpy doesn't null-terminate.
		buffer+=++i;
		desanitize_r(data->album);

		// Read the year
		for(i=0;(buffer[i]!=' ')&&(buffer[i]!='\0')&&(buffer[i]!='\r')&&(buffer[i]!='\n')&&i<8;i++);
		data->year=malloc((i+1)*sizeof(char));
		strncpy(data->year, buffer, i++);
		data->year[i]='\0'; // strncpy doesn't null-terminate.
		desanitize_r(data->year);
		data->year_i=atoi(data->year);

		data->next=malloc(sizeof(node));
		counter++;
		free(free_buffer);
	}	while(1);
	release=data->next; // Release empty extra node
	data->next=NULL;
	free(release);

	printf("%d entries read.\n", counter);

	return root;
}

list sort(list db, int mode){
	node *temp, *temp_l, *base=malloc(sizeof(node));
	int i, size=0, u;
	
	// Determine the size of the list.
	for(temp_l=db;temp_l!=NULL;temp_l=temp_l->next)
		size++;
	base->next=db;

	switch(mode){
		case 1: // Sort up by year
			for(++size;size>0;size--){
				i=0;
				for(temp=base;(temp->next->next!=NULL)&&(i++<size);temp=temp->next){
					if(temp->next->year_i > temp->next->next->year_i){
						temp_l=temp->next;
						temp->next=temp->next->next;
						temp_l->next=temp->next->next;
						temp->next->next=temp_l;
					}
				}
			}
			break;
		case 2: // Sort up by artist
			for(++size;size>0;size--){
				i=0;
				// Run loop twice to sort alphabetically by first two characters.
				for(u=0;u<2;u++)for(temp=base;(temp->next->next!=NULL)&&(i++<size);temp=temp->next){
					if(mash_case(temp->next->artist)[u] > mash_case(temp->next->next->artist)[u]){
						temp_l=temp->next;
						temp->next=temp->next->next;
						temp_l->next=temp->next->next;
						temp->next->next=temp_l;
					}
				}
			}
			break;
		case 3: // Sort up by album
			for(++size;size>0;size--){
				i=0;
				// Run loop twice to sort alphabetically by first two characters.
				for(u=0;u<2;u++)for(temp=base;(temp->next->next!=NULL)&&(i++<size);temp=temp->next){
					if(mash_case(temp->next->album)[u] > mash_case(temp->next->next->album)[u]){
						temp_l=temp->next;
						temp->next=temp->next->next;
						temp_l->next=temp->next->next;
						temp->next->next=temp_l;
					}
				}
			}
			break;
		default:
			// Can't happen.
			printf("No default method for sort.");
	}

	temp_l=base->next;
	free(base);

	return temp_l;
}

// Sanitize string (protecting original string)
char *sanitize(char *buffer){
	int i;
	char*buf=calloc(MAX_L, sizeof(char));

	strcpy(buf, buffer);
	for(i=0;buf[i]!='\n'&&buf[i]!='\0'&&buf[i]!='\r';i++)switch(buf[i]){
		case '_':
			buf[i]='\\';
			break;
		case ' ':
			buf[i]='_';
			break;
	}
	buf[i]='\0';
	return buf;
}

// Sanitizes strings and writes to the given file.
void save_to(list db, FILE *out){
	node *current;
	char *buffer;

	for(current=db;current!=NULL;current=current->next){
		buffer=sanitize(current->artist);
		fputs(buffer, out);
		fputc(' ', out);
		free(buffer);

		buffer=sanitize(current->album);
		fputs(buffer, out);
		fputc(' ', out);
		free(buffer);

		buffer=sanitize(current->year);
		fputs(buffer, out);
		fputc('\n', out);
		free(buffer);
	}
	printf("Ok.\n");
}

// Returns all lower-case. (protects original string)
char *mash_case(char *source){
	int i;
	char *value;
	for(i=0;source[i]!='\0';i++);
	value=calloc(++i, sizeof(char));
	for(i=0;source[i]!='\0';i++)
		if(source[i]>0x40 && source[i]<0x5B)
			value[i]=source[i]+0x20;
		else value[i]=source[i];
	return value;
}

int search(char *src, char *tgt){
	// Searches for a string in each element of the list.
	// If found, returns location of first character, else -1
	char *source, *target;
	int i, p, len;

	// Case insensitive
	source=mash_case(src);
	target=mash_case(tgt);


	// If source is shorter than target, target can't possibly be contained in source.
	for(len=0;target[len]!='\0';len++);
	for(p=0;source[p]!='\0';p++);
	if(p<len)
		return -1;

	len--;

	for(i=0;source[i+len]!='\0';i++){
		for(p=0;source[i+p]==target[p];p++){
			if(p==len)
				return i;
		}
	}

	return -1;
}

// Removes an element at position pos from list db.
// Will remove the last element if pos is out of bounds.
list rm(list db, int pos){
	node *prev, *free_prev, *temp;
	free_prev=prev=malloc(sizeof(node));

	prev->next=db;

	for(temp=db;pos--&&temp!=NULL;temp=temp->next)
		prev=prev->next;

	prev->next=temp->next;
	free(temp);
	temp=free_prev->next;
	free(free_prev);

	printf("Removed.\n");
	return temp;
}
