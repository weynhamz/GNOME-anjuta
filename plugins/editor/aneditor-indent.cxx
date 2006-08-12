/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    aneditor-indent.cxx
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

#include "aneditor-priv.h"

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

void AnEditor::SetLineIndentation(int line, int indent) {
	if (indent < 0)
		return;
	CharacterRange crange = GetSelection();
	int posBefore = GetLineIndentPosition(line);
	SendEditor(SCI_SETLINEINDENTATION, line, indent);
	int posAfter = GetLineIndentPosition(line);
	int posDifference =  posAfter - posBefore;
	if (posAfter > posBefore) {
		// Move selection on
		if (crange.cpMin >= posBefore) {
			crange.cpMin += posDifference;
		}
		if (crange.cpMax >= posBefore) {
			crange.cpMax += posDifference;
		}
	} else if (posAfter < posBefore) {
		// Move selection back
		if (crange.cpMin >= posAfter) {
			if (crange.cpMin >= posBefore)
				crange.cpMin += posDifference;
			else
				crange.cpMin = posAfter;
		}
		if (crange.cpMax >= posAfter) {
			if (crange.cpMax >= posBefore)
				crange.cpMax += posDifference;
			else
				crange.cpMax = posAfter;
		}
	}
	SetSelection(crange.cpMin, crange.cpMax);
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

#if 0

unsigned int AnEditor::GetLinePartsInStyle(int line, int style1, int style2,
										   SString sv[], int len) {
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
		sv[part++] = s;
	}
	return part;
}

