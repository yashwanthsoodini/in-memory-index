#include "stdio.h"
#include <sys/time.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

int avail_order;

FILE *studfile;

typedef struct      /* key - offset pair */
{
    int key;        /* primary key of the record */
    long offset;     /* the record's offset in the student file */
} key_off;

key_off* memindex;   /* in-memory primary key index */
int numrecs;      /* # of records in the student file */

typedef struct      /* availability list entry */
{
    int size;       /* size of the hole */
    long offset;    /* the hole's offset in the student file */
} hole;

hole* avail;      /* in-memory availability list */
int numholes;     /* # of holes in the student file */


/* # of elements of given size in the given file */
int calcNumEls(FILE *file, int elsize){
    long num;
    fseek(file, 0L, SEEK_END);
    num = ftell(file)/elsize;
    rewind(file);
    return num;
}


/* split a record into its constituent fields */
char** splitrec(const char *record, long numchars){
    char **fields = (char**)malloc(4*sizeof(char*));
    int i;
    int j=-1;
    for(i=0;i<4;i++){
        char *field = (char *)calloc(numchars, sizeof(char));
        int k = 0;
        j++;
        while (j<numchars && *(record+j) != '|')
        {
            field[k] = record[j];
            k++;
            j++;
        }
        fields[i] = malloc(sizeof(char)*strlen(field));
        strcpy(fields[i],field);
        free(field);
    }
    return fields;
}

/* split a command into its constituent words */
void split(const char *command, char target[3][256], const char splitby){
    int i=0;
    int j=0;
    int k=0;
    while(1){
        if(command[j]!=splitby){
            target[i][k++] = command[j];
        } else{
            target[i][k++] = '\0';
            k=0;
            i++;
        }
        if (command[j]=='\0')
        {
            break;
        }
        j++;
    }
    return;
}


/* binary search for key in index list */
long binschix(int key, int delete){
    int l = 0;
    int r = numrecs - 1;
    int m;
    while(l <= r){
        m = (int)((l+r)/2);
        if(memindex[m].key < key){
            l = m + 1;
        } else if (memindex[m].key > key)
        {
            r = m - 1;
        } else
        {
            long offset = memindex[m].offset;
            if (delete==1)
            {
                int i;
                for ( i = m; i < numrecs-1; i++)
                {
                    memindex[i] = memindex[i+1];
                }
                numrecs--;
                memindex = (key_off*)realloc(memindex,numrecs * sizeof(key_off));
            }
            return offset;
        }
    }
    return -1;
}


/* binary search for key insertion position in memmemindex list to maintain
   sorted order */
int schixpos(int key){
    if(numrecs==0){
        return 0;
    }
    if(numrecs==1){
        if(memindex[0].key >= key){
            return 0;
        } else{
            return 1;
        }
    }
    int l = 0;
    int r = numrecs - 1;
    int m;
    while(l != r && l<r){
        m = (l+r)/2;
        if(memindex[m].key < key){
            l = m + 1;
        } else
        {
            r = m - 1;
        }
    }
    if(memindex[l].key < key){
        return l+1;
    }
    return l;
}


/* bin search for hole insertion pos in avail list to maintain sorted order */
int schholepos(int size){
    if(numholes==0){
        return 0;
    }
    if(avail_order == 1){
        if(numholes==1){
            if(avail[0].size >= size){
                return 0;
            } else{
                return 1;
            }
        }
        int l = 0;
        int r = numholes - 1;
        int m;
        while(l != r && l<r){
            m = (l+r)/2;
            if(avail[m].size < size){
                l = m + 1;
            } else
            {
                r = m - 1;
            }
        }
        if(avail[l].size < size){
            return l+1;
        }
        return l;
    } else{
        if(numholes==1){
            if(avail[0].size <= size){
                return 0;
            } else{
                return 1;
            }
        }
        int l = 0;
        int r = numholes - 1;
        int m;
        while(l != r && l<r){
            m = (l+r)/2;
            if(avail[m].size < size){
                r = m - 1;
            } else
            {
                l = m + 1;
            }
        }
        if(avail[l].size > size){
            return l+1;
        }
        return l;        
    }
    
}


/* search for hole of given size in the availability list */
long schhole(const int size){
    int i;
    for (i = 0; i < numholes; i++)
    {
        if (avail[i].size >= size)
        {
            long offset = avail[i].offset;
            int hole_size = avail[i].size;
            int j;
            for (j = i; j < numholes - 1; j++)
            {
                avail[j] = avail[j+1];
            }
            if (hole_size > size)
            {
                hole new_hole = {hole_size - size, offset + (long)size};
                avail[numholes - 1] = new_hole;
            } else
            {
                numholes--;
                avail = (hole*)realloc(avail, numholes * sizeof(hole));
            }
            return offset;
        }
    }
    return -1;
}


/* insert a key into the index list */
void insertIndex(int key, const long offset){
    int pos = schixpos(key);
    memindex = (key_off*)realloc(memindex,(numrecs+1)*sizeof(key_off));
    int i;
    for (i = numrecs; i > pos; i--)
    {
        memindex[i] = memindex[i-1];
    }
    key_off new_entry = {key, offset};
    memindex[pos] = new_entry;
    numrecs++;
}

void insertHole(int size, const long offset){
    int pos = schholepos(size);
    avail = (hole*)realloc(avail,(numholes+1)*sizeof(hole));
    int i;
    for(i = numholes; i > pos; i--){
        avail[i] = avail[i-1];
    }
    hole new_hole = {size, offset};
    avail[pos] = new_hole;
    numholes++;
}


