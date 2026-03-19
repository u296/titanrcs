#ifndef CLEANUPSTACK_H
#define CLEANUPSTACK_H
#include "common.h"
#include <stddef.h>

/* you'd typically use this like this

assuming variable name cs for cleanup stack and r for creation result

CLEANUP_START(PipelineCleanup) <---- PipelineCleanup is the type of the context given to the destructor
{dev, *pipeline}				<------ this is the value the context is gonna be set to
CLEANUP_END(pipeline)         <--- pipeline means that the destruction function is called destroy_pipeline

*/
#define CLEANUP_START(cleanuptype) if (cs != NULL && r == VK_SUCCESS) { \
	cleanuptype cleanup_context = 

#define CLEANUP_END(varname)	;cs_push(cs, &cleanup_context, sizeof(cleanup_context), destroy_##varname);\
}

// doesn't require result
#define CLEANUP_START_NORES(cleanuptype) if (cs != NULL) { \
	cleanuptype cleanup_context = 

#define CLEANUP_START_ONORES(cleanuptype) { cleanuptype cleanup_context =

#define CLEANUP_END_O(varname)	;cs_push(&cs, &cleanup_context, sizeof(cleanup_context), destroy_##varname);\
}

#define CLEANUP_BLOB_PTRS 4

typedef struct CleanupEntry {
	void(*destroy)(void*);
    usize blob[CLEANUP_BLOB_PTRS];
} CleanupEntry;

typedef struct CleanupStack {
	struct CleanupEntry* entries;
	u32 n;
	u32 cap;
} CleanupStack;

void cs_init(CleanupStack* cs);

void cs_push(CleanupStack* cs, void* ctx, usize ctxsize, void(*destructor)(void*));
void cs_push_entry(CleanupStack* cs, CleanupEntry ce);

void cs_consume(CleanupStack* cs);

void destroy_memfree(void* obj);

#endif