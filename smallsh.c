/*
 * Mico Santiago
 * Oct 24, 2022
 * CS 344 Operating Systems
 * Oregon State University
 * 
 * How to compile:
 * $ gcc -std=c99 -Wall -Wextra -Wpedantic -Werror -o smallsh smallsh.c 
 * Using makefile:
 * $ make all
 *
 * Statement: The idea of the small shell is simple. Create an imitation of
 *            the shell common to most (if not all) operating systems. The
 *            command line has the least resistence to communicating to the
 *            computer because of the lack of a GUI, however, it is also
 *            highly customizable because of the usage of the C language.
 *            This assignment is not only an exercise with the OS but also
 *            an exercise on the pros and cons of using the C language. This
 *            assignment introduces us to the real world application where C
 *            fails at: parsing strings. Hense one of the reasons C++ was
 *            created. However, C is a structured language with few commands,
 *            giving someone who is experienced with it a lot of flexibility
 *            and greater control of their program, which is especially 
 *            useful for communicating with hardware.
 *
 *            The overall structure of my shell is simple. We take in the
 *            command of the user in the form of a pointer to a string
 *            [char*] then we dynamically expand the string by feeding it
 *            more memory. We take the string and parse the characters
 *            individually i.e. ["c","d"," ","D","I","R"]. The next step
 *            is to tokenize each command because this gives us the
 *            ability to create new features such as built in commands.
 *            In addition, tokenizing will let us use the execv command 
 *            (nicely) which is crutial to giving the user the ability to
 *            use linux built in commands such as "ps", "echo". In the code
 *            you will see this flow: char* -> char** -> exit. With lots of
 *            steps in between. 
 *
 *            Some of the code presented here is inspired by other sources.
 *            These sources can be found under the function comments. The 
 *            pseudocode of the entire program was inspired by Stephan
 *            Brennan in the article titled "Tutorial - Write a Shell in C" 
 *            [ https://brennan.io/2015/01/16/write-a-shell-in-c/ ]. I took
 *            the overall pseudo code of Initializing (anything, in my case
 *            this would be the signal portion of the assignment) -> 
 *            Interpret (the user input) -> Terminate (although free() was
 *            causing issues with the testscript). 
 *
 *            TODO: 
 *              Remove globals
 *              Seperate this program into multiple parts
 *                Make sure functions are only one page long MAX
 *              Format and create more readable code.
 *              Error Checking
 *
 */
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
/* 1 includes */
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
/*2. defines */
#define max_char 2048
#define max_pro 1000
#define max_args 512
/* 3 external declarations */
/* 4 typedefs */
/* 5 global variable declarations */
bool is_fg = false;
bool bg_switch = false;
int status = 0;
int pid_index=0;
int processes[max_pro];
char infile[max_char];
char outfile[max_char];
/* 6 function prototypes */

/* Program Flow */
void initialize_signals(void);
  void handle_SIGINT(int signo);
  void handle_SIGTSTP(int signo);

void prog_loop(void);
  char* get_line(void);
  char** parse_line(char* line);
  int execute(char** args);
    int builtin_cd(char** args);

/*
 * Function: Main
 * --------------
 * Inspired by Stephan Brennan
 * [ https://brennan.io/2015/01/16/write-a-shell-in-c/ ]
 * Initializes signals (ctrl-z & ctrl-c)
 * Initializes program which loops until it doesn't
 * Initializes exit process
 *  TODO: create exit cleanup
 */
int main(){
  //Initialize
  initialize_signals();
	//Interpret
  prog_loop();
  //Terminate
  return EXIT_SUCCESS;
}
/* 
 * Function: get_line
 * ------------------
 * Inspired from user "Vlad from Moscow" : 
 * [ https://stackoverflow.com/questions/63450383/allocating-memory-to-user-input-using-malloc ]
 * Gets the line entered by user
 * Our strategy is that we have a buffer and dynamically expand it
 */
char* get_line(void){
  char* line = NULL;
  if(max_char!=0){
    line=malloc(sizeof(char));
    size_t index=0;
    // Loop until you exceed max characters, hit EOF and hit newline
    for(int c; index<max_char-1 && (c=getchar())!=EOF && c != '\n'; index++){
      line=realloc(line,index+2);
      if(line==NULL){break;}
      line[index]=c;
    }
    if(line){line[index]='\0';}
  }
  return line;
}
/* 
 * Function: parse_line
 * --------------------
 * Parsing of the string inspired by Stephan Brennan
 * [ https://brennan.io/2015/01/16/write-a-shell-in-c/ ]
 * Variable expansion of $$ inspired by user Gaurang Tandon
 * [ https://stackoverflow.com/questions/779875/what-function-is-to-replace-a-substring-from-a-string-in-c ]
 * 
 * Parses line given from get_line
 * Same strategy with get_line except with null-terminated array of pointers
 */
