/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    aneditor-priv.h
    Copyright (C) 2004 Naba Kumar

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef __ANEDITOR_PRIV_H__
#define __ANEDITOR_PRIV_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <locale.h>

#include <gnome.h>

#define GTK
#include "Platform.h"
#include "PropSet.h"
#include "Accessor.h"
#include "WindowAccessor.h"
#include "KeyWords.h"
#include "Scintilla.h"
#include "ScintillaWidget.h"
#include "SciLexer.h"
#include "lexer.h"
#include "properties_cxx.h"
#include "properties.h"
#include "aneditor.h"

#include <libanjuta/resources.h>

#include "tm_tagmanager.h"

#include <string>
#include <memory>

using namespace std;

// #include "debugger.h"

#if 0

#include "sv_unknown.xpm"
#include "sv_class.xpm"
#include "sv_function.xpm"
#include "sv_macro.xpm"
#include "sv_private_fun.xpm"
#include "sv_private_var.xpm"
#include "sv_protected_fun.xpm"
#include "sv_protected_var.xpm"
#include "sv_public_fun.xpm"
#include "sv_public_var.xpm"
#include "sv_struct.xpm"
#include "sv_variable.xpm"

#endif

// #define DEBUG

#define ANE_MARKER_BOOKMARK 0
#define MAX_PATH 260
#define MAXIMUM(x, y)	((x>y)?x:y)
#define MINIMUM(x,y)	((x<y)?x:y)

#define MAX_AUTOCOMPLETE_WORDS 50 /* Maximum number of autocomplete words to show */

/* Colour has mysteriously vanished from scintilla */
/* Using the substitute */
typedef class ColourDesired Colour;

template<int sz>
class EntryMemory {
	SString entries[sz];
public:
	void Insert(SString s) {
		for (int i=0;i<sz;i++) {
			if (entries[i] == s) {
				for (int j=i;j>0;j--) {
					entries[j] = entries[j-1];
				}
				entries[0] = s;
				return;
			}
		}
		for (int k=sz-1;k>0;k--) {
			entries[k] = entries[k-1];
		}
		entries[0] = s;
	}
	int Length() const {
		int len = 0;
		for (int i=0;i<sz;i++)
			if (entries[i].length())
				len++;
		return len;
	}
	SString At(int n)const {
		return entries[n];
	}
};

typedef EntryMemory<10> ComboMemory;

struct StyleAndWords {
	int styleNumber;
	SString words;
	bool IsEmpty() { return words.length() == 0; }
	bool IsSingleChar() { return words.length() == 1; }
};

// will contain the parameters for the recursive function completion
typedef struct _CallTipNode {
	int startCalltipWord;
	int def_index;
	int max_def;
	SString functionDefinition[20];
	int rootlen;
	int start_pos;					// start position in editor
	int call_tip_start_pos;		// start position in calltip
	
} CallTipNode, *CallTipNode_ptr;

// Related to Utf8_16::encodingType but with additional values at end
enum UniMode {
	uni8Bit=0, uni16BE=1, uni16LE=2, uniUTF8=3,
	uniCookie=4
};

// Codes representing the effect a line has on indentation.
enum IndentationStatus {
	isNone,		// no effect on indentation
	isBlockStart,	// indentation block begin such as "{" or VB "function"
	isBlockEnd,	// indentation end indicator such as "}" or VB "end"
	isKeyWordStart	// Keywords that cause indentation
};

class AnEditor {

protected:
	char fileName[MAX_PATH+20]; // filename kept here temporarily

	ComboMemory memFinds;
	ComboMemory memReplaces;
	ComboMemory memFiles;

	int characterSet;
	SString language;
	int lexLanguage;
	SString overrideExtension;	// User has chosen to use a particular language
	enum {numWordLists=5};
	GtkAccelGroup* accelGroup;

	int indentSize;
	bool indentOpening;
	bool indentClosing;
	bool indentMaintain;
	int statementLookback;
	StyleAndWords statementIndent;
	StyleAndWords statementEnd;
	StyleAndWords blockStart;
	StyleAndWords blockEnd;
	enum { noPPC, ppcStart, ppcMiddle, ppcEnd, ppcDummy };	///< Indicate the kind of preprocessor condition line
	char preprocessorSymbol;	///< Preprocessor symbol (in C: #)
	WordList preprocCondStart;	///< List of preprocessor conditional start keywords (in C: if ifdef ifndef)
	WordList preprocCondMiddle;	///< List of preprocessor conditional middle keywords (in C: else elif)
	WordList preprocCondEnd;	///< List of preprocessor conditional end keywords (in C: endif)

