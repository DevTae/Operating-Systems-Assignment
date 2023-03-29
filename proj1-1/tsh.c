/*
 * Copyright(c) 2023 All rights reserved by Heekuck Oh.
 * 이 프로그램은 한양대학교 ERICA 컴퓨터학부 학생을 위한 교육용으로 제작되었다.
 * 한양대학교 ERICA 학생이 아닌 이는 프로그램을 수정하거나 배포할 수 없다.
 * 프로그램을 수정할 경우 날짜, 학과, 학번, 이름, 수정 내용을 기록한다.
 */
/*
 * Last Edited by Kim Taehyeon (2023.3.20)
 * Implemented Standard IO Redirection, Pipe features
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE 80             /* 명령어의 최대 길이 */

// 리다이렉션 모드 구분을 위한 enum 선언 (23.3.19)
// cmdexec 함수에서 red_mode 변수를 리다이렉션 모드로 할당하여 사용
// 직관적인 구분을 위한 enum 사용하였음
typedef enum
{
    NONE = 0b00, STDIN = 0b01, STDOUT = 0b10
} redirect_mode;

// 파일 경로와 입출력 방향을 바탕으로 리다이렉션 설정하는 함수 (23.3.19)
// 인자에 포인터(int* red_mode)를 받아옴으로써, 호출함과 동시에 초기화를 시켜줄 수 있음.
static void redirect(char *path, int *red_mode) 
{
    int fd;

    // 만약 설정된 리다이렉션 모드가 입력일 때
    if(*red_mode == STDIN)
    {
	fd = open(path, O_RDONLY); // 파일 읽기 모드로 fd 받아오기
	dup2(fd, STDIN_FILENO); // 표준입력에 fd 복제하기
	*red_mode = NONE; // red_mode 초기화
    }
    // 만약 설정된 리다이렉션 모드가 출력일 때
    else if(*red_mode == STDOUT) {
	fd = open(path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH); // 파일 쓰기 모드로 fd 받아오기
	dup2(fd, STDOUT_FILENO); // 표준출력에 fd 복제하기
	*red_mode = NONE; // red_mode 초기화
    }
}

// 파이프 실행 시에 전 명령어 실행이 완료되었을 때 argv, argc 초기화하는 함수 (23.3.20)
static void clean_argvc(char *argv[], int *argc)
{
    // argv 초기화
    for(int i = 0; i < *argc; i++) {
	argv[i] = NULL;
    }

    // argc 초기화
    *argc = 0;
}

/*
 * cmdexec - 명령어를 파싱해서 실행한다.
 * 스페이스와 탭을 공백문자로 간주하고, 연속된 공백문자는 하나의 공백문자로 축소한다. 
 * 작은 따옴표나 큰 따옴표로 이루어진 문자열을 하나의 인자로 처리한다.
 * 기호 '<' 또는 '>'를 사용하여 표준 입출력을 파일로 바꾸거나,
 * 기호 '|'를 사용하여 파이프 명령을 실행하는 것도 여기에서 처리한다.
 */
