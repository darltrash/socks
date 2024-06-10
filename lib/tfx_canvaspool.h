#include "tinyfx.h"


typedef struct {
    unsigned int w, h;

    tfx_format format;
    bool msaa, cube;
} tfc_request;

tfx_canvas tfc_alloc(tfc_request request);
void tfc_free(tfx_canvas canvas);
void tfc_refresh();


#ifdef TFC_IMPL
#ifndef TFC_MAX_CANVASES
#define TFC_MAX_CANVASES 32
#endif

tfx_canvas tfc_alloc(tfc_request request) {

}

#endif