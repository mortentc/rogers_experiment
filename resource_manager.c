#include "external/delilah/include/verified_functions.h"
#include <pthread.h>
#include <malloc.h>

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

struct arr_queue {
    struct triplet_node {
        obtain_triplet triplet;
        struct triplet_node *next;
    } *first, *last;
};

struct stb_queue {
    struct time_triplet_node {
        obtain_triplet triplet;
        int local_time;
        struct time_triplet_node *next;
    } *first, *last;
};

struct rel_queue {
    struct req_set_node {
        request_set reqs;
        struct req_set_node *next;
    } *first, *last;
};

typedef struct time_item {
    permission p;
    int local_time;
    struct time_item *next;
} future_list;

struct future_list_entry {
    int id;
    int next_write;
    future_list *first, *last;
};

struct arr_queue arrivals;
struct stb_queue standby;
struct rel_queue releases;
struct future_list_entry* future_lists[MAX_REGIONS];
int global_time = 1;

void pop_future(struct future_list_entry *fut_list){
    void *to_free = fut_list->first;
    fut_list->first = fut_list->first->next;
    free(to_free);
}

void append_future(struct future_list_entry *fut_list, permission p, int local_time){
    future_list *node = malloc(sizeof(future_list));
    node->p = p; node->local_time = local_time;
    if(fut_list->last != NULL) fut_list->last->next = node;
    fut_list->last = node;
}

struct future_list_entry* lookup_future(int id){
    int i = id % MAX_REGIONS;
    while(future_lists[i] != NULL && future_lists[i]->id != id)
        i = (i+1) % MAX_REGIONS;
    return future_lists[i];
}

void enqueue_req(struct arr_queue *q, obtain_triplet obs){
    struct triplet_node *node = malloc(sizeof(struct triplet_node));
    node->triplet = obs;
    if(q->last != NULL) q->last->next = node;
    q->last = node;
}

obtain_triplet dequeue_req(struct arr_queue *q){
    obtain_triplet res = q->first->triplet;
    void *to_free = q->first;
    q->first = q->first->next;
    free(to_free);
    return res;
}

void enqueue_stb(struct stb_queue *q, obtain_triplet obs, int local_time){
    struct time_triplet_node *node = malloc(sizeof(struct time_triplet_node));
    node->triplet = obs;
    node->local_time = local_time;
    if(q->last != NULL) q->last->next = node;
    q->last = node;
}

void enqueue_rel(struct rel_queue *q, request_set reqs){
    struct req_set_node *node = malloc(sizeof(struct req_set_node));
    node->reqs = reqs;
    if(q->last != NULL) q->last->next = node;
    q->last = node;
}

request_set dequeue_rel(struct rel_queue *q){
    request_set res = q->first->reqs;
    void *to_free = q->first;
    q->first = q->first->next;
    free(to_free);
    return res;
}

int min(int a, int b) { return a < b ? a : b; }

void init_manager(){
}

int can_be_granted(request_set *reqs, int local_time){
    for(int i = 0; i<reqs->count; i++){
        int next_conflict = 0;
        request cur = reqs->reqs[i];
        struct future_list_entry *f = lookup_future(cur.id);

        if(cur.p == Read) next_conflict = f->next_write;
        else next_conflict = f->first->local_time;

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
                append_future(fut, cur.p, global_time);
                if(cur.p == Write)
                    fut->next_write = min(global_time, fut->next_write);
            }
            if(can_be_granted(&trip.reqs, global_time)) acquire(trip);
            else enqueue_stb(&standby, trip, global_time);
            global_time += 1;
        }
    }
}

void unlock(request_set reqs){
    for(int i = 0; i<reqs.count; i++){
        request cur = reqs.reqs[i];
        struct future_list_entry *f = lookup_future(cur.id);
        pop_future(f);
        if(cur.p == Write){
            f->next_write = INT32_MAX;
            future_list *next = f->first;
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

void handle_release(){
    int was_released = releases.first != releases.last;
    while(releases.first != releases.last){
        request_set reqs = dequeue_rel(&releases);
        unlock(reqs);
    }
    if(was_released){
        struct time_triplet_node *cur = standby.first, *prev = NULL;
        while(cur != NULL){
            if(can_be_granted(&cur->triplet.reqs, cur->local_time)){
                acquire(cur->triplet);
                if(cur == standby.first) standby.first = cur->next;
                if(cur == standby.last) standby.last = prev;
                if(prev != NULL) prev->next = cur->next;
                free(cur);
                cur = prev != NULL ? prev : standby.first;
            }
            prev = cur; cur = cur->next;
        }
    }
}

// allocate_shared();

void main_routine(){
    while(1){
        handle_arrival();
        handle_release();
    }
}