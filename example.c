#include <leakchk.h>

int main (void) {
    if (lc_init () != 0) {
        return -1;
    }

    void *p = lc_malloc (23);

    /* The tracking info will be printed at the library deinitialization */
    lc_deinit ();
    return 0;
}