	Window wEditor;

	SciFnDirect fnEditor;
	long ptrEditor;
	SciFnDirect fnOutput;
	long ptrOutput;
	bool tbVisible;
	bool tabVisible;
	bool tabMultiLine;
	bool sbVisible;
	int visHeightTools;
	int visHeightTab;
	int visHeightStatus;
	int visHeightEditor;
	int heightBar;

	int heightOutput;
	int heightOutputStartDrag;
	Point ptStartDrag;
	bool capturedMouse;
	bool firstPropertiesRead;
	bool splitVertical;
	bool bufferedDraw;
	bool bracesCheck;
	bool bracesSloppy;
	int bracesStyle;
	int braceCount;
	SString sbValue;
	bool wrapLine;
	bool isReadOnly;

	bool indentationWSVisible;

	bool autoCompleteIgnoreCase;
	bool callTipIgnoreCase;
	bool autoCCausedByOnlyOne;
	SString calltipWordCharacters;
	SString calltipEndDefinition;
	SString autoCompleteStartCharacters;
	SString autoCompleteFillUpCharacters;
	SString wordCharacters;
	int startCalltipWord;
	
	GQueue *call_tip_node_queue;
	CallTipNode call_tip_node;
	
	GCompletion *autocompletion;
	
	// needed for calltips caret moving
	int lastPos;
	
	bool margin;
	int marginWidth;
	enum { marginWidthDefault = 20};

	bool foldMargin;
	int foldMarginWidth;
	enum { foldMarginWidthDefault = 14};

	bool lineNumbers;
	int lineNumbersWidth;
	enum { lineNumbersWidthDefault = 40};

	bool usePalette;
	bool clearBeforeExecute;
	bool allowMenuActions;
	bool isDirty;
	
	bool calltipShown;
	bool debugTipOn;
	static AnEditorID focusedID;
	friend void aneditor_set_focused_ed_ID(AnEditorID id);
	friend void eval_output_arrived_for_aneditor(GList* lines, gpointer data);

	PropSetFile *props;

	int LengthDocument();
	int GetCaretInLine();
	void GetLine(SString & text, int line = -1);
	int  GetFullLine(SString & text, int line = -1);
	void GetRange(Window &win, int start, int end, char *text);
	int IsLinePreprocessorCondition(const char *line);
	bool FindMatchingPreprocessorCondition(int &curLine, int direction,
										   int condEnd1, int condEnd2);
	bool FindMatchingPreprocCondPosition(bool isForward, int &mppcAtCaret,
										 int &mppcMatch);
	bool FindMatchingBracePosition(bool editor, int &braceAtCaret,
								   int &braceOpposite, bool sloppy);
	void BraceMatch(bool editor);

	bool GetCurrentWord(char* buffer, int maxlength);

	bool FindWordInRegion(char *buffer, int maxlength, SString &linebuf,
						  int current);
	bool GetWordAtPosition(char* buffer, int maxlength, int pos);

	void IndentationIncrease();
	void IndentationDecrease();
	void ClearDocument();
	void CountLineEnds(int &linesCR, int &linesLF, int &linesCRLF);
	CharacterRange GetSelection();
	void SelectionWord(char *word, int len);
	void WordSelect();
	void LineSelect();
	void SelectionIntoProperties();
	long Find (long flags, char* text);
	bool HandleXml(char ch);
	SString FindOpenXmlTag(const char sel[], int nSize);
	void GoMatchingBrace(bool select);
	void GetRange(guint start, guint end, gchar *text, gboolean styled);
	bool StartCallTip_new();
	void ContinueCallTip_new();
	void SaveCallTip();
	void ResumeCallTip(bool pop_from_stack = true);
	void ShutDownCallTip();
	void SetCallTipDefaults( );
	void CompleteCallTip();