char** parse_line(char* line){
  // Parsing Initialize
  size_t buffer=64;
  size_t index=0;
  char** tokens=malloc(buffer*(sizeof(char*)));
  char* token;
  // 3. Expansion of Variable $$
  while(strstr(line,"$$")!=NULL){
    char pid_buffer[max_char];
    char *pid_ptr;
    // grab pointer of the location of $$
    pid_ptr=strstr(line,"$$");
    // copy chars from start of input to $$
    strncpy(pid_buffer,line,pid_ptr-line);
    pid_buffer[pid_ptr-line]='\0';
    // copy the other input to the buffer
    sprintf(pid_buffer+(pid_ptr-line),"%d%s",getpid(),pid_ptr+strlen("$$"));
    // copy buffer back into input
    strcpy(line,pid_buffer);
  }
  //7. Executing Commands in Foreground and Background
  //8. SIGTSTP CTRL-Z 
  size_t length = strlen(line);
  if(strcmp(&line[length-1],"&")==0){
    if(is_fg==false){
      bg_switch=true;
      fflush(stdout);
      line[length-1]='\0';
    }
    else if(is_fg==true){
      fflush(stdout);
      line[length-1]='\0';
    }
  }
  token=strtok(line, " ");
  while(token!=NULL){
    tokens[index]=token;
    index++;
    //reallocates the array of pointers until no token is returned by strtok
    if(index>=buffer){
      buffer+=64;
      tokens=realloc(tokens,buffer*sizeof(char*));
    }
    token=strtok(NULL," ");
  }
  tokens[index]=NULL;
  return tokens;
}
/*
 * Function: builtin_cd
 * --------------------
 *
 */
int builtin_cd(char** args){
  char path[max_args];
  getcwd(path,sizeof path);

  if(args[1]==NULL){
    chdir(getenv("HOME"));
  } else {
    strcat(path,"/");
    strcat(path,args[1]);
    chdir(path);
  }//else
   
  getcwd(path,sizeof path);
  return 1;
}
/*
 * Function: execute
 * -----------------
 * Structure of returning 0 or 1 to main program loop
 * so that the user can terminate the program AFTER one run
 * is inspired by Stephan Brennan. However the fork() process 
 * is inspired by OS1 Learning Modules. 
 * TODO:
 *  Functions should not be longer then one page. Seperate to
 *  individual functions.
 */
