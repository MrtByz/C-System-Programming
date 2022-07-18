#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include<stdlib.h>
#include<errno.h>
#include <ctype.h>

#define BUFSZ 5000
#define BUFFER_SIZE 1000

ssize_t readline (char *buf, size_t sz, char *fn, off_t *offset, const char *oldWord, const char *newWord, int is_Head, int is_Tail, int is_Case);
void replaceAll(char *str, const char *oldWord, const char *newWord, int is_Head, int is_Tail, int is_Case);
char **arg_split(char *a_str, const char a_delim, int *counter);
char* removeStar(char line[]);
char** get_strings(char * string);
void fix_star();
int check_if_in_list(char* search, int is_Case);
void destroyArray(char** arr);
void get_star_types(char* test, int index);
int deal_cosecutive(char * str, char regex, int is_Case);
int create_newword(char * str, const char* newWord, int index, int is_Case);
char ** result;
char* tempo;
int* indexes;
struct flock fl = { F_WRLCK, SEEK_SET, 0,       0,     0 };
int fd;
int number_of_variations;


int main (int argc, char **argv) {

    if (argc < 2) return 1;

    if(argv[2] == NULL){
        printf("Missing argument!\n");
        printf("Correct Usage IS:\n");
        printf("./hw1 /str1/str2/ test_file_path.txt\n");
        return 1;
    }

    // char arrays for old and new words
    char oldWord[100], newWord[100];
    // variable to hold how many commands are there in the argument
    int command_count;
    // variable to hold how many tokens are there in the command
    int token_count;
    //int index;
    // 2d char array to hold all the commands and dividing the argument with ';' character
    char ** commands = arg_split(argv[1], ';', &command_count);
    char ** words;
    // char arrays to hold text files path and template file name
    char nameBuff[32];
    //char tempnameBuf[32];
    char file_to_read[200];
    strcpy(file_to_read, argv[2]);

    // for every command in 2d array find the tokens change the words save in a temp file name it as argv[2] so the next iteration can read the last version of the string
    int z = 0;
    for (z=0;z<command_count+1;z++){

        // necessary variables for creating the temporary file
        char line[BUFSZ] = {0};
        off_t offset = 0;
        ssize_t len = 0;
        size_t i = 0;

        // create the temporary file
        int fTemp;
        memset(nameBuff,0,sizeof(nameBuff));
        strncpy(nameBuff,"myTmpFile-XXXXXX",18);
        fTemp = mkstemp(nameBuff);

        // check if the temporary file created properly
        if(fTemp<1)
        {
            printf("\n Creation of temp file failed with error [%s]\n",strerror(errno));
            return 1;
        }
        else
        {
            printf("\n Temporary file created\n");
        }

        // 2d char array to hold all tokens in a command
        words = arg_split(commands[z], '/', &token_count);
        

        // assign first word in words array to oldWord
        strcpy(oldWord, words[0]);
        strcpy(newWord, words[1]);

        // DEFINE THE CONDITIONS
        int is_Head = 0;
        int is_Tail = 0;
        int is_Case = 0;

        char * pos;
        if((pos = strstr(oldWord, "[")) != NULL){
            result = get_strings(oldWord);
        }
        else{
            number_of_variations = 1;
            result = malloc(number_of_variations*sizeof(char*));
            for (i = 0; i < number_of_variations; i++)
                result[i] = (char*)malloc(number_of_variations * sizeof(char));
            strcpy(result[0], oldWord);
        }
        tempo = malloc(number_of_variations*sizeof(char));
        indexes = malloc(number_of_variations*sizeof(int));
        int iter;
        for(iter=0;iter<number_of_variations;iter++)
            removeStar(result[iter]);
        
        fix_star();

        if (words[2] != NULL && strcmp(words[2], "i")==0)
        {
            for(iter=0;iter<number_of_variations;iter++){
                char *up_old =  (char*)malloc(100 * sizeof(char));
                int j=0;
                while (result[iter][j]) {  
                    up_old[j] = tolower(result[iter][j]);
                    j++; 
                }
                strcpy(result[iter], up_old);
                free(up_old);
            }
            
            is_Case =1;
        }

        //
        if(oldWord[0] == '^'){
            is_Head = 1;
        }

        if (oldWord[strlen(oldWord)-1] == '$'){
            is_Tail = 1;
        }
        removeStar(oldWord);
        //printf("WİTHOUT PUNCS: %s\n", oldWord);
        while ((len = readline (line, BUFSZ, file_to_read, &offset, oldWord, newWord, is_Head, is_Tail, is_Case)) != -1){
            printf("LINE AFTER PROCESS: %s\n", line);
            strcat(line, "\n");
            if(-1 == write(fTemp,line,strlen(line)))
            {
                printf("\n write failed with error [%s]\n","5");
                return 1;
            }
        }

        printf("\nSuccessfully replaced all occurrences of '%s' with '%s'.\n", oldWord, newWord);
        
        remove(argv[2]);

        /* Rename temp file as original file */
        rename(nameBuff, argv[2]);
        destroyArray(result);
        number_of_variations = 0;

    }
    int j;
    for(j=0;j<token_count;j++){
        free(words[j]);
    }
    
    for(j=0;j<command_count;j++){
        free(commands[j]);
    }
    for(j=0;j<number_of_variations;j++){
        free(result[j]);
    }
    free(tempo);
    free(indexes);

    
    fl.l_type = F_UNLCK;
    if (fcntl(fd, F_SETLK, &fl) == -1) {
            perror("fcntl");
            exit(1);
        }
    close(fd);

    return 0;
}

