#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Forward declare to avoid exposing any Tesseract internals
struct TessBaseAPI;
typedef struct TessBaseAPI TessBaseAPI;
struct Pix;
typedef struct Pix Pix;

TessBaseAPI* tess_create();
void tess_end(TessBaseAPI* handle);
void tess_delete(TessBaseAPI* handle);
int tess_init(TessBaseAPI* handle, const char* datapath, const char* language);
void tess_clear(TessBaseAPI* handle);
void tess_set_image(TessBaseAPI* handle, struct Pix* pix);
void tess_set_page_seg_mode(TessBaseAPI* handle, int mode);
char* tess_get_utf8_text(TessBaseAPI* handle);
void tess_delete_text(char* text);

#ifdef __cplusplus
}
#endif
