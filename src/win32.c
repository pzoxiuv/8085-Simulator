#include <stdint.h>	
#include <string.h>
#include "common.h"

#ifdef G_OS_WIN32

#include <windows.h>

void win32FileSelect (int8_t *filename) {
    OPENFILENAME of;
    uint32_t retval;
    int8_t filenameTmp [256];	
    filenameTmp [0] = '\0';

    memset(&of, 0, sizeof (of));
    of.lStructSize = sizeof (of);
    of.hwndOwner = NULL;
    of.lpstrFilter = "Hex files (*.hex)\0*.hex\0all files\0*\0";
    of.lpstrCustomFilter = NULL;
    of.nFilterIndex = 1;
    of.lpstrFile = (char *) filenameTmp;
    of.nMaxFile = sizeof (filenameTmp);
    of.lpstrFileTitle = NULL;
    of.lpstrInitialDir = NULL;
    of.lpstrTitle = NULL;
    of.lpstrDefExt = "hex";
    of.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	retval = GetOpenFileName(&of);

    if (!retval) {
		if (CommDlgExtendedError())
			g_print ("Error getting file");
    }
    g_print ("\n%s\n", filenameTmp);
    strcpy ((char *) filename, (const char *) filenameTmp);
    g_print ("%s\n", filename);
}

#endif