void get_star_types(char* test, int index){
    int i,j=0;

    char * temp = malloc(strlen(test)-1*sizeof(char));
    for(i=0;i<index-1;i++){
        temp[j] = test[i];
        j++;
    }
    i += 2;
    for(i=i;i<strlen(test);i++){
        temp[j] = test[i];
        j++;
    }
    // printf("%s\n", temp);
    strcpy(test, temp);
    free(temp);
}

void fix_star(){
    int i = 0;
    int index;
    char* pos;
    
    for(i=0;i<number_of_variations;i++){
        //printf("%s\n", result[i]);
        if((pos = strstr(result[i], "*")) != NULL){
            index = pos - result[i];
            //printf("%d\n", index);
            //printf("HEEEEY %c\n", result[i][index-1]);
            tempo[i] = result[i][index-1];
            indexes[i] = index-1;
            get_star_types(result[i], index);
        }
    }
}

void destroyArray(char** arr)
{
    free(*arr);
    free(arr);
}

int check_if_in_list(char* search, int is_Case){
    int i=0;
    for(i=0;i<number_of_variations;i++){
        if(is_Case == 0){
            if(deal_cosecutive(search, tempo[i], is_Case) == 1){
                return 1;
            }
        }
        else{
            if(deal_cosecutive(search, tempo[i], is_Case) == 1){
                return 1;
            }
        }
    }
    return 0;
}

char** get_strings(char * string){

    char **result;
    
    char *temp = malloc(100*sizeof(char*));
    int i, j=0;;
    int start_index=0;
    int end_index=0;
    for(i=0;i<strlen(string);++i){
        if(string[i] == '[')
            start_index = i;
        else if(string[i] == ']'){
            end_index = i;
            
        }
    }

    number_of_variations = end_index - start_index - 1;
    result = malloc(number_of_variations*sizeof(char*));

    for (i = 0; i < number_of_variations; i++)
        result[i] = (char*)malloc(number_of_variations * sizeof(char));

    if(start_index == 0){
        
        for(j=0;j<number_of_variations;j++){
            temp[0] = string[start_index+j+1];
            for(i=1;i<strlen(string);i++){
                temp[i] = string[end_index+i];
            }
            strcpy(result[j], temp);
        }

    }
    else if(end_index == strlen(string)-1){
        for(j=0;j<number_of_variations;j++){
            for(i=0;i<start_index;i++){
                temp[i] = string[i];
            }
            temp[i] = string[start_index+1+j];
            strcpy(result[j], temp);
        }
    }
    else if(start_index != 0 && end_index != strlen(string)-1){
        for(j=0;j<number_of_variations;j++){
            for(i=0;i<start_index;i++){
                temp[i] = string[i];
            }
            temp[i] = string[start_index+1+j];
            int k = i+1;
            //i++;
            for(i=end_index+1;i<strlen(string);i++){
                temp[k] = string[i];
                k++;
            }
            strcpy(result[j], temp);
        }        
    }

    free(temp);
    return result;

}


