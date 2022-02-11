#include <stdlib.h>
#include <gst/gst.h>

int main(int argc, char *argv[]) {
    if (argc == 3) {
      GstPlugin *plugin;
      gst_init(NULL, NULL);

      if ((plugin = gst_registry_find_plugin (gst_registry_get (), argv[2]))) {
        gst_object_unref (plugin);
        gst_deinit ();
        goto run_test;
      } else {
        gst_deinit ();
        goto skip_test;
      }
    }

run_test:
    return system(argv[1]);
skip_test:
    return 77;
}
