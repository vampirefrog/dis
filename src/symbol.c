/* $Id: symbol.c,v 1.1 1996/11/07 08:04:02 ryo freeze $
 *
 *	�\�[�X�R�[�h�W�F�l���[�^
 *	�V���{���l�[���Ǘ����W���[��
 *	Copyright (C) 1989,1990 K.Abe
 *	All rights reserved.
 *	Copyright (C) 1997-2010 Tachibana
 *
 */

#include <stdio.h>
#include <string.h>

#include "estruct.h"
#include "etc.h"
#include "global.h"
#include "hex.h"		/* itoxd() */
#include "label.h"
#include "output.h"		/* outputf() */
#include "symbol.h"


USEOPTION	option_g;
short	Output_Symbol = 1;	/* 0:���Ȃ� 1:�萔�̂� 2:.xdef �� */


#define	SYM_BUF_UNIT	16		/* �o�b�t�@�g�厞�̑����v�f�� */
static int	Symbolbufsize;		/* �V���{���o�b�t�@�̐� */
static symbol*	Symtable;		/* �V���{���e�[�u���̓� */
static int	Symnum;			/* �V���{���e�[�u���̗v�f�� */


extern void
init_symtable (void)
{
    return;
}

/*

  ��n��

  �V���{�������X�g�ƃV���{�����o�b�t�@���������K�v�����邯�ǎ蔲��.

*/
extern void
free_symbuf (void)
{
    Mfree (Symtable);
}

extern boolean
is_exist_symbol (void)
{
    return Symnum ? TRUE : FALSE;
}


#ifndef	OSKDIS

/*
	�\�[�X�R�[�h�ɃV���{����`���o�͂���

	����:
		op_equ	.equ �^������("\t.equ\t")
		colon	�R����("::" -C �I�v�V�����ɂ���ĕύX)
	�Ԓl
		�o�͂����V���{����(�s��)

	Human68k �� .x �^���s�t�@�C���`���Ɉˑ�.
*/
extern int
output_symbol_table (const char* op_equ, const char* op_xdef, const char* colon)
{
    char* ptr = (char*)(Top + Head.text + Head.data + Head.offset);
    ULONG end = (ULONG)ptr + Head.symbol
	      - (sizeof (UWORD) + sizeof (address) + 2);
    int out_xdef = (Output_Symbol == 2);
    int count = 0;

    if ((int)ptr & 1 || !Output_Symbol)
	return count;

    while ((ULONG)ptr <= end) {
	UWORD type;
	address adrs;

	type = peekw (ptr);
	ptr += sizeof (UWORD);
	adrs = (address) peekl (ptr);
	ptr += sizeof (address);

	if (*ptr && *ptr != (char)'*') {
	    char buf[16];
	    char* tab;

	    switch (type) {
	    case XDEF_ABS:
		/* �uxxx:: .equ $nn�v�̌`���ŏo�͂��� */
				  /* TAB-2 */
		tab = (strlen (ptr) < (8-2)) ? "\t" : "";
		itoxd (buf, (ULONG) adrs, 8);
		outputf ("%s%s%s%s%s" CR, ptr, colon, tab, op_equ, buf);
		count++;
		break;

	    case XDEF_COMMON:
	    case XDEF_TEXT:
	    case XDEF_DATA:
	    case XDEF_BSS:
	    case XDEF_STACK:
		if (out_xdef) {
		    outputf ("%s%s" CR, op_xdef, ptr);
		    count++;
		}
		break;

	    default:
		break;
	    }
	}

	/* �����������V���{�����΂� */
	while (*ptr++)
	    ;
	ptr += (int)ptr & 1;
    }

    return count;
}


/*
	���x���t�@�C���Œ�`���ꂽ�V���{���̑�����
	�V���{�����̑����ɕύX����

	Human68k �� .x �^���s�t�@�C���`���Ɉˑ�.
*/
static void
change_sym_type (symbol* symbolptr, int type, char* ptr)
{
    symlist* sym = &symbolptr->first;

    do {
	if (strcmp (sym->sym, ptr) == 0) {
	    sym->type = type;
	    return;
	}
	sym = sym->next;
    } while (sym);
}