ssize_t readline (char *buf, size_t sz, char *fn, off_t *offset, const char *oldWord, const char *newWord, int is_Head, int is_Tail, int is_Case)
{
    
    fl.l_type = F_RDLCK;
    fd = open (fn, O_RDONLY);
    if (fd == -1) {
        fprintf (stderr, "%s() error: file open failed '%s'.\n",
                __func__, fn);
        return -1;
    }
    if (fcntl(fd, F_SETLKW, &fl) == -1) {
            perror("fcntl");
            exit(1);
        }

    ssize_t nchr = 0;
    ssize_t idx = 0;
    char *p = NULL;

    if ((nchr = lseek (fd, *offset, SEEK_SET)) != -1)
        nchr = read (fd, buf, sz);
    //close (fd);

    if (nchr == -1) { 
        fprintf (stderr, "%s() error: read failure in '%s'.\n",
                __func__, fn);
        return nchr;
    }

    if (nchr == 0) return -1;

    p = buf; 
    while (idx < nchr && *p != '\n') p++, idx++;
    *p = 0;

    if (idx == nchr) { 
        *offset += nchr;

        return nchr < (ssize_t)sz ? nchr : 0;
    }

    *offset += idx + 1;
    replaceAll(buf, oldWord, newWord, is_Head, is_Tail, is_Case);
    return idx;
}