/* append a hole into the availability list */
void appendHole(const int size, const long offset){
    numholes++;
    avail = (hole*)realloc(avail,numholes*sizeof(hole));
    hole new_hole = {size,offset};
    avail[numholes-1] = new_hole;
}


/* add given record to the student file */
void add(const char* record){
    int len = strlen(record)-1;
    char fields[256][256];
    split(record,fields,'|');
    int key = strtol(fields[0], NULL, 10 );
    if (binschix(key,0) != -1)
    {
        printf("Record with SID=%d exists\n",key);
        return;
    }

    int fitholeoff = schhole(sizeof(int)+len);

    if (fitholeoff!=-1)
    {
        fseek(studfile, fitholeoff, SEEK_SET);
    }   else
    {
        fseek(studfile, 0L, SEEK_END);
    }

    long offset = ftell(studfile);
    fwrite(&len, sizeof(int), 1,studfile);
    // fseek(studfile, sizeof(int), SEEK_CUR);
    fwrite(record, sizeof(char), len, studfile);
    insertIndex(key,offset);
    return;
}


/* find a record by key */
void find(const char* keystr){
    int key = strtol(keystr, NULL, 10 );
    int offset = binschix(key,0);
    if (offset==-1)
    {
        printf("No record with SID=%d exists\n",key);
        return;
    }
    char *record;
    int recsize;
    fseek(studfile, offset, SEEK_SET);
    fread(&recsize,sizeof(int),1,studfile);
    record = (char*)malloc(recsize+1);
    fread(record,sizeof(char),recsize,studfile);
    record[recsize] = '\0';
    printf("%s\n",record);
    free(record);
    return;
}


/* del a record by key */
void del(const char* keystr){
    int key = strtol(keystr, NULL, 10);
    int offset = binschix(key,1);
    if (offset==-1)
    {
        printf("No record with SID=%d exists\n",key);
        return; 
    }
    int recsize;
    fseek(studfile, offset, SEEK_SET);
    fread(&recsize,sizeof(int),1,studfile);
    if(avail_order == 0){
        appendHole(recsize + sizeof(int), offset);
    } else{
        insertHole(recsize + sizeof(int), offset);
    }
}


int main(int argc, char const *argv[])
{
    if(strcmp(argv[1],"--first-fit")==0){
        avail_order = 0;
    } else if(strcmp(argv[1],"--best-fit")==0){
        avail_order = 1;
    } else{
        avail_order = -1;
    }

    char filename[64];
    strcpy(filename, argv[2]);
    filename[strlen(filename)-3] = '\0';
    strcat(filename, argv[1]);
    strcat(filename, ".db");

    FILE *indexfile;
    char indexfilename[64];
    strcpy(indexfilename,filename);
    indexfilename[strlen(indexfilename)-3] = '\0';
    strcat(indexfilename,"_index.bin");

    FILE *availfile;
    char availfilename[64];
    strcpy(availfilename,filename);
    availfilename[strlen(availfilename)-3] = '\0';
    strcat(availfilename,"_avail.bin");

    /* if the student file with the specified name does not exist, create a new one */
    if ((studfile = fopen(filename,"rb+"))==NULL)
    {
        studfile = fopen(filename,"wb+");
        indexfile = fopen(indexfilename,"wb+");
        availfile = fopen(availfilename,"wb+");
        numrecs = 0;
        numholes = 0;
    } else {
        indexfile = fopen(indexfilename,"rb+");
        availfile = fopen(availfilename,"rb+");
        numrecs = calcNumEls(indexfile,sizeof(key_off));
        numholes = calcNumEls(availfile,sizeof(hole));

        memindex = (key_off*)realloc(memindex,numrecs * sizeof(key_off));
        fseek(indexfile, 0L, SEEK_SET);
        fread(memindex,sizeof(key_off),numrecs,indexfile);

        avail = (hole*)realloc(avail,numholes * sizeof(hole));
        fseek(availfile, 0L, SEEK_SET);
        fread(avail,sizeof(hole),numholes,availfile);
    }
    
    char input[1000];
    fgets(input,1000,stdin);

    while(input[1]!='n'){
        char comm[3][256];
        split(input,comm,' ');
        if(strcmp(comm[0],"add")==0){
            add(comm[2]);
        } else if(strcmp(comm[0],"find")==0){
            find(comm[1]);
        } else if(strcmp(comm[0],"del")==0){
            del(comm[1]);
        } else{
            printf("\n Invalid command \n");
        }
        fgets(input,1000,stdin);
    }

    int i;

    printf("Index:\n");
    
    for(i=0; i<numrecs; i++){
        printf( "key=%d: offset=%ld\n", memindex[i].key, memindex[i].offset );
    }

    printf("Availability:\n");

    int totholesize = 0;
    for(i=0; i<numholes; i++){
        printf( "size=%d: offset=%ld\n", avail[i].size, avail[i].offset );
        totholesize = totholesize + avail[i].size;
    }

    printf( "Number of holes: %d\n", numholes );

    printf( "Hole space: %d\n", totholesize );


    fseek(indexfile,0L,SEEK_SET);
    fwrite(memindex,sizeof(key_off),numrecs,indexfile);
    fseek(availfile,0L,SEEK_SET);
    fwrite(avail,sizeof(hole),numholes,availfile);

    fclose(studfile);
    fclose(indexfile);
    fclose(availfile);

    return 0;
}
