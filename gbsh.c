#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <pwd.h>
#include <limits.h>
#include <string.h>
#include <dirent.h>
#include <sys/dir.h>
#include <sys/param.h>
#include <fcntl.h>
#include <signal.h>

//! some utility and helper functions
extern int alphasort(); //Inbuilt sorting function

void die(char *msg)
{
	perror(msg);
	//exit(0);
}

int file_select(struct direct *entry)
{
	if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0))
		return (0);
	else
		return (1);
}
char hostname[HOST_NAME_MAX];
char username[LOGIN_NAME_MAX];
// For shell Info
int printShellInfo()
{

	int result;
	// !getting cwd
	long size;
	char *buf;
	char *ptr;
	size = pathconf(".", _PC_PATH_MAX);

	if ((buf = (char *)malloc((size_t)size)) != NULL)
		ptr = getcwd(buf, (size_t)size);
	result = gethostname(hostname, HOST_NAME_MAX);
	if (result)
	{
		perror("gethostname");
		return EXIT_FAILURE;
	}
	result = getlogin_r(username, LOGIN_NAME_MAX);
	if (result)
	{
		perror("getlogin_r");
		return EXIT_FAILURE;
	}
	printf("\033[1;36m"); //set colours
	result = printf("%s@%s:",
					username, hostname);
	printf("\033[0m"); //reset colors
	if (result < 0)
	{
		perror("printf");
		return EXIT_FAILURE;
	}
	printf("\033[0;32m"); //set colors
	result = printf("%s ", ptr);
	printf("\033[0m"); //reset colors
	printf("> ");
	if (result < 0)
	{
		perror("printf");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
// implementing sigint property
void sigint_handler(int signum)
{ //Handler for SIGINT
	//Reset handler to catch SIGINT next time.
	printf("\n");
	printShellInfo();
	signal(SIGINT, sigint_handler);
	fflush(stdout);
	//printShellInfo();
}
struct command
{
	char **argv;
};
void utility_proc(int in, int out, struct command *cmd)
{
	if (!fork())
	{
		if (in != 0)
		{
			dup2(in, 0);
			close(in);
		}

		if (out != 1)
		{
			dup2(out, 1);
			close(out);
		}

		execvp(cmd->argv[0], (char *)cmd->argv);
		exit(0);
	}
	else
	{
		wait(NULL);
	}
}
int fork_pipes(int n, struct command *cmd)
{
	int i;
	pid_t pid;
	int in, fd[2];
	/* The first process should get its input from the original file descriptor 0.  */
	in = 0;

	/* Note the loop bound, we spawn here all, but the last stage of the pipeline.  */
	for (i = 0; i < n - 1; i++)
	{
		pipe(fd);

		/* f [1] is the write end of the pipe, we carry `in` from the prev iteration.  */

		utility_proc(in, fd[1], cmd + i);

		/* No need for the write end of the pipe, the child will write here.  */
		close(fd[1]);

		/* Keep the read end of the pipe, the next child will read from there.  */
		in = fd[0];
	}
	/* Last stage of the pipeline - set stdin be the read end of the previous pipe
     and output to the original file descriptor 1. */
	if (in != 0)
		dup2(in, 0);
	return i;
}
int main(int argc, char *argv[], char **envp)
{
	char val[40];
	char *token[40]; // list of cmds on terminal
	int tok = 0;
	if (!fork())
	{
		execlp("clear", "clear", NULL);
		printf("command failed\n");
		exit(0);
	}
	else
	{
		wait(NULL);
	}

	// setting shell env
	long size;
	char *buf;
	char *ptr;

	size = pathconf(".", _PC_PATH_MAX);

	if ((buf = (char *)malloc((size_t)size)) != NULL)
		ptr = getcwd(buf, (size_t)size);
	// setting shell default env variable
	setenv("shell", ptr, 0);
	printf("Welcome to my Linux terminal\n");
	printShellInfo();
	signal(SIGINT, sigint_handler);
	while (1)
	{
		scanf("%[^\n]%*c", val);
		int pipeCount = 0;
		for (int i = 0; val[i] != '\0'; i++)
		{
			if (val[i] == '|')
			{
				pipeCount++;
			}
		}
		// redirection for pipes
		if (pipeCount > 0)
		{
			// redirecting pipe output to last entered file
			// if | > used together
			char *anotherToken[20];
			int anothertok = 0;
			anotherToken[anothertok] = strtok(val, ">");
			anothertok++;
			// loop through the string to extract all other tokens
			while (anotherToken[anothertok - 1] != NULL)
			{
				anotherToken[anothertok] = strtok(NULL, ">");
				anothertok++;
			}
			anothertok--;
			for (int i = 0; val[i] != '\0'; i++)
			{
				if (val[i] == '>')
				{
					val[i] = ' ';
				}
			}
			char *pipeToken[20];
			int pipetok = 0;
			pipeToken[pipetok] = strtok(val, "|");
			pipetok++;
			// loop through the string to extract all other tokens
			while (pipeToken[pipetok - 1] != NULL)
			{
				pipeToken[pipetok] = strtok(NULL, "|");
				pipetok++;
			}
			pipetok--;
			//struct command cmd[] = {{ls}, {awk}, {sort}, {uniq}};
			struct command *cmd = (struct command *)malloc(pipetok * sizeof(struct command));
			int *cmdToken[10];
			int cmdtok = 0;
			for (int i = 0; i < pipetok; i++)
			{
				cmdtok = 0;
				cmdToken[cmdtok] = strtok(pipeToken[i], " ");
				cmdtok++;
				// loop through the string to extract all other tokens
				while (cmdToken[cmdtok - 1] != NULL)
				{

					cmdToken[cmdtok] = strtok(NULL, " ");
					cmdtok++;
				}
				cmdtok--;
				cmd[i].argv = (char *)malloc((cmdtok + 1) * sizeof(char));
				for (int j = 0; j < cmdtok; j++)
				{
					cmd[i].argv[j] = cmdToken[j];
				}
				cmd[i].argv[cmdtok] = 0;
			}
			int stdin = dup(0);
			int stdout = dup(1);
			int i = fork_pipes(pipetok, cmd);
			/* Execute the last stage with the current process. */
			if (anothertok > 0)
			{
				creat(anotherToken[1], 0666);
				int fd = open(anotherToken[1], O_WRONLY);
				dup2(fd, 1);
				if (!fork())
				{
					execvp(cmd[i].argv[0], (char *)cmd[i].argv);
					printf("command failed\n");
					exit(0);
				}
				else
				{
					wait(NULL);
				}
			}
			else
			{
				if (!fork())
				{
					execvp(cmd[i].argv[0], (char *)cmd[i].argv);
					printf("command failed\n");
					exit(0);
				}
				else
				{
					wait(NULL);
				}
			}
			dup2(stdin, 0);
			dup2(stdout, 1);
			printShellInfo();
			continue;
		}
		int backgroundProcess = 0;
		for (int i = 0; val[i] != '\0'; i++)
		{
			if (val[i] == '&')
			{
				val[i] = ' ';
				backgroundProcess = 1;
			}
		}
		token[tok] = strtok(val, " ");
		tok++;
		// loop through the string to extract all other tokens
		while (token[tok - 1] != NULL)
		{

			token[tok] = strtok(NULL, " ");
			tok++;
		}
		tok--;

		// !! Different if condtions for checking different commands
		if (strcmp(token[0], "exit") == 0)
		{
			tok = 0;
			printf("\033[1;31m"); //set colors
			printf("	EXITING MY TERMINAL\n");
			printf("\033[0m"); //reset colors
			exit(0);
		}
		if (strcmp(token[0], "pwd") == 0)
		{
			long size;
			char *buf;
			char *ptr;
			int flag = 0;
			char *fileN;
			//! I/O redirection checking ">"
			for (int j = 0; j < tok; j++)
			{
				if (strcmp(token[j], ">") == 0)
				{
					flag = 1;
					fileN = token[j + 1];
					break;
				}
			}
			if (backgroundProcess == 1)
			{
				if (!fork())
				{
					setpgid(0, 0);
					if (flag == 1)
					{
						int stdout = dup(1);
						creat(fileN, 0666);
						int fout = open(fileN, O_WRONLY);
						dup2(fout, 1);
						size = pathconf(".", _PC_PATH_MAX);

						if ((buf = (char *)malloc((size_t)size)) != NULL)
							ptr = getcwd(buf, (size_t)size);
						printf("%s\n", ptr);
						dup2(stdout, 1);
						close(fout);
					}
					else
					{
						size = pathconf(".", _PC_PATH_MAX);

						if ((buf = (char *)malloc((size_t)size)) != NULL)
							ptr = getcwd(buf, (size_t)size);
						printf("%s\n", ptr);
					}
					exit(0);
				}
				else
				{
					tok = 0;
					printShellInfo();
					continue;
				}
			}
			else
			{
				if (flag == 1)
				{
					int stdout = dup(1);
					creat(fileN, 0666);
					int fout = open(fileN, O_WRONLY);
					dup2(fout, 1);
					size = pathconf(".", _PC_PATH_MAX);

					if ((buf = (char *)malloc((size_t)size)) != NULL)
						ptr = getcwd(buf, (size_t)size);
					printf("%s\n", ptr);
					dup2(stdout, 1);
					close(fout);
				}
				else
				{
					size = pathconf(".", _PC_PATH_MAX);

					if ((buf = (char *)malloc((size_t)size)) != NULL)
						ptr = getcwd(buf, (size_t)size);
					printf("%s\n", ptr);
				}
				tok = 0;
				printShellInfo();
				continue;
			}
		}
		if (strcmp(token[0], "clear") == 0)
		{
			if (!fork())
			{
				execlp("clear", "clear", NULL);
				printf("command failed\n");
				exit(0);
			}
			else
			{
				wait(NULL);
				tok = 0;
				printShellInfo();
				continue;
			}
		}
		if (strcmp(token[0], "ls") == 0 && tok < 2)
		{
			// checking for i/o redirection
			// code for ls
			if (backgroundProcess == 1)
			{
				if (!fork())
				{
					setpgid(0, 0);
					int count, i;
					struct direct **files;
					if (tok == 2)
					{
						count = scandir(token[1], &files, file_select, alphasort);

						/* If no files found, make a non-selectable menu item */
						if (count <= 0)
							die("No files in this directory\n");
						else
						{
							for (i = 1; i < count + 1; ++i)
							{
								printf("%s  ", files[i - 1]->d_name);
								printf("\n");
							}
						}
					}
					else if (tok == 1)
					{
						char pathname[MAXPATHLEN];
						if (!getcwd(pathname, sizeof(pathname)))
							die("Error getting pathname\n");

						count = scandir(pathname, &files, file_select, alphasort);

						/* If no files found, make a non-selectable menu item */
						if (count <= 0)
							die("No files in this directory\n");
						else
						{
							for (i = 1; i < count + 1; ++i)
							{
								printf("%s  ", files[i - 1]->d_name);
								printf("\n");
							}
						}
					}
					exit(0);
				}
				else
				{
					tok = 0;
					printShellInfo();
					continue;
				}
			}
			else
			{

				int count, i;
				struct direct **files;
				if (tok == 2)
				{
					count = scandir(token[1], &files, file_select, alphasort);

					/* If no files found, make a non-selectable menu item */
					if (count <= 0)
						die("No files in this directory\n");
					else
					{
						for (i = 1; i < count + 1; ++i)
						{
							printf("%s  ", files[i - 1]->d_name);
							printf("\n");
						}
					}
				}
				else if (tok == 1)
				{
					char pathname[MAXPATHLEN];
					if (!getcwd(pathname, sizeof(pathname)))
						die("Error getting pathname\n");

					count = scandir(pathname, &files, file_select, alphasort);

					/* If no files found, make a non-selectable menu item */
					if (count <= 0)
						die("No files in this directory\n");
					else
					{
						for (i = 1; i < count + 1; ++i)
						{
							printf("%s  ", files[i - 1]->d_name);
							printf("\n");
						}
					}
				}
				tok = 0;
				printShellInfo();
			}
			continue;
		}
		if (strcmp(token[0], "cd") == 0)
		{
			//code for cd
			int count;
			struct direct **files;
			if (tok > 1)
			{
				//printf("Current Working Directory = %s\n", pathname);
				count = scandir(token[1], &files, file_select, alphasort);

				/* If no files found, make a non-selectable menu item */
				if (count <= 0)
					die("No files in this directory\n");
				else
				{
					chdir(token[1]);
				}
			}
			else
			{
				chdir("/home");
				chdir(username);
			}
			tok = 0;
			printShellInfo();
			continue;
		}
		if (strcmp(token[0], "setenv") == 0)
		{
			if (backgroundProcess == 1)
			{
				if (!fork())
				{
					setpgid(0, 0);
					if (tok == 1)
					{
						printf("Environment Variable Name Not Defined\n");
					}
					else
					{
						if (tok == 3)
						{
							setenv(token[1], token[2], 1);
						}
						else if (tok == 2)
						{
							setenv(token[1], "", 1);
						}
					}
					exit(0);
				}
				else
				{
					tok = 0;
					printShellInfo();
					continue;
				}
			}
			else
			{
				if (tok == 1)
				{
					printf("Environment Variable Name Not Defined\n");
				}
				else
				{
					if (tok == 3)
					{
						setenv(token[1], token[2], 1);
					}
					else if (tok == 2)
					{
						setenv(token[1], "", 1);
					}
				}
				tok = 0;
				printShellInfo();
				continue;
			}
		}
		if (strcmp(token[0], "unsetenv") == 0)
		{
			if (backgroundProcess == 1)
			{
				if (!fork())
				{
					setpgid(0, 0);
					if (tok == 2)
					{
						unsetenv(token[1]);
					}
					else if (tok == 1)
					{
						printf("Environment Variable Name Not Defined\n");
					}
					exit(0);
				}
				else
				{
					tok = 0;
					printShellInfo();
					continue;
				}
			}
			else
			{
				if (tok == 2)
				{
					unsetenv(token[1]);
				}
				else if (tok == 1)
				{
					printf("Environment Variable Name Not Defined\n");
				}
				tok = 0;
				printShellInfo();
				continue;
			}
		}
		if (strcmp(token[0], "environ") == 0)
		{
			int flag = 0;
			char *fileN;
			for (int i = 0; i < tok; i++)
			{
				if (strcmp(token[i], ">") == 0)
				{
					flag = 1;
					fileN = token[i + 1];
					break;
				}
			}
			if (backgroundProcess == 1)
			{
				if (!fork())
				{
					if (flag == 0)
					{
						if (!fork())
						{
							setpgid(0, 0);
							execlp("env", "env", NULL);
							printf("command failed\n");
							exit(0);
						}
						else
						{
							wait(NULL);
						}
					}
					else
					{
						int stdout = dup(1);
						creat(fileN, 0666);
						int fout = open(fileN, O_WRONLY);
						dup2(fout, 1);
						if (!fork())
						{
							setpgid(0, 0);
							execlp("env", "env", NULL);
							printf("command failed\n");
							exit(0);
						}
						else
						{
							wait(NULL);
						}
						dup2(stdout, 1);
						close(fout);
					}
					tok = 0;
					printShellInfo();
					exit(0);
				}
				else
				{
					tok = 0;
					printShellInfo();
					continue;
				}
			}
			else
			{
				if (flag == 0)
				{
					if (!fork())
					{
						execlp("env", "env", NULL);
						printf("command failed\n");
						exit(0);
					}
					else
					{
						wait(NULL);
					}
				}
				else
				{
					int stdout = dup(1);
					creat(fileN, 0666);
					int fout = open(fileN, O_WRONLY);
					dup2(fout, 1);
					if (!fork())
					{
						execlp("env", "env", NULL);
						printf("command failed\n");
						exit(0);
					}
					else
					{
						wait(NULL);
					}
					dup2(stdout, 1);
					close(fout);
				}
				tok = 0;
				printShellInfo();
				continue;
			}
		}

		//! Argument Checking for i/o redirection
		// !AREA for d and e
		char *argument[] = {NULL,
							NULL,
							NULL,
							NULL,
							NULL,
							NULL,
							NULL,
							NULL,
							NULL,
							NULL,
							NULL,
							NULL,
							NULL,
							NULL,
							NULL,
							NULL,
							NULL,
							NULL,
							NULL,
							NULL};
		int argCount = 0;
		int flagfor = 0, anotherFlag = 0;
		if (backgroundProcess == 1)
		{
			if (!fork())
			{
				setpgid(0, 0);
				for (int i = 0; i < tok; i++)
				{
					if (strcmp(token[i], ">") != 0 && strcmp(token[i], "<") != 0)
					{
						argument[argCount] = token[i];
						argCount++;
					}
					else
					{
						break;
					}
				}
				for (int i = 0; i < tok; i++)
				{
					if (strcmp(token[i], "<") == 0)
					{
						int stdin = dup(0);

						int track = -1;
						if (creat(token[i + 3], 0666) != -1)
						{
							track = 0;
						}
						int stdout = dup(1);
						int fout;
						if (track == 0)
						{

							fout = open(token[i + 3], O_WRONLY);
							dup2(fout, 1);
						}
						int fin = open(token[i + 1], O_RDONLY);
						dup2(fin, 0);
						if (!fork())
						{
							execvp(argument[0], argument);
							printf("Command Failed\n");
							exit(0);
						}
						else
						{
							wait(NULL);
						}
						dup2(stdin, 0);
						if (track == 0)
						{
							dup2(stdout, 1);
							close(fout);
						}
						close(fin);
						flagfor = 1;
						anotherFlag = 1;
					}
					if (strcmp(token[i], ">") == 0 && flagfor != 1)
					{
						if (creat(token[i + 1], 0666) == -1)
						{
							printf(" Error\n");
						}
						int fout = open(token[i + 1], O_WRONLY);
						int stdout = dup(1);
						dup2(fout, 1);
						if (!fork())
						{
							execvp(argument[0], argument);
							printf("Command Failed\n");
							exit(0);
						}
						else
						{
							wait(NULL);
						}
						dup2(stdout, 1);
						close(fout);
						anotherFlag = 1;
					}
				}
				if (anotherFlag == 0)
				{
					for (int i = 0; i < tok; i++)
					{
						if (!fork())
						{
							if (strcmp(token[i], "top") == 0)
							{
								execlp("top", "top", NULL);
								printf("command failed\n");
								exit(0);
							}
							else if (strcmp(token[i], "ps") == 0)
							{
								execlp("ps", "ps", NULL);
								printf("command failed\n");
								exit(0);
							}
							else if (strcmp(token[i], "man") == 0)
							{
								if (i + 1 < tok)
								{
									execlp("man", "man", token[i + 1], NULL);
									printf("command failed\n");
									exit(0);
								}
								else
								{
									execlp("man", "man", NULL);
									printf("command failed\n");
									exit(0);
								}
							}
							else // Removing all the zombie processes
							{
								exit(0);
							}
						}
						else
						{
							wait(NULL);
						}
					}
				}
				exit(0);
			}
			else
			{
				tok = 0;
				printShellInfo();
			}
		}
		else
		{
			for (int i = 0; i < tok; i++)
			{
				if (strcmp(token[i], ">") != 0 && strcmp(token[i], "<") != 0)
				{
					argument[argCount] = token[i];
					argCount++;
				}
				else
				{
					break;
				}
			}
			for (int i = 0; i < tok; i++)
			{
				if (strcmp(token[i], "<") == 0)
				{
					int stdin = dup(0);
					int track = -1;
					if (creat(token[i + 3], 0666) != -1)
					{
						track = 0;
					}
					int stdout = dup(1);
					int fout;
					if (track == 0)
					{

						fout = open(token[i + 3], O_WRONLY);
						dup2(fout, 1);
					}
					int fin = open(token[i + 1], O_RDONLY);
					dup2(fin, 0);
					if (!fork())
					{
						execvp(argument[0], argument);
						printf("Command Failed\n");
						exit(0);
					}
					else
					{
						wait(NULL);
					}
					dup2(stdin, 0);
					if (track == 0)
					{
						dup2(stdout, 1);
						close(fout);
					}
					close(fin);
					flagfor = 1;
					anotherFlag = 1;
				}
				if (strcmp(token[i], ">") == 0 && flagfor != 1)
				{
					if (creat(token[i + 1], 0666) == -1)
					{
						printf(" Error\n");
					}
					int fout = open(token[i + 1], O_WRONLY);
					int stdout = dup(1);
					dup2(fout, 1);
					if (!fork())
					{
						execvp(argument[0], argument);
						printf("Command Failed\n");
						exit(0);
					}
					else
					{
						wait(NULL);
					}
					dup2(stdout, 1);
					close(fout);
					anotherFlag = 1;
				}
			}
			if (anotherFlag == 0)
			{
				for (int i = 0; i < tok; i++)
				{
					if (!fork())
					{
						if (strcmp(token[i], "top") == 0)
						{
							execlp("top", "top", NULL);
							printf("command failed\n");
							exit(0);
						}
						else if (strcmp(token[i], "ps") == 0)
						{
							execlp("ps", "ps", NULL);
							printf("command failed\n");
							exit(0);
						}
						else if (strcmp(token[i], "man") == 0)
						{
							if (i + 1 < tok)
							{
								execlp("man", "man", token[i + 1], NULL);
								printf("command failed\n");
								exit(0);
							}
							else
							{
								execlp("man", "man", NULL);
								printf("command failed\n");
								exit(0);
							}
						}
						else // Removing all the zombie processes
						{
							exit(0);
						}
					}
					else
					{
						wait(NULL);
					}
				}
			}
			for (int i = 0; i < tok; i++)
			{
				printf("%s\n", token[i]);
			}
			tok = 0;
			printShellInfo();
		}
	}
	exit(0);
}
