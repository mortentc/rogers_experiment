#include "external/delilah/include/verified_functions.h"
#include <pthread.h>
#include <malloc.h>
#include <stdlib.h>

#define CORES 12
#define MAX_REGIONS 200
// #define DEBUG

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
    pthread_mutex_t lock;
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
    pthread_mutex_t lock;
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

struct hash_map_region
{
    struct region_item {
        int id; char *addr; int length; struct region_item *next;
    }* bins[MAX_REGIONS];
};

struct hash_map_future {
    struct fut_item {
        struct future_list_entry *list; struct fut_item *next;
    }* bins[MAX_REGIONS];
};

struct arr_queue arrivals;
struct stb_queue standby;
struct rel_queue releases;
struct hash_map_future future_lists;
struct hash_map_region regions;
int global_time = 1;

void pop_future(struct future_list_entry *fut_list){
    void *to_free = fut_list->first;
    fut_list->first = fut_list->first->next;
    if(fut_list->first == NULL) fut_list->last = NULL;
    free(to_free);
}

void append_future(struct future_list_entry *fut_list, permission p, int local_time){
    future_list *node = malloc(sizeof(future_list));
    #ifdef DEBUG
        printf("New node\n");
    #endif
    node->p = p; node->local_time = local_time; node->next=NULL;
    #ifdef DEBUG
        printf("Updating pointers\n");
    #endif
    if(fut_list->last != NULL) fut_list->last->next = node;
    else fut_list->first = node;
    #ifdef DEBUG
        printf("Node appended\n");
    #endif
    fut_list->last = node;
}

struct future_list_entry* lookup_future(int id){
    struct fut_item *first = future_lists.bins[id % MAX_REGIONS];
    while(first->list->id != id) first = first->next;
    return first->list;
}

void new_future(int id){
    struct fut_item *new = malloc(sizeof(struct fut_item));
    struct future_list_entry *node = malloc(sizeof(struct future_list_entry));
    #ifdef DEBUG
        printf("Init list\n");
    #endif
    new->list = node;
    new->next = NULL;
    node->id = id;
    node->next_write = 0;
    node->first = NULL; node->last = NULL;
    append_future(node, Write, 0);
    #ifdef DEBUG
        printf("Future list created\n");
    #endif
    new->next = future_lists.bins[id % MAX_REGIONS];
    future_lists.bins[id % MAX_REGIONS] = new;
}

void enqueue_req(struct arr_queue *q, obtain_triplet obs){
    struct triplet_node *node = malloc(sizeof(struct triplet_node));
    node->triplet = obs; node->next = NULL;
    pthread_mutex_lock(&q->lock);
    struct triplet_node *last = q->last;
    if(last != NULL) last->next = node;
    else q->first = node;
    q->last = node;
    pthread_mutex_unlock(&q->lock);
}

struct region_item* lookup_region(int id){
    struct region_item *cur = regions.bins[id % MAX_REGIONS];
    while(cur->id != id) cur = cur->next;
    return cur;
}

void new_region(int id, char *addr, int length){
    struct region_item *node = malloc(sizeof(struct region_item));
    node->addr = addr;
    node->length = length;
    node->id = id;
    node->next = regions.bins[id % MAX_REGIONS];
    regions.bins[id % MAX_REGIONS] = node;
}

int region_exists(int id){
    for(struct region_item *cur = regions.bins[id % MAX_REGIONS]; cur != NULL; cur = cur->next)
        if(cur->id == id) return 1;
    return 0;
}

obtain_triplet dequeue_req(struct arr_queue *q){
    #ifdef DEBUG
        printf("Starting dequeue\n");
    #endif
    obtain_triplet res = q->first->triplet;
    #ifdef DEBUG
        printf("Triplet dequeued\n");
    #endif
    void *to_free = q->first;
    q->first = q->first->next;
    if(q->first == NULL) q->last = NULL;
    #ifdef DEBUG
        printf("Trying to free memory\n");
    #endif
    pthread_mutex_lock(&q->lock);
    free(to_free);
    pthread_mutex_unlock(&q->lock);
    return res;
}

