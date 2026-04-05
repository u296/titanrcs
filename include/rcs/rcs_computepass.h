#ifndef RCS_COMPUTEPASS_H
#define RCS_COMPUTEPASS_H

#include "context.h"

void run_computepass(RenderContext* ctx);

void run_visual_computepass(RenderContext* ctx, CleanupStack* swp_cs);

#endif