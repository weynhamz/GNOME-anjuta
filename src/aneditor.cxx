/*
    aneditor.cxx
    Copyright (C) 2000  Kh. Naba Kumar Singh

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
/*
 * Most of the code stolen from SciTE and heavily modified.
 * If code sections are later imported from SciTE, utmost care
 * should be taken to ensure that it does not conflict with the present code.
 */

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

/* Use Gtk+ */
#include <gtk/gtk.h>

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
#include "properties.h"
#include "aneditor.h"

#include "tm_tagmanager.h"

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
};

class AnEditor {

protected:
	char fileName[MAX_PATH+20]; // filename kept here temporarily

	ComboMemory memFinds;
	ComboMemory memReplaces;
	ComboMemory memFiles;

	int codePage;
	int characterSet;
	SString language;
	int lexLanguage;
	SString overrideExtension;	// User has chosen to use a particular language
	enum {numWordLists=5};
	SString functionDefinition;
	GtkAccelGroup* accelGroup;

	int indentSize;
	bool indentOpening;
	bool indentClosing;
	int statementLookback;
	StyleAndWords statementIndent;
	StyleAndWords statementEnd;
	StyleAndWords blockStart;
	StyleAndWords blockEnd;

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
	int visHeightStatus;int visHeightEditor;
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


	bool indentationWSVisible;

	bool autoCompleteIgnoreCase;
	bool callTipIgnoreCase;
	
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

	PropSetFile *props;

	int LengthDocument();
	int GetLine(char *text, int sizeText, int line=-1);
	void GetRange(Window &win, int start, int end, char *text);
	bool FindMatchingBracePosition(int &braceAtCaret, int &braceOpposite, bool sloppy);
	void BraceMatch();
	bool GetCurrentWord(char* buffer, int maxlength);

	void IndentationIncrease();
	void IndentationDecrease();
	void ClearDocument();
	void CountLineEnds(int &linesCR, int &linesLF, int &linesCRLF);
	CharacterRange GetSelection();
	void SelectionWord(char *word, int len);
	void SelectionIntoProperties();
	long Find (long flags, char* text);
	void GoMatchingBrace(bool select);
	void GetRange(guint start, guint end, gchar *text);
	bool StartCallTip();
	void ContinueCallTip();
	bool StartAutoComplete();
	bool StartAutoCompleteWord();
	void GetLinePartsInStyle(int line, int style1, int style2, SString sv[], int len);
	int SetLineIndentation(int line, int indent);
	int GetLineIndentation(int line);
	int GetLineIndentPosition(int line);
	int GetIndentState(int line);
	void AutomaticIndentation(char ch);
	void CharAdded(char ch);
	void FoldChanged(int line, int levelNow, int levelPrev);
	void FoldChanged(int position);
	void Expand(int &line, bool doExpand, bool force=false, 
		int visLevels=0, int level=-1);
	bool MarginClick(int position,int modifiers);
	void Notify(SCNotification *notification);

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
	void ReadPropertiesInitial();
	static void NotifySignal(GtkWidget *w, gint wParam, gpointer lParam, AnEditor *scitew);
	SString ExtensionFileName();
	void Command(int command, unsigned long wParam, long lparam);
	void ReadProperties(const char* fileForExt);
	long SendEditor(unsigned int msg, unsigned long wParam=0, long lParam=0);
	long SendEditorString(unsigned int msg, unsigned long wParam, const char *s);
	void SetOverrideLanguage(int ID);
	void ViewWhitespace(bool view);
	bool RangeIsAllWhitespace(int start, int end);
	void SaveToHTML(const char *saveName);
	void SaveToRTF(const char *saveName);
	void SetSelection(int anchor, int currentPos);
	int GetCurrentLineNumber();
	int GetCurrentScrollPosition();
	void FoldOpenAll();
	void FoldCloseAll();
	void FoldCode(bool expanding);
	void FoldToggle();
	void SelectBlock ();
	int GetBlockStartLine();
	int GetBlockEndLine();
	void GotoBlockStart();
	void GotoBlockEnd();
	void EnsureRangeVisible(int posStart, int posEnd);
	void SetAccelGroup(GtkAccelGroup* acl);
	int GetBookmarkLine( const int nLineStart );
	void DefineMarker(int marker, int makerType, Colour fore, Colour back);
	
public:

	AnEditor(PropSetFile* p);
	~AnEditor();
	WindowID GetID() { return wEditor.GetID(); }
	long Command(int cmdID, long wParam, long lParam);
};

static void lowerCaseString(char *s);

static const char *extList[] = {
    "", "x", "x.cpp", "x.cpp", "x.html", "x.xml", "x.js", "x.vbs", "x.mak", "x.java",
    "x.lua", "x.py", "x.pl", "x.sql", "x.spec", "x.php3", "x.tex", "x.diff", "x.pas",
    "x.cs", "x.properties", "x.conf", "x.bc", "x.adb"
};

AnEditor::AnEditor(PropSetFile* p) {

	characterSet = 0;
	language = "java";
	lexLanguage = SCLEX_CPP;
	functionDefinition = 0;
	indentSize = 8;
	indentOpening = true;
	indentClosing = true;
	statementLookback = 10;

	fnEditor = 0;
	ptrEditor = 0;
	fnOutput = 0;
	ptrOutput = 0;
	tbVisible = false;
	sbVisible = false;
	tabVisible = false;
	tabMultiLine = false;
	visHeightTools = 0;
	visHeightStatus = 0;
	visHeightEditor = 1;
	heightBar = 7;

	heightOutput = 0;

	allowMenuActions = true;
	isDirty = false;

	ptStartDrag.x = 0;
	ptStartDrag.y = 0;
	capturedMouse = false;
	firstPropertiesRead = true;
	splitVertical = false;
	bufferedDraw = true;
	bracesCheck = true;
	bracesSloppy = false;
	bracesStyle = 0;
	braceCount = 0;

	indentationWSVisible = true;

	autoCompleteIgnoreCase = false;
	callTipIgnoreCase = false;

	margin = false;
	marginWidth = marginWidthDefault;
	foldMargin = true;
	foldMarginWidth = foldMarginWidthDefault;
	lineNumbers = false;
	lineNumbersWidth = lineNumbersWidthDefault;
	usePalette = false;

	accelGroup = NULL;

	fileName[0] = '\0';
	props = p;
	
	wEditor = scintilla_new();
	scintilla_set_id(SCINTILLA(wEditor.GetID()), 0);

	fnEditor = reinterpret_cast<SciFnDirect>(Platform::SendScintilla(
		wEditor.GetID(), SCI_GETDIRECTFUNCTION, 0, 0));

	ptrEditor = Platform::SendScintilla(wEditor.GetID(), 
		SCI_GETDIRECTPOINTER, 0, 0);

	gtk_signal_connect(GTK_OBJECT(wEditor.GetID()), "notify",
		GtkSignalFunc(NotifySignal), this);

	/* We will handle all accels ourself */
	/* SendEditor(SCI_CLEARALLCMDKEYS); */

	/*We have got our own popup menu */
	SendEditor(SCI_USEPOPUP, false);
	/* Set default editor mode */
	SendEditor(SCI_SETEOLMODE, SC_EOL_LF);
}

void
AnEditor::SetAccelGroup(GtkAccelGroup* acl) {
	accelGroup = acl;
}

AnEditor::~AnEditor() {
}

long AnEditor::SendEditor(unsigned int msg, unsigned long wParam, long lParam) {
	return fnEditor(ptrEditor, msg, wParam, lParam);
}

long AnEditor::SendEditorString(unsigned int msg, unsigned long wParam, const char *s) {
	return SendEditor(msg, wParam, reinterpret_cast<long>(s));
}

void AnEditor::ViewWhitespace(bool view) {
	if (view && indentationWSVisible)
		SendEditor(SCI_SETVIEWWS, SCWS_VISIBLEALWAYS);
	else if (view)
		SendEditor(SCI_SETVIEWWS, SCWS_VISIBLEAFTERINDENT);
	else
		SendEditor(SCI_SETVIEWWS, SCWS_INVISIBLE);
}

StyleAndWords AnEditor::GetStyleAndWords(const char *base) {
	StyleAndWords sw;
	SString fileNameForExtension = ExtensionFileName();
	SString sAndW = props->GetNewExpand(base, fileNameForExtension.c_str());
	sw.styleNumber = sAndW.value();
	const char *space = strchr(sAndW.c_str(), ' ');
	if (space)
		sw.words = space + 1;
	return sw;
}

void AnEditor::AssignKey(int key, int mods, int cmd) {
	SendEditor(SCI_ASSIGNCMDKEY,
	           Platform::LongFromTwoShorts(static_cast<short>(key),
	                                       static_cast<short>(mods)), cmd);
}

void AnEditor::SetOverrideLanguage(int ID) {
	overrideExtension = extList[ID - TE_LEXER_BASE];
}

int AnEditor::LengthDocument() {
	return SendEditor(SCI_GETLENGTH);
}

int AnEditor::GetLine(char *text, int sizeText, int line) {
	if (line == -1) {
		return SendEditor(SCI_GETCURLINE, sizeText, reinterpret_cast<long>(text));
	} else {
		short buflen = static_cast<short>(sizeText);
		memcpy(text, &buflen, sizeof(buflen));
		return SendEditor(SCI_GETLINE, line, reinterpret_cast<long>(text));
	}
}

void AnEditor::GetRange(Window &win, int start, int end, char *text) {
	TextRange tr;
	tr.chrg.cpMin = start;
	tr.chrg.cpMax = end;
	tr.lpstrText = text;
	Platform::SendScintilla(win.GetID(), SCI_GETTEXTRANGE, 0, reinterpret_cast<long>(&tr));
}

