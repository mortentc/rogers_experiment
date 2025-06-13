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
#define CORES 12
#define MAX_REGIONS 50

typedef struct req_set {
    int count;
    request *reqs;
} request_set;

typedef struct obtain {
    request_set reqs;
    mem_rng *res;
    int *flag;
} obtain_triplet;

struct req_queue {
    obtain_triplet triplets[CORES];
    int first, last;
};

struct rel_queue {
    request_set reqs[CORES];
    int first, last;
};

typedef struct time_item {
    permission p;
    int local_time;
    struct time_item *next;
} future_list;

struct future_list_entry {
    int id;
    int next_write;
    future_list *future;
};

struct req_queue arrivals;
struct req_queue standby;
struct rel_queue releases;
struct future_list_entry* future_lists[50];
int global_time = 1;


struct future_list_entry* lookup_future(int id){
    int i = id % MAX_REGIONS;
    while(future_lists[i] != NULL && future_lists[i]->id != id)
        i = (i+1) % MAX_REGIONS;
    return future_lists[i];
}

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

request_set dequeue_rel(struct rel_queue *q){
    request_set res = q->reqs[q->first];
    q->first+=1;
    return res;
}

int min(int a, int b) { return a < b ? a : b; }

void init_manager(){
    arrivals.first = 0;
    arrivals.last = 0;
    standby.first = 0;
    standby.last = 0;
    releases.first = 0;
    releases.last = 0;
}

int can_be_granted(request_set *reqs, int local_time){
    for(int i = 0; i<reqs->count; i++){
        int next_conflict = 0;
        request cur = reqs->reqs[i];
        struct future_list_entry *f = lookup_future(cur.id);

        if(cur.p == Read) next_conflict = f->next_write;
        else next_conflict = f->future->local_time;

        if(local_time > next_conflict) return 0;
    }
    return 1;
} 


void acquire(obtain_triplet trip){
    for(int i = 0; i<trip.reqs.count; i++){
        //append_regions(res, regions)
    }
    *trip.flag = 1;
}

void handle_arrival(){
    if(arrivals.first != arrivals.last){
        obtain_triplet trip = dequeue_req(&arrivals);
        for(int i = 0; i<trip.reqs.count; i++){
            //if (member(trip.reqs.reqs[i], regions)) {
            *trip.flag = -1;
            break;
            //}
        }
        if(*trip.flag != -1){
            for(int i = 0; i<trip.reqs.count; i++){
                request cur = trip.reqs.reqs[i];
                struct future_list_entry *fut = lookup_future(cur.id);
                // append(fut.list, (cur.perm, global_time))
                if(cur.p == Write) fut->next_write = min(global_time, fut->next_write);
            }
            if(can_be_granted(&trip.reqs, global_time)) acquire(trip);
            //else enqueue_req(&standby, global_time + trip);
            global_time += 1;
        }
    }
}

void handle_release(){
    int was_released = releases.first != releases.last;
    while(releases.first != releases.last){
        request_set reqs = dequeue_rel(&releases);
        unlock(reqs);
    }
    if(was_released){
        //traverse standbyqueue;
    }
}

// allocate_shared();

void unlock(request_set reqs){
    for(int i = 0; i<reqs.count; i++){
        request cur = reqs.reqs[i];
        struct future_list_entry *f = lookup_future(cur.id);
        //pop(f->future)
        if(cur.p == Write){
            f->next_write = INT32_MAX;
            struct time_item *next = f->future->next;
            while(next != NULL){
                if(next->p == Write){
                    f->next_write = next->local_time;
                    break;
                }
                next = next->next;
            }
        }
    }
}

void main_routine(){
    while(1){
        handle_arrival();
        handle_release();
    }
}