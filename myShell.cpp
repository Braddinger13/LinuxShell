#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <list>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/wait.h>

using namespace std;

//struct to be used when looking for and setting context switches
struct commContext{
    char* inputFile;
    char* outputFile;
    bool hasInput;
    bool hasTrunc;
    bool hasCat;
    bool hasBackground;
    bool hasPipe;
}context;

void cd(char* args[], int argc){

    //checks whether you want show current directory or a specified directory with arg count
    if(argc == 2){ //prints current directory
        char buff[1024];
        size_t len = (sizeof(buff)/sizeof(buff[0])) - 1;

        getcwd(buff, len); //current working directory copied into buffer

        cout<<"cwd="<<buff<<endl;

    }else if(argc == 3){
        if(chdir(args[1]) != 0){ //looks for directory passed
            cerr<<"Couldn't find directory"<<endl; //couldn't find directory
        }
    }else{
        cerr<<"Incorrect cd arguments"<<endl; //incorrect arguments
    }
}

void dir(char* args[], int argc){
    DIR* useDirec = NULL;

    if(context.hasInput == true){
        cout<<"dir does not take input files."<<endl;
        return;
    }

    if(argc==2){ //if no arguments passed, use current directory
        useDirec=opendir(".");
    }
    else if(argc>=3){ //opens directory passed
        
        useDirec = opendir(args[1]); //opens directory specified for output
    }
    else{ //error check
        cerr<<"Incorrect dir arguments"<<endl;
        return;

    }


    struct dirent* dirRec = NULL;

    if(useDirec==NULL){ //checks if directory exists
        cerr<<"Error: couldn't open directory"<<endl;
    }else{
        while((dirRec = readdir(useDirec))){ //reads directory contents
            if(dirRec!=NULL){
                cout<<dirRec->d_name<<endl; //output
            }
            else{
                cerr<<"Error reading directory records"<<endl;
            }
        }
    }
    closedir(useDirec); //closed directory used

}

void environment(char* args[], int argc, char **envp){
    if(argc >= 2){ 

        for(char **env = envp; *env!=0; env++){ //loops through and prints all environment variables
            char *currEnv = *env;
            cout<<currEnv<<endl; 
        }

        char buff[1024];
        ssize_t len = ::readlink("/proc/self/exe", buff, sizeof(buff)-1); //path to shell executable
        if(len != -1){
            buff[len] = '\0';
            cout<<"Shell="<<buff<<endl; //print shell path at end of all env variables
        }else{
            cerr<<"Error"<<endl;
        }

    }else{
        cerr<<"error"<<endl;
    }
}

void help(char* args[], int argc){

    if(argc>=2){
        FILE *fp = fopen("readme_doc", "r"); //open file
        if(fp==NULL){
            cout<<"Could not open file."<<endl;
        }else{//outputs everthing in the file README.txt
            char buffer[sizeof(fp)];
            while(fgets(buffer, sizeof(fp), fp)){
                cout<<buffer;
            }
            cout<<endl;
            fclose(fp); //close file
        }
    }else{
        cerr<<"error"<<endl;
    }
}

void clr(char* args[]){ //Clears terminal screen
    cout << "\033[H\033[2J";
}

void quit(char* args[]){ //quits shell
    exit(1);
}

char pause(char* args[], int argc){
    return cin.get(); //waits for the '\n' character returns
}

void echo(char* args[], int argc){
    if(argc==2){ //empty line
        cout<<endl;
    }else if(argc>=3){
        if(context.hasInput == true){ //checks for input file
            FILE *fp=fopen(context.inputFile, "r"); //open input file
            if(fp==NULL){ //error opening check
                cout<<"Couldn't open file"<<endl;
            }else{
                char buffer[sizeof(fp)]; //Sets buffer size to file size
                while(fgets(buffer, sizeof(fp), fp)){ //fgets every string in the file and prints it out
                    printf("%s", buffer);
                }
                fclose(fp); //Closes the file when done with it
            }

        }
        else{ //no input file
            for(int i=1; i<argc-1; i++){
                if(args[i]!=NULL && args[i] != context.outputFile){
                    cout<<args[i]<<" "; //output
                    }    
                }
            }
            cout<<endl;
        }
    else{
        cerr<<"Error has occured"<<endl;        
    }

}