// Find if there is a brace next to the caret, checking before caret first, then
// after caret. If brace found also find its matching brace.
// Returns true if inside a bracket pair.
bool AnEditor::FindMatchingBracePosition(int &braceAtCaret, int &braceOpposite, bool sloppy) {
	bool isInside = false;
	Window &win = wEditor;
	int bracesStyleCheck = bracesStyle;
	int caretPos = Platform::SendScintilla(win.GetID(), SCI_GETCURRENTPOS, 0, 0);
	braceAtCaret = -1;
	braceOpposite = -1;
	char charBefore = '\0';
	char styleBefore = '\0';
	WindowAccessor acc(win.GetID(), *props);
	if (caretPos > 0) {
		charBefore = acc[caretPos - 1];
		styleBefore = static_cast<char>(acc.StyleAt(caretPos - 1) & 31);
	}
	// Priority goes to character before caret
	if (charBefore && strchr("[](){}", charBefore) &&
	        ((styleBefore == bracesStyleCheck) || (!bracesStyle))) {
		braceAtCaret = caretPos - 1;
	}
	bool colonMode = false;
	if (lexLanguage == SCLEX_PYTHON && ':' == charBefore) {
		braceAtCaret = caretPos - 1;
		colonMode = true;
	}
	bool isAfter = true;
	if (sloppy && (braceAtCaret < 0)) {
		// No brace found so check other side
		char charAfter = acc[caretPos];
		char styleAfter = static_cast<char>(acc.StyleAt(caretPos) & 31);
		if (charAfter && strchr("[](){}", charAfter) && (styleAfter == bracesStyleCheck)) {
			braceAtCaret = caretPos;
			isAfter = false;
		}
		if (lexLanguage == SCLEX_PYTHON && ':' == charAfter) {
			braceAtCaret = caretPos;
			colonMode = true;
		}
	}
	if (braceAtCaret >= 0) {
		if (colonMode) {
			int lineStart = Platform::SendScintilla(win.GetID(), SCI_LINEFROMPOSITION, braceAtCaret);
			int lineMaxSubord = Platform::SendScintilla(win.GetID(), SCI_GETLASTCHILD, lineStart, -1);
			braceOpposite = Platform::SendScintilla(win.GetID(), SCI_GETLINEENDPOSITION, lineMaxSubord);
		} else {
			braceOpposite = Platform::SendScintilla(win.GetID(), SCI_BRACEMATCH, braceAtCaret, 0);
		}
		if (braceOpposite > braceAtCaret) {
			isInside = isAfter;
		} else {
			isInside = !isAfter;
		}
	}
	return isInside;
}

void AnEditor::GetRange(guint start, guint end, gchar *text) {
	TextRange tr;
	tr.chrg.cpMin = start;
	tr.chrg.cpMax = end;
	tr.lpstrText = text;
	SendEditor (SCI_GETTEXTRANGE, 0, reinterpret_cast<long>(&tr));
}

void AnEditor::BraceMatch() {
	if (!bracesCheck)
		return;
	int braceAtCaret = -1;
	int braceOpposite = -1;
	FindMatchingBracePosition(braceAtCaret, braceOpposite, bracesSloppy);
	Window &win = wEditor;
	if ((braceAtCaret != -1) && (braceOpposite == -1)) {
		Platform::SendScintilla(win.GetID(), SCI_BRACEBADLIGHT, braceAtCaret, 0);
		SendEditor(SCI_SETHIGHLIGHTGUIDE, 0);
	} else {
		char chBrace = static_cast<char>(Platform::SendScintilla(
		                                         win.GetID(), SCI_GETCHARAT, braceAtCaret, 0));
		Platform::SendScintilla(win.GetID(), SCI_BRACEHIGHLIGHT, braceAtCaret, braceOpposite);
		int columnAtCaret = Platform::SendScintilla(win.GetID(), SCI_GETCOLUMN, braceAtCaret, 0);
		if (chBrace == ':') {
			int lineStart = Platform::SendScintilla(win.GetID(), SCI_LINEFROMPOSITION, braceAtCaret);
			int indentPos = Platform::SendScintilla(win.GetID(), SCI_GETLINEINDENTPOSITION, lineStart, 0);
			int indentPosNext = Platform::SendScintilla(win.GetID(), SCI_GETLINEINDENTPOSITION, lineStart + 1, 0);
			columnAtCaret = Platform::SendScintilla(win.GetID(), SCI_GETCOLUMN, indentPos, 0);
			int columnAtCaretNext = Platform::SendScintilla(win.GetID(), SCI_GETCOLUMN, indentPosNext, 0);
			indentSize = props->GetInt("indent.size");
			if (columnAtCaretNext - indentSize > 1)
				columnAtCaret = columnAtCaretNext - indentSize;
		}


		int columnOpposite = Platform::SendScintilla(win.GetID(), SCI_GETCOLUMN, braceOpposite, 0);
		if (props->GetInt("highlight.indentation.guides"))
			Platform::SendScintilla(win.GetID(), SCI_SETHIGHLIGHTGUIDE, Platform::Minimum(columnAtCaret, columnOpposite), 0);
	}
}

CharacterRange AnEditor::GetSelection() {
	CharacterRange crange;
	crange.cpMin = SendEditor(SCI_GETSELECTIONSTART);
	crange.cpMax = SendEditor(SCI_GETSELECTIONEND);
	return crange;
}

void AnEditor::SetSelection(int anchor, int currentPos) {
	SendEditor(SCI_SETSEL, anchor, currentPos);
}

static bool iswordcharforsel(char ch) {
	return !strchr("\t\n\r !\"#$%&'()*+,-./:;<=>?@[\\]^`{|}~", ch);
}

void AnEditor::SelectionWord(char *word, int len) {
	int lengthDoc = LengthDocument();
	CharacterRange cr = GetSelection();
	int selStart = cr.cpMin;
	int selEnd = cr.cpMax;
	if (selStart == selEnd) {
		WindowAccessor acc(wEditor.GetID(), *props);
		// Try and find a word at the caret
		if (iswordcharforsel(acc[selStart])) {
			while ((selStart > 0) && (iswordcharforsel(acc[selStart - 1])))
				selStart--;
			while ((selEnd < lengthDoc - 1) && (iswordcharforsel(acc[selEnd + 1])))
				selEnd++;
			if (selStart < selEnd)
				selEnd++;   	// Because normal selections end one past
		}
	}
	word[0] = '\0';
	if ((selStart < selEnd) && ((selEnd - selStart + 1) < len)) {
		GetRange(wEditor, selStart, selEnd, word);
	}
}

void AnEditor::SelectionIntoProperties() {
	CharacterRange cr = GetSelection();
	char currentSelection[1000];
	if ((cr.cpMin < cr.cpMax) && ((cr.cpMax - cr.cpMin + 1) < static_cast<int>(sizeof(currentSelection)))) {
		GetRange(wEditor, cr.cpMin, cr.cpMax, currentSelection);
		int len = strlen(currentSelection);
		if (len > 2 && iscntrl(currentSelection[len - 1]))
			currentSelection[len - 1] = '\0';
		if (len > 2 && iscntrl(currentSelection[len - 2]))
			currentSelection[len - 2] = '\0';
		props->Set("CurrentSelection", currentSelection);
	}
	char word[200];
	SelectionWord(word, sizeof(word));
	props->Set("CurrentWord", word);
}

long AnEditor::Find (long flags, char* findWhat) {
	if (!findWhat) return -1;
	TextToFind ft = {{0, 0}, 0, {0, 0}};
	CharacterRange crange = GetSelection();
	if (flags & ANEFIND_REVERSE_FLAG) {
		ft.chrg.cpMin = crange.cpMin - 1;
		ft.chrg.cpMax = 0;
	} else {
		ft.chrg.cpMin = crange.cpMax;
		ft.chrg.cpMax = LengthDocument();
	}
	ft.lpstrText = findWhat;
	ft.chrgText.cpMin = 0;
	ft.chrgText.cpMax = 0;
	int posFind = SendEditor(SCI_FINDTEXT, flags, reinterpret_cast<long>(&ft));
	if (posFind >= 0) {
		EnsureRangeVisible(ft.chrgText.cpMin, ft.chrgText.cpMax);
		SetSelection(ft.chrgText.cpMin, ft.chrgText.cpMax);
	}
	return posFind;
}

void AnEditor::BookmarkToggle( int lineno ) {
	if (lineno == -1)
		lineno = GetCurrentLineNumber();
	int state = SendEditor(SCI_MARKERGET, lineno);
	if ( state & (1 << ANE_MARKER_BOOKMARK))
		SendEditor(SCI_MARKERDELETE, lineno, ANE_MARKER_BOOKMARK);
	else {
		SendEditor(SCI_MARKERADD, lineno, ANE_MARKER_BOOKMARK);
	}
}

void AnEditor::BookmarkFirst() {
	int lineno = GetCurrentLineNumber();
	int nextLine = SendEditor(SCI_MARKERNEXT, 0, 1 << ANE_MARKER_BOOKMARK);
	if (nextLine < 0 || nextLine == lineno)
		gdk_beep(); // how do I beep? -- like this ;-)
	else {
		SendEditor(SCI_ENSUREVISIBLE, nextLine);
		SendEditor(SCI_GOTOLINE, nextLine);
	}
}

void AnEditor::BookmarkPrev() {
	int lineno = GetCurrentLineNumber();
	int nextLine = SendEditor(SCI_MARKERPREVIOUS, lineno - 1, 1 << ANE_MARKER_BOOKMARK);
	if (nextLine < 0 || nextLine == lineno)
		gdk_beep(); // how do I beep? -- like this ;-)
	else {
		SendEditor(SCI_ENSUREVISIBLE, nextLine);
		SendEditor(SCI_GOTOLINE, nextLine);
	}
}

void AnEditor::BookmarkNext() {
	int lineno = GetCurrentLineNumber();
	int nextLine = SendEditor(SCI_MARKERNEXT, lineno + 1, 1 << ANE_MARKER_BOOKMARK);
	if (nextLine < 0 || nextLine == lineno)
		gdk_beep(); // how do I beep? -- like this ;-)
	else {
		SendEditor(SCI_ENSUREVISIBLE, nextLine);
		SendEditor(SCI_GOTOLINE, nextLine);
	}
}

void AnEditor::BookmarkLast() {
	int lineno = GetCurrentLineNumber();
	int nextLine = SendEditor(SCI_MARKERPREVIOUS,
		SendEditor(SCI_GETLINECOUNT), 1 << ANE_MARKER_BOOKMARK);
	if (nextLine < 0 || nextLine == lineno)
			gdk_beep(); // how do I beep? -- like this ;-)
	else {
		SendEditor(SCI_ENSUREVISIBLE, nextLine);
		SendEditor(SCI_GOTOLINE, nextLine);
	}
}

void AnEditor::BookmarkClear() {
	SendEditor(SCI_MARKERDELETEALL, ANE_MARKER_BOOKMARK);
}

