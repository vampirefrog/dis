/* $Id: table.h,v 1.1 1996/10/24 04:27:50 ryo freeze $
 *
 *	�\�[�X�R�[�h�W�F�l���[�^
 *	�e�[�u���������W���[���w�b�_
 *	Copyright (C) 1989,1990 K.Abe
 *	All rights reserved.
 *	Copyright (C) 1997-2010 Tachibana
 *
 */

#ifndef	TABLE_H
#define	TABLE_H

#include "etc.h"	/* peekl */


/* p �ɂ� Ofst �𑫂��Ă��Ȃ��l��n�� */
#define PEEK_BYTE(p) ((UBYTE)(BeginBSS <= (p) ? 0 : *((p) + Ofst)))
#define PEEK_WORD(p) ((UWORD)(BeginBSS <= (p) ? 0 : (((p) + Ofst)[0] << 8) \
							 + ((p) + Ofst)[1]))
#if defined (__mc68020__) || defined (__i386__)
#define PEEK_LONG(p) ((ULONG)(BeginBSS <= (p) ? 0 : peekl ((p) + Ofst)))
#else
#define PEEK_LONG(p) ((ULONG)(BeginBSS <= (p) ? 0 : ((UINTPTR)(p) & 1) ? \
		(peekl ((p) + Ofst - 1) << 8) + ((p) + Ofst)[3] : peekl ((p) + Ofst)))
#endif


/*
	�e�[�u���̍\��
	�P�A�h���X�ɂP�� table ������
	table.formulaptr[ 0 ... table.lines - 1 ] ���e�[�u���̃����o��ێ����Ă���
*/

typedef struct {
#if 0
		int     count;	/* now, count is evaluated each time */
#endif
		opesize id;
		boolean hidden;
		int     line;
		char*   expr;
} formula;

typedef struct {
		address tabletop;
		int     loop;
		int     lines;
		formula *formulaptr;	/* formula �̔z��ւ̃|�C���^ */
} table;

extern void	read_tablefile (char*);
extern table*	search_table (address);

typedef enum {
		PARSE_ANALYZING ,
		PARSE_GENERATING ,
} parse_mode;


extern parse_mode ParseMode;
extern char*	Lexptr;
extern int	Eval_Value;
extern address	Eval_TableTop;
extern address	Eval_PC;
extern int	Eval_Bytes;
extern opesize	Eval_ID;
extern int	Eval_Count;
extern int	Eval_Break;
extern opesize	Eval_SizeID;
extern char	Eval_ResultString[256];


#endif	/* TABLE_H */

/* EOF */
