#include "swapchaincontext/swapchaincontext.h"
#include "cleanupstack.h"
#include "swapchaincontext/swapchain.h"
//#include "swapchaincontext/

#include<assert.h>

#define CHECK assert(f == false)

bool make_swapchain_context(
    RenderBackend rb,
    SwapchainContext* swpctx,
    CleanupStack* swp_cs
) {
    bool f;

    Error e;

    f = make_swapchain(rb.physdev, rb.dev, rb.queues, rb.surf, rb.wnd, &swpctx->swpch, &swpctx->format, &swpctx->swpch_ext, &swpctx->n_swpch_img, &swpctx->swpch_imgs, &e, swp_cs);
	CHECK;

	f = make_swapchain_imageviews(rb.dev, swpctx->n_swpch_img, swpctx->swpch_imgs, swpctx->format, &swpctx->swpch_imgvs, &e, swp_cs);
	CHECK;

    return false;
}