void enqueue_stb(struct stb_queue *q, obtain_triplet obs, int local_time){
    struct time_triplet_node *node = malloc(sizeof(struct time_triplet_node));
    node->triplet = obs;
    node->local_time = local_time;
    node->next = NULL;
    if(q->last != NULL) q->last->next = node;
    else q->first = node;
    q->last = node;
}

void enqueue_rel(struct rel_queue *q, request_set reqs){
    struct req_set_node *node = malloc(sizeof(struct req_set_node));
    node->reqs = reqs;
    node->next = NULL;
    pthread_mutex_lock(&q->lock);
    if(q->last != NULL) q->last->next = node;
    else q->first = node;
    q->last = node;
    pthread_mutex_unlock(&q->lock);
}

request_set dequeue_rel(struct rel_queue *q){
    request_set res = q->first->reqs;
    void *to_free = q->first;
    q->first = q->first->next;
    if(q->first == NULL) q->last = NULL;
    pthread_mutex_lock(&q->lock);
    free(to_free);
    pthread_mutex_unlock(&q->lock);
    return res;
}

int min(int a, int b) { return a < b ? a : b; }

int can_be_granted(request_set *reqs, int local_time){
    for(int i = 0; i<reqs->count; i++){
        int next_conflict = 0;
        request cur = reqs->reqs[i];
        struct future_list_entry *f = lookup_future(cur.id);

        if(cur.p == Read) next_conflict = f->next_write;
        else next_conflict = f->first->local_time;
        #ifdef DEBUG
            printf("Next conflict at: %d\n", next_conflict);
        #endif
        if(local_time > next_conflict) return 0;
    }
    #ifdef DEBUG
            printf("Request can be granted\n");
    #endif
    return 1;
} 

void acquire(obtain_triplet trip){
    #ifdef DEBUG
        printf("Acquiring regions.\n");
    #endif
    for(int i = 0; i<trip.reqs.count; i++){
        struct region_item *cur = lookup_region(trip.reqs.reqs[i].id);
        trip.res[i].ptr = cur->addr; trip.res[i].length = cur->length;
    }
    *trip.flag = 1;
}

void handle_arrival(){
    if(arrivals.first != NULL){
        #ifdef DEBUG
            printf("Arrival queue is not empty. Processing...\n");
        #endif
        obtain_triplet trip = dequeue_req(&arrivals);
        #ifdef DEBUG
            printf("Found request for region %d\n", trip.reqs.reqs[0].id);
        #endif
        for(int i = 0; i<trip.reqs.count; i++){
            if (!region_exists(trip.reqs.reqs[i].id)) {
                #ifdef DEBUG
                    printf("Request for non-existant region\n");
                #endif
                *trip.flag = -1;
                break;
            }
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

#define MAGIC_NUM 100
int id_seed = 0;
int new_id(){
    int id = rand() % MAGIC_NUM + id_seed;
    id_seed += MAGIC_NUM;
    return id;
}

void handle_release(){
    int was_released = releases.last != NULL;
    while(releases.last != NULL){
        #ifdef DEBUG
            printf("Trying to release\n");
        #endif
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

int allocate_shared(int length, mem_rng *res){
    char *addr = malloc(length);
    int id = new_id();
    #ifdef DEBUG
        printf("Creating new region %d.\n", id);
    #endif
    new_region(id, addr, length);
    #ifdef DEBUG
        printf("Creating new future list.\n");
    #endif
    new_future(id);
    #ifdef DEBUG
        printf("Success!\n");
    #endif
    res->ptr = addr; res->length = length;
    return id;
}

// free_shared();

void* main_routine(){
    while(1){
        handle_arrival();
        handle_release();
    }
}

pthread_t *init_manager(){
    pthread_t *t = malloc(sizeof(pthread_t));
    pthread_create(t, NULL, main_routine, NULL);
    pthread_mutex_init(&arrivals.lock, NULL);
    pthread_mutex_init(&releases.lock, NULL);
    return t;
}

void destroy_manager(pthread_t *t){
    pthread_cancel(*t);
    pthread_mutex_destroy(&arrivals.lock);
    pthread_mutex_destroy(&releases.lock);
    free(t);
}