/* Encoding stuff */

#ifndef theater_SUBTITLE_ENCODING_H
#define theater_SUBTITLE_ENCODING_H

#include <gtk/gtk.h>

void theater_subtitle_encoding_init (GtkComboBox *combo);
void theater_subtitle_encoding_set (GtkComboBox *combo, const char *encoding);
const char * theater_subtitle_encoding_get_selected (GtkComboBox *combo);

#endif /* SUBTITLE_ENCODING_H */
