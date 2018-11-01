/**************************************************************************
Initial author: Paul F. Richards (paulrichards321@gmail.com) 2016-2017
https://github.com/paulrichards321/jpg2svs

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*************************************************************************/
#include "vmslideviewer-gtk.h"
#include <windows.h>

void SlideWindow::openFileDlg(GtkWidget* window, gpointer data)
{
  GtkFileChooserNative *native_dlg;
  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
  gint res;
  SlideWindow *slideWindow = (SlideWindow*) data;

  if (data==0) return;

  native_dlg = gtk_file_chooser_native_new ("Open File", GTK_WINDOW(slideWindow->mainWindow),
                                      action,
                                      "_Open",
                                      "_Cancel");
  res = gtk_native_dialog_run (GTK_NATIVE_DIALOG (native_dlg));
  if (res == GTK_RESPONSE_ACCEPT)
  {
    char *filename;
    GtkFileChooser *chooser = GTK_FILE_CHOOSER (native_dlg);
    filename = gtk_file_chooser_get_filename (chooser);
    slideWindow->slide.close();  
    if (slideWindow->slide.open(filename))
    {
      slideWindow->set1PT25x(slideWindow->mainWindow, slideWindow);
    }
    g_free (filename);
  }
}



