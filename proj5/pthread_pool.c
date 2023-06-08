/*
 * Copyright(c) 2021-2023 All rights reserved by Heekuck Oh.
 * 이 프로그램은 한양대학교 ERICA 컴퓨터학부 학생을 위한 교육용으로 제작되었다.
 * 한양대학교 ERICA 학생이 아닌 이는 프로그램을 수정하거나 배포할 수 없다.
 * 프로그램을 수정할 경우 날짜, 학과, 학번, 이름, 수정 내용을 기록한다.
 */
// Developed by DevTae (Kim Taehyeon / 컴퓨터학과 / 3학년 2019061658)

#include "pthread_pool.h"
#include <stdlib.h>
#define MAX(a, b) ((a > b) ? a : b) // MAX 함수 선언

/*
 * 풀에 있는 일꾼(일벌) 스레드가 수행할 함수이다.
 * FIFO 대기열에서 기다리고 있는 작업을 하나씩 꺼내서 실행한다.
 * 대기열에 작업이 없으면 새 작업이 들어올 때까지 기다린다.
 * 이 과정을 스레드풀이 종료될 때까지 반복한다.
 */
static void *worker(void *param)
{
    // pool 주소 받아오기 (23.6.6)
    pthread_pool_t *pool = (pthread_pool_t *)param;

    while (true) {
        // 상호배타 mutex 획득 (23.6.6)
        pthread_mutex_lock(&(pool->mutex));
        
        // 대기열에 기다리는 함수가 있는지 확인 (23.6.7)
        while(pool->q_len == 0) {
            if (!pool->running) {
                pthread_mutex_unlock(&(pool->mutex));
                pthread_exit(NULL);
            }
            pthread_cond_wait(&(pool->full), &(pool->mutex));
        }
        
        // 실행할 작업의 위치를 저장함 (23.6.8)
        int to_exec = pool->q_front;
        task_t task = pool->q[to_exec];

        // 큐 인덱스 갱신 (23.6.8)
        pool->q_front = (pool->q_front + 1) % pool->q_size;
        pool->q_len--;
        
        // 조건변수 시그널 및 뮤텍스 반환 (23.6.8)
        pthread_cond_signal(&(pool->empty));
        pthread_mutex_unlock(&(pool->mutex));
    
        // 대기열에서 기다리는 함수 실행 (23.6.7)
        (*(task.function))(task.param);
    }
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
    pthread_pool_t *pool_p;
    if((pool_p = (pthread_pool_t *)malloc(sizeof(pthread_pool_t))) == NULL) {
        return POOL_FAIL;
    }
    *pool = *pool_p;
    
    // 스레드풀의 실행 또는 종료 상태
    pool->running = true;
    
    // FIFO 작업 대기열로 사용할 원형 버퍼
    if((pool->q = (task_t *)malloc(sizeof(task_t) * queue_size)) == NULL) {
        return POOL_FAIL;
    } 
    
    // 원형 버퍼 q 배열의 크기
    pool->q_size = queue_size;
    // 대기열에서 다음에 실행될 작업의 위치
    pool->q_front = 0;
    // 대기열의 길이
    pool->q_len = 0;

    // 일꾼(일벌) 스레드의 ID를 저장하기 위한 배열
    if((pool->bee = (pthread_t *)malloc(sizeof(pthread_t) * bee_size)) == NULL) {
        return POOL_FAIL;
    }
    
    // bee 배열의 크기로 일꾼 스레드의 수를 의미
    pool->bee_size = bee_size;
    // 대기열을 접근하기 위해 사용되는 상호배타 락
    pthread_mutex_init(&(pool->mutex), NULL);
    // 빈 대기열에 새 작업이 들어올 때까지 기다리는 곳
    pthread_cond_init(&(pool->full), NULL);
    // 대기열에 빈 자리가 발생할 때까지 기다리는 곳
    pthread_cond_init(&(pool->empty), NULL);
    
    // worker 함수 할당 (23.6.6)
    for(int i = 0; i < bee_size; i++) {
        pthread_create(pool->bee + i, NULL, worker, pool); // pool pointer 전달
    }
    
    // pool 생성 성공 시 POOL_SUCCESS 반환 (23.6.6)
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
    /* (23.6.6)
     * pool : 스레드풀 포인터
     * f : void* 를 매개변수로 받는 함수
     * p : 함수에 입력된 매개변수
     * flag : POOL_NOWAIT 또는 POOL_WAIT 옵션
     */

    // 상호배제 mutex 획득 (23.6.8)
    pthread_mutex_lock(&(pool->mutex));
    
    // 다음 조건이 모두 성립할 때, 조건변수를 바탕으로 대기함 (23.6.8)
    // 1. 큐가 가득 찼음
    // 2. pool 이 running 상태임
    // 3. POOL_WAIT 옵션임
    while (pool->q_len == pool->q_size && pool->running && flag == POOL_WAIT) {
        pthread_cond_wait(&(pool->empty), &(pool->mutex));
    }

    // POOL_NOWAIT 에 꽉 찼다면 POOL_FULL 반환
    if (pool->q_len == pool->q_size) {
        pthread_mutex_unlock(&(pool->mutex));
        return POOL_FULL;
    }

    // pool 이 running 상태가 아닌 경우  POOL_FAIL 반환
    if (!pool->running) {
        pthread_mutex_unlock(&(pool->mutex));
        return POOL_FAIL;
    }

    // 대기열 빈 자리 인덱스 저장 공간 (23.6.8)
    int index = (pool->q_front + pool->q_len) % pool->q_size;

    // 넣을 공간을 찾았다면 그대로 집어넣기 (23.6.8)
    pool->q[index].function = f;
    pool->q[index].param = p;
    pool->q_len++;

    pthread_cond_signal(&(pool->full));

    // 상호배제 mutex 반환 (23.6.8)
    pthread_mutex_unlock(&(pool->mutex));

    return POOL_SUCCESS;
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
    // 상호배제 mutex 획득 (23.6.8)
    pthread_mutex_lock(&(pool->mutex));

    // 더 이상의 요청을 받지 않음.
    pool->running = false;

    // how 에 따라 처리 진행 (23.6.8)
    switch (how) {
        case POOL_DISCARD:
            // 대기열 모두 삭제 (23.6.8)
            pool->q_len = 0;
            break;
            
        case POOL_COMPLETE:
            break;
    }
    
    // 종료할 스레드는 모두 종료시키도록 신호를 보냄 (23.6.8)
    pthread_cond_broadcast(&(pool->full));
    pthread_cond_broadcast(&(pool->empty));

    // 상호배제 mutex 반환 (23.6.8)
    pthread_mutex_unlock(&(pool->mutex));
    
    // 종료한 스레드들 join 진행 (23.6.8)
    for(int i = 0; i < pool->bee_size; i++) {
        pthread_join(pool->bee[i], NULL);
    }

    // 스레드풀 메모리 및 뮤텍스, 조건변수 할당 해제 (23.6.8)
    free(pool->bee);
    free(pool->q);
    pthread_cond_destroy(&(pool->empty));
    pthread_cond_destroy(&(pool->full));
    pthread_mutex_destroy(&(pool->mutex));

    return POOL_SUCCESS;
}
