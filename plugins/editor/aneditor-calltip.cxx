/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    aneditor-calltip.cxx
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

bool AnEditor::StartCallTip_new() {
	TMTagAttrType attrs[] =
	{
		tm_tag_attr_name_t, tm_tag_attr_type_t, tm_tag_attr_none_t
	};
	SString linebuf;
	GetLine(linebuf);
	int current = GetCaretInLine();
	call_tip_node.start_pos = SendEditor(SCI_GETCURRENTPOS);
	call_tip_node.call_tip_start_pos = current;
	
	int braces;
	do {
		braces = 0;
		while (current > 0 && (braces || linebuf[current - 1] != '(')) {
			if (linebuf[current - 1] == '(')
				braces--;
			else if (linebuf[current - 1] == ')')
				braces++;
			current--;
			call_tip_node.start_pos--;
		}
		if (current > 0) {
			current--;
			call_tip_node.start_pos--;
		} else
			break;
		while (current > 0 && isspace(linebuf[current - 1])) {
			current--;
			call_tip_node.start_pos--;
		}
	} while (current > 0 &&
			 !calltipWordCharacters.contains(linebuf[current - 1]));
	
	if (current <= 0)
		return true;

	call_tip_node.startCalltipWord = current -1;
	
	while (call_tip_node.startCalltipWord > 0 &&
		   calltipWordCharacters.contains(linebuf[call_tip_node.startCalltipWord - 1]))
		call_tip_node.startCalltipWord--;

	linebuf.change(current, '\0');
	call_tip_node.rootlen = current - call_tip_node.startCalltipWord;
	
	call_tip_node.max_def = 0;
	call_tip_node.def_index = 0;
	
	const GPtrArray *tags = tm_workspace_find(linebuf.c_str() +
											  call_tip_node.startCalltipWord,
											  tm_tag_prototype_t |
											  tm_tag_function_t |
											  tm_tag_macro_with_arg_t,
											  attrs, FALSE);
	if (tags && (tags->len > 0))
	{
		call_tip_node.max_def = (tags->len < 20)? tags->len:20;
		printf ("Number of calltips found %d\n", tags->len);
		for (unsigned int i = 0; (i < tags->len) && (i < 20); i++) {
			TMTag *tag = (TMTag *) tags->pdata[0];
			char *tmp;
			tmp = g_strdup_printf("%s %s%s", NVL(tag->atts.entry.var_type, ""),
								  tag->name, NVL(tag->atts.entry.arglist, ""));
			call_tip_node.functionDefinition[i] = tmp;
			g_free(tmp);
		}
		char *real_tip;
		if (call_tip_node.max_def > 1)
			real_tip = g_strconcat ("\002",
									call_tip_node.functionDefinition[0].c_str(),
									NULL);
		else
			real_tip = g_strdup (call_tip_node.functionDefinition[0].c_str());
		
		SendEditorString(SCI_CALLTIPSHOW, call_tip_node.start_pos -
						 call_tip_node.rootlen + 1, real_tip);
		g_free (real_tip);
		ContinueCallTip_new();
	}
	return true;
}

static bool IsCallTipSeparator(char ch) {
	return (ch == ',') || (ch == ';');
}

void AnEditor::ContinueCallTip_new() {
	SString linebuf;
	GetLine(linebuf);
	unsigned current = GetCaretInLine();
	int commas = 0;
	
	for (unsigned i = call_tip_node.call_tip_start_pos; i < current; i++) {
		
		unsigned char ch = linebuf[i];

		// check whether the are some other functions nested.
		//	if found	we'll skip them to evitate commas problems
		if ( ch == '(' ) {
			int braces = 1;
			for (unsigned k = i+1; k < linebuf.length(); k++ ) {
				if ( linebuf[k] == '(' ) {
					braces++;
				}
				else
					if ( linebuf[k] == ')' ) {
						braces--;
					}
				if ( braces == 0 ) {
					i = k;
					break;
				}
			}			
		}
		
		if (IsCallTipSeparator(ch))
			commas++;		
	}

	// lets start from 0
	int startHighlight = 0;
	
	while (call_tip_node.functionDefinition[call_tip_node.def_index][startHighlight] &&
		   call_tip_node.functionDefinition[call_tip_node.def_index][startHighlight] != '(')
		startHighlight++;
	
	if (call_tip_node.functionDefinition[call_tip_node.def_index][startHighlight] == '(') {
		startHighlight++;
	}
	
	// printf(const char*, ...
	// -------^
	
	while (call_tip_node.functionDefinition[call_tip_node.def_index][startHighlight] &&
											commas > 0) {
		if (IsCallTipSeparator(call_tip_node.functionDefinition[call_tip_node.def_index][startHighlight] ) ||
			call_tip_node.functionDefinition[call_tip_node.def_index][startHighlight] == ')')
			commas--;
		startHighlight++;
	}
	
	if (IsCallTipSeparator(call_tip_node.functionDefinition[call_tip_node.def_index][startHighlight]) ||
			call_tip_node.functionDefinition[call_tip_node.def_index][startHighlight] == ')')
		startHighlight++;
	
	int endHighlight = startHighlight;
	
	if (call_tip_node.functionDefinition[call_tip_node.def_index][endHighlight])
		endHighlight++;
	
	while (call_tip_node.functionDefinition[call_tip_node.def_index][endHighlight] &&
			!IsCallTipSeparator(call_tip_node.functionDefinition[call_tip_node.def_index][endHighlight])
			&& call_tip_node.functionDefinition[call_tip_node.def_index][endHighlight] != ')')
		endHighlight++;

	SendEditor(SCI_CALLTIPSETHLT, startHighlight, endHighlight);
}