bool AnEditor::StartCallTip() {
	char linebuf[1000];
	int current = GetLine(linebuf, sizeof(linebuf));
	int pos = SendEditor(SCI_GETCURRENTPOS);
	int braces;
	TMTagAttrType attrs[] = { tm_tag_attr_name_t, tm_tag_attr_type_t, tm_tag_attr_none_t};
	
	do {
		braces = 0;
		while (current > 0 && (braces || linebuf[current - 1] != '(')) {
			if (linebuf[current - 1] == '(')
				braces--;
			else if (linebuf[current - 1] == ')')
				braces++;
			current--;
			pos--;
		}
		if (current > 0) {
			current--;
			pos--;
		} else
			break;
		while (current > 0 && isspace(linebuf[current - 1])) {
			current--;
			pos--;
		}
	} while (current > 0 && nonFuncChar(linebuf[current - 1]));
	if (current <= 0)
		return true;

	int startword = current - 1;
	while (startword > 0 && !nonFuncChar(linebuf[startword - 1]))
		startword--;
	linebuf[current] = '\0';
	int rootlen = current - startword;
	functionDefinition = "";
	const GPtrArray *tags = tm_workspace_find(linebuf+startword, tm_tag_prototype_t
	  | tm_tag_function_t | tm_tag_macro_with_arg_t, attrs, FALSE);
	if (tags && (tags->len > 0))
	{
		TMTag *tag = (TMTag *) tags->pdata[0];
		char *tmp = g_strdup_printf("%s %s%s", NVL(tag->atts.entry.var_type, "")
		  , tag->name, NVL(tag->atts.entry.arglist, ""));
		functionDefinition = tmp;
		SendEditorString(SCI_CALLTIPSHOW, pos - rootlen + 1, tmp);
		g_free(tmp);
		ContinueCallTip();
	}
	return true;
}

static bool IsCallTipSeparator(char ch) {
	return (ch == ',') || (ch == ';');
}

void AnEditor::ContinueCallTip() {
	char linebuf[1000];
	int current = GetLine(linebuf, sizeof(linebuf));

	int commas = 0;
	for (int i = 0; i < current; i++) {
		if (IsCallTipSeparator(linebuf[i]))
			commas++;
	}

	int startHighlight = 0;
	while (functionDefinition[startHighlight] && functionDefinition[startHighlight] != '(')
		startHighlight++;
	if (functionDefinition[startHighlight] == '(')
		startHighlight++;
	while (functionDefinition[startHighlight] && commas > 0) {
		if (IsCallTipSeparator(functionDefinition[startHighlight]) || functionDefinition[startHighlight] == ')')
			commas--;
		startHighlight++;
	}
	if (IsCallTipSeparator(functionDefinition[startHighlight]) || functionDefinition[startHighlight] == ')')
		startHighlight++;
	int endHighlight = startHighlight;
	if (functionDefinition[endHighlight])
		endHighlight++;
	while (functionDefinition[endHighlight] && !IsCallTipSeparator(functionDefinition[endHighlight])
			&& functionDefinition[endHighlight] != ')')
		endHighlight++;

	SendEditor(SCI_CALLTIPSETHLT, startHighlight, endHighlight);
}

bool AnEditor::StartAutoComplete() {
	char linebuf[1000];
	int current = GetLine(linebuf, sizeof(linebuf));

	int startword = current;
	while (startword> 0 && !nonFuncChar(linebuf[startword - 1]))
		startword--;
	if (startword == current)
		return true;
	linebuf[current] = '\0';
	const char *root = linebuf + startword;
	int rootlen = current - startword;
	const GPtrArray *tags = tm_workspace_find(root, tm_tag_max_t, NULL, TRUE);
	if (NULL != tags)
	{
		GString *words = g_string_sized_new(100);
		TMTag *tag;
		for (guint i = 0; ((i < tags->len) && (i < MAX_AUTOCOMPLETE_WORDS)); ++i)
		{
			tag = (TMTag *) tags->pdata[i];
			if (i > 0)
				g_string_append_c(words, ' ');
			g_string_append(words, tag->name);
		}
		SendEditorString(SCI_AUTOCSHOW, rootlen, words->str);
		g_string_free(words, TRUE);
	}
	return true;
}

bool AnEditor::GetCurrentWord(char* buffer, int lenght){
	char linebuf[1000];
	int current = GetLine(linebuf, sizeof(linebuf));

	int startword = current;
	while (startword> 0 && !nonFuncChar(linebuf[startword - 1]))
		startword--;
	int endword = current;
	while (linebuf[endword] && !nonFuncChar(linebuf[endword]))
		endword++;
	if(startword == endword)
		return false;
	
	linebuf[endword] = '\0';
	int cplen = (lenght < (endword-startword+1))?lenght:(endword-startword+1);
	strncpy (buffer, &linebuf[startword], cplen);
	return true;
}

#if 0 // Already defined in glib
const char *strcasestr(const char *str, const char *pattn) {
	int i;
	int pattn0 = tolower (pattn[0]);

	for (; *str; str++) {
		if (tolower (*str) == pattn0) {
			for (i = 1; tolower (str[i]) == tolower (pattn[i]); i++)
				if (pattn[i] == '\0')
					return str;
			if (pattn[i] == '\0')
				return str;
		}
	}
	return NULL;
}
#endif

bool AnEditor::StartAutoCompleteWord() {
	char linebuf[1000];
	int current = GetLine(linebuf, sizeof(linebuf));

	int startword = current;
	while (startword > 0 && !nonFuncChar(linebuf[startword - 1]))
		startword--;
	if (startword == current)
		return true;
	linebuf[current] = '\0';
	const char *root = linebuf + startword;
	int rootlen = current - startword;

	/* Added word-completion feature back - Biswa */
	int doclen = LengthDocument();
	TextToFind ft = {{0, 0}, 0, {0, 0}};
	ft.lpstrText = const_cast<char*>(root);
	ft.chrg.cpMin = 0;
	ft.chrgText.cpMin = 0;
	ft.chrgText.cpMax = 0;
	int flags = SCFIND_WORDSTART | (autoCompleteIgnoreCase ? 0 : SCFIND_MATCHCASE);
	int posCurrentWord = SendEditor (SCI_GETCURRENTPOS) - rootlen;
	GString *words = g_string_sized_new(100);
	char wordstart[100];
	char *wordend, *wordbreak, *wordpos;
	int wordlen;
	for (;;)
	{
		ft.chrg.cpMax = doclen;
		int posFind = SendEditor(SCI_FINDTEXT, flags
		  , reinterpret_cast<long>(&ft));
		if (posFind == -1 || posFind >= doclen)
			break;
		if (posFind == posCurrentWord)
		{
			ft.chrg.cpMin = posFind + rootlen;
			continue;
		}
		GetRange(wEditor, posFind, Platform::Minimum(posFind+99, doclen)
		  , wordstart);
		wordend = wordstart + rootlen;
		while (iswordcharforsel(*wordend))
			wordend++;
		*wordend = '\0';
		wordlen = wordend - wordstart;
		wordbreak = words->str;
		for (;;)
		{
			wordpos = strstr (wordbreak, wordstart);
			if (!wordpos)
				break;
			if (wordpos > words->str && wordpos[ -1] != ' ' ||
			  wordpos[wordlen] && wordpos[wordlen] != ' ')
				wordbreak = wordpos + wordlen;
			else
				break;
		}
		if (!wordpos)
		{
			if (words->len > 0)
				g_string_append_c(words, ' ');
			g_string_append(words, wordstart);
		}
		ft.chrg.cpMin = posFind + wordlen;
	}
	/* Now for the TM based autocompletion */
	const GPtrArray *tags = tm_workspace_find(root, tm_tag_max_t, NULL, TRUE);
	if ((tags) && (tags->len > 0))
	{
		TMTag *tag;
		for (guint i=0; ((i < tags->len) && (i < MAX_AUTOCOMPLETE_WORDS)); ++i)
		{
			tag = (TMTag *) tags->pdata[i];
			if (words->len > 0)
				g_string_append_c(words, ' ');
			g_string_append(words, tag->name);
		}
	}
	if (words->len > 0)
		SendEditorString(SCI_AUTOCSHOW, rootlen, words->str);
	g_string_free(words, TRUE);
	return true;
}

int AnEditor::GetCurrentLineNumber() {
	CharacterRange crange = GetSelection();
	int selStart = crange.cpMin;
	return SendEditor(SCI_LINEFROMPOSITION, selStart);
}

int AnEditor::GetCurrentScrollPosition() {
	int lineDisplayTop = SendEditor(SCI_GETFIRSTVISIBLELINE);
	return SendEditor(SCI_DOCLINEFROMVISIBLE, lineDisplayTop);
}

int AnEditor::SetLineIndentation(int line, int indent) {
	SendEditor(SCI_SETLINEINDENTATION, line, indent);
	int pos = GetLineIndentPosition(line);
	SetSelection(pos, pos);
	return pos;
}

void AnEditor::IndentationIncrease(){
	CharacterRange crange = GetSelection();
	if (crange.cpMin != crange.cpMax)
	{
		SendEditor (SCI_TAB);
		return;
	}
	int line =SendEditor(SCI_LINEFROMPOSITION, SendEditor (SCI_GETCURRENTPOS));
	int indent =GetLineIndentation(line);
	indent +=SendEditor(SCI_GETINDENT);
	SetLineIndentation(line, indent);
}

void AnEditor::IndentationDecrease(){
	CharacterRange crange = GetSelection();
	if (crange.cpMin != crange.cpMax)
	{
		SendEditor (SCI_BACKTAB);
		return;
	}
	int line =SendEditor(SCI_LINEFROMPOSITION, SendEditor (SCI_GETCURRENTPOS));
	int indent = GetLineIndentation(line);
	indent -=SendEditor(SCI_GETINDENT);
	if (indent < 0) indent = 0;
	SetLineIndentation(line, indent);
}

int AnEditor::GetLineIndentation(int line) {
	return SendEditor(SCI_GETLINEINDENTATION, line);
}

int AnEditor::GetLineIndentPosition(int line) {
	return SendEditor(SCI_GETLINEINDENTPOSITION, line);
}