static bool includes(const StyleAndWords &symbols, const SString value) {
	if (symbols.words.length() == 0) {
		return false;
	} else if (IsAlphabetic(symbols.words[0])) {
		// Set of symbols separated by spaces
		size_t lenVal = value.length();
		const char *symbol = symbols.words.c_str();
		while (symbol) {
			const char *symbolEnd = strchr(symbol, ' ');
			size_t lenSymbol = strlen(symbol);
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
	} else {
		// Set of individual characters. Only one character allowed for now
		char ch = symbols.words[0];
		return strchr(value.c_str(), ch) != 0;
	}
	return false;
}

#define ELEMENTS(a)	(sizeof(a) / sizeof(a[0]))

IndentationStatus AnEditor::GetIndentState(int line) {
	// C like language indentation defined by braces and keywords
	IndentationStatus indentState = isNone;
	SString controlWords[20];
	unsigned int parts = GetLinePartsInStyle(line, statementIndent.styleNumber,
		-1, controlWords, ELEMENTS(controlWords));
	for (unsigned int i = 0; i < parts; i++) {
		if (includes(statementIndent, controlWords[i]))
			indentState = isKeyWordStart;
	}
	// Braces override keywords
	SString controlStrings[20];
	parts = GetLinePartsInStyle(line, blockEnd.styleNumber,
		-1, controlStrings, ELEMENTS(controlStrings));
	for (unsigned int j = 0; j < parts; j++) {
		if (includes(blockEnd, controlStrings[j]))
			indentState = isBlockEnd;
		if (includes(blockStart, controlStrings[j]))
			indentState = isBlockStart;
	}
	return indentState;
}

int AnEditor::IndentOfBlockProper(int line) {
	
	if (line < 0)
		return 0;
	
	int indentSize = SendEditor(SCI_GETINDENT);
	int indentBlock = GetLineIndentation(line);
	int minIndent = indentBlock;
	int backLine = line;
	
	IndentationStatus indentState = isNone;
	if (statementIndent.IsEmpty() && blockStart.IsEmpty() && blockEnd.IsEmpty())
		indentState = isBlockStart;	// Don't bother searching backwards

	int lineLimit = line - statementLookback;
	if (lineLimit < 0)
		lineLimit = 0;
	bool noIndentFound = true;
	while ((backLine >= lineLimit) && (indentState == 0)) {
		indentState = GetIndentState(backLine);
		if (indentState != 0) {
			noIndentFound = false;
			indentBlock = GetLineIndentation(backLine);
			if (indentState == isBlockStart) {
				if (!indentOpening)
					indentBlock += indentSize;
			}
			if (indentState == isBlockEnd) {
				if (indentClosing)
					indentBlock -= indentSize;
				if (indentBlock < 0)
					indentBlock = 0;
			}
			if ((indentState == isKeyWordStart) && (backLine == line))
				indentBlock += indentSize;
		} else if (noIndentFound) {
			int currentIndent = GetLineIndentation(backLine);
			minIndent = MIN(minIndent, currentIndent);
		}
		backLine--;
	}
	if (noIndentFound && minIndent > 0)
		indentBlock = minIndent;
	return indentBlock;
}

int AnEditor::IndentOfBlock(int line) {

	int backLine = line;

	/* Find mismatched parenthesis */
	if (lexLanguage == SCLEX_CPP) {
		WindowAccessor acc(wEditor.GetID(), *props);
		int currentPos = SendEditor(SCI_POSITIONFROMLINE, backLine + 1);
		
		while (backLine >= 0) {
			int thisLineStart = SendEditor(SCI_POSITIONFROMLINE, backLine);
			int nextLineStart = currentPos;
			// printf("Scanning at line: %d (%d)\n", backLine, currentPos);
	
			int foundTerminator = false;
			int pos;
			for (pos = nextLineStart - 1; pos >= thisLineStart; pos--) {
	
				// printf ("Style at %d = %d\n", pos, acc.StyleAt(pos));
	
				if ((acc.StyleAt(pos) == 10)) {
					char c[2];
					c[0] = acc[pos];
					c[1] = '\0';
					// printf ("Looking at: \"%s\"\n", c);
					if (includes(blockEnd, c) ||
						includes(blockStart, c) ||
						includes(statementEnd, c))
					{
						// printf ("Found block start/block end/statement end. Breaking\n");
						foundTerminator = true;
						break;
					}
					if (c[0] == '(' || c[0] == '[') {
						int indentBlock = SendEditor(SCI_GETCOLUMN, pos) + 1;// - thisLineStart;
						// printf ("Found Unmatched '(' at col: %d. Returning\n", indentBlock);
						return indentBlock;
					}
					if (c[0] == ')' || c[0] == ']') {
						int braceOpp;
						int lineOpp;
						braceOpp = SendEditor(SCI_BRACEMATCH, pos, 0);
						// printf ("Brace opp. at %d\n", braceOpp);
						if (braceOpp >= 0) {
							currentPos = braceOpp - 1;
							lineOpp = SendEditor (SCI_LINEFROMPOSITION,
												  braceOpp);
							if (backLine != lineOpp) {
								backLine = lineOpp;
								// printf ("Jumping to line %d (%d)\n", backLine, currentPos);
								break;
							} else {
								pos = currentPos;
							}
						} else {
							foundTerminator = true;
							break;
						}
					}
				} else if ((acc.StyleAt(pos) == statementIndent.styleNumber) &&
						   (acc.StyleAt(pos+1) != statementIndent.styleNumber)) {
					char buffer[128];
					if (GetWordAtPosition (buffer, 128, pos)) {
						if (includes (statementIndent, buffer)) {
							printf ("Found keyword terminator before unmatched (\n");
							foundTerminator = true;
							break;
						}
					}
				}
			}
			if (foundTerminator)
				break;
			if (pos < thisLineStart) {
				backLine--;
				currentPos = pos;
			}
		}
	}
	return IndentOfBlockProper(backLine);
}
#endif

void AnEditor::MaintainIndentation(char ch) {
	int eolMode = SendEditor(SCI_GETEOLMODE);
	int curLine = GetCurrentLineNumber();
	int lastLine = curLine - 1;
	int indentAmount = 0;

	if (((eolMode == SC_EOL_CRLF || eolMode == SC_EOL_LF) && ch == '\n') ||
		 (eolMode == SC_EOL_CR && ch == '\r')) {
		if (props->GetInt("indent.automatic")) {
			while (lastLine >= 0 && GetLineLength(lastLine) == 0)
				lastLine--;
		}
		if (lastLine >= 0) {
			indentAmount = GetLineIndentation(lastLine);
		}
		if (indentAmount > 0) {
			SetLineIndentation(curLine, indentAmount);
		}
	}
}

#if 0
void AnEditor::AutomaticIndentation(char ch) {
	CharacterRange crange = GetSelection();
	int selStart = crange.cpMin;
	int curLine = GetCurrentLineNumber();
	int thisLineStart = SendEditor(SCI_POSITIONFROMLINE, curLine);
	int indentSize = SendEditor(SCI_GETINDENT);

	if (blockEnd.IsSingleChar() && ch == blockEnd.words[0]) {
		// Dedent maybe
		if (!indentClosing) {
			if (RangeIsAllWhitespace(thisLineStart, selStart - 1)) {
				int indentBlock = IndentOfBlockProper(curLine - 1);
				SetLineIndentation(curLine, indentBlock - indentSize);
			}
		}
	} else if (!blockEnd.IsSingleChar() && (ch == ' ')) {
		// Dedent maybe
		if (!indentClosing && (GetIndentState(curLine) == isBlockEnd)) {}
	} else if (ch == blockStart.words[0]) {
		// Dedent maybe if first on line and previous line was starting keyword
		if (!indentOpening &&
			(GetIndentState(curLine - 1) == isKeyWordStart)) {
			if (RangeIsAllWhitespace(thisLineStart, selStart - 1)) {
				int indentBlock = IndentOfBlockProper(curLine - 1);
				SetLineIndentation(curLine, indentBlock - indentSize);
			}
		}
	} else if ((ch == '\r' || ch == '\n') && (selStart == thisLineStart)) {
		// printf("New line block\n");
		int indentBlock = IndentOfBlock(curLine - 1);
		if (!indentClosing && !blockEnd.IsSingleChar()) {
			// Dedent previous line maybe
			
			SString controlWords[1];
			// printf ("First if\n");
			
			if (GetLinePartsInStyle(curLine-1, blockEnd.styleNumber,
				-1, controlWords, ELEMENTS(controlWords))) {
				// printf ("Second if\n");
				if (includes(blockEnd, controlWords[0])) {
					// printf ("Third if\n");
					// Check if first keyword on line is an ender
					SetLineIndentation(curLine-1,
									   IndentOfBlockProper(curLine-2)
									   - indentSize);
					// Recalculate as may have changed previous line
					indentBlock = IndentOfBlock(curLine - 1);
				}
			}
		}
		SetLineIndentation(curLine, indentBlock);
		
		// Home cursor.
		if (SendEditor (SCI_GETCOLUMN,
						SendEditor(SCI_GETCURRENTPOS)) < indentBlock)
			SendEditor (SCI_VCHOME);
		
	} else if (lexLanguage == SCLEX_CPP) {
		if ((ch == '\t')) {
		
			int indentBlock = IndentOfBlock(curLine - 1);
			int indentState = GetIndentState (curLine);
			
			if (blockStart.IsSingleChar() && indentState == isBlockStart) {
				if (!indentOpening) {
					if (RangeIsAllWhitespace(thisLineStart, selStart - 1)) {
						// int indentBlock = IndentOfBlockProper(curLine - 1);
						SetLineIndentation(curLine, indentBlock - indentSize);
					}
				}
			} else if (blockEnd.IsSingleChar() && indentState == isBlockEnd) {
				if (!indentClosing) {
					if (RangeIsAllWhitespace(thisLineStart, selStart - 1)) {
						// int indentBlock = IndentOfBlockProper(curLine - 1);
						SetLineIndentation(curLine, indentBlock - indentSize);
					}
				}
			} else {
				SetLineIndentation(curLine, indentBlock);
			}
			
			// Home cursor.
			if (SendEditor (SCI_GETCOLUMN,
							SendEditor(SCI_GETCURRENTPOS)) < indentBlock)
				SendEditor (SCI_VCHOME);
		}
	}
}
#endif
