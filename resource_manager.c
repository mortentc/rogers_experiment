#include "external/delilah/include/verified_functions.h"
#include <pthread.h>

/*
    Arrivals : Queue
    Standby : Queue
    Releases : Queue
    FutureLists : Hashtable<id, list<id>>
    Regions : Hashtable<id, mem_range>
    Time : int
*/

typedef struct req_set {
    int count;
    request *reqs;
} request_set;

typedef struct obtain {
    request_set reqs;
    mem_range *res;
    int *flag;
} obtain_triplet;

struct req_queue {
    obtain_triplet triplets[12];
    int first, last;
};

struct rel_queue {
    request_set reqs[12];
    int first, last;
};

struct req_queue arrivals;
struct req_queue standby;
struct rel_queue releases;
int time = 1;

struct time_item {
    permission p;
    int time;
    struct time_item *next;
};

struct future_lists {
    struct time_item lists[50];
};

void enqueue_req(struct req_queue *q, obtain_triplet obs){
    q->triplets[q->last] = obs;
    q->last += 1;
}

obtain_triplet dequeue_req(struct req_queue *q){
    obtain_triplet res = q->triplets[q->first];
    q->first+=1;
    return res;
}

void enqueue_rel(struct rel_queue *q, request_set reqs){
    q->reqs[q->last] = reqs;
    q->last += 1;
}

request_set dequeue_req(struct rel_queue *q){
    request_set res = q->reqs[q->first];
    q->first+=1;
    return res;
}

void init_manager(){
    arrivals.first = 0;
    arrivals.last = 0;
    standby.first = 0;
    standby.last = 0;
    releases.first = 0;
    releases.last = 0;
}

// HandleArrival()
// HandleRelease()

int can_be_granted(request_set *reqs, int local_time){
    for(int i = 0; i<reqs->count; i++){
        int next_conflict = 0;
        request cur = reqs->reqs[i];
        // if(cur.p == Read) next_conflict =
    }
} 

/*
void main_routine(){
    while(1){
        handle_arrival();
        handle_release();
    }
}
*/