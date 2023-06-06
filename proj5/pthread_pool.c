/*
 * Copyright(c) 2021-2023 All rights reserved by Heekuck Oh.
 * 이 프로그램은 한양대학교 ERICA 컴퓨터학부 학생을 위한 교육용으로 제작되었다.
 * 한양대학교 ERICA 학생이 아닌 이는 프로그램을 수정하거나 배포할 수 없다.
 * 프로그램을 수정할 경우 날짜, 학과, 학번, 이름, 수정 내용을 기록한다.
 */
// Developed by DevTae (Kim Taehyeon / 컴퓨터학과 / 3학년 2019061658)

#include "pthread_pool.h"
#include <stdlib.h>
#include <stdio.h>
#define MAX(a, b) ((a > b) ? a : b) // MAX 함수 선언

/*
 * 풀에 있는 일꾼(일벌) 스레드가 수행할 함수이다.
 * FIFO 대기열에서 기다리고 있는 작업을 하나씩 꺼내서 실행한다.
 * 대기열에 작업이 없으면 새 작업이 들어올 때까지 기다린다.
 * 이 과정을 스레드풀이 종료될 때까지 반복한다.
 */
static void *worker(void *param)
{
    printf("123");
}

/*
 * 스레드풀을 생성한다. bee_size는 일꾼(일벌) 스레드의 개수이고, queue_size는 대기열의 용량이다.
 * bee_size는 POOL_MAXBSIZE를, queue_size는 POOL_MAXQSIZE를 넘을 수 없다.
 * 일꾼 스레드와 대기열에 필요한 공간을 할당하고 변수를 초기화한다.
 * 일꾼 스레드의 동기화를 위해 사용할 상호배타 락과 조건변수도 초기화한다.
 * 마지막 단계에서는 일꾼 스레드를 생성하여 각 스레드가 worker() 함수를 실행하게 한다.
 * 대기열로 사용할 원형 버퍼의 용량이 일꾼 스레드의 수보다 작으면 효율을 극대화할 수 없다.
 * 이런 경우 사용자가 요청한 queue_size를 bee_size로 상향 조정한다.
 * 성공하면 POOL_SUCCESS를, 실패하면 POOL_FAIL을 리턴한다.
 */
int pthread_pool_init(pthread_pool_t *pool, size_t bee_size, size_t queue_size)
{
    // pool 조건 확인 (23.6.6)
    if (bee_size > POOL_MAXBSIZE || queue_size > POOL_MAXQSIZE)
        return POOL_FAIL;

    // bee_size > queue_size 인 상황에서의 queue_size 상향 (23.6.6)
    queue_size = MAX(bee_size, queue_size);

    // pool 변수 초기화 및 할당 (23.6.6)
    if((pool = (pthread_pool_t *)malloc(sizeof(pthread_pool_t))) == NULL) {
        return POOL_FAIL;
    }
    pool->running = true;                                    // 스레드풀의 실행 또는 종료 상태
    
    if((pool->q = (task_t *)malloc(sizeof(task_t) * queue_size)) == NULL) { // FIFO 작업 대기열로 사용할 원형 버퍼
        return POOL_FAIL;
    }
    pool->q_size = queue_size;                               // 원형 버퍼 q 배열의 크기
    pool->q_front = 0;                                       // 대기열에서 다음에 실행될 작업의 위치
    pool->q_len = 0;                                         // 대기열의 길이
    if((pool->bee = (pthread_t *)malloc(sizeof(pthread_t) * bee_size)) == NULL) { // 일꾼(일벌) 스레드의 ID를 저장하기 위한 배열
        return POOL_FAIL;
    }
    if(pool->bee == NULL) return POOL_FAIL;
    pool->bee_size = bee_size;                               // bee 배열의 크기로 일꾼 스레드의 수를 의미
    pthread_mutex_init(&(pool->mutex), NULL);                // 대기열을 접근하기 위해 사용되는 상호배타 락
    pthread_cond_init(&(pool->full), NULL);                  // 빈 대기열에 새 작업이 들어올 때까지 기다리는 곳
    pthread_cond_init(&(pool->empty), NULL);                 // 대기열에 빈 자리가 발생할 때까지 기다리는 곳

    // worker 함수 할당 (23.6.6)
    int tmp[bee_size];
    for(int i = 0; i < bee_size; i++) {
        tmp[i] = i;
        pthread_create(&(pool->bee[tmp[i]]), NULL, worker, tmp + i);
    }

    // pool 생성 성공 시 POOL_SUCCESS 반환
    return POOL_SUCCESS;
}

/*
 * 스레드풀에서 실행시킬 함수와 인자의 주소를 넘겨주며 작업을 요청한다.
 * 스레드풀의 대기열이 꽉 찬 상황에서 flag이 POOL_NOWAIT이면 즉시 POOL_FULL을 리턴한다.
 * POOL_WAIT이면 대기열에 빈 자리가 나올 때까지 기다렸다가 넣고 나온다.
 * 작업 요청이 성공하면 POOL_SUCCESS를 리턴한다.
 */
int pthread_pool_submit(pthread_pool_t *pool, void (*f)(void *p), void *p, int flag)
{
    // 여기를 완성하세요
    return 0;
}

/*
 * 스레드풀을 종료한다. 일꾼 스레드가 현재 작업 중이면 그 작업을 마치게 한다.
 * how의 값이 POOL_COMPLETE이면 대기열에 남아 있는 모든 작업을 마치고 종료한다.
 * POOL_DISCARD이면 대기열에 새 작업이 남아 있어도 더 이상 수행하지 않고 종료한다.
 * 부모 스레드는 종료된 일꾼 스레드와 조인한 후에 스레드풀에 할당된 자원을 반납한다.
 * 스레드를 종료시키기 위해 철회를 생각할 수 있으나 바람직하지 않다.
 * 락을 소유한 스레드를 중간에 철회하면 교착상태가 발생하기 쉽기 때문이다.
 * 종료가 완료되면 POOL_SUCCESS를 리턴한다.
 */
int pthread_pool_shutdown(pthread_pool_t *pool, int how)
{
    // 여기를 완성하세요
    return 0;
}