bool AnEditor::RangeIsAllWhitespace(int start, int end) {
	WindowAccessor acc(wEditor.GetID(), *props);
	for (int i = start;i < end;i++) {
		if ((acc[i] != ' ') && (acc[i] != '\t'))
			return false;
	}
	return true;
}

void AnEditor::GetLinePartsInStyle(int line, int style1, int style2, SString sv[], int len) {
	for (int i = 0; i < len; i++)
		sv[i] = "";
	WindowAccessor acc(wEditor.GetID(), *props);
	SString s;
	int part = 0;
	int thisLineStart = SendEditor(SCI_POSITIONFROMLINE, line);
	int nextLineStart = SendEditor(SCI_POSITIONFROMLINE, line + 1);
	for (int pos = thisLineStart; pos < nextLineStart; pos++) {
		if ((acc.StyleAt(pos) == style1) || (acc.StyleAt(pos) == style2)) {
			char c[2];
			c[0] = acc[pos];
			c[1] = '\0';
			s += c;
		} else if (s.length() > 0) {
			if (part < len) {
				sv[part++] = s;
			}
			s = "";
		}
	}
	if ((s.length() > 0) && (part < len)) {
		sv[part] = s;
	}
}

static bool includes(const StyleAndWords &symbols, const SString value) {
	if (symbols.words.length() == 0) {
		return false;
	} else if (strchr(symbols.words.c_str(), ' ')) {
		// Set of symbols separated by spaces
		int lenVal = value.length();
		const char *symbol = symbols.words.c_str();
		while (symbol) {
			const char *symbolEnd = strchr(symbol, ' ');
			int lenSymbol = strlen(symbol);
			if (symbolEnd)
				lenSymbol = symbolEnd - symbol;
			if (lenSymbol == lenVal) {
				if (strncmp(symbol, value.c_str(), lenSymbol) == 0) {
					return true;
				}
			}
			symbol = symbolEnd;
			if (symbol)
				symbol++;
		}
		return false;
	} else {
		// Set of individual characters. Only one character allowed for now
		char ch = symbols.words[0];
		return strchr(value.c_str(), ch) != 0;
	}
}

#define ELEMENTS(a)	(sizeof(a) / sizeof(a[0]))

int AnEditor::GetIndentState(int line) {
	// C like language indentation defined by braces and keywords
	int indentState = 0;
	SString controlWords[10];
	GetLinePartsInStyle(line, SCE_C_WORD, -1, controlWords, ELEMENTS(controlWords));
	for (unsigned int i = 0;i < ELEMENTS(controlWords);i++) {
		if (includes(statementIndent, controlWords[i]))
			indentState = 2;
	}
	// Braces override keywords
	SString controlStrings[10];
	GetLinePartsInStyle(line, SCE_C_OPERATOR, -1, controlStrings, ELEMENTS(controlStrings));
	for (unsigned int j = 0;j < ELEMENTS(controlStrings);j++) {
		if (includes(blockEnd, controlStrings[j]))
			indentState = -1;
		if (includes(blockStart, controlStrings[j]))
			indentState = 1;
	}
	return indentState;
}

void AnEditor::AutomaticIndentation(char ch) {
	CharacterRange crange = GetSelection();
	int selStart = crange.cpMin;
	int curLine = GetCurrentLineNumber();
	int thisLineStart = SendEditor(SCI_POSITIONFROMLINE, curLine);
	int indent = GetLineIndentation(curLine - 1);
	int indentBlock = indent;
	int backLine = curLine - 1;
	int indentState = 0;
	if (statementIndent.IsEmpty() && blockStart.IsEmpty() && blockEnd.IsEmpty())
		indentState = 1;	// Do not bother searching backwards

	int lineLimit = curLine - statementLookback;
	if (lineLimit < 0)
		lineLimit = 0;
	while ((backLine >= lineLimit) && (indentState == 0)) {
		indentState = GetIndentState(backLine);
		if (indentState != 0) {
			indentBlock = GetLineIndentation(backLine);
			if (indentState == 1) {
				if (!indentOpening)
					indentBlock += indentSize;
			}
			if (indentState == -1) {
				if (indentClosing)
					indentBlock -= indentSize;
				if (indentBlock < 0)
					indentBlock = 0;
			}
			if ((indentState == 2) && (backLine == (curLine - 1)))
				indentBlock += indentSize;
		}
		backLine--;
	}
	if (ch == blockEnd.words[0]) {	// Dedent maybe
		if (!indentClosing) {
			if (RangeIsAllWhitespace(thisLineStart, selStart - 1)) {
				int pos = SetLineIndentation(curLine, indentBlock - indentSize);
				// Move caret after '}'
				SetSelection(pos + 1, pos + 1);
			}
		}
	} else if (ch == blockStart.words[0]) {	// Dedent maybe if first on line
		if (!indentOpening) {
			if (RangeIsAllWhitespace(thisLineStart, selStart - 1)) {
				int pos = SetLineIndentation(curLine, indentBlock - indentSize);
				// Move caret after '{'
				SetSelection(pos + 1, pos + 1);
			}
		}
	} else if ((ch == '\r' || ch == '\n') && (selStart == thisLineStart)) {
		SetLineIndentation(curLine, indentBlock);
	}
}

// Upon a character being added, AnEditor may decide to perform some action
// such as displaying a completion list.
void AnEditor::CharAdded(char ch) {
	CharacterRange crange = GetSelection();
	int selStart = crange.cpMin;
	int selEnd = crange.cpMax;
	if ((selEnd == selStart) && (selStart > 0)) {
		int style = SendEditor(SCI_GETSTYLEAT, selStart - 1, 0);
		if (style != 1) {
			if (SendEditor(SCI_CALLTIPACTIVE)) {
				if (ch == ')') {
					braceCount--;
					if (braceCount < 1)
						SendEditor(SCI_CALLTIPCANCEL);
				} else if (ch == '(') {
					braceCount++;
				} else {
					ContinueCallTip();
				}
			} else if (SendEditor(SCI_AUTOCACTIVE)) {
				if (ch == '(') {
					braceCount++;
					StartCallTip();
				} else if (ch == ')') {
					braceCount--;
				} else if (!isalpha(ch) && (ch != '_')) {
					SendEditor(SCI_AUTOCCANCEL);
				}
			} else {
				if (ch == '(') {
					braceCount = 1;
					StartCallTip();
				} else {
					if (props->GetInt("indent.automatic"))
						AutomaticIndentation(ch);
				}
			}
		}
	}
}

void AnEditor::GoMatchingBrace(bool select) {
	int braceAtCaret = -1;
	int braceOpposite = -1;
	bool isInside = FindMatchingBracePosition(braceAtCaret, braceOpposite, true);
	// Convert the character positions into caret positions based on whether
	// the caret position was inside or outside the braces.
	if (isInside) {
		if (braceOpposite > braceAtCaret) {
			braceAtCaret++;
		} else {
			braceOpposite++;
		}
	} else {    // Outside
		if (braceOpposite > braceAtCaret) {
			braceOpposite++;
		} else {
			braceAtCaret++;
		}
	}
	if (braceOpposite >= 0) {
		EnsureRangeVisible(braceOpposite, braceOpposite);
		if (select) {
			SetSelection(braceAtCaret, braceOpposite);
		} else {
			SetSelection(braceOpposite, braceOpposite);
		}
	}
}

int ControlIDOfCommand(unsigned long wParam) {
	return wParam & 0xffff;
}