static void cmdexec(char *cmd)
{
    char *argv[MAX_LINE/2+1];   /* 명령어 인자를 저장하기 위한 배열 */
    int argc = 0;               /* 인자의 개수 */
    char *p, *q;                /* 명령어를 파싱하기 위한 변수 */
    
    int oldfd_in;		/* 기존 표준입력 fd 백업 변수 (23.3.29) */
    int oldfd_out;		/* 기존 표준출력 fd 백업 변수 (23.3.29) */
    int red_mode = NONE;	/* 리다이렉션 모드 변수 (23.3.19) */

    // 기존 표준 입출력 fd 를 백업한다.  (23.3.19)
    dup2(STDIN_FILENO, oldfd_in);
    dup2(STDOUT_FILENO, oldfd_out);

    /*
     * 명령어 앞부분 공백문자를 제거하고 인자를 하나씩 꺼내서 argv에 차례로 저장한다.
     * 작은 따옴표나 큰 따옴표로 이루어진 문자열을 하나의 인자로 처리한다.
     */
    p = cmd; p += strspn(p, " \t");
    do {
        /*
         * 공백문자, 큰 따옴표, 작은 따옴표가 있는지 검사한다.
         */ 
	// 표준입출력 리다이렉션의 '<' 와 '>', 파이프의 '|' 또한 추가하였다. (23.3.20)
        q = strpbrk(p, " \t\'\"<>|");
        /*
         * 공백문자가 있거나 아무 것도 없으면 공백문자까지 또는 전체를 하나의 인자로 처리한다.
         */
        if (q == NULL || *q == ' ' || *q == '\t') {
            q = strsep(&p, " \t");
	    if (*q)
		// 여기서 반복문을 돌며 q 에는 한 키워드씩 들어간다. (23.3.19)
		// ex) cat "test file.txt" -> 'cat', '\"test file.txt\"'
		// 이 키워드에 대해 구했을 때마다 argv 에 쌓을 것인지 리다이렉션을 진행할 것인지에 대해서만 정하면 된다.
		// 앞으로의 if (*q) 부분이 모두 동일하다. (이하 생략)
		
		// red_mode 가 NONE 이라면
		if (red_mode == NONE)
		    argv[argc++] = q; // 리다이렉션 대상 파일 이름이 아니니 argv 에 집어넣어진다.
		else // red_mode 가 NONE 이 아니라면
		    redirect(q, &red_mode); // 리다이렉션 대상 파일 이름이니 리다이렉션을 진행한다.
        }
	// 표준입력 리다이렉션 (23.3.19)
	else if (*q == '<') {
	    q = strsep(&p, "<");
	    if (*q)
		// red_mode 가 NONE 이라면
		if (red_mode == NONE)
		    argv[argc++] = q;
	        else // red_mode 가 NONE 이 아니라면
		    redirect(q, &red_mode); // 리다이렉션 대상 파일 이름이니 리다이렉션을 진행한다.
		
	    // 해당 '<' 기호가 나온 뒤의 q 가 리다이렉션 대상 파일 이름이기 때문에 red_mode 에 STDIN 을 할당한다. (23.3.19)
	    // 그리고서 다음 while 문에서 q 가 구해지면 리다이렉션을 진행할 수 있다.
	    red_mode = STDIN; // red_mode 에 STDIN 할당
	}
	// 표준출력 리다이렉션 (23.3.19)
	else if (*q == '>') {
	    q = strsep(&p, ">");
	    if (*q)
		// red_mode 가 NONE 이라면
		if (red_mode == NONE)
		    argv[argc++] = q;
	        else // red_mode 가 NONE 이 아니라면
		    redirect(q, &red_mode); // 리다이렉션 대상 파일 이름이니 리다이렉션을 진행한다.

	    // 해당 '>' 기호가 나온 뒤의 q 가 리다이렉션 대상 파일 이름이기 때문에 red_mode 에 STDOUT 를 할당한다. (23.3.19)
	    red_mode = STDOUT; // red_mode 에 STDOUT 할당
	}
	// 파이프 기능 구현 (23.3.20)
	else if (*q == '|') {
	    q = strsep(&p, "|");
	    if (*q)
		// red_mode 가 NONE 이라면
		if (red_mode == NONE)
		    argv[argc++] = q; // argv 에 q 저장
	        else // red_mode 가 NONE 이 아니라면
		    redirect(q, &red_mode); // 리다이렉션 대상 파일 이름이므로 리다이렉션 진행

	    // 파이프 fd 를 저장할 변수 선언 (23.3.20)
	    int pid;
	    int fd[2];
	    // fd[2] 에 해당 프로세스와 해당 프로세스의 자식 프로세스와 통신할 수 있는 파이프의 fd 를 할당함
	    if(pipe(fd) == -1) { // pipe 할당에 실패했을 경우
		fprintf(stderr, "Pipe failed\n");
		return; // 함수 종료
	    }
	    if((pid = fork()) == -1) { // fork() 함수 호출에 실패했을 경우
		fprintf(stderr, "fork() error\n");
		return; // 함수 종료
	    }
	    // 만약 해당 함수의 자식 프로세스라면 (23.3.20)
	    else if(pid == 0) {
		close(fd[STDIN_FILENO]); // 사용하지 않는 fd[0] 닫기
		dup2(fd[STDOUT_FILENO], STDOUT_FILENO); // 해당 자식 프로세스의 표준 출력 파일 디스크립터를 파이프의 fd[1] 로 변경
		break; // 반복문 종료 후 쌓인 argv, argc 를 바탕으로 파이프 기준 왼쪽 명령문 실행
	    }
	    // 만약 해당 함수의 본인이라면 (23.3.20)
	    else if(pid > 0) {
		wait(NULL); // 자식 프로세스가 종료되길 기다린다.
		close(fd[STDOUT_FILENO]); // 사용하지 않는 fd[1] 닫기
		dup2(fd[STDIN_FILENO], STDIN_FILENO); // 해당 본인 프로세스의 표준 입력 파일 디스크립터를 파이프의 fd[0] 로 변경
		clean_argvc(argv, &argc); // 이미 저장되어 있는 argv, argc 를 바탕으로 실행되었기 때문에 지워준다.
		continue; // 이어서 p 를 탐색한다.
	    }
	}
        /*
         * 작은 따옴표가 있으면 그 위치까지 하나의 인자로 처리하고, 
         * 작은 따옴표 위치에서 두 번째 작은 따옴표 위치까지 다음 인자로 처리한다.
         * 두 번째 작은 따옴표가 없으면 나머지 전체를 인자로 처리한다.
         */
        else if (*q == '\'') {
            q = strsep(&p, "\'");
            if (*q)
		// red_mode 가 NONE 이라면 (23.3.19)
		if (red_mode == NONE)
		    argv[argc++] = q; // 리다이렉션 대상 파일이 아니므로 argv 데이터에 추가
	        else // red_mode 가 NONE 이 아니라면
		    redirect(q, &red_mode); // 리다이렉션 대상 파일 이름이므로 리다이렉션 함수 호출
            q = strsep(&p, "\'");
            if (*q)
		// red_mode 가 NONE 이라면 (23.3.19)
		if (red_mode == NONE)
		    argv[argc++] = q; // 리다이렉션 대상 파일이 아니므로 argv 데이터에 추가
	        else // red_mode 가 NONE 이 아니라면
		    redirect(q, &red_mode); // 리다이렉션 대상 파일 이름이므로 리다이렉션 함수 호출
        }
        /*
         * 큰 따옴표가 있으면 그 위치까지 하나의 인자로 처리하고, 
         * 큰 따옴표 위치에서 두 번째 큰 따옴표 위치까지 다음 인자로 처리한다.
         * 두 번째 큰 따옴표가 없으면 나머지 전체를 인자로 처리한다.
         */
        else {
            q = strsep(&p, "\"");
            if (*q) 
		// red_mode 가 NONE 이라면 (23.3.19)
		if (red_mode == NONE)
		    argv[argc++] = q; // 리다이렉션 대상 파일이 아니므로 argv 데이터에 추가
	        else // red_mode 가 NONE 이 아니라면
		    redirect(q, &red_mode); // 리다이렉션 대상 파일 이름이므로 리다이렉션 함수 호출
            q = strsep(&p, "\"");
            if (*q)
		// red_mode 가 NONE 이라면 (23.3.19)
		if (red_mode == NONE)
		    argv[argc++] = q; // 리다이렉션 대상 파일이 아니므로 argv 데이터에 추가
	        else // red_mode 가 NONE 이 아니라면
		    redirect(q, &red_mode); // 리다이렉션 대상 파일 이름이므로 리다이렉션 함수 호출
        }        
    } while (p);
    argv[argc] = NULL;
    /*
     * argv에 저장된 명령어를 실행한다.
     */
    if (argc > 0) {
        execvp(argv[0], argv);
	dup2(oldfd_in, STDIN_FILENO); // 기존 표준입력 fd 복구 (23.3.19)
	dup2(oldfd_out, STDOUT_FILENO); // 기존 표준출력 fd 복구 (23.3.19)
    }
}

