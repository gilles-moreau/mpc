/*
 * libdict -- red-black tree interface.
 *
 * Copyright (c) 2001-2011, Farooq Mela
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by Farooq Mela.
 * 4. Neither the name Farooq Mela nor the names of contributors may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY FAROOQ MELA ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL FAROOQ MELA BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _RB_TREE_H_
#define _RB_TREE_H_

#include "dict.h"

BEGIN_DECL

typedef struct rb_tree rb_tree;

rb_tree*	rb_tree_new(dict_compare_func cmp_func,
			    dict_delete_func del_func);
dict*		rb_dict_new(dict_compare_func cmp_func,
			    dict_delete_func del_func);
size_t		rb_tree_free(rb_tree* tree);
rb_tree*	rb_tree_clone(rb_tree* tree,
			      dict_key_datum_clone_func clone_func);

bool		rb_tree_insert(rb_tree* tree, void* key,
			       void*** datum_location);
void*		rb_tree_search(rb_tree* tree, const void* key);
bool		rb_tree_remove(rb_tree* tree, const void* key);
size_t		rb_tree_clear(rb_tree* tree);
size_t		rb_tree_traverse(rb_tree* tree, dict_visit_func visit);
size_t		rb_tree_count(const rb_tree* tree);
size_t		rb_tree_height(const rb_tree* tree);
size_t		rb_tree_mheight(const rb_tree* tree);
size_t		rb_tree_pathlen(const rb_tree* tree);
const void*	rb_tree_min(const rb_tree* tree);
const void*	rb_tree_max(const rb_tree* tree);
bool		rb_tree_verify(const rb_tree* tree);

typedef struct rb_itor rb_itor;

rb_itor*	rb_itor_new(rb_tree* tree);
dict_itor*	rb_dict_itor_new(rb_tree* tree);
void		rb_itor_free(rb_itor* tree);

bool		rb_itor_valid(const rb_itor* itor);
void		rb_itor_invalidate(rb_itor* itor);
bool		rb_itor_next(rb_itor* itor);
bool		rb_itor_prev(rb_itor* itor);
bool		rb_itor_nextn(rb_itor* itor, size_t count);
bool		rb_itor_prevn(rb_itor* itor, size_t count);
bool		rb_itor_first(rb_itor* itor);
bool		rb_itor_last(rb_itor* itor);
bool		rb_itor_search(rb_itor* itor, const void* key);
const void*	rb_itor_key(const rb_itor* itor);
void**		rb_itor_data(rb_itor* itor);
bool		rb_itor_remove(rb_itor* itor);

END_DECL

#endif /* !_RB_TREE_H_ */