long AnEditor::Command(int cmdID, long wParam, long lParam) {
	switch (cmdID) {
			
	case ANE_INSERTTEXT:
		SendEditor(SCI_INSERTTEXT,wParam,lParam);
		break;
	
	case ANE_GETBOOKMARK_POS:
		return GetBookmarkLine( wParam );
		break;	/* pleonastico */

	case ANE_BOOKMARK_TOGGLE_LINE:
		BookmarkToggle( wParam );
		break;

	case ANE_UNDO:
		SendEditor(SCI_UNDO);
		break;

	case ANE_REDO:
		SendEditor(SCI_REDO);
		break;

	case ANE_CUT:
		SendEditor(SCI_CUT);
		break;

	case ANE_COPY:
		SendEditor(SCI_COPY);
		break;

	case ANE_PASTE:
		SendEditor(SCI_PASTE);
		break;

	case ANE_CLEAR:
		SendEditor(SCI_CLEAR);
		break;

	case ANE_SELECTALL:
		SendEditor(SCI_SELECTALL);
		break;

	case ANE_FIND:
		return Find (wParam, (char*) lParam);

	case ANE_GOTOLINE:
		SendEditor(SCI_GOTOLINE, wParam);
		break;
	
	case ANE_SETZOOM:
		SendEditor(SCI_SETZOOM, wParam);
		break;

	case ANE_MATCHBRACE:
		GoMatchingBrace(false);
		break;

	case ANE_SELECTBLOCK:
		SelectBlock();
		break;

	case ANE_SELECTTOBRACE:
		GoMatchingBrace(true);
		break;
	
	case ANE_GETBLOCKSTARTLINE:
		return GetBlockStartLine();

	case ANE_GETBLOCKENDLINE:
		return GetBlockEndLine();
	
	case ANE_GETCURRENTWORD:
		return GetCurrentWord((char*)wParam, (int)lParam);

	case ANE_SHOWCALLTIP:
		StartCallTip();
		break;

	case ANE_COMPLETE:
		StartAutoComplete();
		break;

	case ANE_COMPLETEWORD:
		StartAutoCompleteWord();
		break;

	case ANE_TOGGLE_FOLD:
		FoldToggle();
		break;

	case ANE_OPEN_FOLDALL:
		FoldOpenAll();
		break;

	case ANE_CLOSE_FOLDALL:
		FoldCloseAll();
		break;

	case ANE_UPRCASE:
		SendEditor(SCI_UPPERCASE);
		break;

	case ANE_LWRCASE:
		SendEditor(SCI_LOWERCASE);
		break;

	case ANE_EXPAND:
		SendEditor(SCI_TOGGLEFOLD, GetCurrentLineNumber());
		break;

	case ANE_LINENUMBERMARGIN:
		lineNumbers = wParam;
		SendEditor(SCI_SETMARGINWIDTHN, 0, lineNumbers ? lineNumbersWidth : 0);
		break;

	case ANE_SELMARGIN:
		margin = wParam;
		SendEditor(SCI_SETMARGINWIDTHN, 1, margin ? marginWidth : 0);
		break;

	case ANE_FOLDMARGIN:
		foldMargin = wParam;
		SendEditor(SCI_SETMARGINWIDTHN, 2, foldMargin ? foldMarginWidth : 0);
		break;

	case ANE_VIEWEOL:
		SendEditor(SCI_SETVIEWEOL, wParam);
		break;

	case ANE_EOL_CRLF:
		SendEditor(SCI_SETEOLMODE, SC_EOL_CRLF);
		break;

	case ANE_EOL_CR:
		SendEditor(SCI_SETEOLMODE, SC_EOL_CR);
		break;

	case ANE_EOL_LF:
		SendEditor(SCI_SETEOLMODE, SC_EOL_LF);
		break;

	case ANE_EOL_CONVERT:
		switch (wParam) {
			case ANE_EOL_CRLF:
				SendEditor(SCI_SETEOLMODE, SC_EOL_CRLF);
				SendEditor(SCI_CONVERTEOLS, SC_EOL_CRLF);
				break;
			case ANE_EOL_LF:
				SendEditor(SCI_SETEOLMODE, SC_EOL_LF);
				SendEditor(SCI_CONVERTEOLS, SC_EOL_LF);
				break;
			case ANE_EOL_CR:
				SendEditor(SCI_SETEOLMODE, SC_EOL_CR);
				SendEditor(SCI_CONVERTEOLS, SC_EOL_CR);
				break;
			default:
				SendEditor(SCI_CONVERTEOLS, SendEditor(SCI_GETEOLMODE));
				break;
		}
		break;

	case ANE_WORDPARTLEFT:
		SendEditor(SCI_WORDPARTLEFT);
		break;

	case ANE_WORDPARTLEFTEXTEND:
		SendEditor(SCI_WORDPARTLEFTEXTEND);
		break;

	case ANE_WORDPARTRIGHT:
		SendEditor(SCI_WORDPARTRIGHT);
		break;

	case ANE_WORDPARTRIGHTEXTEND:
		SendEditor(SCI_WORDPARTRIGHTEXTEND);
		break;
	
	case ANE_VIEWSPACE:
		ViewWhitespace(wParam);
		break;

	case ANE_VIEWGUIDES:
		SendEditor(SCI_SETINDENTATIONGUIDES, wParam);
		break;

	case ANE_BOOKMARK_TOGGLE:
		BookmarkToggle();
		break;
	case ANE_BOOKMARK_FIRST:
		BookmarkFirst();
		break;
	case ANE_BOOKMARK_PREV:
		BookmarkPrev();
		break;
	case ANE_BOOKMARK_NEXT:
		BookmarkNext();
		break;
	case ANE_BOOKMARK_LAST:
		BookmarkLast();
		break;
	case ANE_BOOKMARK_CLEAR:
		BookmarkClear();
		break;

	case ANE_SETTABSIZE:
		SendEditor(SCI_SETTABWIDTH, wParam);
		break;

	case ANE_SETLANGUAGE:
		SetOverrideLanguage(wParam);
		break;

	case ANE_SETHILITE:
		ReadProperties((char*)wParam);
		SendEditor(SCI_COLOURISE, 0, -1);
		break;

	case ANE_SETACCELGROUP:
		SetAccelGroup((GtkAccelGroup*)wParam);
		break;

	case ANE_GETTEXTRANGE: {
			guint start, end;
			if(wParam == lParam) return 0;
			start = (guint) MINIMUM(wParam, lParam);
			end = (guint) MAXIMUM(wParam, lParam);
			gchar *buff = (gchar*) g_malloc(end-start+10);
			if(!buff) return 0;
			GetRange(start, end, buff);
			return (long) buff;
		}
		break;

	case ANE_INDENT_INCREASE:
		IndentationIncrease();
		break;
	
	case ANE_INDENT_DECREASE:
		IndentationDecrease();
		break;
	
	case ANE_GETLENGTH:
		return SendEditor(SCI_GETLENGTH);

	case ANE_GET_LINENO:
		return GetCurrentLineNumber();

	default:
		break;
	}
	return 0;
}

void AnEditor::FoldChanged(int line, int levelNow, int levelPrev) {
	if (levelNow & SC_FOLDLEVELHEADERFLAG) {
		SendEditor(SCI_SETFOLDEXPANDED, line, 1);
	} else if (levelPrev & SC_FOLDLEVELHEADERFLAG) {
		if (!SendEditor(SCI_GETFOLDEXPANDED, line)) {
			// Removing the fold from one that has been contracted so should expand
			// otherwise lines are left invisible with no way to make them visible
			Expand(line, true, false, 0, levelPrev);
		}
	}
}

void AnEditor::Expand(int &line, bool doExpand, bool force, int visLevels, int level) {
	int lineMaxSubord = SendEditor(SCI_GETLASTCHILD, line, level);
	line++;
	while (line <= lineMaxSubord) {
		if (force) {
			if (visLevels > 0)
				SendEditor(SCI_SHOWLINES, line, line);
			else
				SendEditor(SCI_HIDELINES, line, line);
		} else {
			if (doExpand)
				SendEditor(SCI_SHOWLINES, line, line);
		}
		int levelLine = level;
		if (levelLine ==-1)
			levelLine = SendEditor(SCI_GETFOLDLEVEL, line);
		if (levelLine & SC_FOLDLEVELHEADERFLAG) {
			if (force) {
				if (visLevels > 1)
					SendEditor(SCI_SETFOLDEXPANDED, line, 1);
				else
					SendEditor(SCI_SETFOLDEXPANDED, line, 0);
				Expand(line, doExpand, force, visLevels - 1);
			} else {
				if (doExpand && SendEditor(SCI_GETFOLDEXPANDED, line)) {
					Expand(line, true, force, visLevels - 1);
				} else {
					Expand(line, false, force, visLevels - 1);
				}
			}
		} else {
			line++;
		}
	}
}

void AnEditor::FoldCode(bool expanding) {
	int maxLine = SendEditor (SCI_GETTEXTLENGTH);
	SendEditor(SCI_COLOURISE, 0, -1);
	for (int line = 0; line < maxLine; line++) {
		int level = SendEditor(SCI_GETFOLDLEVEL, line);
		if ((level & SC_FOLDLEVELHEADERFLAG) &&
		        (SC_FOLDLEVELBASE == (level & SC_FOLDLEVELNUMBERMASK))) {
			if (expanding) {
				SendEditor(SCI_SETFOLDEXPANDED, line, 1);
				Expand(line, true);
				line--;
			} else {
				int lineMaxSubord = SendEditor(SCI_GETLASTCHILD, line, -1);
				SendEditor(SCI_SETFOLDEXPANDED, line, 0);
				if (lineMaxSubord > line)
					SendEditor(SCI_HIDELINES, line + 1, lineMaxSubord);
			}
		}
	}
}

void AnEditor::FoldOpenAll() {
	FoldCode (true);
}

void AnEditor::FoldCloseAll() {
	FoldCode (false);
}

void AnEditor::FoldToggle() {
	int curLine = SendEditor(SCI_LINEFROMPOSITION, SendEditor (SCI_GETCURRENTPOS));
	int level = SendEditor(SCI_GETFOLDLEVEL, curLine);
	if (level & SC_FOLDLEVELHEADERFLAG) {
		SendEditor(SCI_TOGGLEFOLD, curLine);
		return;
	}
	int parent = SendEditor (SCI_GETFOLDPARENT, curLine);
	int lastChild = SendEditor (SCI_GETLASTCHILD, parent, -1);
	if (curLine > parent && curLine <= lastChild)
	{
		SendEditor(SCI_TOGGLEFOLD, parent);
		SendEditor(SCI_SETCURRENTPOS, SendEditor (SCI_POSITIONFROMLINE, parent));
		SendEditor(SCI_GOTOLINE, parent);
	}
	else
		gdk_beep ();
}

void AnEditor::SelectBlock () {
	int curLine = SendEditor(SCI_LINEFROMPOSITION, SendEditor (SCI_GETCURRENTPOS));
	int parent = SendEditor (SCI_GETFOLDPARENT, curLine);
	int lastChild = SendEditor (SCI_GETLASTCHILD, parent, -1);
	if (curLine > parent && curLine <= lastChild)
	{
		gint start, end;
		start = SendEditor(SCI_POSITIONFROMLINE, parent);
		end = SendEditor(SCI_POSITIONFROMLINE, lastChild+1);
		SetSelection(start, end);
	}
	else
		gdk_beep ();
}

int AnEditor::GetBlockStartLine () {
	int curLine = SendEditor(SCI_LINEFROMPOSITION, SendEditor (SCI_GETCURRENTPOS));
	int level = SendEditor(SCI_GETFOLDLEVEL, curLine);
	if (level & SC_FOLDLEVELHEADERFLAG) {
		return curLine;
	}
	int parent = SendEditor (SCI_GETFOLDPARENT, curLine);
	int lastChild = SendEditor (SCI_GETLASTCHILD, parent, -1);
	if (curLine > parent && curLine <= lastChild)
	{
		return parent;
	}
	else
		return -1;
}

int AnEditor::GetBlockEndLine () {
	int curLine = SendEditor(SCI_LINEFROMPOSITION, SendEditor (SCI_GETCURRENTPOS));
	int parent = SendEditor (SCI_GETFOLDPARENT, curLine);
	int lastChild = SendEditor (SCI_GETLASTCHILD, parent, -1);
	if (curLine > parent && curLine <= lastChild)
	{
		return lastChild;
	}
	else
		return -1;
}

void AnEditor::EnsureRangeVisible(int posStart, int posEnd) {
	int lineStart = SendEditor(SCI_LINEFROMPOSITION, Platform::Minimum(posStart, posEnd));
	int lineEnd = SendEditor(SCI_LINEFROMPOSITION, Platform::Maximum(posStart, posEnd));
	for (int line = lineStart; line <= lineEnd; line++) {
		SendEditor(SCI_ENSUREVISIBLE, line);
	}
}