/*
 * 기능이 간단한 유닉스 셸인 tsh (tiny shell)의 메인 함수이다.
 * tsh은 프로세스 생성과 파이프를 통한 프로세스간 통신을 학습하기 위한 것으로
 * 백그라운드 실행, 파이프 명령, 표준 입출력 리다이렉션 일부만 지원한다.
 */
int main(void)
{
    char cmd[MAX_LINE+1];       /* 명령어를 저장하기 위한 버퍼 */
    int len;                    /* 입력된 명령어의 길이 */
    pid_t pid;                  /* 자식 프로세스 아이디 */
    int background;             /* 백그라운드 실행 유무 */
    
    /*
     * 종료 명령인 "exit"이 입력될 때까지 루프를 무한 반복한다.
     */
    while (true) {
        /*
         * 좀비 (자식)프로세스가 있으면 제거한다.
         */
        pid = waitpid(-1, NULL, WNOHANG);
        if (pid > 0)
            printf("[%d] + done\n", pid);
        /*
         * 셸 프롬프트를 출력한다. 지연 출력을 방지하기 위해 출력버퍼를 강제로 비운다.
         */
        printf("tsh> "); fflush(stdout);
        /*
         * 표준 입력장치로부터 최대 MAX_LINE까지 명령어를 입력 받는다.
         * 입력된 명령어 끝에 있는 새줄문자를 널문자로 바꿔 C 문자열로 만든다.
         * 입력된 값이 없으면 새 명령어를 받기 위해 루프의 처음으로 간다.
         */
        len = read(STDIN_FILENO, cmd, MAX_LINE);
        if (len == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        cmd[--len] = '\0';
        if (len == 0)
            continue;
        /*
         * 종료 명령이면 루프를 빠져나간다.
         */
        if(!strcasecmp(cmd, "exit"))
            break;
        /*
         * 백그라운드 명령인지 확인하고, '&' 기호를 삭제한다.
         */
        char *p = strchr(cmd, '&');
        if (p != NULL) {
            background = 1;
            *p = '\0';
        }
        else
            background = 0;
        /*
         * 자식 프로세스를 생성하여 입력된 명령어를 실행하게 한다.
         */
        if ((pid = fork()) == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        /*
         * 자식 프로세스는 명령어를 실행하고 종료한다.
         */
        else if (pid == 0) {
            cmdexec(cmd);
            exit(EXIT_SUCCESS);
        }
        /*
         * 포그라운드 실행이면 부모 프로세스는 자식이 끝날 때까지 기다린다.
         * 백그라운드 실행이면 기다리지 않고 다음 명령어를 입력받기 위해 루프의 처음으로 간다.
         */
        else if (!background)
            waitpid(pid, NULL, 0);
    }
    return 0;
}