void replaceAll(char *str, const char *oldWord, const char *newWord, int is_Head, int is_Tail, int is_Case)
{
    char temp[BUFFER_SIZE];

    printf("LINE BEFORE PROCESS: %s\n", str);

    if (!strcmp(oldWord, newWord)) {
        return;
    }
    int i;
    int num_of_words = 0;
    for(i=0;str[i];i++)  
    {
        if(str[i]==' ')
            num_of_words++;
 
    }
    
    num_of_words = num_of_words+1;

    int position = 0;
    strcpy(temp, str);
    //printf("%s\n", temp);
    char *tok = strtok(temp, " ");
    char *temp_line;
    temp_line = (char*)malloc(1000 * sizeof(char));

    while (tok)
    {
        if(is_Case == 0){
            position++;
            if(check_if_in_list(tok, is_Case) == 1){

                if (is_Head==0 && is_Tail==0)
                {
                    char temp_tok[100];
                    int b = 0;
                    for(b=0;b<number_of_variations;b++){
                        strcpy(temp_tok, tok);
                        create_newword(temp_tok, newWord, b, is_Case);
                        if(strcmp(temp_tok, tok) != 0){

                            break;
                        }
                    }
                    strcat(temp_line, temp_tok);
                    if(position != num_of_words)
                        strcat(temp_line, " ");
                }
                else if(is_Head==1 && position==1){
                    char temp_tok[100];
                    int b = 0;
                    for(b=0;b<number_of_variations;b++){
                        strcpy(temp_tok, tok);
                        create_newword(temp_tok, newWord, b, is_Case);
                        if(strcmp(temp_tok, tok) != 0){
                            break;
                        }
                    }
                    strcat(temp_line, temp_tok);
                    if(position != num_of_words)
                        strcat(temp_line, " ");
                }
                else if(is_Tail==1 && position==num_of_words){
                    char temp_tok[100];
                    int b = 0;
                    for(b=0;b<number_of_variations;b++){
                        strcpy(temp_tok, tok);
                        create_newword(temp_tok, newWord, b, is_Case);
                        if(strcmp(temp_tok, tok) != 0){
                            break;
                        }
                    }
                    strcat(temp_line, temp_tok);
                    if(position != num_of_words)
                        strcat(temp_line, " ");
                }
                else{
                    //printf("GİRDİM ABBE\n");
                    strcat(temp_line, tok);
                    if(position != num_of_words)
                        strcat(temp_line, " ");
                }
            }
            else{
                strcat(temp_line, tok);
                if(position != num_of_words)
                        strcat(temp_line, " ");
            }
            
            tok = strtok(0, " ");
        }
        else{
            position++;
            char *low_old = (char*)malloc(100 * sizeof(char));
            i=0;
            while (oldWord[i]) {  
                low_old[i] = tolower(oldWord[i]);
                i++; 
            }
            char *up_old =  (char*)malloc(100 * sizeof(char));
            i=0;
            while (tok[i]) {  
                up_old[i] = tolower(tok[i]);
                i++; 
            }
            if(check_if_in_list(up_old, is_Case) == 1){
                if (is_Head==0 && is_Tail==0)
                {
                    char temp_tok[100];
                    int b = 0;
                    for(b=0;b<number_of_variations;b++){
                        strcpy(temp_tok, tok);
                        create_newword(temp_tok, newWord, b, is_Case);
                        if(strcmp(temp_tok, tok) != 0){
                            break;
                        }
                    }
                    strcat(temp_line, temp_tok);
                    if(position != num_of_words)
                        strcat(temp_line, " ");
                }
                else if(is_Head==1 && position==1){
                    char temp_tok[100];
                    int b = 0;
                    for(b=0;b<number_of_variations;b++){
                        strcpy(temp_tok, tok);
                        create_newword(temp_tok, newWord, b, is_Case);
                        if(strcmp(temp_tok, tok) != 0){
                            break;
                        }
                    }
                    strcat(temp_line, temp_tok);
                    if(position != num_of_words)
                        strcat(temp_line, " ");
                }
                else if(is_Tail==1 && position==num_of_words){
                    char temp_tok[100];
                    int b = 0;
                    for(b=0;b<number_of_variations;b++){
                        strcpy(temp_tok, tok);
                        create_newword(temp_tok, newWord, b, is_Case);
                        if(strcmp(temp_tok, tok) != 0){
                            break;
                        }
                    }
                    strcat(temp_line, temp_tok);
                    if(position != num_of_words)
                        strcat(temp_line, " ");
                }
                else{
                    strcat(temp_line, tok);
                    if(position != num_of_words)
                        strcat(temp_line, " ");
                }
            }
            else{
                strcat(temp_line, tok);
                if(position != num_of_words)
                        strcat(temp_line, " ");
            }
            
            tok = strtok(0, " ");
        }
    }
    strcpy(str, temp_line);
}


char **arg_split(char *a_str, const char a_delim, int *counter)
{
    char **result = 0;
    size_t count = 0;
    char *tmp = a_str;
    char *last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }
    *counter = (int)count;

    count += last_comma < (a_str + strlen(a_str) - 1);

    count++;

    result = malloc(sizeof(char *) * count);

    if (result)
    {
        size_t idx = 0;
        char *token = strtok(a_str, delim);

        while (token)
        {
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        *(result + idx) = 0;
    }

    return result;
}


char* removeStar(char line[]){
    int i, j;
    for (i = 0; line[i] != '\0'; ++i) {
      while (!(line[i] >= 'a' && line[i] <= 'z') && !(line[i] >= 'A' && line[i] <= 'Z') && !(line[i] == '\0') && !(line[i] >= '0' && line[i] <= '9') && !(line[i] == '.') && !(line[i] == '*')) {
         for (j = i; line[j] != '\0'; ++j) {
            line[j] = line[j + 1];
         }
         line[j] = '\0';
      }
   }
   return line;
}