bool AnEditor::MarginClick(int position, int modifiers) {
	int lineClick = SendEditor(SCI_LINEFROMPOSITION, position);
	//	SendEditor(SCI_GETFOLDLEVEL, lineClick) & SC_FOLDLEVELHEADERFLAG);
	if (modifiers & SCMOD_SHIFT) {
		FoldCloseAll();
	} else if (modifiers & SCMOD_CTRL) {
		FoldOpenAll();
	} else if (SendEditor(SCI_GETFOLDLEVEL, lineClick) & SC_FOLDLEVELHEADERFLAG) {
		if (modifiers & SCMOD_SHIFT) {
			// Ensure all children visible
			SendEditor(SCI_SETFOLDEXPANDED, lineClick, 1);
			Expand(lineClick, true, true, 100);
		} else if (modifiers & SCMOD_CTRL) {
			if (SendEditor(SCI_GETFOLDEXPANDED, lineClick)) {
				// Contract this line and all children
				SendEditor(SCI_SETFOLDEXPANDED, lineClick, 0);
				Expand(lineClick, false, true, 0);
			} else {
				// Expand this line and all children
				SendEditor(SCI_SETFOLDEXPANDED, lineClick, 1);
				Expand(lineClick, true, true, 100);
			}
		} else {
			// Toggle this line
			SendEditor(SCI_TOGGLEFOLD, lineClick);
		}
	}
	return true;
}

void AnEditor::NotifySignal(GtkWidget *, gint /*wParam*/, gpointer lParam, AnEditor *anedit) {
	anedit->Notify(reinterpret_cast<SCNotification *>(lParam));
}

void AnEditor::Notify(SCNotification *notification) {
	switch (notification->nmhdr.code) {
	case SCN_KEY:{
			if(!accelGroup) break;
			int mods = 0;
			if (notification->modifiers & SCMOD_SHIFT)
				mods |= GDK_SHIFT_MASK;
			if (notification->modifiers & SCMOD_CTRL)
				mods |= GDK_CONTROL_MASK;
			if (notification->modifiers & SCMOD_ALT)
				mods |= GDK_MOD1_MASK;
			gtk_accel_group_activate(accelGroup, notification->ch,
				static_cast<GdkModifierType>(mods));
		}

	case SCN_CHARADDED:
		CharAdded(static_cast<char>(notification->ch));
		break;

	case SCN_SAVEPOINTREACHED:
		isDirty = false;
		break;

	case SCN_SAVEPOINTLEFT:
		isDirty = true;
		break;

	case SCN_UPDATEUI:
		BraceMatch();
		break;

	case SCN_MODIFIED:
		if (notification->modificationType == SC_MOD_CHANGEFOLD) {
			FoldChanged(notification->line,
			            notification->foldLevelNow, notification->foldLevelPrev);
		}
		break;

	case SCN_MARGINCLICK:
		if (notification->margin == 2)
			MarginClick(notification->position, notification->modifiers);
		break;

	case SCN_NEEDSHOWN: {
			EnsureRangeVisible(notification->position, notification->position + notification->length);
		}
		break;
	}
}

static int IntFromHexDigit(const char ch) {
	if (isdigit(ch))
		return ch - '0';
	else if (ch >= 'A' && ch <= 'F')
		return ch - 'A' + 10;
	else if (ch >= 'a' && ch <= 'f')
		return ch - 'a' + 10;
	else
		return 0;
}

static Colour ColourFromString(const char *val) {
	int r = IntFromHexDigit(val[1]) * 16 + IntFromHexDigit(val[2]);
	int g = IntFromHexDigit(val[3]) * 16 + IntFromHexDigit(val[4]);
	int b = IntFromHexDigit(val[5]) * 16 + IntFromHexDigit(val[6]);
	return Colour(r, g, b);
}

void AnEditor::SetOneStyle(Window &win, int style, const char *s) {
	char *val = StringDup(s);
	char *opt = val;
	while (opt) {
		char *cpComma = strchr(opt, ',');
		if (cpComma)
			*cpComma = '\0';
		char *colon = strchr(opt, ':');
		if (colon)
			*colon++ = '\0';
		if (0 == strcmp(opt, "italics"))
			Platform::SendScintilla(win.GetID(), SCI_STYLESETITALIC, style, 1);
		if (0 == strcmp(opt, "notitalics"))
			Platform::SendScintilla(win.GetID(), SCI_STYLESETITALIC, style, 0);
		if (0 == strcmp(opt, "bold"))
			Platform::SendScintilla(win.GetID(), SCI_STYLESETBOLD, style, 1);
		if (0 == strcmp(opt, "notbold"))
			Platform::SendScintilla(win.GetID(), SCI_STYLESETBOLD, style, 0);
		if (0 == strcmp(opt, "font"))
			Platform::SendScintilla(win.GetID(), SCI_STYLESETFONT, style, reinterpret_cast<long>(colon));
		if (0 == strcmp(opt, "fore"))
			Platform::SendScintilla(win.GetID(), SCI_STYLESETFORE, style, ColourFromString(colon).AsLong());
		if (0 == strcmp(opt, "back"))
			Platform::SendScintilla(win.GetID(), SCI_STYLESETBACK, style, ColourFromString(colon).AsLong());
		if (0 == strcmp(opt, "size"))
			Platform::SendScintilla(win.GetID(), SCI_STYLESETSIZE, style, atoi(colon));
		if (0 == strcmp(opt, "eolfilled"))
			Platform::SendScintilla(win.GetID(), SCI_STYLESETEOLFILLED, style, 1);
		if (0 == strcmp(opt, "noteolfilled"))
			Platform::SendScintilla(win.GetID(), SCI_STYLESETEOLFILLED, style, 0);
		if (0 == strcmp(opt, "underlined"))
			Platform::SendScintilla(win.GetID(), SCI_STYLESETUNDERLINE, style, 1);
		if (0 == strcmp(opt, "notunderlined"))
			Platform::SendScintilla(win.GetID(), SCI_STYLESETUNDERLINE, style, 0);
		if (cpComma)
			opt = cpComma + 1;
		else
			opt = 0;
	}
	if (val)
		delete []val;
	Platform::SendScintilla(win.GetID(), SCI_STYLESETCHARACTERSET, style, characterSet);
}

void AnEditor::SetStyleFor(Window &win, const char *lang) {
	for (int style = 0; style <= STYLE_MAX; style++) {
		if (style != STYLE_DEFAULT) {
			char key[200];
			sprintf(key, "style.%s.%0d", lang, style);
			SString sval = props->GetExpanded(key);
			// g_print ("Style for %s:%0d == %s\n", lang, style, sval.c_str());
			SetOneStyle(win, style, sval.c_str());
		}
	}
}

void lowerCaseString(char *s) {
	while (*s) {
		*s = static_cast<char>(tolower(*s));
		s++;
	}
}

SString AnEditor::ExtensionFileName() {
	if (overrideExtension.length())
		return overrideExtension;
	else if (fileName[0]) {
		// Force extension to lower case
		char fileNameWithLowerCaseExtension[MAX_PATH];
		strcpy(fileNameWithLowerCaseExtension, fileName);
		char *extension = strrchr(fileNameWithLowerCaseExtension, '.');
		if (extension) {
			lowerCaseString(extension);
		}
		return SString(fileNameWithLowerCaseExtension);
	} else
		return props->Get("default.file.ext");
}

