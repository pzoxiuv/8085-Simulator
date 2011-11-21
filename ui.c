#include <stdint.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <glib.h>
#include "common.h"

enum {
	ROW_BOLD_COLUMN,
	ADDR_COLUMN,
	ASM_COLUMN,
	NUM_COLUMNS
};

void quitProgram (void);
void loadFileClicked (void);
void runStepButtonClicked (gpointer);
void resetButtonClicked (void);
void stopButtonClicked (void);
void rowDoubleClicked (GtkTreeView *, GtkTreePath *, GtkTreeViewColumn *, gpointer);
void keyPress (void);

uint8_t inputBuf;
int8_t *filename;
GCond *inputCond;
GMutex *inputLock;
GtkLabel *regLbls [10];
GtkLabel *statusLabel;
GtkTextBuffer *textBuf;
GtkTextView *textWindow;
GtkTextIter startIter, endIter;
GtkListStore *codeList, *memList;
GtkTreeView *codeView, *memView;
GtkTreeViewColumn *asmColumn;
GtkWidget *window;
GThread *machineThread;

int8_t initUI (int argc, char **argv) {
	g_thread_init (NULL);	
	gdk_threads_init ();	
	gdk_threads_enter ();

	GtkBuilder *builder;
	GtkButton *runButton;
	GtkButton *stepButton;
	GtkButton *resetButton;
	GtkButton *stopButton;
	GtkImageMenuItem *quitMenu, *loadMenu;
	GError *error = NULL;

	gtk_init (&argc, &argv);
	builder = gtk_builder_new ();

	if (!gtk_builder_add_from_file (builder, "ui2.glade", &error)) {
		g_error ("%s", error->message);
		g_free (error);
		return 1;
	}

	window = GTK_WIDGET (gtk_builder_get_object (builder, "window1"));
	runButton = (GtkButton *) GTK_WIDGET (gtk_builder_get_object (builder, "runButton"));
	stepButton = (GtkButton *) GTK_WIDGET (gtk_builder_get_object (builder, "stepButton"));
	resetButton = (GtkButton *) GTK_WIDGET (gtk_builder_get_object (builder, "resetButton"));
	stopButton = (GtkButton *) GTK_WIDGET (gtk_builder_get_object (builder, "stopButton"));
	codeView = (GtkTreeView *) GTK_WIDGET (gtk_builder_get_object (builder, "codeView"));
	//memView = (GtkTreeView *) GTK_WIDGET (gtk_builder_get_object (builder, "memView"));
	statusLabel = (GtkLabel *) GTK_WIDGET (gtk_builder_get_object (builder, "statusLabel"));
	regLbls [0] = (GtkLabel *) GTK_WIDGET (gtk_builder_get_object (builder, "regA"));
	regLbls [1] = (GtkLabel *) GTK_WIDGET (gtk_builder_get_object (builder, "regB"));
	regLbls [2] = (GtkLabel *) GTK_WIDGET (gtk_builder_get_object (builder, "regC"));
	regLbls [3] = (GtkLabel *) GTK_WIDGET (gtk_builder_get_object (builder, "regD"));
	regLbls [4] = (GtkLabel *) GTK_WIDGET (gtk_builder_get_object (builder, "regE"));
	regLbls [5] = (GtkLabel *) GTK_WIDGET (gtk_builder_get_object (builder, "regH"));
	regLbls [6] = (GtkLabel *) GTK_WIDGET (gtk_builder_get_object (builder, "regL"));
	regLbls [7] = (GtkLabel *) GTK_WIDGET (gtk_builder_get_object (builder, "regPC"));
	regLbls [8] = (GtkLabel *) GTK_WIDGET (gtk_builder_get_object (builder, "regSP"));
	regLbls [9] = (GtkLabel *) GTK_WIDGET (gtk_builder_get_object (builder, "regFLAGS"));
	quitMenu = (GtkImageMenuItem *) GTK_WIDGET (gtk_builder_get_object (builder, "quitMenu"));
	loadMenu = (GtkImageMenuItem *) GTK_WIDGET (gtk_builder_get_object (builder, "loadMenu"));

	textWindow = (GtkTextView *) GTK_WIDGET (gtk_builder_get_object (builder, "textWindow"));
	textBuf = gtk_text_view_get_buffer (textWindow);
	gtk_text_buffer_get_end_iter (textBuf, &endIter);
	//gtk_text_view_set_editable (textWindow, FALSE);

	codeList = gtk_list_store_new (NUM_COLUMNS, G_TYPE_INT, G_TYPE_UINT, G_TYPE_STRING);
	memList = gtk_list_store_new (4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

	gtk_builder_connect_signals (builder, NULL);
	g_object_unref (G_OBJECT (builder));

	g_signal_connect (window, "destroy", G_CALLBACK (quitProgram), NULL);
	g_signal_connect_swapped (runButton, "clicked", G_CALLBACK (runStepButtonClicked), GINT_TO_POINTER (0));
	g_signal_connect_swapped (stepButton, "clicked", G_CALLBACK (runStepButtonClicked), GINT_TO_POINTER (1));
	g_signal_connect (resetButton, "clicked", G_CALLBACK (resetButtonClicked), NULL);
	g_signal_connect (stopButton, "clicked", G_CALLBACK (stopButtonClicked), NULL);
	g_signal_connect (textWindow, "key-release-event", G_CALLBACK (keyPress), NULL);	
	g_signal_connect (quitMenu, "activate", G_CALLBACK (quitProgram), NULL);	
	g_signal_connect (loadMenu, "activate", G_CALLBACK (loadFileClicked), NULL);	
	g_signal_connect (codeView, "row-activated", G_CALLBACK (rowDoubleClicked), NULL);	

	inputLock = g_mutex_new ();
	bufLock = g_mutex_new ();
	stepLock = g_mutex_new ();
	inputCond = g_cond_new ();
	bufCond = g_cond_new ();
	stepCond = g_cond_new ();
	runLock = g_mutex_new ();
	runCond = g_cond_new ();

	filename = NULL;

	gtk_widget_show (window);
	gtk_main (); 
	gdk_threads_leave ();

	return 0;
}

void quitProgram (void) {
	stopProgram ();
	gtk_main_quit ();
}

void stopButtonClicked (void) {
	if ((programStatus & PROGRAM_STATUS_RUNNING) != PROGRAM_STATUS_RUNNING)
		return;
	programStatus &= ~PROGRAM_STATUS_RUNNING;
	programStatus |= PROGRAM_STATUS_DONE;
	updateCodeView ();
	updateRegLbls ();
	
	if ((programStatus & PROGRAM_STATUS_INPUT) == PROGRAM_STATUS_INPUT) 
		keyPress ();			/* If we're waiting for input, maunally call keypress to end the program */
}

void resetButtonClicked (void) {
	if ((programStatus & PROGRAM_STATUS_DONE) == PROGRAM_STATUS_DONE) {
		gtk_label_set_text (statusLabel, "Status: Ready");
		programStatus |= PROGRAM_STATUS_RUNNING;
		g_cond_signal (stepCond);
	}
}

void runStepButtonClicked (gpointer whichButton) {
	GtkMessageDialog *errorDialog;
	uint32_t stepButton = GPOINTER_TO_INT (whichButton);
	
	if ((programStatus & PROGRAM_STATUS_LOADED) != PROGRAM_STATUS_LOADED) {	/* No file loaded, return */
		errorDialog = (GtkMessageDialog *) gtk_message_dialog_new (GTK_WINDOW (window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
			GTK_BUTTONS_CLOSE, "Please load a file!");
		gtk_dialog_run (GTK_DIALOG (errorDialog));
		g_signal_connect (errorDialog, "response", G_CALLBACK (gtk_widget_destroy), errorDialog);
	}
	else if ((programStatus & PROGRAM_STATUS_DONE) == PROGRAM_STATUS_DONE && 	/* Program stopped but reset not clicked, return */
		(programStatus & PROGRAM_STATUS_RUNNING) != PROGRAM_STATUS_RUNNING)
		return;	
	else if ((programStatus & PROGRAM_STATUS_DONE) == PROGRAM_STATUS_DONE) {		/* Reset clicked, start running from the top */
		programStatus &= ~PROGRAM_STATUS_DONE;
		if (stepButton) {
			programStatus |= PROGRAM_STATUS_STEPPING;
			gtk_label_set_text (statusLabel, "Status: Stepping");
		}
		else {
			programStatus &= ~PROGRAM_STATUS_STEPPING;
			gtk_label_set_text (statusLabel, "Status: Running");
		}
		g_cond_signal (stepCond);
	}
	else if ((programStatus & PROGRAM_STATUS_RUNNING) != PROGRAM_STATUS_RUNNING) {	/* Not run yet, but loaded, now start running */
		programStatus |= PROGRAM_STATUS_RUNNING;
		if (stepButton) {
			programStatus |= PROGRAM_STATUS_STEPPING;
			gtk_label_set_text (statusLabel, "Status: Stepping");
		}
		else 
			gtk_label_set_text (statusLabel, "Status: Running");
		machineThread = g_thread_create ((GThreadFunc) runProgram, NULL, (gboolean) TRUE, NULL);
	}
	else if ((programStatus & PROGRAM_STATUS_BREAKPOINT) == PROGRAM_STATUS_BREAKPOINT) { /* Stopped at BP, start running again */
		programStatus &= ~PROGRAM_STATUS_BREAKPOINT;
		if (stepButton) {
			programStatus |= PROGRAM_STATUS_STEPPING;
			gtk_label_set_text (statusLabel, "Status: Stepping");
		}
		else {
			programStatus &= ~PROGRAM_STATUS_STEPPING;
			gtk_label_set_text (statusLabel, "Status: Running");
		}
		g_cond_signal (stepCond);
	}
	else if ((programStatus & PROGRAM_STATUS_STEPPING) == PROGRAM_STATUS_STEPPING && stepButton) {	/* If we're here, we're running and stepping so send step signal */
		g_cond_signal (stepCond);
	}
}

/* TODO: Clear all breakpoints menuitem */

void rowDoubleClicked (GtkTreeView *view, GtkTreePath *pathToRow, GtkTreeViewColumn *selectedColumn, gpointer data) {
	GtkTreeIter iter;
	GtkMessageDialog *errorDialog;
	uint32_t clickedAddr, currentTextWeight, i = 0;
	
	if (currentBreakpoint > 19) {
		errorDialog = (GtkMessageDialog *) gtk_message_dialog_new (GTK_WINDOW (window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
			GTK_BUTTONS_CLOSE, "Sorry, only 20 breakpoints allowed.");	
		gtk_dialog_run (GTK_DIALOG (errorDialog));
		g_signal_connect (errorDialog, "response", G_CALLBACK (gtk_widget_destroy), errorDialog);
		return;
	}
	
	gtk_tree_model_get_iter (gtk_tree_view_get_model (codeView), &iter, pathToRow);
	gtk_tree_model_get (gtk_tree_view_get_model (codeView), &iter, ADDR_COLUMN, &clickedAddr, -1);
	gtk_tree_model_get (gtk_tree_view_get_model (codeView), &iter, ROW_BOLD_COLUMN, &currentTextWeight, -1);
	
	if (currentTextWeight == 400) {		/* Not currently set as a breakpoint */
		gtk_list_store_set (GTK_LIST_STORE (gtk_tree_view_get_model (codeView)), &iter, ROW_BOLD_COLUMN, 800, -1);
		breakpoints [currentBreakpoint] = clickedAddr;
		currentBreakpoint++;
	}
	else {
		gtk_list_store_set (GTK_LIST_STORE (gtk_tree_view_get_model (codeView)), &iter, ROW_BOLD_COLUMN, 400, -1);
		while (breakpoints [i] != clickedAddr) 
			i++;
		breakpoints [i] = 0;		/* Incase this is the only breakpoint, make sure it gets set to 0 */
		for (; i < currentBreakpoint - 1; i++)
			breakpoints [i] = breakpoints [i+1];
		currentBreakpoint--;
	}
}

void loadFileClicked (void) {
	GtkMessageDialog *errorDialog;

#ifdef G_OS_WIN32
	filename = g_malloc0 (256);
	win32FileSelect (filename);
#else

	GtkFileFilter *hexFilter;
	GtkWidget *fileChooser;
	
	hexFilter = gtk_file_filter_new ();
	fileChooser = gtk_file_chooser_dialog_new ("Open File", (GtkWindow *) window, GTK_FILE_CHOOSER_ACTION_OPEN, 
						GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_filter_add_pattern (hexFilter, "*.hex");
	gtk_file_filter_set_name (hexFilter, "Hex file");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (fileChooser), hexFilter);
	
	if ((programStatus & PROGRAM_STATUS_RUNNING) == PROGRAM_STATUS_RUNNING ||
		(programStatus & PROGRAM_STATUS_DONE) == PROGRAM_STATUS_DONE) {
		stopButtonClicked ();	
		gtk_list_store_clear (codeList);
		gtk_tree_view_remove_column (codeView, asmColumn);
		resetButtonClicked ();
	}
	
	if (gtk_dialog_run (GTK_DIALOG (fileChooser)) == GTK_RESPONSE_ACCEPT) 
		filename = (int8_t *) gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fileChooser));	
	else {
		gtk_widget_destroy (fileChooser);
		return;
	}
	
	gtk_widget_destroy (fileChooser);
	
#endif
	
	if (loadProgram (filename) == -1) {
		errorDialog = (GtkMessageDialog *) gtk_message_dialog_new (GTK_WINDOW (window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
			GTK_BUTTONS_CLOSE, "Error loading file.");	
		gtk_dialog_run (GTK_DIALOG (errorDialog));
		g_signal_connect (errorDialog, "response", G_CALLBACK (gtk_widget_destroy), errorDialog);
		return;
	}
	
	gtk_label_set_text (statusLabel, "Status: Ready");
	populateCodeList ();
	g_free (filename);
	
	programStatus |= PROGRAM_STATUS_LOADED;
}

gboolean updateStatus (gpointer status) {
	if (GPOINTER_TO_INT (status) == 1)
		gtk_label_set_text (statusLabel, "Status: Done running");
	else if (GPOINTER_TO_INT (status) == 2)
		gtk_label_set_text (statusLabel, "Status: Waiting for input");
	else if (GPOINTER_TO_INT (status) == 3)
		gtk_label_set_text (statusLabel, "Status: Breakpoint reached");
	else if (GPOINTER_TO_INT (status) == 4)
		gtk_label_set_text (statusLabel, "Status: Stepping");
	
	return FALSE;
}

gboolean populateCodeList (void) {
	uint32_t oldAddr, addr = 0x100;
	uint8_t op;
	uint16_t imm16;
	gchar *buf;
	uint16_t totalMemSize = getTotalMemSize ();

	GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
	GtkTreeIter codeListIter;

	while (addr-0x100 < totalMemSize) {
		op = getByte (addr);
		oldAddr = addr;
		if (instrs [op].size > 1) {
			addr++;
			imm16 = getByte (addr);
			if (instrs [op].size == 3) {
				addr++;
				imm16 |= getByte (addr) << 8;
				buf = g_strdup_printf ("%04X:\t%02x %04x\t%s %04x", oldAddr, op, imm16, instrs [op].name, imm16);
			}
			else {
				buf = g_strdup_printf ("%04X:\t%02x %02x\t%s %02x", oldAddr, op, imm16, instrs [op].name, imm16);
			}
		}
		else {
			buf = g_strdup_printf ("%04X:\t%02x\t%s", oldAddr, op, instrs [op].name);
		}	
		
		gtk_list_store_append (codeList, &codeListIter);
		gtk_list_store_set (codeList, &codeListIter, ROW_BOLD_COLUMN, 400, ADDR_COLUMN, oldAddr,
			ASM_COLUMN, buf, -1);
	
		addr++;
	}

	asmColumn = gtk_tree_view_column_new_with_attributes ("", renderer, "text", ASM_COLUMN, "weight", ROW_BOLD_COLUMN, NULL);
	gtk_tree_view_append_column (codeView, asmColumn);
	
	gtk_tree_view_set_model (codeView, (GtkTreeModel *) codeList);
	gtk_tree_view_set_grid_lines (codeView, GTK_TREE_VIEW_GRID_LINES_HORIZONTAL);
	
	g_free (buf);

	return FALSE;
}

gboolean updateCodeView (void) {
	GtkTreeIter codeIter;
	GtkTreeModel *model;
	GtkTreePath *path;
	uint8_t regs [13];
	uint16_t addr;
	uint32_t currentAddr = 0;
	
	getRegs (regs);
	addr = (regs [7] << 8) | regs [8];
	
	model = gtk_tree_view_get_model (codeView);
	gtk_tree_model_get_iter_first (model, &codeIter);

	while (1) {
		gtk_tree_model_get (model, &codeIter, ADDR_COLUMN, &currentAddr, -1);
		
		if (currentAddr == addr) {
			break;
		}
		if (!gtk_tree_model_iter_next (model, &codeIter)) {
			g_print ("Couldn't find addr\n");
			break;
		}
	}
	
	path = gtk_tree_model_get_path (model, &codeIter);
	//if ((programStatus & PROGRAM_STATUS_STEPPING) == PROGRAM_STATUS_STEPPING)
		gtk_tree_path_prev (path);	/* Since PC is updated before updating code view, the current PC 
									actually points to the next instruction.  So, move one back */
	gtk_tree_view_scroll_to_cell (codeView, path, asmColumn, FALSE, 0, 0);
	gtk_tree_view_set_cursor (codeView, path, asmColumn, FALSE);

	return FALSE;
}

gboolean updateRegLbls (void) {	
	uint8_t i;
	uint8_t buf [3];
	uint8_t regs [13];
	getRegs (regs);

	for (i = 0; i < 7; i++) {
		sprintf ((char *) buf, "%02X", regs [i]);
		gtk_label_set_text (regLbls [i], (gchar *) buf);
	}
	sprintf ((char *) buf, "%04X", (regs [7] << 8) | regs [8]);
	gtk_label_set_text (regLbls [7], (gchar *) buf);
	sprintf ((char *) buf, "%04X", (regs [9] << 8) | regs [10]);
	gtk_label_set_text (regLbls [8], (gchar *) buf);
	sprintf ((char *) buf, "%02X", regs [11]);
	gtk_label_set_text (regLbls [9], (gchar *) buf);

	return FALSE;
}

gboolean writeBuf (uint8_t *buf) {
	g_mutex_lock (bufLock);
	uint8_t len = buf [0];
	gtk_text_buffer_insert_at_cursor (textBuf, (gchar *) buf+1, len);

	g_mutex_unlock (bufLock);
	g_cond_signal (bufCond);

	return FALSE;
}

void keyPress (void) {
	g_mutex_lock (inputLock);								//Lock input mutex until we're done getting the new letter
	gtk_text_buffer_get_end_iter (textBuf, &startIter);					//Get iter at end of buf
	gtk_text_iter_backward_char (&startIter);						//Move back one space	
	gtk_text_buffer_get_end_iter (textBuf, &endIter);					//And get the iter at the end again
	uint8_t temp = *((uint8_t *) gtk_text_buffer_get_text (textBuf, &startIter, &endIter, FALSE));		//Get the character between the two (the last char in the buf)
	if (temp == 0xA || temp == 0xD) {							//convert lf to cr and move back a line (only change lines if the user tells us to!)
		temp = 0xD;
		gtk_text_buffer_delete (textBuf, &startIter, &endIter);
	}
	inputBuf = temp;
	g_mutex_unlock (inputLock);								//inputBuf has been updated, unlock the mutex
	g_cond_signal (inputCond);								//and signal the waiting thread that its input is ready
}

uint8_t readChar (void) {
	programStatus |= PROGRAM_STATUS_INPUT;
	g_idle_add ((GSourceFunc) updateStatus, GINT_TO_POINTER (2));	
	g_mutex_lock (inputLock);								//lock the input mutex
	g_idle_add ((GSourceFunc) updateRegLbls, NULL);		/* Read char is called from machine thread,  */
	g_idle_add ((GSourceFunc) updateCodeView, NULL);	/* so we can't call GTK stuff directly */
												//let the user type in the text window
	g_cond_wait (inputCond, inputLock);							//unlocks the input mutex, then the thread waits until receiving a signal
												//at which point the input mutex is automatically locked again, and we make the text window
	g_mutex_unlock (inputLock);								//uneditable.  Then unlock the input mutex, and return the new character.
	
	programStatus &= ~PROGRAM_STATUS_INPUT;
	
	if ((programStatus & PROGRAM_STATUS_STEPPING) == PROGRAM_STATUS_STEPPING)
		g_idle_add ((GSourceFunc) updateStatus, GINT_TO_POINTER (4));
		
	return inputBuf;
}

/*gboolean populateMemList () { 
	uint8_t i;
	uint16_t addr;
	uint8_t addrBuf [7];
	uint8_t memBuf [50];
	uint8_t asciiBuf [17];
	uint8_t numZeroLines = 0;
	uint8_t zeroLine = 0;

	GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
	GtkTreeIter memListIter;
	GtkTreeViewColumn *addrColumn, *memColumn, *asciiColumn;

	for (addr = 0; addr < 0x1000; addr += 0x10) {
		sprintf ((char *) addrBuf, "%04X:\t", addr);
		
		zeroLine = 0;
		for (i = 0; i < 16; i++) {
			if (memory [addr+i] >= ' ' && memory [addr+i] <= '~') {
				asciiBuf [i] = memory [addr+i];							
				zeroLine = 1;
			}
			else {
				if (memory [addr+i] != 0) {
					zeroLine = 1;
				}
				asciiBuf [i] = '.';			
			}
		}
		asciiBuf [16] = 0;

		if (addr == 0x0FF0 && zeroLine == 0) {
			zeroLine = 1;
			numZeroLines++;
		}
		if (zeroLine == 0) {
			numZeroLines++;
		}
		else {
			if (numZeroLines > 4) {
				for (i = numZeroLines; i > 3; i--) {
					gtk_tree_model_iter_previous ((GtkTreeModel *) memList, &memListIter);
					gtk_list_store_remove (memList, &memListIter);
				}
				if (addr != 0x0FF0) {
					gtk_tree_model_iter_previous ((GtkTreeModel *) memList, &memListIter);
				}
				gtk_list_store_set (memList, &memListIter, 0, (gchar *) "*", 1, (gchar *) "", 2, (gchar *) "", 3, (gchar *) "Monospaced 11", -1);		
				gtk_tree_model_iter_next ((GtkTreeModel *) memList, &memListIter);
				numZeroLines = 0;
			}
		}

		sprintf ((char *) memBuf, "%02x %02x %02x %02x %02x %02x %02x %02x\t%02x %02x %02x %02x %02x %02x %02x %02x", 
			memory [addr], memory [addr+1], memory [addr+2], memory [addr+3], memory [addr+4], memory [addr+5], memory [addr+6], memory [addr+7], 
			memory [addr+8], memory [addr+9], memory [addr+10], memory [addr+11], memory [addr+12], memory [addr+13], memory [addr+14], memory [addr+15]);

		gtk_list_store_append (memList, &memListIter);
		gtk_list_store_set (memList, &memListIter, 0, (gchar *) addrBuf, 1, (gchar *) memBuf, 2, (gchar *) asciiBuf, 3, (gchar *) "Monospaced 11", -1);		
		
	}

	addrColumn = gtk_tree_view_column_new_with_attributes ("", renderer, "text", 0, "font", 3, NULL);
	memColumn = gtk_tree_view_column_new_with_attributes ("", renderer, "text", 1, NULL);
	asciiColumn = gtk_tree_view_column_new_with_attributes ("", renderer, "text", 2, NULL);

	gtk_tree_view_append_column (memView, addrColumn);
	gtk_tree_view_append_column (memView, memColumn);
	gtk_tree_view_append_column (memView, asciiColumn);
	
	gtk_tree_view_set_model (memView, (GtkTreeModel *) memList);
	gtk_tree_view_set_grid_lines (memView, GTK_TREE_VIEW_GRID_LINES_BOTH);

	return FALSE;
}*/
