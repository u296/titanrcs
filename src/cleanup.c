#include "cleanupstack.h"
#include <stdlib.h>
#include <string.h>


#define INIT_CS_SIZE 16
void cs_init(struct CleanupStack* cs) {
	cs->entries = malloc(INIT_CS_SIZE*sizeof(struct CleanupEntry));
	cs->n = 0;
	cs->cap = INIT_CS_SIZE;
}

void cs_push(struct CleanupStack* cs, void* ctx, usize ctxsize, void(*destructor)(void*)) {
    if (cs->n + 1 > cs->cap) {
		cs->entries = realloc(cs->entries, cs->cap * 2 * sizeof(struct CleanupEntry));
		cs->cap *= 2;
	}
	memcpy(&cs->entries[cs->n].blob, ctx, ctxsize);
    cs->entries[cs->n].destroy = destructor;

	cs->n += 1;
}

void cs_push_entry(struct CleanupStack* cs, struct CleanupEntry ce) {
	if (cs->n + 1 > cs->cap) {
		cs->entries = realloc(cs->entries, cs->cap * 2 * sizeof(struct CleanupEntry));
		cs->cap *= 2;
	}
	cs->entries[cs->n] = ce;
	cs->n += 1;
}

void cs_consume(struct CleanupStack* cs) {
	
	for (u32 i_p1 = cs->n; i_p1 > 0; i_p1--) {
		cs->entries[i_p1-1].destroy(cs->entries[i_p1-1].blob);
	}
	if (cs->cap != 0 && cs->entries != NULL) {
		free(cs->entries);
	}
	cs->entries = NULL;
	cs->cap = 0;
	cs->n = 0;
}

void destroy_memfree(void* obj) {
    void** themem = (void**)obj;
	free(*themem);
}