void AnEditor::ReadProperties(const char *fileForExt) {
	//DWORD dwStart = timeGetTime();
	SString fileNameForExtension;
	if(overrideExtension.length())
		fileNameForExtension = overrideExtension;
	else
		fileNameForExtension = fileForExt;

	language = props->GetNewExpand("lexer.", fileNameForExtension.c_str());

	if (language == "python") {
		lexLanguage = SCLEX_PYTHON;
	} else if (language == "cpp" || language == "header") {
		lexLanguage = SCLEX_CPP;
	} else if (language == "hypertext") {
		lexLanguage = SCLEX_HTML;
	} else if (language == "xml") {
		lexLanguage = SCLEX_XML;
	} else if (language == "perl") {
		lexLanguage = SCLEX_PERL;
	} else if (language == "sql") {
		lexLanguage = SCLEX_SQL;
	} else if (language == "vb") {
		lexLanguage = SCLEX_VB;
	} else if (language == "props") {
		lexLanguage = SCLEX_PROPERTIES;
	} else if (language == "errorlist") {
		lexLanguage = SCLEX_ERRORLIST;
	} else if (language == "makefile") {
		lexLanguage = SCLEX_MAKEFILE;
	} else if (language == "batch") {
		lexLanguage = SCLEX_BATCH;
	} else if (language == "latex") {
		lexLanguage = SCLEX_LATEX;
	} else if (language == "lua") {
		lexLanguage = SCLEX_LUA;
	} else if (language == "diff") {
		lexLanguage = SCLEX_DIFF;
	} else if (language == "container") {
		lexLanguage = SCLEX_CONTAINER;
	} else if (language == "conf") {
		lexLanguage = SCLEX_CONF;
	} else if (language == "pascal") {
		lexLanguage = SCLEX_PASCAL;
	} else if (language == "baan") {
		lexLanguage = SCLEX_BAAN;
	} else if (language == "ada") {
		lexLanguage = SCLEX_ADA;
	} else {
		lexLanguage = SCLEX_NULL;
	}

	if ((lexLanguage == SCLEX_HTML) || (lexLanguage == SCLEX_XML))
		SendEditor(SCI_SETSTYLEBITS, 7);
	else
		SendEditor(SCI_SETSTYLEBITS, 5);

	SendEditor(SCI_SETLEXER, lexLanguage);
	
	SString kw0 = props->GetNewExpand("keywords.", fileNameForExtension.c_str());
	SendEditorString(SCI_SETKEYWORDS, 0, kw0.c_str());
	/* For C/C++ projects, get list of typedefs for colorizing */
	if (SCLEX_CPP == lexLanguage)
	{
		const TMWorkspace *workspace = tm_get_workspace();

		/* Assign global keywords */
		if ((workspace) && (workspace->global_tags))
		{
			GPtrArray *g_typedefs = tm_tags_extract(workspace->global_tags
			  , tm_tag_typedef_t | tm_tag_struct_t | tm_tag_class_t);
			if ((g_typedefs) && (g_typedefs->len > 0))
			{
				GString *s = g_string_sized_new(g_typedefs->len * 10);
				for (guint i = 0; i < g_typedefs->len; ++i)
				{
					if (!(TM_TAG(g_typedefs->pdata[i])->atts.entry.scope))
					{
						g_string_append(s, TM_TAG(g_typedefs->pdata[i])->name);
						g_string_append_c(s, ' ');
					}
				}
				SendEditorString(SCI_SETKEYWORDS, 2, s->str);
				g_string_free(s, TRUE);
			}
			g_ptr_array_free(g_typedefs, TRUE);
		}

		/* Assign project keywords */
		if ((workspace) && (workspace->work_object.tags_array))
		{
			GPtrArray *typedefs = tm_tags_extract(workspace->work_object.tags_array
			  , tm_tag_typedef_t | tm_tag_struct_t | tm_tag_class_t);
			if ((typedefs) && (typedefs->len > 0))
			{
				GString *s = g_string_sized_new(typedefs->len * 10);
				for (guint i = 0; i < typedefs->len; ++i)
				{
					if (!(TM_TAG(typedefs->pdata[i])->atts.entry.scope))
					{
						if (!TM_TAG(typedefs->pdata[i])->name)
						{
							g_warning("NULL tag name encountered!");
							sleep(10000);
						}
						g_string_append(s, TM_TAG(typedefs->pdata[i])->name);
						g_string_append_c(s, ' ');
					}
				}
				SendEditorString(SCI_SETKEYWORDS, 1, s->str);
				g_string_free(s, TRUE);
			}
			g_ptr_array_free(typedefs, TRUE);
		}
	}
	else
	{
		SString kw1 = props->GetNewExpand("keywords2.", fileNameForExtension.c_str());
		SendEditorString(SCI_SETKEYWORDS, 1, kw1.c_str());
		SString kw2 = props->GetNewExpand("keywords3.", fileNameForExtension.c_str());
		SendEditorString(SCI_SETKEYWORDS, 2, kw2.c_str());
		SString kw3 = props->GetNewExpand("keywords4.", fileNameForExtension.c_str());
		SendEditorString(SCI_SETKEYWORDS, 3, kw3.c_str());
		SString kw4 = props->GetNewExpand("keywords5.", fileNameForExtension.c_str());
		SendEditorString(SCI_SETKEYWORDS, 4, kw4.c_str());
	}

	SString fold = props->Get("fold");
	SendEditorString(SCI_SETPROPERTY, reinterpret_cast<unsigned long>("fold"),
	                 fold.c_str());

	SString fold_compact = props->Get("fold.compact");
	SendEditorString(SCI_SETPROPERTY, reinterpret_cast<unsigned long>("fold.compact"),
	                 fold_compact.c_str());

	SString fold_comment = props->Get("fold.comment");
	SendEditorString(SCI_SETPROPERTY, reinterpret_cast<unsigned long>("fold.comment"),
					fold_comment.c_str());
	
	SString fold_comment_python = props->Get("fold.comment.python");
	SendEditorString(SCI_SETPROPERTY, reinterpret_cast<unsigned long>("fold.comment.python"),
					fold_comment_python.c_str());
					
	SString fold_quotes_python = props->Get("fold.quotes.python");
	SendEditorString(SCI_SETPROPERTY, reinterpret_cast<unsigned long>("fold.quotes.python"),
					fold_quotes_python.c_str());
					
	SString fold_html = props->Get("fold.html");
	SendEditorString(SCI_SETPROPERTY, reinterpret_cast<unsigned long>("fold.html"),
					fold_html.c_str());


	SString stylingWithinPreprocessor = props->Get("styling.within.preprocessor");
	SendEditorString(SCI_SETPROPERTY, reinterpret_cast<unsigned long>("styling.within.preprocessor"),
	                 stylingWithinPreprocessor.c_str());
	SString ttwl = props->Get("tab.timmy.whinge.level");
	SendEditorString(SCI_SETPROPERTY, reinterpret_cast<unsigned long>("tab.timmy.whinge.level"),
	                 ttwl.c_str());

	characterSet = props->GetInt("character.set");

	SString colour;
	colour = props->Get("caret.fore");
	if (colour.length()) {
		SendEditor(SCI_SETCARETFORE, ColourFromString(colour.c_str()).AsLong());
	}

	SendEditor(SCI_SETCARETWIDTH, props->GetInt("caret.width", 1));

	colour = props->Get("calltip.back");
	if (colour.length()) {
		SendEditor(SCI_CALLTIPSETBACK, ColourFromString(colour.c_str()).AsLong());
	}

	SString caretPeriod = props->Get("caret.period");
	if (caretPeriod.length()) {
		SendEditor(SCI_SETCARETPERIOD, caretPeriod.value());
	}

	SendEditor(SCI_SETEDGECOLUMN, props->GetInt("edge.column", 0));
	SendEditor(SCI_SETEDGEMODE, props->GetInt("edge.mode", EDGE_NONE));
	colour = props->Get("edge.colour");
	if (colour.length()) {
		SendEditor(SCI_SETEDGECOLOUR, ColourFromString(colour.c_str()).AsLong());
	}

	SString selfore = props->Get("selection.fore");
	if (selfore.length()) {
		SendEditor(SCI_SETSELFORE, 1, ColourFromString(selfore.c_str()).AsLong());
	} else {
		SendEditor(SCI_SETSELFORE, 0, 0);
	}
	colour = props->Get("selection.back");
	if (colour.length()) {
		SendEditor(SCI_SETSELBACK, 1, ColourFromString(colour.c_str()).AsLong());
	} else {
		if (selfore.length())
			SendEditor(SCI_SETSELBACK, 0, 0);
		else	// Have to show selection somehow
			SendEditor(SCI_SETSELBACK, 1, Colour(0xC0, 0xC0, 0xC0).AsLong());
	}

	char bracesStyleKey[200];
	sprintf(bracesStyleKey, "braces.%s.style", language.c_str());
	bracesStyle = props->GetInt(bracesStyleKey, 0);

	char key[200];
	SString sval;

	sprintf(key, "calltip.%s.ignorecase", "*");
	sval = props->GetNewExpand(key, "");
	callTipIgnoreCase = sval == "1";
	sprintf(key, "calltip.%s.ignorecase", language.c_str());
	sval = props->GetNewExpand(key, "");
	if (sval != "")
		callTipIgnoreCase = sval == "1";

	sprintf(key, "autocomplete.%s.ignorecase", "*");
	sval = props->GetNewExpand(key, "");
	autoCompleteIgnoreCase = sval == "1";
	sprintf(key, "autocomplete.%s.ignorecase", language.c_str());
	sval = props->GetNewExpand(key, "");
	if (sval != "")
		autoCompleteIgnoreCase = sval == "1";
	SendEditor(SCI_AUTOCSETIGNORECASE, autoCompleteIgnoreCase ? 1 : 0);

	int autoCChooseSingle = props->GetInt("autocomplete.choose.single");
	SendEditor(SCI_AUTOCSETCHOOSESINGLE, autoCChooseSingle),

	// Set styles
	// For each window set the global default style,
	// then the language default style,
	// then the other global styles,
	// then the other language styles

	SendEditor(SCI_STYLERESETDEFAULT, 0, 0);

	sprintf(key, "style.%s.%0d", "*", STYLE_DEFAULT);
	sval = props->GetNewExpand(key, "");
	SetOneStyle(wEditor, STYLE_DEFAULT, sval.c_str());

	sprintf(key, "style.%s.%0d", language.c_str(), STYLE_DEFAULT);
	sval = props->GetNewExpand(key, "");
	SetOneStyle(wEditor, STYLE_DEFAULT, sval.c_str());

	SendEditor(SCI_STYLECLEARALL, 0, 0);

	SetStyleFor(wEditor, "*");
	SetStyleFor(wEditor, language.c_str());

	if (firstPropertiesRead) {
		ReadPropertiesInitial();
	}

	/* Gtk handles it correctly */
	SendEditor(SCI_SETUSEPALETTE, 0);
	
	SendEditor(SCI_SETPRINTMAGNIFICATION, props->GetInt("print.magnification"));
	SendEditor(SCI_SETPRINTCOLOURMODE, props->GetInt("print.colour.mode"));

	int blankMarginLeft = props->GetInt("blank.margin.left", 1);
	int blankMarginRight = props->GetInt("blank.margin.right", 1);
	SendEditor(SCI_SETMARGINLEFT, 0, blankMarginLeft);
	SendEditor(SCI_SETMARGINRIGHT, 0, blankMarginRight);

	SendEditor(SCI_SETMARGINWIDTHN, 1, margin ? marginWidth : 0);
	SendEditor(SCI_SETMARGINWIDTHN, 0, lineNumbers ?lineNumbersWidth : 0);

	bufferedDraw = props->GetInt("buffered.draw", 1);
	SendEditor(SCI_SETBUFFEREDDRAW, bufferedDraw);

	bracesCheck = props->GetInt("braces.check");
	bracesSloppy = props->GetInt("braces.sloppy");

	SString wordCharacters = props->GetNewExpand("word.characters.", fileNameForExtension.c_str());
	if (wordCharacters.length()) {
		SendEditorString(SCI_SETWORDCHARS, 0, wordCharacters.c_str());
	} else {
		SendEditor(SCI_SETWORDCHARS, 0, 0);
	}

	SendEditor(SCI_MARKERDELETEALL, static_cast<unsigned long>( -1));

	int tabSize = props->GetInt("tabsize");
	if (tabSize) {
		SendEditor(SCI_SETTABWIDTH, tabSize);
	}
	indentSize = props->GetInt("indent.size");
	SendEditor(SCI_SETINDENT, indentSize);
	indentOpening = props->GetInt("indent.opening");
	indentClosing = props->GetInt("indent.closing");
	SString lookback = props->GetNewExpand("statement.lookback.", fileNameForExtension.c_str());
	statementLookback = lookback.value();
	statementIndent = GetStyleAndWords("statement.indent.");
	statementEnd =GetStyleAndWords("statement.end.");
	blockStart = GetStyleAndWords("block.start.");
	blockEnd = GetStyleAndWords("block.end.");

	SendEditor(SCI_SETUSETABS, props->GetInt("use.tabs", 1));
	if (props->GetInt("vc.home.key", 1)) {
		AssignKey(SCK_HOME, 0, SCI_VCHOME);
		AssignKey(SCK_HOME, SCMOD_SHIFT, SCI_VCHOMEEXTEND);
	} else {
		AssignKey(SCK_HOME, 0, SCI_HOME);
		AssignKey(SCK_HOME, SCMOD_SHIFT, SCI_HOMEEXTEND);
	}
	SendEditor(SCI_SETHSCROLLBAR, props->GetInt("horizontal.scrollbar", 1));

	SendEditor(SCI_SETFOLDFLAGS, props->GetInt("fold.flags"));

	// To put the folder markers in the line number region
	//SendEditor(SCI_SETMARGINMASKN, 0, SC_MASK_FOLDERS);

	SendEditor(SCI_SETMODEVENTMASK, SC_MOD_CHANGEFOLD);

	// Create a margin column for the folding symbols
	SendEditor(SCI_SETMARGINTYPEN, 2, SC_MARGIN_SYMBOL);

	SendEditor(SCI_SETMARGINWIDTHN, 2, foldMargin ? foldMarginWidth : 0);

	SendEditor(SCI_SETMARGINMASKN, 2, SC_MASK_FOLDERS);
	SendEditor(SCI_SETMARGINSENSITIVEN, 2, 1);
	
	int fold_symbols = props->GetInt("fold.symbols");
	switch(fold_symbols)
	{
		case 0:
			// Arrow pointing right for contracted folders, arrow pointing down for expanded
			DefineMarker(SC_MARKNUM_FOLDEROPEN, SC_MARK_ARROWDOWN, Colour(0, 0, 0), Colour(0, 0, 0));
			DefineMarker(SC_MARKNUM_FOLDER, SC_MARK_ARROW, Colour(0, 0, 0), Colour(0, 0, 0));
			DefineMarker(SC_MARKNUM_FOLDERSUB, SC_MARK_EMPTY, Colour(0, 0, 0), Colour(0, 0, 0));
			DefineMarker(SC_MARKNUM_FOLDERTAIL, SC_MARK_EMPTY, Colour(0, 0, 0), Colour(0, 0, 0));
			DefineMarker(SC_MARKNUM_FOLDEREND, SC_MARK_EMPTY, Colour(0xff, 0xff, 0xff), Colour(0, 0, 0));
			DefineMarker(SC_MARKNUM_FOLDEROPENMID, SC_MARK_EMPTY, Colour(0xff, 0xff, 0xff), Colour(0, 0, 0));
			DefineMarker(SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_EMPTY, Colour(0xff, 0xff, 0xff), Colour(0, 0, 0));
			break;
		case 1:
			// Plus for contracted folders, minus for expanded
			DefineMarker(SC_MARKNUM_FOLDEROPEN, SC_MARK_MINUS, Colour(0xff, 0xff, 0xff), Colour(0, 0, 0));
			DefineMarker(SC_MARKNUM_FOLDER, SC_MARK_PLUS, Colour(0xff, 0xff, 0xff), Colour(0, 0, 0));
			DefineMarker(SC_MARKNUM_FOLDERSUB, SC_MARK_EMPTY, Colour(0xff, 0xff, 0xff), Colour(0, 0, 0));
			DefineMarker(SC_MARKNUM_FOLDERTAIL, SC_MARK_EMPTY, Colour(0xff, 0xff, 0xff), Colour(0, 0, 0));
			DefineMarker(SC_MARKNUM_FOLDEREND, SC_MARK_EMPTY, Colour(0xff, 0xff, 0xff), Colour(0, 0, 0));
			DefineMarker(SC_MARKNUM_FOLDEROPENMID, SC_MARK_EMPTY, Colour(0xff, 0xff, 0xff), Colour(0, 0, 0));
			DefineMarker(SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_EMPTY, Colour(0xff, 0xff, 0xff), Colour(0, 0, 0));
			break;
		case 2:
			// Like a flattened tree control using circular headers and curved joins
			DefineMarker(SC_MARKNUM_FOLDEROPEN, SC_MARK_CIRCLEMINUS, Colour(0xff, 0xff, 0xff), Colour(0x40, 0x40, 0x40));
			DefineMarker(SC_MARKNUM_FOLDER, SC_MARK_CIRCLEPLUS, Colour(0xff, 0xff, 0xff), Colour(0x40, 0x40, 0x40));
			DefineMarker(SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE, Colour(0xff, 0xff, 0xff), Colour(0x40, 0x40, 0x40));
			DefineMarker(SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNERCURVE, Colour(0xff, 0xff, 0xff), Colour(0x40, 0x40, 0x40));
			DefineMarker(SC_MARKNUM_FOLDEREND, SC_MARK_CIRCLEPLUSCONNECTED, Colour(0xff, 0xff, 0xff), Colour(0x40, 0x40, 0x40));
			DefineMarker(SC_MARKNUM_FOLDEROPENMID, SC_MARK_CIRCLEMINUSCONNECTED, Colour(0xff, 0xff, 0xff), Colour(0x40, 0x40, 0x40));
			DefineMarker(SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNERCURVE, Colour(0xff, 0xff, 0xff), Colour(0x40, 0x40, 0x40));
			break;
		case 3:
			// Like a flattened tree control using square headers
			DefineMarker(SC_MARKNUM_FOLDEROPEN, SC_MARK_BOXMINUS, Colour(0xff, 0xff, 0xff), Colour(0x80, 0x80, 0x80));
			DefineMarker(SC_MARKNUM_FOLDER, SC_MARK_BOXPLUS, Colour(0xff, 0xff, 0xff), Colour(0x80, 0x80, 0x80));
			DefineMarker(SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE, Colour(0xff, 0xff, 0xff), Colour(0x80, 0x80, 0x80));
			DefineMarker(SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNER, Colour(0xff, 0xff, 0xff), Colour(0x80, 0x80, 0x80));
			DefineMarker(SC_MARKNUM_FOLDEREND, SC_MARK_BOXPLUSCONNECTED, Colour(0xff, 0xff, 0xff), Colour(0x80, 0x80, 0x80));
			DefineMarker(SC_MARKNUM_FOLDEROPENMID, SC_MARK_BOXMINUSCONNECTED, Colour(0xff, 0xff, 0xff), Colour(0x80, 0x80, 0x80));
			DefineMarker(SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER, Colour(0xff, 0xff, 0xff), Colour(0x80, 0x80, 0x80));
			break;
	}

	// Well, unlike scite, we want it everytime.
	firstPropertiesRead = true;
}