void parser(list<char*> parsingList, char **env){

    // context truct init
    context.hasInput=false;
    context.hasTrunc=false;
    context.hasCat=false;
    context.hasBackground=false;
    context.hasPipe=false;
    
    list <char*> :: iterator it;

    //take out bash commands and decide their context.
    int contextCount=0;
    int kcount=0; //keeps count used for piping arrays
    int kSpot;
    for(it=parsingList.begin(); it!=parsingList.end(); it++){
        string k = *it;
        if(k == "<"){
            context.hasInput = true;
            it++;
            context.inputFile = *it; //next token after "<"
            contextCount++;
        }else if(k == ">"){
            context.hasTrunc = true;
            it++;
            context.outputFile = *it; //next token after ">"
            contextCount++;
        }else if(k == ">>"){
            context.hasCat = true; //next token after ">>"
            it++;
            context.outputFile = *it;
            contextCount++;
        }else if(k == "&"){
            context.hasBackground = true;
            contextCount++;
        }else if(k == "|"){
            context.hasPipe = true;
            kSpot = kcount;
            contextCount++;
        }
        kcount++;
    }

    //bach command error checking
    if(context.hasInput || context.hasTrunc || context.hasCat){
        if(context.hasBackground || context.hasPipe){
            cerr<<"Error with bash arguments"<<endl;
            return;
        }
    }
    if(context.hasPipe){
        if(context.hasInput || context.hasTrunc || context.hasCat || context.hasBackground){
            cerr<<"Error with bash arguments"<<endl;
            return;
        }
    }
    if(context.hasBackground){
        if(context.hasInput || context.hasTrunc || context.hasCat || context.hasPipe){
            cerr<<"Error with bash arguments"<<endl;
            return;
        }
    }

    

    //creating argument array
    char* args[parsingList.size()-contextCount+1];
    
    int count=0;
    //putting arguments and commands into array for execution
    for(it=parsingList.begin(); it!=parsingList.end(); it++){
        string j = *it;
        if(j != ">" && j != ">>" && j != "<" && j != "&" && j != "|"){
            args[count] = *it;
            count++;
        }
    }
        
    args[(parsingList.size() - contextCount)] = NULL; //makes array null terminating
    int argc = (sizeof(args)/sizeof(args[0]));

    char* readArgs[kSpot+1];
    char* writeArgs[argc-kSpot];
    if(context.hasPipe){//creates read/write arrays for piping
        

        //creates read array for pipe
        int readCount=0;
        for(it=parsingList.begin(); it!=parsingList.end(); it++){
            string j = *it;

            if(j=="|"){
                break;
            }
            readArgs[readCount] = *it;
            readCount++;
        }
        
        readArgs[kSpot] = NULL;
        //creates write array for pipe
        int writeCount=0;
        for(int i=kSpot; i<argc-1; i++){
            writeArgs[writeCount] = args[i];
            writeCount++;
        }
        writeArgs[argc-kSpot-1] = NULL;
    }

    args[(parsingList.size() - contextCount)] = NULL; //makes array null terminating
    string command = args[0]; //sets a string to be the first command passed

    //checks to make sure a bash command is not in the command position
    if(command=="< " || command==">" || command==">>" || command=="|" || command=="&"){
        cerr<<"Not a proper command"<<endl;
        return;
    }
    
    

    //file descriptor save variables
    int saved_stdout = dup(1);
    int saved_stdin = dup(0);
    int fileDescIn;
    int fileDescOut;

    if(context.hasInput){ //checks context
        //checks to see if files exists and opens if so
        if((fileDescIn = open(context.inputFile, O_RDONLY, S_IRWXU | S_IRWXG | S_IRWXO)) == -1){
            cerr<<"Error opening file for reading"<<endl;
        }else{
            if((dup2(fileDescIn, 0)) == -1){ //dups file descriptors to redirect input
                cerr<<"Error duplicating file"<<endl;
            }
        }    
        close(fileDescIn); //closes file descriptor
    }
    if(context.hasTrunc){//checks context
        //create or overwrite output

        //checks if file exists and opens it if so
        if((fileDescOut = open(context.outputFile, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO)) == -1){
            cerr<<"Error opening file for writing"<<endl;
        }else{
            if((dup2(fileDescOut, 1)) == -1){//dups file descriptors to redirect output in truncating mode
                cerr<<"Error duplicating file"<<endl;
            }
        }    
        close(fileDescOut); 
    }
    if(context.hasCat){ //checks context
        //create or concatenate output
        if((fileDescOut = open(context.outputFile, O_WRONLY | O_CREAT | O_APPEND, S_IRWXU | S_IRWXG | S_IRWXO)) == -1){
            cerr<<"Error opening file for writing"<<endl;
        }else{
            if((dup2(fileDescOut, 1)) == -1){ //dups file descriptors to redirect output in append mode
                cerr<<"Error duplicating file"<<endl;
            }
        }
        close(fileDescOut);
    }

    if(context.hasPipe){ //checks if pipe was passed
        int pipefd[2];
        pid_t pid;
        if((pipe(pipefd)) == -1){ //initiates pipe
            cerr<<"Error piping"<<endl;
        }

        pid = fork(); //forks process

        if(pid==0){ //enters read child

            if((dup2(pipefd[1], 1)) == -1){ //copys write side of pipe into stdout
                cerr<<"Error"<<endl;
                exit(1);
            }

            //closes read side of pipe and unused file descriptor
            close(pipefd[0]);
            close(pipefd[1]);

            execvp(readArgs[0], readArgs);

            cerr<<"Error"<<endl; //error checking for execution.
            exit(1);
        }else{
            pid=fork();
            if(pid==0){ //enters write child

                if((dup2(pipefd[0], 0)) == -1){ //copys read side of pipe into stdin
                    cerr<<"Error"<<endl;
                    exit(1);
                }
                //closes write side of pipe and unused file descriptor
                close(pipefd[1]);
                close(pipefd[0]);

                execvp(writeArgs[0], writeArgs);

                cerr<<"Error"<<endl; //error checking for execution
                exit(1);
            }else{
                //waits for child processes to be completed
                int status;
                close(pipefd[0]);
                close(pipefd[1]);
                waitpid(pid, &status, 0);
            }
        }    
        
    }else{
        //checks for a built-in
        if(command == "quit"){
            quit(args);
        }else if(command == "clr"){
            clr(args);
        }else if(command == "help"){
            help(args, argc);
        }else if(command == "echo"){
            echo(args, argc);
        }else if(command == "environ"){
            environment(args, argc, env);
        }else if(command == "pause"){
            while(pause(args, argc) != '\n'){

            }
        }else if(command == "dir"){
            dir(args, argc);
        }else if(command == "cd"){
            cd(args, argc);
        }else{ //check to see if command exists in path

            pid_t pid = fork();
            if(pid<0){
                cout<<"error forking"<<endl;
            }else if(pid == 0){//child process
                if(execvp(args[0], args) == -1){ // calls the command and checks if it exists in path
                    cout<<"Error executing command"<<endl;
                }
            }else{//parent
                int status;
                if(context.hasBackground == false){
                    if(waitpid(pid, &status, 0) == -1){
                        cerr<<"Wait error"<<endl;
                    }
                }else{
                    cout<<"PID: "<<pid<<endl;
                }
            }
        }

    }

    //restore file descriptors
    dup2(saved_stdout, 1);
    dup2(saved_stdin, 0);
    close(saved_stdout);
    close(saved_stdin);

}


int main(int argc, char **argv, char **envp){
        
        //for mode switching
        bool isBatch;

        //switches to batch mode if myshell has a batch argument
        if(argc==1){
            isBatch = false;
        }else if(argc==2){
            isBatch = true;
        }else{
            cout<<"Incorrect arguments."<<endl;
            exit(1);
        }

        //initialize for interactive mode
        FILE *fp = stdin;
        if (isBatch == true){
            fp = fopen(argv[1], "r");
        }

        while(true){
            
            if (isBatch == false){
                cout << "myshell> ";
            }

            char *line = NULL;
            size_t len = 0;
            ssize_t read;

            //Creates list for tokens
            list <char*> parsingList;

            //reading lines from input
            while ((read = getline(&line, &len, fp)) != -1){
                //tokenization
                const char delim[6] = " \t\n";
                char *token;
                token = strtok(line, delim);
                while(token != NULL){
                    parsingList.push_back(token); //pushing tokens into list
                    token = strtok(NULL, delim);
                }
                break;
            }

            parser(parsingList, envp); //send to parser
            free(line);
        }
}