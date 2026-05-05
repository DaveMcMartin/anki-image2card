#include <tesseract/capi.h>
#include <leptonica/allheaders.h>

TessBaseAPI* tess_create() {
    return TessBaseAPICreate();
}

void tess_end(TessBaseAPI* handle) {
    TessBaseAPIEnd(handle);
}

void tess_delete(TessBaseAPI* handle) {
    TessBaseAPIDelete(handle);
}

int tess_init(TessBaseAPI* handle, const char* datapath, const char* language) {
    return TessBaseAPIInit3(handle, datapath, language);
}

void tess_clear(TessBaseAPI* handle) {
    TessBaseAPIClear(handle);
}

void tess_set_image(TessBaseAPI* handle, struct Pix* pix) {
    TessBaseAPISetImage2(handle, pix);
}

void tess_set_page_seg_mode(TessBaseAPI* handle, int mode) {
    TessBaseAPISetPageSegMode(handle, (TessPageSegMode)mode);
}

char* tess_get_utf8_text(TessBaseAPI* handle) {
    return TessBaseAPIGetUTF8Text(handle);
}

void tess_delete_text(char* text) {
    TessDeleteText(text);
}