// Anjuta: In our case, we read it everytime
void AnEditor::ReadPropertiesInitial() {
	indentationWSVisible = props->GetInt("view.indentation.whitespace", 1);
	ViewWhitespace(props->GetInt("view.whitespace"));
	SendEditor(SCI_SETINDENTATIONGUIDES, props->GetInt("view.indentation.guides"));
	SendEditor(SCI_SETVIEWEOL, props->GetInt("view.eol"));

	lineNumbersWidth = 0;
	SString linenums = props->Get("margin.linenumber.width");
	if (linenums.length())
		lineNumbersWidth = linenums.value();
	lineNumbers = lineNumbersWidth;
	if (lineNumbersWidth == 0)
		lineNumbersWidth = lineNumbersWidthDefault;

	marginWidth = 0;
	SString margwidth = props->Get("margin.marker.width");
	if (margwidth.length())
		marginWidth = margwidth.value();
	margin = marginWidth;
	if (marginWidth == 0)
		marginWidth = marginWidthDefault;

	foldMarginWidth = props->GetInt("margin.fold.width", foldMarginWidthDefault);
	foldMargin = foldMarginWidth;
	if (foldMarginWidth == 0)
		foldMarginWidth = foldMarginWidthDefault;

	lineNumbers = props->GetInt("margin.linenumber.visible", 0);
	SendEditor(SCI_SETMARGINWIDTHN, 0, lineNumbers ? lineNumbersWidth : 0);

	margin = props->GetInt("margin.marker.visible", 0);
	SendEditor(SCI_SETMARGINWIDTHN, 1, margin ? marginWidth : 0);

	foldMargin = props->GetInt("margin.fold.visible", 1);
	SendEditor(SCI_SETMARGINWIDTHN, 2, foldMargin ? foldMarginWidth : 0);
}



void AnEditor::DefineMarker(int marker, int markerType, Colour fore, Colour back)
{
	SendEditor(SCI_MARKERDEFINE, marker, markerType);
	SendEditor(SCI_MARKERSETFORE, marker, fore.AsLong());
	SendEditor(SCI_MARKERSETBACK, marker, back.AsLong());
}


int AnEditor::GetBookmarkLine( const int nLineStart )
{
	int nNextLine = SendEditor(SCI_MARKERNEXT, nLineStart+1, 1 << ANE_MARKER_BOOKMARK);
	//printf( "...look %d --> %d\n", nLineStart, nNextLine );
	if ( (nNextLine < 0) || (nNextLine == nLineStart) )
		return -1 ;
	else
		return nNextLine;
}

static GList* editors;

static AnEditor*
aneditor_get(AnEditorID id)
{
	AnEditor* ed;
	if(id >= g_list_length(editors))
	{
		g_warning("Invalid AnEditorID supplied");
		return NULL;
	}
	ed = (AnEditor*)g_list_nth_data(editors, (guint)id);
	if(!ed)
	{
		g_warning("Trying to use already destroyed AnEditor Object");
		return NULL;
	}
	return ed;
}

AnEditorID
aneditor_new(gpointer propset)
{
  AnEditor* ed = new AnEditor((PropSetFile*)propset);
  if (!ed)
  {
     g_warning("Memory allocation error.");
     return (AnEditorID)-1;
  }
  editors = g_list_append(editors, ed);
  return (AnEditorID)(g_list_length(editors) - 1);
}

void
aneditor_destroy(AnEditorID id)
{
  AnEditor* ed;
  ed = aneditor_get(id);
  if(!ed) return;
  
  /* We will not remove the editor from the list */
  /* so that already assigned handles work properly */
  /* We'll simply make it NULL to indicate that the */
  /* editor is destroyed */

  g_list_nth(editors, id)->data = NULL;
  delete ed;
}

GtkWidget*
aneditor_get_widget(AnEditorID handle)
{
   AnEditor *ed;
   ed = aneditor_get(handle);
   if(!ed) return NULL;

   // Forced conversion is safe here, so do it.
   return (GtkWidget*)ed->GetID();
}

glong
aneditor_command(AnEditorID handle, gint command, glong wparam, glong lparam)
{
   AnEditor *ed;
   ed = aneditor_get(handle);
   if(!ed) return 0;
   return ed->Command(command, wparam, lparam);
}