/*
	���s�t�@�C���ɕt������V���{���e�[�u����o�^����
	���x���t�@�C���̓ǂݍ��݂���ɌĂяo�����

	Human68k �� .x �^���s�t�@�C���`���Ɉˑ�.
*/
extern void
make_symtable (void)
{
    char* ptr = (char*)(Top + Head.text + Head.data + Head.offset);
    ULONG end = (ULONG)ptr + Head.symbol
	      - (sizeof (UWORD) + sizeof (address) + 2);

    if ((int)ptr & 1)
	return;

    while ((ULONG)ptr <= end) {
	UWORD type;
	address adrs;

	type = peekw (ptr);
	ptr += sizeof (UWORD);
	adrs = (address) peekl (ptr);
	ptr += sizeof (address);

	if (!*ptr) {
#if 0
	    eprintf ("�V���{�������󕶎���ł�(%#.6x %#.6x)", type, adrs);
#endif
	} else if (*ptr == (char)'*') {
	    eprintf ("�A�h���X���E���̃V���{���ł�(%#.6x %#.6x %s)\n",
							type, adrs, ptr);
	}

	else {
	    symbol* sym;

	    /* �X�^�b�N�T�C�Y�̎��� */
	    if (type == XDEF_STACK && BeginBSS <= adrs && adrs < BeginSTACK)
		BeginSTACK = adrs;

	    switch (type) {
	    case XDEF_ABS:
		break;

	    case XDEF_COMMON:
		type = XDEF_BSS;
		/* fall through */
	    case XDEF_STACK:
	    case XDEF_TEXT:
	    case XDEF_DATA:
	    case XDEF_BSS:
		sym = symbol_search (adrs);

		if (!sym)
		    regist_label (adrs, DATLABEL | UNKNOWN | SYMLABEL);

		/* -g �w�莞�̓V���{���e�[�u������o�^���Ȃ� */
		/* �������A���������͗��p����		     */
		if (!option_g)
		    add_symbol (adrs, type, ptr);
		else if (sym)
		    change_sym_type (sym, type, ptr);
		break;

	    default:
		eprintf ("���Ή��̃V���{�����ł�(%#.6x %#.6x %s)\n",
							type, adrs, ptr);
		break;

	    }
	}

	/* �����������V���{�����΂� */
	while (*ptr++)
	    ;
	ptr += (int)ptr & 1;
    }
}


/*
	�w�肵�������̃V���{����T��
*/
extern symlist*
symbol_search2 (address adrs, int type)
{
    symbol* symbolptr = symbol_search (adrs);

    if (symbolptr) {
	symlist* sym = &symbolptr->first;

	while (sym->type != (UWORD)type && (sym = sym->next))
	    ;
	return sym;
    }
    return (symlist*)NULL;
}

#endif	/* !OSKDIS */


extern void
add_symbol (address adrs, int type, char *symstr)
{			   /* type == 0 : labelfile�ł̒�` */
    int  i;
    symbol* sym = symbol_search (adrs);

    /* ���ɓo�^�ς݂Ȃ�V���{������ǉ����� */
    if (sym) {
	symlist* ptr = &sym->first;

	while (ptr->next)
	    ptr = ptr->next;

	/* symlist ���m�ۂ��āA�����Ɍq���� */
	ptr = ptr->next = Malloc (sizeof (symlist));

	/* �m�ۂ��� symlist �������� */
	ptr->next = NULL;
	ptr->type = (UWORD)type;
	ptr->sym = symstr;

	return;
    }

    if (Symnum == Symbolbufsize) {
	/* �o�b�t�@���g�傷�� */
	Symbolbufsize += SYM_BUF_UNIT;
	Symtable = Realloc (Symtable, Symbolbufsize * sizeof (symbol));
    }

    for (i = Symnum - 1; (i >= 0) && (Symtable[ i ].adrs > adrs); i--)
	Symtable[ i + 1 ] = Symtable[ i ];

    Symnum++;
    sym = &Symtable[ ++i ];
    sym->adrs = adrs;

    /* �ŏ��̃V���{�����L�^ */
    sym->first.next = NULL;
    sym->first.type = (UWORD)type;
    sym->first.sym  = symstr;

#ifdef	DEBUG
    printf ("type %.4x adrs %.6x sym:%s\n", sym->first.type, sym->.adrs, sym->first.sym);
#endif
}


/*

  adrs ���V���{���e�[�u������{��
  adrs �� common text data bss �̂����ꂩ
  ����������|�C���^�A�łȂ���� NULL ��Ԃ�

*/
extern symbol*
symbol_search (address adrs)
{
    int step;
    symbol* ptr;

    /* �����͑����Ȃ�? */
    if (Symnum == 0)
	return NULL;

    ptr = Symtable;
    for (step = Symnum >> 1; step > 4; step >>= 1)
	if ((ptr + step)->adrs <= adrs)		/* binary search */
	    ptr += step;

    for (; ptr < Symtable + Symnum; ptr++) {
	if (ptr->adrs == adrs)
	    return ptr;
	else
	    if (adrs < ptr->adrs)
		return NULL;
    }
    return NULL;
}


/* �ȉ��͖��g�p�֐� */

#if 0
extern void
del_symbol (address adrs)
{
    symbol* sym = symbol_search (adrs);

    if (sym) {
	symlist* ptr = sym->first.next;

	while (ptr) {
	    symlist* next = ptr->next;
	    Mfree (ptr);
	    ptr = next;
	}

	Symnum--;
	for (; sym < &Symtable[ Symnum ]; sym++)
	    *sym = *(sym + 1);
    }
}
#endif

/* EOF */
