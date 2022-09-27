/* Andy Jiang 118243733 ajiang25 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h> 
#include <sysexits.h> 
#include <err.h> 
#include <sys/stat.h>
#include <unistd.h> 
#include <sys/types.h> 
#include <string.h> 
#include <fcntl.h>
#include "command.h"
#include "executor.h"
#define OPEN_FLAGS (O_WRONLY | O_TRUNC | O_CREAT)
#define DEF_MODE 0664

/* static void print_tree(struct tree *t); */
static int execute_aux(struct tree *t, int parent_input_fd, int parent_output_fd);

int execute(struct tree *t) {
    if (t->conjunction == NONE) {
        /* exit command */
        if (strcmp(t->argv[0], "exit") == 0) {
            exit(0);
        /* change directory command */
        } else if (strcmp(t->argv[0], "cd") == 0) {
            if (t->argv[1] == NULL) {
                getenv("HOME");
            } else {
                chdir(t->argv[1]);
            }
        }
    }

    return execute_aux(t, STDIN_FILENO, STDOUT_FILENO);
    
    /* print_tree(t); */ 

}

static int execute_aux(struct tree *t, int parent_input_fd, int parent_output_fd) {
    int fd, status, and_status, pipe_status, input, output, pipe_fd[2];
    pid_t pid;


    /* if it's a leaf node */
    if (t->conjunction == NONE) {
        /* checks if fork's successful */
        if ((pid = fork()) < 0) {
            perror("fork error");
	    exit(EX_OSERR);
        /* child process begins */
        } else if (pid > 0) {
            wait(&status);
            return status;
	} else if (pid == 0) {
            /* input redirection */
            if (t->input != NULL) {
                if ((input = open(t->input, O_RDONLY)) < 0) {
                    perror("opening input error");
                    exit(EX_OSERR);
		}
                if (dup2(input, 0) < 0) {
                    perror("dup2 error");
		    exit(EX_OSERR);
                }
            } else {
		if (dup2(parent_input_fd, 0) < 0) {
		    perror("dup2 error");
		    exit(EX_OSERR);
		}
	    }
            /* output redirection */
            if (t->output != NULL) {
                if ((output = open(t->output, OPEN_FLAGS, DEF_MODE)) < 0) {
                    perror("opening output error");
		    exit(EX_OSERR);
                }
                if (dup2(output, 1) < 0) {
                    perror("dup2 error");
		    exit(EX_OSERR);
                }
	    } else {
		if (dup2(parent_output_fd, 1) < 0) {
		    perror("dup2 error");
		    exit(EX_OSERR);
		}
	    }
	    if (execvp(t->argv[0], t->argv) < 0) {
                fprintf(stderr, "Failed to execute %s\n", t->argv[0]);
                fflush(stdout);
                exit(EX_OSERR);
            }
	    close(input);
	    close(output);
	}
    } else if (t->conjunction == AND) {
	/* execute left first then go to right */
	and_status = execute(t->left);
	if (and_status != 0) {
	    return and_status;
	} else {
	    fflush(stdout);
	    and_status = execute(t->right);
	    return and_status;
	}
    } else if (t->conjunction == PIPE) {
	if (t->left->output != NULL) {
	    printf("Ambiguous output redirect.\n");
	    fflush(stdout);
	} else if (t->right->input != NULL) {
	    printf("Ambiguous input redirect.\n");
	    fflush(stdout);
	} else {
	    if (t->input != NULL) {
	        if ((input = open(t->input, O_RDONLY)) < 0) {
                   perror("opening input error");
		   exit(EX_OSERR);
                }
	    } else {
		input = parent_input_fd;
	    }
	    if (t->output != NULL) {
               if ((output = open(t->output, OPEN_FLAGS, DEF_MODE)) < 0) {
                   perror("opening output error");
       	           exit(EX_OSERR);
               }
	    } else {
		output = parent_output_fd;
	    }
            if (pipe(pipe_fd) < 0) {
                perror("pipe error");
	        exit(EX_OSERR);
	    }
	    if ((fd = fork()) < 0) {
	        perror("fork error");
	        exit(EX_OSERR);
	    }
	    if (fd > 0) {
		wait(&pipe_status);
		close(pipe_fd[1]);
		if (dup2(pipe_fd[0], 0) < 0) {
                    perror("dup2 error");
                    exit(EX_OSERR);
                }
                execute_aux(t->right, pipe_fd[0], output);
                close(pipe_fd[0]);
	    } else if (fd == 0) {
		close(pipe_fd[0]);
		if (dup2(pipe_fd[1], 1) < 0) {
                    perror("dup2 error");
                    exit(EX_OSERR);
                }

                execute_aux(t->left, input, pipe_fd[1]);
                close(pipe_fd[1]);
	    }
        }
    } else if (t->conjunction == SUBSHELL) {
	if ((pid = fork()) < 0) {
            perror("fork error");
            exit(EX_OSERR);
        /* child process begins */
        } else if (pid == 0) {
	    /* input redirection */
            if (t->input != NULL) {
                if ((input = open(t->input, O_RDONLY)) < 0) {
                    perror("opening input error");
		    exit(EX_OSERR);
                }
            } else {
	        input = parent_input_fd;
	    }
            /* output redirection */
            if (t->output != NULL) {
                if ((output = open(t->output, OPEN_FLAGS, DEF_MODE)) < 0) {
                    perror("opening output error");
                    exit(EX_OSERR);
                }
            } else {
	        output = parent_output_fd;
	    }
	    pipe_status = execute_aux(t->left, input, output);
	    if (close(input) < 0 || close(output) < 0) {
		perror("close error");
		exit(EX_OSERR);
	    }
	} else if (pid > 0){
	    wait(&pipe_status);
	    return pipe_status;
	}
    }
    return 0;
}
/*
static void print_tree(struct tree *t) {
   if (t != NULL) {
      print_tree(t->left);

      if (t->conjunction == NONE) {
         printf("NONE: %s, ", t->argv[0]);
      } else {
         printf("%s, ", conj[t->conjunction]);
      }
      printf("IR: %s, ", t->input);
      printf("OR: %s\n", t->output);

      print_tree(t->right);
   }
}
*/