int execute(char** args){
  //2. A blank line should do nothing.
  if(args[0]==NULL){return 1;}
  //2. # should be ignored
  if(strcmp(args[0],"#")==0){return 1;}
  if(strcmp(args[0],"cd")==0){
    builtin_cd(args);
    return 1;
  }
  //3. Exit Command
  if(strcmp(args[0],"exit")==0){
    if(pid_index==0){
      exit(0);
    }else{
      for(size_t i=0;i< sizeof processes;i++){
        kill(processes[i],SIGTERM);
      }
    }
    return 0;
  }
  //4. Status Command
  if(strcmp(args[0],"status")==0){
    if(WIFSIGNALED(status)){
      printf("exit value %d\n", WTERMSIG(status));
      fflush(stdout);
      return 1;
    }else{
      printf("exit value %d\n", WEXITSTATUS(status));
      fflush(stdout);
      return 1;
    }
  }
  //6. Input & Output Redirection
  bool is_outfile=false;
  bool is_infile=false;
  for(int i=0;args[i]!=NULL;i++){
    if(strcmp(args[i],"<")==0){
     is_infile=true;
     args[i]=NULL;
     strcpy(infile,args[i+1]);
     i++;
    }
    else if(strcmp(args[i],">")==0){
     is_outfile=true;
     args[i]=NULL;
     strcpy(outfile,args[i+1]);
     i++;
    }
  }
  //5. Executing Other Commands
  pid_t cpid;

  cpid= -5;
  cpid=fork();
  
  struct sigaction SIGINT_action = {0};

  switch(cpid){
    case -1:
      //fork() fails
      perror("fork() failed!\n");
      status=1;
      exit(1);
      break;
    case 0:
      //child process
	    SIGINT_action.sa_handler = SIG_DFL;
	    sigaction(SIGINT, &SIGINT_action, NULL);
      //6. Input & Output Redirection
      if(is_outfile==true){
        int currfd=0;
        currfd = open(outfile,O_WRONLY | O_CREAT | O_TRUNC, 0640);
        if(currfd==-1){
           fprintf(stderr, "cannot open %s for input\n", outfile);
           fflush(stdout);
           exit(1);  
        }
        int dest=dup2(currfd,1);
        if(dest==-1){
          perror("dup2\n");
          exit(2);
        }
      }
      if(is_infile==true){
        int currfd=0;
        currfd = open(infile,O_RDONLY, 0640);
        if(currfd==-1){
           fprintf(stderr, "cannot open %s for input\n", infile);
           fflush(stdout);
           exit(1);  
        }
        int dest=dup2(currfd,0);
        if(dest==-1){
          perror("dup2\n");
          exit(2);
        }
      }
      //Error Handling
      if(execvp(args[0],args)==-1){
        perror(args[0]);
      }
      // execute command
      int k=0;
      char* ch=args[k];
      while(strcmp(ch,"\0")!=0){
        ch=args[++k];
      }
      args[k]=NULL;
      execvp(args[0],args);
      perror("");
      printf("\n");
      fflush(stdout);
      exit(1);
      break;
    default:
      //parent process
      if(bg_switch==false){
        waitpid(cpid,&status,0);
      }
      else if(bg_switch==true){
        if(is_fg==false){
          printf("background pid is %d\n",cpid);
          fflush(stdout);
          bg_switch=false;
        }
        else if(is_fg==true){
          waitpid(cpid,&status,0);  
          bg_switch=false;
        }
      } 
  }
  // Wait for the child to finish
	pid_t pid;
  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    printf("Background pid %d is done ", pid);
    fflush(stdout);
    //waitpid(getpid(),&status,0);
    if(WIFSIGNALED(status)){printf("terminated by signal %d\n",WTERMSIG(status));fflush(stdout);}
    if(WIFEXITED(status)){printf("terminated by signal %d\n",WEXITSTATUS(status));fflush(stdout);}
  }
  //executed by both the parent and child
  return 1;
}

/* 
 * Function: prog_loop
 * -------------------
 * line : grabs the user input and returns the first word
 * args : parses line and returns the line as an array of pointers
 * execute : executes builts ins and returns state as an int
 */
void prog_loop(void){
  char* line;
  char** args;
  int state;
  
  do{
    fprintf(stdout,": ");
    fflush(stdout);
    line=get_line();
    args=parse_line(line);
    state=execute(args);
    
  }while(state);
  free(line);
  for(int i=0;i<max_args;i++){free(args[i]);}
  free(args);
}
/*
 * Function: handle_SIGTSTP
 * ------------------------
 * Inspired by Learning Modules
 * Ctrl-c is disabled in main
 */
void handle_SIGINT(int signo){
  (void)(signo);
  return;
}

/*
 * Function: handle_SIGTSTP
 * ------------------------
 * Inspired by Learning Modules
 *  Ctrl-z signal toggles foreground-only mode
 *  ignores &
 */
void handle_SIGTSTP(int signo){
  (void)(signo);
  // enable fg mode
  if(is_fg){
    char* message = ("Exiting foreground-only mode\n");
    write(STDOUT_FILENO, message, 29);
    fflush(stdout);
    is_fg = false;
  // disable fg mode
  }else{
    char* message = ("Entering foreground-only mode (& is now ignored)\n");
    write(STDOUT_FILENO, message, 49);
    fflush(stdout);
    is_fg = true;
  }
}
/* 
 * Function: initialize_signals
 * ----------------------------
 * Inspired by Learning Modules
 * Enables ctrl-c and ctrl-z
 */
void initialize_signals(void){
  // Initialize SIGINT_action struct to be empty
	struct sigaction SIGINT_action = {0}, SIGTSTP_action = {0};

	// Fill out the SIGINT_action struct
	// Register handle_SIGINT as the signal handler
	SIGINT_action.sa_handler = handle_SIGINT;
	// Block all catchable signals while handle_SIGINT is running
	sigfillset(&SIGINT_action.sa_mask);
	// No flags set
	SIGINT_action.sa_flags = 0;
  // Initialize SIGTSTP
  SIGTSTP_action.sa_handler = handle_SIGTSTP;
	sigfillset(&SIGTSTP_action.sa_mask);
	SIGTSTP_action.sa_flags = 0;

	// Install our signal handlers
	sigaction(SIGINT, &SIGINT_action, NULL);
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);

}