	TMTag ** FindTypeInLocalWords(GPtrArray *CurrentFileTags, const char *root,
								  const bool type, bool *retptr, int *count);
	bool SendAutoCompleteRecordsFields(const GPtrArray *CurrentFileTags,
									   const char *ScanType);
	bool StartAutoCompleteRecordsFields(char ch);
 	bool StartAutoComplete();
	bool StartAutoCompleteWord(int autoShowCount);
	bool StartBlockComment();
	bool CanBeCommented(bool box_stream);
	bool StartBoxComment();
	bool StartStreamComment();
	SString GetMode(SString language); 
	bool InsertCustomIndent();

	unsigned int GetLinePartsInStyle(int line, int style1, int style2,
									 SString sv[], int len);
	void SetLineIndentation(int line, int indent);
	int GetLineIndentation(int line);
	int GetLineIndentPosition(int line);
	IndentationStatus GetIndentState(int line);
	int IndentOfBlockProper(int line);
	int IndentOfBlock(int line);
	void MaintainIndentation(char ch);
	void AutomaticIndentation(char ch);
	void CharAdded(char ch);
	void FoldChanged(int line, int levelNow, int levelPrev);
	void FoldChanged(int position);
	void Expand(int &line, bool doExpand, bool force=false, 
				int visLevels=0, int level=-1);
	bool MarginClick(int position,int modifiers);
	void HandleDwellStart(int mousePos);
	void Notify(SCNotification *notification);
	static void NotifySignal(GtkWidget *w, gint wParam, gpointer lParam,
							 AnEditor *scitew);
	int KeyPress (unsigned int state, unsigned int keyval);
	static gint KeyPressEvent(GtkWidget *w, GdkEventKey *event,
							  AnEditor *editor);

	void BookmarkToggle( int lineno = -1 );
	void BookmarkFirst();
	void BookmarkPrev();
	void BookmarkNext();
	void BookmarkLast();
	void BookmarkClear();

	void AssignKey(int key, int mods, int cmd);
	StyleAndWords GetStyleAndWords(const char *base);
	void SetOneStyle(Window &win, int style, const char *s);
	void SetStyleFor(Window &win, const char *language);
	SString ExtensionFileName();
	void Command(int command, unsigned long wParam, long lparam);
	void ForwardPropertyToEditor(const char *key);
	SString FindLanguageProperty(const char *pattern,
								 const char *defaultValue="");
	void ReadPropertiesInitial();
	void ReadProperties(const char* fileForExt);
	long SendEditor(unsigned int msg, unsigned long wParam=0, long lParam=0);
	long SendEditorString(unsigned int msg, unsigned long wParam,
						  const char *s);
	void SetOverrideLanguage(int ID);
	void ViewWhitespace(bool view);
	bool RangeIsAllWhitespace(int start, int end);
	void SaveToHTML(const char *saveName);
	void SaveToRTF(const char *saveName);
	void SetSelection(int anchor, int currentPos);
	int GetLineLength(int line);
	int GetCurrentLineNumber();
	int GetCurrentScrollPosition();
	void FoldOpenAll();
	void FoldCloseAll();
	void FoldCode(bool expanding);
	void FoldToggle();
	void SelectBlock ();
	int GetBlockStartLine(int lineno = -1);
	int GetBlockEndLine(int lineno = -1);
	void GotoBlockStart();
	void GotoBlockEnd();
	void EnsureRangeVisible(int posStart, int posEnd);
	void SetAccelGroup(GtkAccelGroup* acl);
	int GetBookmarkLine( const int nLineStart );
	void DefineMarker(int marker, int makerType, Colour fore, Colour back);
	void SetLineWrap(bool wrap);
	void SetReadOnly(bool readonly);
	void SetFoldSymbols(SString fold_symbols);
	bool iswordcharforsel(char ch);
	
public:

	AnEditor(PropSetFile* p);
	~AnEditor();
	WindowID GetID() { return wEditor.GetID(); }
	long Command(int cmdID, long wParam, long lParam);

	void FocusInEvent(GtkWidget* widget);
	void FocusOutEvent(GtkWidget* widget);
	void EvalOutputArrived(GList* lines, int textPos,
						   const string &expression);
	void EndDebugEval();
	void SetParent(AnEditor *parent);
};

class ExpressionEvaluationTipInfo
// Utility class to help displaying expression values in tips.
{
public:
	AnEditor *editor;
	int textPos;
	string expression;

	ExpressionEvaluationTipInfo(AnEditor *e, int pos, const string &expr)
	  : editor(e), textPos(pos), expression(expr) {}
};

#endif