int deal_cosecutive(char * str, char regex, int is_Case){

    char *test = malloc(strlen(str)*sizeof(char));
    strcpy(test, str);
    int i,j,len,len1;
    len = strlen(test);
    len1=0;
    int flag = 0;

    for(i=0; i<(len-len1);)
    {
        if(test[i] == regex && strlen(str) == 2){
            for(j=i;j<(len-len1);j++){
                //flag = 1;
                test[j]=test[j+1];
            }
            len1++;
            flag = 0;
        }
        else if(test[i] != test[i+1] && i == indexes[i] && test[i] == regex){
            for(j=i;j<(len-len1);j++){
                test[j]=test[j+1];
            }
            len1++;
        }
        else if(test[i]==test[i+1] && test[i] == regex)
        {
            for(j=i;j<(len-len1);j++){
                flag = 1;
                test[j]=test[j+1];
            }
            len1++;
        }
        else if(flag == 1){
            flag = 0;
        }
        else
        {
            i++;
        }
    }
    char *low_old = (char*)malloc(100 * sizeof(char));
    char* pos;
    for(i=0;i<number_of_variations;i++){
        if(is_Case == 1){
            int j=0;
            while (result[i][j]) {  
                low_old[j] = tolower(result[i][j]);
                j++; 
            }
            if((pos = strstr(test, low_old)) != NULL || strcmp(test, low_old) == 0){
                free(low_old);
                return 1;
            }
        }
        else{
            if((pos = strstr(test, result[i])) != NULL || strcmp(test, result[i]) == 0){
                free(low_old);
                return 1;
            }
        }
    }
    free(low_old);
    return 0;
}



int create_newword(char * str, const char* newWord, int index, int is_Case){
    char *test = malloc(strlen(str)*sizeof(char));
    strcpy(test, str);
    int i,len,len1;
    int j=0;
    if(is_Case == 1){
        while (test[j]) {  
            test[j] = tolower(test[j]);
            j++; 
        }
    }

    len = strlen(test);
    len1=0;
    int flag = 0;
    for(i=0; i<(len-len1);)
    {
        if(test[i] == tempo[index] && strlen(str) == 2){
            for(j=i;j<(len-len1);j++){
                test[j]=test[j+1];
            }
            len1++;
            flag = 0;
        }
        else if(test[i] != test[i+1] && i == indexes[i] && test[i] == tempo[index]){
            for(j=i;j<(len-len1);j++){
                test[j]=test[j+1];
            }
            len1++;
        }
        else if(test[i]==test[i+1] && test[i] == tempo[index])
        {
            for(j=i;j<(len-len1);j++){
                flag = 1;
                test[j]=test[j+1];
            }
            len1++;
        }
        else if(flag == 1){
            flag = 0;
        }
        else
        {
            i++;
        }
    }

    char *pos, temp[BUFFER_SIZE];
    char *temp2 = malloc(1000*sizeof(char));
    int indexim = 0;
    int owlen;

    owlen = strlen(result[index]);


    if (!strcmp(result[index], newWord)) {
        return 0;
    }
    if(strlen(result[index]) == 1){
        int j, temp_index=0;
        for(j=0;j<strlen(test);j++){
            if(strlen(test) == 1 && str[0] == result[index][0]){
                strcat(temp2, newWord);
            }
            else if(test[j] == result[index][0]){
                strcat(temp2, newWord);
                temp_index = temp_index + strlen(newWord);
            }
            else{
                temp2[temp_index] = test[j];
                temp_index++;
            }
        }
        strcpy(str, temp2);
        free(temp2);
    }
    else{
        if(strcmp(test, result[index]) == 0){
            strcpy(str, newWord);
        }
        else{
            while ((pos = strstr(test, result[index])) != NULL)
            {

                strcpy(temp, test);

                indexim = pos - test;

                test[indexim] = '\0';

                strcat(test, newWord);

                strcat(test, temp + indexim + owlen);
                strcpy(str, test);
            }
        }
    }

    return 0;
}