//------------------------------------------------------------------------------
//	we're going to save the current status of call_tip_node in call_tip_node_list
// to let another *new* call_tip to show up
//

void AnEditor::SaveCallTip() {

	CallTipNode *ctn = new CallTipNode;
//	g_message( "***saving calltip..." );
	
	ctn->startCalltipWord = call_tip_node.startCalltipWord;
	ctn->def_index = call_tip_node.def_index;
	ctn->max_def = call_tip_node.max_def;
	for (int i = 0; i < ctn->max_def; i++) {
		ctn->functionDefinition[i] = call_tip_node.functionDefinition[i];
	}
	ctn->start_pos = call_tip_node.start_pos;
	ctn->rootlen = call_tip_node.rootlen;
	ctn->call_tip_start_pos = call_tip_node.call_tip_start_pos;
	
	// push it
	g_queue_push_tail( call_tip_node_queue, ctn );

	SetCallTipDefaults();
}

void AnEditor::ResumeCallTip(bool pop_from_stack) {

	if (pop_from_stack) {
		if (g_queue_is_empty (call_tip_node_queue)) {
			ShutDownCallTip();
			return;
		}
		
		CallTipNode_ptr tmp_node;
		
		// set up next CallTipNode parameters in AnEditor::call_tip_node 
		tmp_node = (CallTipNode_ptr)g_queue_pop_tail( call_tip_node_queue );
	
		g_return_if_fail( tmp_node != NULL );
		
		call_tip_node.startCalltipWord = tmp_node->startCalltipWord;
		call_tip_node.def_index = tmp_node->def_index;
		call_tip_node.max_def = tmp_node->max_def;
		for (int i = 0; i < call_tip_node.max_def; i++)
			call_tip_node.functionDefinition[i] =
				tmp_node->functionDefinition[i];
		call_tip_node.start_pos = tmp_node->start_pos;
		call_tip_node.rootlen = tmp_node->rootlen;
		call_tip_node.call_tip_start_pos = tmp_node->call_tip_start_pos;
		
		// in response to g_malloc on SaveCallTip
		delete tmp_node;
	}
	if (call_tip_node.max_def > 1 && 
		call_tip_node.def_index == 0) {
		
		char *real_tip;
		real_tip = g_strconcat ("\002",
								call_tip_node.functionDefinition[call_tip_node.def_index].c_str(),
								NULL);
		SendEditorString(SCI_CALLTIPSHOW, call_tip_node.start_pos -
						 call_tip_node.rootlen + 1, real_tip);
		g_free (real_tip);
	} else if (call_tip_node.max_def > 1 &&
			   call_tip_node.def_index == (call_tip_node.max_def - 1)) {
		char *real_tip;
		real_tip = g_strconcat ("\001",
								call_tip_node.functionDefinition[call_tip_node.def_index].c_str(),
								NULL);
		SendEditorString(SCI_CALLTIPSHOW, call_tip_node.start_pos -
						 call_tip_node.rootlen + 1, real_tip);
		g_free (real_tip);
	} else if (call_tip_node.max_def > 1) {
		char *real_tip;
		real_tip = g_strconcat ("\001\002",
								call_tip_node.functionDefinition[call_tip_node.def_index].c_str(),
								NULL);
		SendEditorString(SCI_CALLTIPSHOW, call_tip_node.start_pos -
						 call_tip_node.rootlen + 1, real_tip);
		g_free (real_tip);
	} else {
		SendEditorString(SCI_CALLTIPSHOW, call_tip_node.start_pos -
						 call_tip_node.rootlen + 1,
						 call_tip_node.functionDefinition[call_tip_node.def_index].c_str());
	}
}

//------------------------------------------------------------------------------
//

void AnEditor::ShutDownCallTip() {

//	g_message( "***shutdowncalltip: length %d", g_queue_get_length( call_tip_node_queue ));
	
	while ( g_queue_is_empty( call_tip_node_queue ) != TRUE ) {
		CallTipNode_ptr tmp_node;
		
		tmp_node = (CallTipNode_ptr)g_queue_pop_tail( call_tip_node_queue );

		// in response to g_malloc on SaveCallTip
		delete tmp_node;
	}
	// reset
	SetCallTipDefaults();
}

//------------------------------------------------------------------------------
//
void AnEditor::SetCallTipDefaults( ) {

	// we're going to set the default values to this.call_tip_node struct
	call_tip_node.def_index = 0;
	call_tip_node.max_def = 0;
	call_tip_node.start_pos = 0;
	call_tip_node.rootlen = 0;
	call_tip_node.startCalltipWord = 0;	
	call_tip_node.call_tip_start_pos = 0;
}
