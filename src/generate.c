/* $Id: generate.c,v 1.1 1996/11/07 08:03:36 ryo freeze $
 *
 *	�\�[�X�R�[�h�W�F�l���[�^
 *	�\�[�X�R�[�h�W�F�l���[�g���W���[��
 *	Copyright (C) 1989,1990 K.Abe, 1994 R.ShimiZu
 *	All rights reserved.
 *	Copyright (C) 1997-2010 Tachibana
 *
 */

#include <time.h>		/* time ctime */
#include <ctype.h>		/* isprint */
#include <stdio.h>
#include <stdlib.h>		/* getenv */
#include <string.h>
#include <unistd.h>

#include "disasm.h"
#include "estruct.h"
#include "etc.h"	/* charout, strupr, <jctype.h> */
#include "fpconv.h"
#include "generate.h"
#include "global.h"
#include "hex.h"
#include "include.h"	/* Doscall_mac_path , etc. */
#include "label.h"	/* search_label etc. */
#include "offset.h"	/* depend_address, nearadrs */
#include "output.h"
#include "symbol.h"	/* symbol_search */
#include "table.h"

#if defined (__LIBC__)
  #include <sys/xstart.h>	/* char* _comline */
  #define COMMAND_LINE _comline
#else
  private char*		make_cmdline (char** argv);
  #define COMMAND_LINE  make_cmdline (argv + 1)
  #define NEED_MAKE_CMDLINE
#endif


/* Macros */
#define MIN_BIT(x) ((x) & -(x))

#ifdef	DEBUG
#define	DEBUG_PRINT(arg) eprintf arg
#else
#define	DEBUG_PRINT(arg)
#endif


USEOPTION option_q, option_r, option_x,
	  option_B, option_M, option_N, /* option_Q, */
	  option_S, option_U;


/* main.c */
extern	char	CommentChar;


/* static �֐��v���g�^�C�v */
private lblbuf*		gen (char* section, address end, lblbuf* lptr, int type);
#ifndef OSKDIS
private lblbuf*		gen2(char* section, address end, lblbuf* lptr, int type);
#endif
private void		output_file_open_next (int sect);
private address		textgen (address, address);
private void		labelchange (disasm* code, operand* op);
private void		a7toSP (operand*);
private void		syntax_new_to_old (operand* op);
private address		datagen (address, address, opesize);
private void		datagen_sub (address, address, opesize);

private void		dataout (address, ULONG, opesize);
private void		byteout (address, ULONG, boolean);
private void		wordout (address, ULONG);
private void		longout (address, ULONG);
private void		quadout (address pc, ULONG byte);
private void		floatout (address pc, ULONG byte);
private void		doubleout (address pc, ULONG byte);
private void		extendout (address pc, ULONG byte);
private void		packedout (address pc, ULONG byte);

private void		strgen (address, address);
private void		relgen (address, address);
private void		rellonggen (address, address);
private void		byteout_for_moption (address, char*, int);
private void		zgen (address, address);
private address		tablegen (address, address, lblmode);
private address		tablegen_sub (formula*, address, address, int);
private void		tablegen_label (address, address, lblbuf*);
private void		tablegen_dc (address, lblbuf*, char*, int);
private void		bssgen (address, address, opesize);
private void		label_line_out (address adrs, lblmode mode);
private void		label_line_out_last (address adrs, lblmode mode);
private void		label_op_out (address);
private void		output_opecode (char*);
private char*		mputype_numstr (mputypes m);
private void		makeheader (char*, time_t, char*, char*);

#ifdef	OSKDIS
private void		oskdis_gen (void);
private lblbuf*		idatagen (address end, lblbuf* lptr);
private void		vlabelout (address, lblmode);
private void		wgen (address pc, address pcend);
#endif	/* OSKDIS */


int	SymbolColonNum = 2;
short	sp_a7_flag;		/* a7 �� sp �Əo�͂��邩? */
short	Old_syntax;

int Xtab = 7;			/* /x option TAB level */
int Atab = 8;			/* /a option TAB level */
int Mtab = 5;			/* /M option TAB level */

int Data_width = 8;		/* �f�[�^�̈�̉��o�C�g�� */
int String_width = 60;		/* ������̈�̉��o�C�g�� */
int Compress_len = 64;		/* �f�[�^�o�͂�dcb.?�ɂ���ŏ��o�C�g�� */

size_mode Generate_SizeMode = SIZE_NORMAL;

char	Label_first_char = 'L';	/* ���x���̐擪���� */

char	opsize[] = "bwlqssdxp";

static mputypes	Current_Mputype;
static UWORD	SectionType;


#ifdef	OSKDIS
char*	OS9label [0x100];	/* os9	 F$???(I$???)	*/
char*	MATHlabel[0x100];	/* tcall T$Math,T$???	*/
char*	CIOlabel [0x100];	/* tcall CIO$Trap,C$??? */
#else
char**	IOCSlabel;
char	IOCSCallName[16] = "IOCS";
const char*	Header_filename;
#endif	/* OSKDIS */


/* �[������ */

#ifdef	OSKDIS
#define PSEUDO	"\t"
#define EVEN	"align"
#else
#define PSEUDO	"\t."
#define EVEN	"even"
#define DCB_B	"dcb.b\t"
#define DCB_W	"dcb.w\t"
#define DCB_L	"dcb.l\t"
#define DCB_Q	"dcb.d\t"
#define DCB_S	"dcb.s\t"
#define DCB_D	"dcb.d\t"
#define DCB_X	"dcb.x\t"
#define DCB_P	"dcb.p\t"
#endif	/* !OSKDIS */

#define INCLUDE	"include\t"
#define CPU	"cpu\t"
#define FPID	"fpid\t"
#define EQU	"equ\t"
#define XDEF	"xdef\t"

#define DC_B	"dc.b\t"
#define DC_W	"dc.w\t"
#define DC_L	"dc.l\t"
#define DC_Q	"dc.d\t"
#define DC_S	"dc.s\t"
#define DC_D	"dc.d\t"
#define DC_X	"dc.x\t"
#define DC_P	"dc.p\t"

#define DS_B	"ds.b\t"
#define DS_W	"ds.w\t"
#define DS_L	"ds.l\t"

#define DEFAULT_FPID	1


/* Inline Functions */

static INLINE char*
strcpy2 (char* dst, char* src)
{
	while ((*dst++ = *src++) != 0)
	;
	return --dst;
}


/*

  Check displacement of Relative branch

  return FALSE;      short    �ōς� displacement �� word �̏ꍇ
	�������́Ashort||word �ōς� displacement �� long �̏ꍇ
  return TRUE; �T�C�Y���ȗ��\�ȏꍇ.

  as.x�̃o�O�ւ̑Ή��͍폜(1997/10/10).
  jmp,jsr��code.size==NOTHING�ł���ׁA���֌W(�Ăяo���Ă����Q).

*/

static INLINE boolean
check_displacement (address pc, disasm* code)
{
	long dist = (ULONG)code->jmp - (ULONG)pc;
	unsigned char c = code->opecode[0];

	/* bra, bsr, bcc */
	if (c == 'b') {
	if (code->size == WORDSIZE)
		return ((dist <= 127) && (-dist <= 128 + 2)) ? FALSE : TRUE;
	if (code->size == LONGSIZE)
		return ((dist <= 32767) && (-dist <= 32768 + 2)) ? FALSE : TRUE;
	return TRUE;
	}

	/* dbcc, fdbcc, pdbcc : always 16bit displacement */
	if ((c == 'd') || (code->opecode[1] == (char)'d')) {
	return TRUE;
	}

	/* fbcc, pbcc : 16bit or 32bit displacement */
	{
	if (code->size == LONGSIZE)
		return ((dist <= 32767) && (-dist <= 32768 + 2)) ? FALSE : TRUE;
	return TRUE;
	}

	/* NOT REACHED */
}


/*

  �A�Z���u���\�[�X���o�͂���

  input:
	xfilename	: execute file name
	sfilename	: output file name
	filedate	: execute file' time stamp
	argc	: same to argc given to main()	// ���ݖ��g�p
	argv	: same to argv given to main()

*/
extern void
generate (char* xfilename, char* sfilename,
		time_t filedate, int argc, char* argv[])
{

	/* �j�[���j�b�N�͕K�v */
	Disasm_String = TRUE;
	Current_Mputype = MIN_BIT (MPU_types);

	if (option_x)
	Atab += 2;

	init_output ();
	output_file_open (sfilename, 0);

#ifdef __LIBC__
	/* �W���o�͂ɏ������ޏꍇ�A�W���G���[�o�͂Əo�͐悪�����ꍇ�� */
	/* �\�[�X�R�[�h�o�͒��̕\����}������ */
	if (is_confuse_output ()) {
	eputc ('\n');
	option_q = TRUE;
	}
#endif

	makeheader (xfilename, filedate, argv[0], COMMAND_LINE);

#ifdef	OSKDIS
	oskdis_gen ();
#else
	{
	lblbuf* lptr = next (BeginTEXT);

	lptr = gen  ("\t.text"	CR CR, BeginDATA,  lptr, XDEF_TEXT);
	lptr = gen  ("\t.data"	CR CR, BeginBSS,   lptr, XDEF_DATA);
	lptr = gen2 ("\t.bss"	CR CR, BeginSTACK, lptr, XDEF_BSS);
	lptr = gen2 ("\t.stack" CR CR, Last,	   lptr, XDEF_STACK);

	/* �����ɂ��鑮�� 0 �̃V���{�����o�͂��� */
	SectionType = (UWORD)0;
	label_line_out_last (lptr->label, lptr->mode);

	output_opecode (CR "\t.end\t");
	label_op_out (Head.exec);
	}
#endif	/* !OSKDIS */

	newline (Last);
	output_file_close ();
}


/*

  �P�Z�N�V�������o�͂���

  input:
	section : section name ; ".text" or ".data"
	end     : block end address
	lptr    : lblbuf* ( contains block start address )

*/
private lblbuf*
gen (char* section, address end, lblbuf* lptr, int type)
{
	address  pc = lptr->label;
	lblmode  nmode = lptr->mode;

	/* 0 �o�C�g�̃Z�N�V�����ŁA���V���{�������݂��Ȃ���� */
	/* �Z�N�V�����^�����߂��o�͂��Ȃ�.			    */
	if (pc == end && !symbol_search2 (pc, type))
	return lptr;

	/* �K�v�Ȃ�o�̓t�@�C���� *.dat �ɐ؂芷���� */
	output_file_open_next (type);

	outputa (CR);
	output_opecode (section);
	SectionType = (UWORD)type;

	while (pc < end) {
	lblmode  mode = nmode;
	address  nlabel;

	do {
		lptr = Next (lptr);
	} while (lptr->shift);
	nlabel = lptr->label;
	nmode  = lptr->mode;

	label_line_out (pc, mode);

	if (isPROLABEL (mode)) {
		textgen (pc, nlabel);
		pc = nlabel;
	} else if (isTABLE (mode)) {
		pc = tablegen (pc, nlabel, mode);
		lptr = next (pc);
		nmode = lptr->mode;
	} else {
		datagen (pc, nlabel, mode & 0xff);
		pc = nlabel;
	}
	}

	/* �Z�N�V���������̃��x��(�O����`�V���{���̂�) */
	label_line_out_last (pc, nmode);

	return lptr;
}

#ifndef	OSKDIS
/*

	.bss/.stack ���o�͂���

*/
private lblbuf*
gen2 (char* section, address end, lblbuf* lptr, int type)
{
	address  pc = lptr->label;
	lblmode  nmode = lptr->mode;

	/* 0 �o�C�g�̃Z�N�V�����ŁA���V���{�������݂��Ȃ���� */
	/* �Z�N�V�����^�����߂��o�͂��Ȃ�.			    */
	if (pc == end && !symbol_search2 (pc, type))
	return lptr;

	/* �K�v�Ȃ�o�̓t�@�C���� *.bss �ɐ؂芷���� */
	output_file_open_next (type);

	outputa (CR);
	output_opecode (section);
	SectionType = (UWORD)type;

	while (pc < end) {
	lblmode  mode = nmode;
	address  nlabel;

	lptr   = Next (lptr);
	nlabel = lptr->label;
	nmode  = lptr->mode;

	label_line_out (pc, mode);

	if (isTABLE (mode)) {
		pc = tablegen (pc, nlabel, mode);
		lptr = next (pc);
		nmode = lptr->mode;
	} else {
		bssgen (pc, nlabel, mode & 0xff);
		pc = nlabel;
	}
	}

	/* �Z�N�V���������̃��x��(�O����`�V���{���̂�) */
	label_line_out_last (pc, nmode);

	return lptr;
}


/*

  �K�v�Ȃ�o�̓t�@�C����؂芷����.

*/
private void
output_file_open_next (int sect)
{
	static int now = 0;			/* �ŏ��� .text */
	int type;

	if (!option_S || sect == XDEF_TEXT)
	return;

	/* .data �� -1�A.bss �y�� .stack �� -2 */
	type = (sect >= XDEF_BSS) ? -2 : -1;

	if (now != type) {
	now = type;
	output_file_close ();
	output_file_open (NULL, now);
	}
}

#endif	/* !OSKDIS */


/*
  -M�A-x �I�v�V�����̃^�u�o��
  input:
	tabnum : Mtab or Xtab
	buffer : pointer to output buffer
  return:
	pointer to end of output buffer
*/

static INLINE char*
tab_out (int tab, char* buf)
{
	int len;
	unsigned char c;

	/* �s���܂ł̒����𐔂��� */
	for (len = 0; (c = *buf++) != '\0'; len++)
	if (c == '\t')
		len |= 7;
	buf--;
	len /= 8;			/* ���݂̃^�u�ʒu */

	/* �Œ��̃^�u���o�͂��镪�A���[�v�񐔂� 1 ���炷 */
	tab--;

	/* now, buffer points buffer tail */
	for (tab -= len; tab > 0; tab--)
	*buf++ = '\t';

	*buf++ = '\t';
	*buf++ = CommentChar;
	return buf;
}


/*

  �e�L�X�g�u���b�N���o�͂���

  input :
	pc : text begin address
	pcend : text end address

  return :
	pcend

*/
private address
textgen (address pc, address pcend)
{
	short   made_to_order = (!sp_a7_flag || Old_syntax || option_U);
	address store = pc + Ofst;

	charout ('.');
	PCEND = pcend;

	while (pc < pcend) {
	static char prev_fpuid = DEFAULT_FPID;
	disasm code;
	opesize size;
	address store0 = store;
	address pc0 = pc;
	char  buffer[128];
	char* ptr;
	char* l;			/* IOCS �R�[���� */

	store += dis (store, &code, &pc);

	/* .cpu �؂芷�� */
	if ((code.mputypes & Current_Mputype) == 0) {
		Current_Mputype = MIN_BIT (code.mputypes & MPU_types);
		output_opecode (PSEUDO CPU);
		outputa (mputype_numstr (Current_Mputype));
		newline (pc0);
	}

	/* .fpid �؂芷�� */
	if ((code.fpuid >= 0) && (code.fpuid != prev_fpuid)) {
		prev_fpuid = code.fpuid;
		output_opecode (PSEUDO FPID);
		outputfa ("%d", (int)code.fpuid);
		newline (pc0);
	}

	/* code.size �̓T�C�Y�ȗ����� NOTHING �ɏ�����������̂� */
	/* ���l�̃R�����g�o�͗p�ɁA�{���̃T�C�Y��ۑ����Ă���	   */
	size = code.size;

	if (code.flag == OTHER) {
		if (size == code.default_size && option_N)
		code.size = NOTHING;
	}
	else if (code.jmpea == PCDISP) {
		if ((Generate_SizeMode == SIZE_NORMAL && size != NOTHING
			&& check_displacement (pc, &code))	/* -b0: �\�Ȃ�T�C�Y�ȗ� */
		|| Generate_SizeMode == SIZE_OMIT)	/* -b1: ��ɃT�C�Y�ȗ� */
		code.size = NOTHING;
	}

	if (made_to_order)
		modify_operand (&code);
	else {				/* "sp"�A�������A�V�\�L�Ȃ���ʈ��� */
		if (code.op1.operand[0]) {
		a7toSP (&code.op1);
		labelchange (&code, &code.op1);

		if (code.op2.operand[0]) {
			a7toSP (&code.op2);
			labelchange (&code, &code.op2);

			if (code.op3.operand[0]) {
			a7toSP (&code.op3);
			labelchange (&code, &code.op3);

			if (code.op4.operand[0]) {
				a7toSP (&code.op4);
				labelchange (&code, &code.op4);
			}
			}
		}
		}
	}

	ptr = buffer;
	*ptr++ = '\t';

#ifdef	OSKDIS
#define OS9CALL(n, v, t)  (peekw (store0) == (n) && (v) < 0x100 && (l = (t)[(v)]))
	if (OS9CALL (0x4e40, code.op1.opval, OS9label))		/* trap #0 (OS9) */
		strcat (strcpy (ptr, "OS9\t"), l);
	else if (OS9CALL (0x4e4d, code.op2.opval, CIOlabel))	/* trap #13 (CIO$Trap) */
		strcat (strcpy (ptr, "TCALL\tCIO$Trap,"), l);
	else if (OS9CALL (0x4e4f, code.op2.opval, MATHlabel))	/* trap #15 (T$Math) */
		strcat (strcpy (ptr, "TCALL\tT$Math,"), l);
#else
	if (*(UBYTE*)store0 == 0x70		/* moveq #imm,d0 + trap #15 �Ȃ� */
	 && peekw (store) == 0x4e4f		/* IOCS �R�[���ɂ���		 */
	 && IOCSlabel && (l = IOCSlabel[*(UBYTE*)(store0 + 1)]) != NULL
	 && (pc < pcend)) {
		ptr = strcpy2 (ptr, IOCSCallName);
		*ptr++ = '\t';
		strcpy2 (ptr, l);
		store += 2;
		pc += 2;
		code.opflags &= ~FLAG_NEED_COMMENT;	/* moveq �̃R�����g�͕t���Ȃ� */
	}
#endif	/* OSKDIS */
	else {
		ptr = strcpy2 (ptr, code.opecode);
		if (code.size < NOTHING) {
		*ptr++ = '.';
		*ptr++ = opsize[code.size];
		}

		if (code.op1.operand[0]) {
		*ptr++ = '\t';
		ptr = strcpy2 (ptr, code.op1.operand);

		if (code.op2.operand[0]) {
			if (code.op2.ea != BitField && code.op2.ea != KFactor)
			*ptr++ = ',';
			ptr = strcpy2 (ptr, code.op2.operand);

			if (code.op3.operand[0]) {
			if (code.op3.ea != BitField && code.op3.ea != KFactor)
				*ptr++ = ',';
			ptr = strcpy2 (ptr, code.op3.operand);

			if (code.op4.operand[0]) {
				if (code.op4.ea != BitField && code.op4.ea != KFactor)
				*ptr++ = ',';
				strcpy2 (ptr, code.op4.operand);
			}
			}
		}
		}
	}

	if (code.flag == UNDEF) {	/* -M �Ɠ������ɏo�� */
		strcpy (tab_out (Mtab, buffer), "undefined inst.");
	}

	else if ((code.opflags & FLAG_NEED_COMMENT) && option_M)
		byteout_for_moption (pc0, buffer, size);

	if (option_x)
		byteout_for_xoption (pc0, pc - pc0, buffer);

	outputa (buffer);
	newline (pc0);

	/* rts�Ajmp�Abra �̒���ɋ�s���o�͂���		*/
	/* �������A-B �I�v�V���������w��Ȃ� bra �͏���	*/
	if ((code.opflags & FLAG_NEED_NULSTR)
	 && (option_B || (code.opecode[0] != 'b' && code.opecode[0] != 'B')))
		outputa (CR);

	}

	return pcend;
}



/*

	�o�͌`���̎w��ɏ]���ăI�y�����h���C������.

	1)���W�X�^��"a7"��"sp"�ɂ���("za7"��"zsp"�ɂ���).
	2)�I�y�R�[�h�y�ъe�I�y�����h��啶���ɂ���.
	3)���x���ɕύX�ł��鐔�l�̓��x���ɂ���.
	4)�A�h���b�V���O�\�L���Â������ɂ���.

*/

extern void
modify_operand (disasm *code)
{
	int upper_flag = (option_U && !(code->opflags & FLAG_CANNOT_UPPER));

	if (upper_flag)
	strupr (code->opecode);

	if (code->op1.operand[0]) {
	if (sp_a7_flag) a7toSP (&code->op1);
	if (upper_flag) strupr (code->op1.operand);
	labelchange (code, &code->op1);
	if (Old_syntax) syntax_new_to_old (&code->op1);

	if (code->op2.operand[0]) {
		if (sp_a7_flag) a7toSP (&code->op2);
		if (upper_flag) strupr (code->op2.operand);
		labelchange (code, &code->op2);
		if (Old_syntax) syntax_new_to_old (&code->op2);

		if (code->op3.operand[0]) {
		if (sp_a7_flag) a7toSP (&code->op3);
		if (upper_flag) strupr (code->op3.operand);
		labelchange (code, &code->op3);
		if (Old_syntax) syntax_new_to_old (&code->op3);

		if (code->op4.operand[0]) {
			if (sp_a7_flag) a7toSP (&code->op4);
			if (upper_flag) strupr (code->op4.operand);
			labelchange (code, &code->op4);
			if (Old_syntax) syntax_new_to_old (&code->op4);
		}
		}
	}
	}
}


/*

�@a7 �\�L�� sp �\�L�ɕς���

*/
private void
a7toSP (operand* op)
{
	char *ptr = op->operand;

	switch (op->ea) {
	case AregD:
	break;
	case AregID:
	case AregIDPI:
	ptr++;
	break;
	case AregIDPD:
	ptr += 2;
	break;
	case AregDISP:
	ptr = strchr (ptr, ',') + 1;
	break;
	case AregIDX:
	ptr = strrchr (ptr, ',') - 2;
	if (ptr[1] == '7') {
		ptr[0] = 's';
		ptr[1] = 'p';
	}
	ptr += 3;
	break;
	case PCIDX:
	ptr = strrchr (ptr, ',') + 1;
	break;

	case RegPairID:
	ptr++;
	if (ptr[0] == 'a' && ptr[1] == '7') {
		ptr[0] = 's';
		ptr[1] = 'p';
	}
	ptr += 5;
	break;

	case PCIDXB:
	case PCPOSTIDX:
	case PCPREIDX:
	case AregIDXB:
	case AregPOSTIDX:
	case AregPREIDX:
	if (*++ptr == '[')
		ptr++;
	while (1) {
		if (*ptr == 'z')
		ptr++;
		if (ptr[0] == 'a' && ptr[1] == '7') {
		*ptr++ = 's';
		*ptr++ = 'p';
		}
		if ((ptr = strchr (ptr, ',')) == NULL)
		return;
		ptr++;
	}
	return;

	default:
	return;
	}

	if (ptr[0] == 'a' && ptr[1] == '7') {
	*ptr++ = 's';
	*ptr++ = 'p';
	}

}


/*

	�A�h���b�V���O�`���������̕\�L�ɕς���.

*/

private void
syntax_new_to_old (operand *op)
{
	char *optr = op->operand;
	char *p;

	switch (op->ea) {
	case AbShort:			/* (abs)[.w] -> abs[.w] */
	case AbLong:			/* (abs)[.l] -> abs[.l] */
		p = strchr (optr, ')');
		strcpy (p, p + 1);
		strcpy (optr, optr + 1);
		break;

	case AregIDX:			/* (d8,an,ix) -> d8(an,ix) */
		if (optr[1] == 'a' || optr[1] == 'A')
		break;			/* �f�B�X�v���[�X�����g�ȗ� */
		/* fall through */
	case AregDISP:			/* (d16,an)   -> d16(an)   */
	case PCIDX:			/* (d8,pc,ix) -> d8(pc,ix) */
	case PCDISP:			/* (d16,pc)   -> d16(pc)   */
		if ((p = strchr (optr, ',')) == NULL)
		break;			/* ���Ε��򖽗� */
		*p = '(';
		strcpy (optr, optr + 1);
		break;

	default:
		break;
	}
}



/*

  �f�[�^�u���b�N���o�͂���

  input :
	pc : data begin address
	pcend : data end address
	size : data size

  return :
	pcend

*/
private address
datagen (address pc, address pcend, opesize size)
{
	address next_adrs;

	while (pc < pcend) {
	if ((next_adrs = nearadrs (pc)) < pcend) {
		if (next_adrs != pc) {
		datagen_sub (pc, next_adrs, size);
		pc = next_adrs;
		}
		output_opecode (PSEUDO DC_L);
		label_op_out ((address) peekl (next_adrs + Ofst));
		newline (next_adrs);
		pc += 4;
	}
	else {
		datagen_sub (pc, pcend, size);
		pc = pcend;
	}
	}
	return pcend;
}


/*

  �f�[�^�u���b�N���o�͂���
  ���� : pc ���� pcend �܂łɂ̓A�h���X�Ɉˑ����Ă���Ƃ���͂Ȃ�

  input :
	pc : data begin address
	pcend : data end address
	size : data size

*/
private void
datagen_sub (address pc, address pcend, opesize size)
{
	switch (size) {
	case STRING:
	strgen (pc, pcend);
	break;
	case RELTABLE:
	relgen (pc, pcend);
	break;
	case RELLONGTABLE:
	rellonggen (pc, pcend);
	break;
	case ZTABLE:
	zgen (pc, pcend);
	break;
#ifdef	OSKDIS
	case WTABLE:
	wgen (pc, pcend);
	break;
#endif	/* OSKDIS */
	default:
	dataout (pc, pcend - pc, size);
	break;
	}
}


/*

  �f�[�^�u���b�N�̏o��

  input :
	pc : data begin address
	byte : number of bytes
	size : data size ( UNKNOWN , BYTESIZE , WORDSIZE , LONGSIZE only )

*/
private void
dataout (address pc, ULONG byte, opesize size)
{
	address store = pc + Ofst;
	int odd_flag = (UINTPTR)store & 1;

	DEBUG_PRINT (("dataout : store %x byte %x size %x\n", store, byte, size));
	charout ('#');

	switch (size) {
	case WORDSIZE:
		if (odd_flag || (byte % sizeof (UWORD)))
		break;
		if (byte == sizeof (UWORD)) {
		output_opecode (PSEUDO DC_W);
#if 0
		outputax4_without_0supress (peekw (store));
#else
		outputaxd (peekw (store), 4);
#endif
		newline (pc);
		} else
		wordout (pc, byte);
		return;

	case LONGSIZE:
		if (odd_flag || (byte % sizeof (ULONG)))
		break;
		if (byte == sizeof (ULONG)) {
		output_opecode (PSEUDO DC_L);
#if 0
		outputax8_without_0supress (peekl (store));
#else
		outputaxd (peekl (store), 8);
#endif
		newline (pc);
		} else
		longout (pc, byte);
		return;

	case QUADSIZE:
		if (odd_flag || (byte % sizeof (quadword)))
		break;
		quadout (pc, byte);
		return;

	case SINGLESIZE:
		if (odd_flag || (byte % sizeof (float)))
		break;
		floatout (pc, byte);
		return;

	case DOUBLESIZE:
		if (odd_flag || (byte % sizeof (double)))
		break;
		doubleout (pc, byte);
		return;

	case EXTENDSIZE:
		if (odd_flag || (byte % sizeof (long double)))
		break;
		extendout (pc, byte);
		return;

	case PACKEDSIZE:
		if (odd_flag || (byte % sizeof (packed_decimal)))
		break;
		packedout (pc, byte);
		return;

	case BYTESIZE:
		if (byte == sizeof (UBYTE)) {
		output_opecode (PSEUDO DC_B);
#if 0
		outputax2_without_0supress (*(UBYTE*)store);
#else
		outputaxd (*(UBYTE*)store, 2);
#endif
		newline (pc);
		return;
		}
		break;

	default:
		break;
	}

	byteout (pc, byte, FALSE);
}


/*

  �f�[�^�u���b�N�̏o��

  input :
	pc : data begin address
	byte : number of bytes
	roption_flag : hex comment to string flag
	( case this function is used for string output )

*/
private void
byteout (address pc, ULONG byte, boolean roption_flag)
{
	address store = pc + Ofst;

#ifndef OSKDIS	/* OS-9/68K �� r68 �ɂ� dcb �ɑ�������^�����߂����� */
	if (!roption_flag
	 && (byte >= 1 * 2) && Compress_len && (byte >= Compress_len)) {
	UBYTE* ptr = (UBYTE*) store;
	int val = *ptr;

	while (ptr < store + byte && *ptr == val)
		ptr++;
	if (ptr == store + byte) {
		if (val == 0) {
		output_opecode (PSEUDO DS_B);
		outputfa ("%d", byte);
		} else {
		output_opecode (PSEUDO DCB_B);
		outputfa ("%d,", byte);
		outputax2_without_0supress (val);
		}
		newline (pc);
		return;
	}
	}
#endif	/* !OSKDIS */

	{
	ULONG max = (byte - 1) / Data_width;
	ULONG mod = (byte - 1) % Data_width;
	int i, j;

	for (i = 0; i <= max; i++) {
		if (roption_flag) {
		static char comment[] = ";\t\t";
		comment[0] = CommentChar;
		outputa (comment);
		} else
		output_opecode (PSEUDO DC_B);

		for (j = 1; j < (i == max ? mod + 1 : Data_width); j++) {
		outputax2_without_0supress (*store++);
		outputca (',');
		}
		outputax2_without_0supress (*store++);
		newline (pc);
		pc += Data_width;
	}
	}
}


private void
wordout (address pc, ULONG byte)
{
	address store = pc + Ofst;

#ifndef OSKDIS
	if ((byte >= sizeof (UWORD) * 2) && Compress_len && (byte >= Compress_len)) {
	UWORD* ptr = (UWORD*) store;
	int val = peekw (ptr);

	while ((address) ptr < store + byte && peekw (ptr) == val)
		ptr++;
	if ((address) ptr == store + byte) {
		if (val == 0) {
		output_opecode (PSEUDO DS_W);
		outputfa ("%d", byte / sizeof (UWORD));
		} else {
		output_opecode (PSEUDO DCB_W);
		outputfa ("%d,", byte / sizeof (UWORD));
		outputax4_without_0supress (val);
		}
		newline (pc);
		return;
	}
	}
#endif	/* !OSKDIS */

	{
	int datawidth = (Data_width + sizeof (UWORD) - 1) / sizeof (UWORD);
	ULONG max = (byte / sizeof (UWORD) - 1) / datawidth;
	ULONG mod = (byte / sizeof (UWORD) - 1) % datawidth;
	int i, j;

	for (i = 0; i <= max; i++) {
		output_opecode (PSEUDO DC_W);
		for (j = 1; j < (i == max ? mod + 1 : datawidth); j++) {
		outputax4_without_0supress (peekw (store));
		store += sizeof (UWORD);
		outputca (',');
		}
		outputax4_without_0supress (peekw (store));
		store += sizeof (UWORD);
		newline (pc);
		pc += datawidth * sizeof (UWORD);
	}
	}
}


private void
longout (address pc, ULONG byte)
{
	address store = pc + Ofst;

#ifndef OSKDIS
	if ((byte >= sizeof (ULONG) * 2) && Compress_len && (byte >= Compress_len)) {
	ULONG* ptr = (ULONG*) store;
	int val = peekl (ptr);

	while ((address) ptr < store + byte && peekl (ptr) == val )
		ptr++;
	if ((address) ptr == store + byte) {
		if (val == 0) {
		output_opecode (PSEUDO DS_L);
		outputfa ("%d", byte / sizeof (ULONG));
		} else {
		output_opecode (PSEUDO DCB_L);
		outputfa ("%d,", byte / sizeof (ULONG));
		outputax8_without_0supress (val);
		}
		newline (pc);
		return;
	}
	}
#endif	/* !OSKDIS */

	{
	int datawidth = (Data_width + sizeof (ULONG) - 1) / sizeof (ULONG);
	ULONG max = (byte / sizeof (ULONG) - 1) / datawidth;
	ULONG mod = (byte / sizeof (ULONG) - 1) % datawidth;
	int i, j;

	for (i = 0; i <= max; i++) {
		output_opecode (PSEUDO DC_L);
		for (j = 1; j < (i == max ? mod + 1 : datawidth); j++) {
		outputax8_without_0supress (peekl(store));
		store += sizeof (ULONG);
		outputca (',');
		}
		outputax8_without_0supress (peekl (store));
		store += sizeof (ULONG);
		newline (pc);
		pc += datawidth * sizeof (ULONG);
	}
	}
}


private void
quadout (address pc, ULONG byte)
{
	quadword* store = (quadword*)(pc + Ofst);
	char buf[64];
	int i = (byte / sizeof(quadword));

#ifndef OSKDIS
	if ((i >= 2) && Compress_len && (byte >= Compress_len)
		 && (memcmp (store, store + 1, byte - sizeof(quadword)) == 0)) {
	fpconv_q (buf, store);
	output_opecode (PSEUDO DCB_Q);
	outputfa ("%d,%s", i, buf);
	newline (pc);
	return;
	}
#endif

	do {
	fpconv_q (buf, store++);
	output_opecode (PSEUDO DC_Q);
	outputa (buf);
	newline (pc);
	pc += sizeof(quadword);
	} while (--i);
}


private void
floatout (address pc, ULONG byte)
{
	float* store = (float*)(pc + Ofst);
	char buf[64];
	int i = (byte / sizeof(float));

#ifndef OSKDIS
	if ((i >= 2) && Compress_len && (byte >= Compress_len)
		 && (memcmp (store, store + 1, byte - sizeof(float)) == 0)) {
	fpconv_s (buf, store);
	output_opecode (PSEUDO DCB_S);
	outputfa ("%d,%s", i, buf);
	newline (pc);
	return;
	}
#endif

	do {
	fpconv_s (buf, store++);
	output_opecode (PSEUDO DC_S);
	outputa (buf);
	newline (pc);
	pc += sizeof(float);
	} while (--i);
}


private void
doubleout (address pc, ULONG byte)
{
	double* store = (double*)(pc + Ofst);
	char buf[64];
	int i = (byte / sizeof(double));

#ifndef OSKDIS
	if ((i >= 2) && Compress_len && (byte >= Compress_len)
		 && (memcmp (store, store + 1, byte - sizeof(double)) == 0)) {
	fpconv_d (buf, store);
	output_opecode (PSEUDO DCB_D);
	outputfa ("%d,%s", i, buf);
	newline (pc);
	return;
	}
#endif

	do {
	fpconv_d (buf, store++);
	output_opecode (PSEUDO DC_D);
	outputa (buf);
	newline (pc);
	pc += sizeof(double);
	} while (--i);
}


private void
extendout (address pc, ULONG byte)
{
	long double* store = (long double*)(pc + Ofst);
	char buf[64];
	int i = (byte / sizeof(long double));

#ifndef OSKDIS
	if ((i >= 2) && Compress_len && (byte >= Compress_len)
		 && (memcmp (store, store + 1, byte - sizeof(long double)) == 0)) {
	fpconv_x (buf, store);
	output_opecode (PSEUDO DCB_X);
	outputfa ("%d,%s", i, buf);
	newline (pc);
	return;
	}
#endif

	do {
	fpconv_x (buf, store++);
	output_opecode (PSEUDO DC_X);
	outputa (buf);
	newline (pc);
	pc += sizeof(long double);
	} while (--i);
}


private void
packedout (address pc, ULONG byte)
{
	packed_decimal* store = (packed_decimal*)(pc + Ofst);
	char buf[64];
	int i = (byte / sizeof(packed_decimal));

#ifndef OSKDIS
	if ((i >= 2) && (memcmp (store, store + 1, byte - sizeof(packed_decimal)) == 0)) {
	fpconv_p (buf, store);
	output_opecode (PSEUDO DCB_P);
	outputfa ("%d,%s", i, buf);
	newline (pc);
	return;
	}
#endif

	do {
	fpconv_p (buf, store++);
	output_opecode (PSEUDO DC_P);
	outputa (buf);
	newline (pc);
	pc += sizeof(packed_decimal);
	} while (--i);
}



/*

  ������̏o��

  input :
	pc : string begin address
	pcend : string end address

*/
#ifdef	OSKDIS
#define QUOTE_CHAR	'\"'
#define QUOTE_STR	"\""
#else
#define QUOTE_CHAR	'\''
#define QUOTE_STR	"\'"
#endif	/* OSKDIS */

#define ENTERSTR() \
	if (!strmode) {						\
	if (comma) { outputa ("," QUOTE_STR); column += 2; }	\
	else { outputca (QUOTE_CHAR); column++;}		\
	strmode = TRUE;						\
	}
#define EXITSTR() \
	if (strmode) {		\
	outputca (QUOTE_CHAR);	\
	column++;		\
	strmode = FALSE;	\
	}

static INLINE boolean
is_sb (unsigned char* ptr)
{
	return (isprkana (ptr[0]) && ptr[0] != QUOTE_CHAR);
}

static INLINE boolean
is_mb_zen (unsigned char* ptr)
{
	return (iskanji (ptr[0]) && iskanji2 (ptr[1]));
}

static INLINE boolean
is_mb_han (unsigned char* ptr)
{
	if (ptr[0] == 0x80 || (0xf0 <= ptr[0] && ptr[0] <= 0xf3)) {
	if ((0x20 <= ptr[1] && ptr[1] <= 0x7e)
	 || (0x86 <= ptr[1] && ptr[1] <= 0xfd))
		return TRUE;
	}
	return FALSE;
}

private void
strgen (address pc, address pcend)
{
	address	store = Ofst + pc;
	address	stend = Ofst + pcend;

	charout ('s');

	while (store < stend) {
	address    store0 = store;
	boolean    strmode = FALSE;
	int	column = 0;
	char	comma = 0;		/* 1 �Ȃ� , ���o�͂��� */

	output_opecode (PSEUDO DC_B);

	while (column < String_width && store < stend) {

		if (is_sb (store)) {
		/* ANK ���� */
		ENTERSTR();
		outputca (*(UBYTE*)store++);
		column++;
		} else if (is_mb_zen (store) && store + 1 < stend) {
		/* ��o�C�g�S�p */
		ENTERSTR();
		outputca (*(UBYTE*)store++);
		outputca (*(UBYTE*)store++);
		column += 2;
		} else if (is_mb_han (store) && store + 1 < stend) {
		/* ��o�C�g���p */
		ENTERSTR();
		outputca (*(UBYTE*)store++);
		outputca (*(UBYTE*)store++);
		column++;
		} else {
		unsigned char c = *(UBYTE*)store++;
		EXITSTR();
		if (comma)
			outputca (',');
		outputax2_without_0supress (c);
		column += comma + 3;				/* ,$xx */

		/* \n �܂��� NUL �Ȃ���s���� */
		if (store != stend && (c == '\n' || c == '\0')) {
			/* �������A��ɑ��� NUL �͑S�Ĉ�s�ɔ[�߂� */
			if (*(UBYTE*)store == '\0')
			;
			else
			break;					/* ���s */
		}
		}
		comma = 1;
	}
	if (strmode)
		outputca (QUOTE_CHAR);

	newline (store0 - Ofst);
	if (option_r)
		byteout (store0 - Ofst, store - store0, TRUE);
	}
}
#undef	ENTERSTR
#undef	EXITSTR
#undef	QUOTE_CHAR


/*

  �����e�B�u�I�t�Z�b�g�e�[�u���̏o��

  input :
	pc : relative offset table begin address
	pcend : relative offset table end address

*/
private void
relgen (address pc, address pcend)
{
	char buf[256], tabletop[128];
	char* bufp;
	const address pc0 = pc;

	charout ('r');

	/* �\�߃o�b�t�@�̐擪�� .dc.w ���쐬���Ă��� */
	bufp = strend (strcpy (buf, PSEUDO DC_W));
	if (option_U)
	strupr (buf);

	/* -Lxxxxxx ���쐬���Ă��� */
	tabletop[0] = '-';
	make_proper_symbol (&tabletop[1], pc0);

	while (pc < pcend) {
	int dif = (int)(signed short) peekw (pc + Ofst);
	char* p;

	if ((LONG) (pc0 + dif) < (LONG) BeginTEXT) {
		make_proper_symbol (bufp, BeginTEXT);
		p = strend (bufp);
		*p++ = '-';
		p = itox6d (p, BeginTEXT - (pc0 + dif));
	} else if ((LONG) (pc0 + dif) > (LONG) Last) {
		make_proper_symbol (bufp, Last);
		p = strend (bufp);
		*p++ = '+';
		p = itox6d (p, (pc0 + dif) - Last);
	} else {
		make_proper_symbol (bufp, pc0 + dif);
		p = strend (bufp);
	}

	strcpy (p, tabletop);

	/* ���΃e�[�u���� -x �ŃR�����g���o�͂��� */
	if (option_x)
		byteout_for_xoption (pc, sizeof (WORD), buf);

	outputa (buf);
	newline (pc);

	pc += sizeof (WORD);
	}
}


/*

  �����O���[�h�ȃ����e�B�u�I�t�Z�b�g�e�[�u���̏o��

  input :
	pc : relative offset table begin address
	pcend : relative offset table end address

*/
private void
rellonggen (address pc, address pcend)
{
	char buf[256], tabletop[128];
	char* bufp;
	const address pc0 = pc;

	charout ('R');

	/* �\�߃o�b�t�@�̐擪�� .dc.l ���쐬���Ă��� */
	bufp = strend (strcpy (buf, PSEUDO DC_L));
	if (option_U)
	strupr (buf);

	/* -Lxxxxxx ���쐬���Ă��� */
	tabletop[0] = '-';
	make_proper_symbol (&tabletop[1], pc0);

	while (pc < pcend) {
	int dif = (int) peekl (pc + Ofst);
	char* p;

	if ((LONG) (pc0 + dif) < (LONG) BeginTEXT) {
		make_proper_symbol (bufp, BeginTEXT);
		p = strend (bufp);
		*p++ = '-';
		p = itox6d (p, BeginTEXT - (pc0 + dif));
	} else if ((LONG) (pc0 + dif) > (LONG) Last) {
		make_proper_symbol (bufp, Last);
		p = strend (bufp);
		*p++ = '+';
		p = itox6d (p, (pc0 + dif) - Last);
	} else {
		make_proper_symbol (bufp, pc0 + dif);
		p = strend (bufp);
	}

	strcpy (p, tabletop);

	/* ���΃e�[�u���� -x �ŃR�����g���o�͂��� */
	if (option_x)
		byteout_for_xoption (pc, sizeof (LONG), buf);

	outputa (buf);
	newline (pc);

	pc += sizeof (LONG);
	}
}


/*

  �����O���[�h�e�[�u���̏o��

  input :
	pc : longword table begin address
	pcend : longword table end address

*/
private void
zgen (address pc, address pcend)
{
	address store;
	address limit = pcend + Ofst - sizeof (ULONG);

	charout ('z');

	for (store = pc + Ofst; store <= limit; store += sizeof (ULONG)) {
	char work[128];
	address label = (address) peekl (store);
	char* p;

	if ((LONG) label < (LONG) BeginTEXT) {
		make_proper_symbol (work, BeginTEXT);
		p = strend (work);
		*p++ = '-';
		itox6d (p, (ULONG) (BeginTEXT - label));
	} else if ((LONG) label > (LONG) Last) {
		make_proper_symbol (work, Last);
		p = strend (work);
		*p++ = '+';
		itox6d (p, (ULONG) (label - Last));
	} else
		make_proper_symbol (work, label);

	output_opecode (PSEUDO DC_L);
	outputa (work);
	newline (store - Ofst);
	}

	if (store - Ofst != pcend)
	dataout (store - Ofst, pcend + Ofst - store, UNKNOWN);
}


/*

  ���[�U�[��`�̃e�[�u���̏o��

  input :
	pc : user defined table begin address
	pcend : user defined table end address( not used )
	mode : table attribute( not used )
  return :
	table end address

*/
static boolean	end_by_break;

private address
tablegen (address pc, address pcend, lblmode mode)
{
	int loop, i;
	table* tableptr;

	DEBUG_PRINT (("enter tablegen\n"));
	charout ('t');

	if ((tableptr = search_table (pc)) == NULL)
	err ("Table ������ %06x (!?)\n" , pc);

	end_by_break = FALSE;
	pc = tableptr->tabletop;

	for (loop = 0; loop < tableptr->loop; loop++) {	/* implicit loop */
	for (i = 0; i < tableptr->lines; i++) {		/* line# loop */
		DEBUG_PRINT (("loop=%d i=%d\n", loop, i));
		pc = tablegen_sub (&tableptr->formulaptr[i],
				tableptr->tabletop, pc, i + 1 < tableptr->lines);
#if 0
		if ((lptr = search_label (Eval_PC)) != NULL) {
		DEBUG_PRINT (("THERE IS LABEL (%x)\n", Eval_PC));
		label_line_out (Eval_PC, FALSE);
		}
#endif
		if (end_by_break)
		goto ret;
	}
	}

ret:
	DEBUG_PRINT (("exit tablegen\n"));
	return pc;
}


/*

  ���[�U��`�̃e�[�u���̏o��

  input :
	exprptr : ���[�U��`�̃e�[�u���P�s�̍\����
	tabletop : table top address
	pc : pointing address
	notlast : true iff this line is not last line at table
  return :
	next pointing address

*/

private address
tablegen_sub (formula* exprptr, address tabletop, address pc, int notlast)
{

	ParseMode = PARSE_GENERATING;
	Eval_TableTop = tabletop;
	Eval_Count = 0;
	Eval_PC = pc;

	do {
	extern int	yyparse (void);

	lblbuf*	lptr = next (Eval_PC + 1);
	address	nextlabel = lptr->label;
	int	str_length;
	boolean	pc_inc_inst;

	Eval_ResultString[0] = '\0';
	Lexptr = exprptr->expr;

	DEBUG_PRINT (("parsing %s\n", Lexptr));
	yyparse ();

	/* CRID, BREAKID, EVENID(.even �o�͎�)�ȊO�͑S�� TRUE */
	pc_inc_inst = TRUE;

	switch (exprptr->id) {
	case LONGSIZE:
		tablegen_dc (nextlabel, lptr, PSEUDO DC_L, 4);
		break;
	case WORDSIZE:
		tablegen_dc (nextlabel, lptr, PSEUDO DC_W, 2);
		break;
	case BYTESIZE:
		output_opecode (PSEUDO DC_B);
		outputa (Eval_ResultString);
		newline (Eval_PC);
		Eval_PC++;
		break;

	case SINGLESIZE:
		tablegen_dc (nextlabel, lptr, PSEUDO DC_S, 4);
		break;
	case DOUBLESIZE:
		tablegen_dc (nextlabel, lptr, PSEUDO DC_D, 8);
		break;
	case EXTENDSIZE:
		tablegen_dc (nextlabel, lptr, PSEUDO DC_X, 12);
		break;
	case PACKEDSIZE:
		tablegen_dc (nextlabel, lptr, PSEUDO DC_P, 12);
		break;

#if 0
	case LONGID:
		if (nextlabel < Eval_PC + Eval_Bytes*4)
		tablegen_label (Eval_PC, Eval_PC + Eval_Bytes * 4, lptr);
		else
		datagen (Eval_PC, Eval_PC + Eval_Bytes * 4, LONGSIZE);
		Eval_PC += Eval_Bytes * 4;
		break;

	case WORDID:
		if (nextlabel < Eval_PC + Eval_Bytes * 2)
		tablegen_label (Eval_PC, Eval_PC + Eval_Bytes * 2, lptr);
		else
		datagen (Eval_PC, Eval_PC + Eval_Bytes * 2, WORDSIZE);
		Eval_PC += Eval_Bytes * 2;
		break;
#endif
	case BYTEID:
		if (nextlabel < Eval_PC + Eval_Bytes)
		tablegen_label (Eval_PC, Eval_PC + Eval_Bytes, lptr);
		else
		datagen (Eval_PC, Eval_PC + Eval_Bytes, BYTESIZE);
		Eval_PC += Eval_Bytes;
		break;
	case ASCIIID:
		if (nextlabel < Eval_PC + Eval_Bytes)
		tablegen_label (Eval_PC, Eval_PC + Eval_Bytes, lptr);
		else
		strgen (Eval_PC, Eval_PC + Eval_Bytes);
		Eval_PC += Eval_Bytes;
		break;
	case ASCIIZID:
		str_length = strlen ((char*)Eval_PC + Ofst) + 1;
		if (nextlabel < Eval_PC + str_length)
		tablegen_label (Eval_PC, Eval_PC + str_length, lptr);
		else
		strgen (Eval_PC, Eval_PC + str_length);
		Eval_PC += str_length;
		break;
	case LASCIIID:
		str_length = *(unsigned char*) (Eval_PC + Ofst);
		datagen (Eval_PC, Eval_PC + 1, BYTESIZE);
		Eval_PC++;
		if (nextlabel < Eval_PC + str_length)
		tablegen_label (Eval_PC, Eval_PC + str_length, lptr);
		else
		strgen (Eval_PC, Eval_PC + str_length);
		Eval_PC += str_length;
		break;
	case EVENID:
		output_opecode (PSEUDO EVEN);
		newline (Eval_PC);
		if ((UINTPTR)Eval_PC & 1)
		Eval_PC++;	/* pc_inc_inst = TRUE; */
		else
		pc_inc_inst = FALSE;
		break;
	case CRID:
		newline (Eval_PC);
		pc_inc_inst = FALSE;
		break;
	case BREAKID:
		if (Eval_Break) {
		DEBUG_PRINT (("END_BY_BREAK!(%x)\n", (unsigned int) Eval_PC))
		end_by_break = TRUE;
		goto tableend;
		}
		pc_inc_inst = FALSE;
		break;
	default:	/* reduce warning message */
		break;
	}

	if ((lptr = search_label (Eval_PC)) != NULL) {
		DEBUG_PRINT (("THERE IS LABEL sub (%x)%x\n",
		(unsigned int) Eval_PC, lptr->mode));

#if 0	/* does not work properly */
		if (isTABLE (lptr->mode) && notlast)
		eprintf ("\nEncounter another table in table(%x).\n", Eval_PC);
#endif
		if (isENDTABLE (lptr->mode)) {
		DEBUG_PRINT (("ENCOUNTER ENDTABLE(%x)", (unsigned int) Eval_PC));
		if (notlast && (exprptr+1)->id == EVENID) {
			/* if .even here.... */
			output_opecode (PSEUDO EVEN);
			newline (Eval_PC);
			Eval_PC += (UINTPTR)Eval_PC & 1;
		}
		end_by_break = TRUE;
		goto tableend;
		} else if (pc_inc_inst)
		/* labelout is not required at end of table */
		label_line_out (Eval_PC, FALSE);
	}
	} while (--Eval_Count > 0);

 tableend:
	DEBUG_PRINT (("exit tablegen_sub()\n"));
	return Eval_PC;
}

static void
tablegen_dc (address nextlabel, lblbuf* lptr, char* op, int size)
{
	if (nextlabel < Eval_PC + size)
	tablegen_label (Eval_PC, Eval_PC + size, lptr);
	else {
	output_opecode (op);
	outputa (Eval_ResultString);
	newline (Eval_PC);
	}
	Eval_PC += size;
}


/*

  �e�[�u�����Ƀ��x�������݂����ꍇ�̃f�[�^�u���b�N�o��

  input :
	pc : point address
	pcend : point address' end address
	lptr : �e�[�u�����̃��x��

*/
private void
tablegen_label (address pc, address pcend, lblbuf* lptr)
{
	address nlabel = lptr->label;
	lblmode nmode = lptr->mode;

	DEBUG_PRINT (("tablegen_label(%x,%x,%x)\n", (unsigned int)pc,
			(unsigned int) pcend, (unsigned int) nlabel));

	pc = datagen (pc, nlabel, nmode & 0xff);
	while (pc < pcend) {
	lblmode mode = nmode;

	lptr = next (pc + 1);
	while (lptr->shift)
		lptr = Next (lptr);

	nlabel = lptr->label;
	nmode = lptr->mode;

	label_line_out (pc, mode);
	pc = isPROLABEL (mode) ? textgen (pc, min (nlabel, pcend))
				   : datagen (pc, min (nlabel, pcend), mode & 0xff);
	}

	DEBUG_PRINT (("exit tablegen_label\n"));
}



/*

  -M �I�v�V�����̃R�����g����

  input :
	pc : address of 'cmpi.?, move.b, addi.b, subi.? #??' instruction
	buffer : pointer to output buffer

*/
#define IMM_B1 (*(unsigned char*) (store + 1))
#define IMM_W1 (*(unsigned char*) (store + 0))
#define IMM_W2 (*(unsigned char*) (store + 1))
#define IMM_L1 (*(unsigned char*) (store + 0))
#define IMM_L2 (*(unsigned char*) (store + 1))
#define IMM_L3 (*(unsigned char*) (store + 2))
#define IMM_L4 (*(unsigned char*) (store + 3))

private void
byteout_for_moption (address pc, char* buffer, int size)
{
	char* p;
	address store = pc + Ofst;

	/* moveq.l �͑��l�̈ʒu���Ⴄ */
	if (size == LONGSIZE && (*(char*) store) >= 0x70)
	size = BYTESIZE;
	else
	store += 2;

	/* �\���\�ȕ��������ׂ� */
	if (size == BYTESIZE) {
	if (!isprint (IMM_B1))
		return;
	}
	else if (size == WORDSIZE) {
	/* ��ʃo�C�g�͕\���\�������� 0 */
	if (IMM_W1 && !isprint (IMM_W1))
		return;

	/* ���ʃo�C�g�͕K���\���\ */
	if (!isprint (IMM_W2))
		return;
	}
	else if (size == LONGSIZE) {
	/* �ŏ�ʃo�C�g�͕\���\�������� 0 */
	if (IMM_L1 && !isprint (IMM_L1))
		return;

	/* �^�񒆂̓�o�C�g�͕K���\���\ */
	if (!isprint (IMM_L2) || !isprint (IMM_L3))
		return;

	/* �ŉ��ʃo�C�g�͕\���\�������� 0 */
	if (IMM_L4 && !isprint (IMM_L4))
		return;
	}
	else {	/* pack, unpk */
#ifdef PACK_UNPK_LOOSE
	/* ��ʃo�C�g�͕\���\�������� 0 */
	if (IMM_W1 && !isprint (IMM_W1))
		return;

	/* ���ʃo�C�g�͕\���\�������� 0 */
	if (IMM_W2 && !isprint (IMM_W2))
		reutrn;

	/* �����������Ƃ� 0 �ł͂����Ȃ� */
	if (IMM_W1 == 0 && IMM_W2 == 0)
		return;
#else
	/* ��o�C�g�Ƃ��K���\���\ */
	if (!isprint (IMM_W1) || !isprint (IMM_W2))
		return;
#endif
	}

	p = tab_out (Mtab, buffer);
	*p++ = '\'';

	if (size == BYTESIZE) {
	*p++ = IMM_B1;
	}
	else if (size == WORDSIZE) {
	if (IMM_W1)
		*p++ = IMM_W1;
	*p++ = IMM_W2;
	}
	else if (size == LONGSIZE) {
	if (IMM_L1)
		*p++ = IMM_L1;
	*p++ = IMM_L2;
	*p++ = IMM_L3;
	if (IMM_L4)
		*p++ = IMM_L4;
	else {
		strcpy (p, "'<<8");
		return;
	}
	}
	else {	/* pack, unpk */
#ifdef PACK_UNPK_LOOSE
	if (IMM_W1)
		*p++ = IMM_W1;
	if (IMM_W2)
		*p++ = IMM_W2;
	else {
		strcpy (p, "'<<8");
		return;
	}
#else
	*p++ = IMM_W1;
	*p++ = IMM_W2;
#endif
	}

	*p++ = '\'';
	*p++ = '\0';
}
#undef IMM_B1
#undef IMM_W
#undef IMM_W1
#undef IMM_W2
#undef IMM_L1
#undef IMM_L2
#undef IMM_L3
#undef IMM_L4



/*

  -x �I�v�V�����̃R�����g����

  input :
	pc : address of instruction
	byte : instruction length in byte
	buffer : pointer to output buffer

*/
extern void
byteout_for_xoption (address pc, ULONG byte, char* buffer)
{
	int i;
	char c;
	address store = pc + Ofst;
	char* p = tab_out (Xtab, buffer);

	for (c = '\0', i = 0; i < byte; i += 2, store += 2) {
	if (c)
		*p++ = c;		/* ���ڂ���̓J���}���K�v */
	*p++ = '$';
	p = itox4_without_0supress (p, peekw (store));
	c = ',';
	}
}




/*

  bss �̈�̏o��

  input :
	pc : data begin address
	pcend : data end address
	size : data size

*/
private void
bssgen (address pc, address nlabel, opesize size)
{
	ULONG byte;

	charout ('$');
	nlabel = (address) min ((ULONG) nlabel, (ULONG) Last);
	byte = nlabel - pc;

	if ((LONG)nlabel >= 0) {
	if (size == WORDSIZE && byte == 2)
		output_opecode (PSEUDO DS_W "1");
	else if (size == LONGSIZE && byte == 4)
		output_opecode (PSEUDO DS_L "1");
	else {
		output_opecode (PSEUDO DS_B);
		outputfa ("%d", byte);
	}
	newline (pc);
	}
}


/*

  ���x���̕t���ւ�

  input :
	code : disassembled opecode
	operand : operand ( op1,op2,op3 or op4 )

  return :
	TRUE if output is 'label[+-]??' style ( used to avoid as.x bug )
	(��)as.x�ɂ͑Ή����Ȃ��Ȃ����ׁA�Ԓl�͍폜.

	Ablong		PCDISP		IMMED	    PCIDX
	0	(label)		(label,pc)	#label	    (label,pc,ix)
	1	(label-$num)	(label-$num,pc)	#label-$num (label-$num,pc,ix)
	2	(label+$num)	(label+$num,pc)	#label+$num (label+$num,pc,ix)

(od�̓A�h���X�ˑ�����exod��4�̎�����label������)
	PCIDXB		    PCPOSTIDX		    PCPREIDX
	(label,pc,ix)	    ([label,pc],ix,label)   ([label,pc,ix],label)

(bd,od�̓A�h���X�ˑ�����exbd,exod�����ꂼ��4�̎�����label��)
	AregIDXB	    AregPOSTIDX		    AregPREIDX
	(label,an,ix)	    ([label,an],ix,label)   ([label,an,ix],label)

*/
private void
labelchange (disasm* code, operand* op)
{
	address arg1;
	LONG shift;

	if (op->opval != (address)-1 &&
	(op->labelchange1 ||
	 ((op->ea == AbLong || ((op->ea == IMMED) && (code->size2 == LONGSIZE)))
	   && INPROG (op->opval, op->eaadrs))
	)
	   ) {
	char buff[64];
	char* base = buff;
	char* ext_operand = NULL;

	switch (op->ea) {
	default:
	case IMMED:
		break;

	case AbLong:
		ext_operand = ")";
		break;

	case AregPOSTIDX:
	case AregPREIDX:
		if (!INPROG (op->opval, op->eaadrs))
		goto bd_skip;
		ext_operand = strpbrk (op->operand, "],");
		break;

	case PCDISP:
		if ((char)op->labelchange1 < 0)
		break;		/* bsr label ���͊��ʂȂ� */
		/* fall through */
	case AregIDXB:
	case PCIDX:
		ext_operand = strchr (op->operand, ',');
		break;
	case PCIDXB:
		ext_operand = strchr (op->operand, ',');
		goto check_size;
	case PCPOSTIDX:
	case PCPREIDX:
		ext_operand = strpbrk (op->operand, "],");
check_size: if (ext_operand[-2] == (char)'.')
		ext_operand -= 2;	/* �T�C�Y�t���Ȃ�t�������� */
		break;
	}

	if ((LONG)op->opval < (LONG)BeginTEXT) {
		arg1 = BeginTEXT;
		shift = (LONG)op->opval - (LONG)BeginTEXT;
	} else if ((ULONG)op->opval > (ULONG)Last) {
		arg1 = Last;
		shift = (ULONG)op->opval - (ULONG)Last;
	} else {
		lblbuf* label_ptr = search_label (op->opval);

		if (!label_ptr)
		/* ������ opval ����O�̃��x����T���� */
		/* �}shift �`���ɂ���ׂ���??		 */
		return;
		shift = label_ptr->shift;
		arg1 = op->opval - shift;
	}

	switch (op->ea) {
	default:
		break;
	case IMMED:
		*base++ = '#';
		break;
	case PCDISP:
		if ((char)op->labelchange1 < 0)
		break;		/* bsr label ���͊��ʂȂ� */
		/* fall through */
	case AbLong:
	case PCIDX:
	case AregIDXB:
	case PCIDXB:
		*base++ = '(';
		break;
	case AregPOSTIDX:
	case AregPREIDX:
	case PCPOSTIDX:
	case PCPREIDX:
		*base++ = '(';
		*base++ = '[';
		break;
	}

	base = make_symbol (base, arg1, shift);

	if (ext_operand)
		strcpy (base, ext_operand);

	strcpy (op->operand, buff);
	}

bd_skip:


/* �A�E�^�f�B�X�v���[�X�����g�p */
	if (op->labelchange2 && op->opval2 != (address)-1
	&& INPROG (op->opval2, op->eaadrs2))
	{
	char* ptr;

	if ((LONG)op->opval2 < (LONG)BeginTEXT) {
		arg1 = BeginTEXT;
		shift = (LONG)op->opval2 - (LONG)BeginTEXT;
	} else if ((ULONG)op->opval2 > (ULONG)Last) {
		arg1 = Last;
		shift = (ULONG)op->opval2 - (ULONG)Last;
	} else {
		lblbuf* label_ptr = search_label (op->opval2);

		if (!label_ptr)
		return;
		shift = label_ptr->shift;
		arg1 = op->opval2 - shift;
	}

	ptr = make_symbol (strrchr (op->operand, ',') + 1, arg1, shift);
	*ptr++ = ')';
	*ptr++ = '\0';
	}
}


/* ���x����`�s�̃R�����o�� */
static INLINE void
add_colon (char* ptr, int xdef)
{
	if (SymbolColonNum) {
	*ptr++ = ':';
	/* -C3 ���́A-C2 ���O����`�Ȃ� :: */
	if (SymbolColonNum > 2 || (SymbolColonNum == 2 && xdef))
		*ptr++ = ':';
	}
	*ptr = '\0';
}

/*

  ���x����`�s�o��

  input :
	adrs : label address
	mode : label mode

  �V���{��������`����Ă���΁u�V���{����::�v�A����`�Ȃ�uLxxxxxx:�v
  �Ȃǂ̌`���ŏo�͂���(�R�����̐��⃉�x���擪�����̓I�v�V�����̏�Ԃŕς��).
  �V���{�������ɂ���Ă͑S���o�͂���Ȃ����Ƃ�����.

*/
private void
label_line_out (address adrs, lblmode mode)
{
	char  buf[128];
	symbol*  symbolptr;

	if (isHIDDEN (mode))
	return;

	if (Exist_symbol && (symbolptr = symbol_search (adrs))) {
	symlist* sym = &symbolptr->first;
	do {
		if (sym->type == SectionType || (sym->type == 0)) {
		add_colon (strend (strcpy (buf, sym->sym)), sym->type);
		outputa (buf);
		newline (adrs);
		}
		sym = sym->next;
	} while (sym);
	return;
	}

	/* �V���{������������� Lxxxxxx �̌`���ŏo�͂��� */
	buf[0] = Label_first_char;
	add_colon (itox6_without_0supress (&buf[1], (ULONG)adrs), 0);
	outputa (buf);
	newline (adrs);
}

private void
label_line_out_last (address adrs, lblmode mode)
{
	char  buf[128];
	symbol*  symbolptr;

	if (isHIDDEN (mode))
	return;

	if (Exist_symbol && (symbolptr = symbol_search (adrs))) {
	symlist* sym = &symbolptr->first;
	do {
		/* SectionType == 0 �̃V���{�����͏o�͂��Ȃ�����.	*/
		/* ����͎��̃Z�N�V�����̐擪�ŏo�͂���.		*/
		if (sym->type == SectionType) {
		add_colon (strend (strcpy (buf, sym->sym)), sym->type);
		outputa (buf);
		newline (adrs);
		}
		sym = sym->next;
	} while (sym);
	return;
	}

	/* �\�[�X�R�[�h�Ō�̃��x���͐�΂ɏo�͂���K�v������. */
	if (SectionType == 0)
	label_line_out (adrs, mode);

}



/*

  �I�y�����h�Ƃ��Ẵ��x���o��

  input :
	adrs : label address
	mode : label mode

  .end�y��.dc.l�^�����߂̃I�y�����h���쐬����ׂɌĂ΂��.
  �R�����͕t���Ȃ�.
  +-sht���t���\��������.

*/
private void
label_op_out (address adrs)
{
	char buf[128];

	make_proper_symbol (buf, adrs);
	outputa (buf);
}


/*

  ���x���V���{���̐���

  input :
	buff : buffer of label symbol
	adrs : address

  �ł��߂��V���{����T���A�����l���Ȃ����shift�l���v�Z����
  make_symbol()���Ăяo��.

*/
extern void
make_proper_symbol (char* buf, address adrs)
{
	address arg1;
	LONG shift;

	if ((LONG)adrs < (LONG)BeginTEXT) {		/* must be LONG, not ULONG */
	arg1 = BeginTEXT;
	shift = (LONG)adrs - (ULONG)BeginTEXT;
	} else if ((ULONG)adrs > (ULONG)Last) {
	arg1 = Last;
	shift = (ULONG)adrs - (ULONG)Last;
	} else {
	lblbuf* lblptr = search_label (adrs);

	if (lblptr && (shift = lblptr->shift) != 0)
		arg1 = adrs - shift;
	else {
		arg1 = adrs;
		shift = 0;
	}
	}
	make_symbol (buf, arg1, shift);
}


/*

  �V���{���𐶐�����

  input :
	ptr : symbol buffer
	adrs : address
	sft : shift count

  return :
	pointer to generated symbol' tail

  �V���{���e�[�u��������΁Aadrs �̓V���{���ɒu������A�Ȃ���� L?????? �`���ƂȂ�
  sft != 0 �Ȃ炻�̌�� +sft or -sft ����

  ���x����`�s�̐����ɂ�label_line_out()���g������.

*/
extern char*
make_symbol (char* ptr, address adrs, LONG sft)
{
	symbol* symbol_ptr;

	if (Exist_symbol && (symbol_ptr = symbol_search (adrs)) != NULL) {
	strcpy (ptr, symbol_ptr->first.sym);
	ptr = strend (ptr);
	} else {
	*ptr++ = Label_first_char;
	ptr = itox6_without_0supress (ptr, (ULONG)adrs);
	}

	if (sft == 0)
	return ptr;

	if (sft > 0)
	*ptr++ = '+';
	else {
	*ptr++ = '-';
	sft = -sft;
	}
	return itox6d (ptr, sft);
}


private const char*
ctimez (time_t* clock)
{
	char* p = ctime (clock);
	char* n;

	if (p == NULL)
	return "(invalid time)";

	n = strchr (p, '\n');
	if (n)
	*n = '\0';
	return p;
}


static INLINE FILE*
open_header_file (void)
{
	FILE* fp = NULL;
	const char* fname = Header_filename ? : getenv ("dis_header");

	if (fname && *fname && (fp = fopen (fname, "rt")) == NULL) {
#if 0
	eprintf ("\n"
		 "�w�b�_�L�q�t�@�C�� \"%s\" ���I�[�v���o���܂���.\n"
		 "�W���̓��e���o�͂��܂�.\n", file);
#else
	eprintf ("\n�w�b�_�L�q�t�@�C�����I�[�v���o���܂���.\n");
#endif
	}

	return fp;
}


/*

  �w�b�_������

  input :
	filename	: execute filename
	filedate	: execute file' time stamp
	argv0	: same to argv[0] given to main()
	comline	: commandline

*/
private void
makeheader (char* filename, time_t filedate, char* argv0, char* comline)
{
	char  buffer[256];
	char* envptr;
	char  cc = CommentChar;

	static char	op_include[]	= PSEUDO INCLUDE;
	static char	op_cpu[]	= PSEUDO CPU;
	static char	op_equ[]	= PSEUDO EQU;
	static char	op_xdef[]	= PSEUDO XDEF;

	if (option_U) {
	strupr (op_include);
	strupr (op_cpu);
	strupr (op_equ);
	strupr (op_xdef);
	}

	outputf ("%c=============================================" CR, cc);
	outputf ("%c  Filename %s" CR, cc, filename);
	outputf ("%c  Time Stamp %s" CR, cc, ctimez (&filedate));
	outputf ("%c" CR, cc);
#ifndef OSKDIS
	outputf ("%c  Base address %06x" CR, cc, Head.base);
	outputf ("%c  Exec address %06x" CR, cc, Head.exec);
	outputf ("%c  Text size    %06x byte(s)" CR, cc, Head.text);
	outputf ("%c  Data size    %06x byte(s)" CR, cc, Head.data);
	outputf ("%c  Bss  size    %06x byte(s)" CR, cc, Head.bss);
#endif	/* !OSKDIS */
	outputf ("%c  %d Labels" CR, cc, get_Labelnum ());
	{
	time_t	t = time (NULL);
	outputf ("%c  Code Generate date %s" CR, cc, ctimez (&t));
	}
	if ((envptr = getenv (DIS_ENVNAME)) != NULL)
	outputf ("%c  Environment %s" CR, cc, envptr);

	outputf ("%c  Commandline %s %s" CR, cc, argv0, comline);
	outputf ("%c          DIS version %s" CR, cc, Version);

#ifdef	OSKDIS
	outputf ("%c********************************************" CR, cc);

	outputf ("%c  Revision    %04x"	CR, cc, HeadOSK.sysrev);
	outputf ("%c  Module Size %d bytes" CR, cc, HeadOSK.size);
	outputf ("%c  Owner       %d.%d"	CR, cc, HeadOSK.owner >> 16,
						HeadOSK.owner & 0xffff);
	outputf ("%c  Name Offset %08x" CR, cc, HeadOSK.name);
	outputf ("%c  Permit      %04x" CR, cc, HeadOSK.accs);
	outputf ("%c  Type/Lang   %04x" CR, cc, HeadOSK.type);
	outputf ("%c  Attribute   %04x" CR, cc, HeadOSK.attr);
	outputf ("%c  Edition     %d"   CR, cc, HeadOSK.edition);
	outputf ("%c  Entry       %08x" CR, cc, HeadOSK.exec);
	outputf ("%c  Excpt       %08x" CR, cc, HeadOSK.excpt);

	if ((HeadOSK.type & 0x0F00) == 0x0e00 ||	/* Driver   */
	(HeadOSK.type & 0x0F00) == 0x0b00 ||	/* Trap     */
	(HeadOSK.type & 0x0F00) == 0x0100) {	/* Program  */
	outputf ("%c  Memory Size %d" CR, cc, HeadOSK.mem);
	}
	if ((HeadOSK.type & 0x0F00) == 0x0100 ||	/* Program  */
	(HeadOSK.type & 0x0F00) == 0x0b00) {	/* Trap     */
	outputf ("%c  Stack Size  %d"   CR, cc, HeadOSK.stack);
	outputf ("%c  M$IData     %08x" CR, cc, HeadOSK.idata);
	outputf ("%c  M$IRefs     %08x" CR, cc, HeadOSK.irefs);
	}
	if ((HeadOSK.type & 0x0F00) == 0x0b00) {	/* Trap     */
	outputf ("%c  M$Init      %08x" CR, cc, HeadOSK.init);
	outputf ("%c  M$Term      %08x" CR, cc, HeadOSK.term);
	}

	outputf ("%c********************************************" CR
		 "%c" CR, cc, cc);
	strcpy (buffer, Top - (ULONG)Head.base + HeadOSK.name);	/* ���W���[���� */
	switch (HeadOSK.type & 0x0f00) {
	case 0x0100:	/* Program */
	case 0x0b00:	/* Trap    */
		outputf ("\tpsect\t%s,$%04x,$%04x,%d,%d,L%06x",
			buffer, HeadOSK.type, HeadOSK.attr,
			HeadOSK.edition, HeadOSK.stack, HeadOSK.exec);
		if (HeadOSK.excpt)
		outputf (",L%06x", HeadOSK.excpt);
		break;
	case 0x0200:	/* Subroutine */
	case 0x0c00:	/* System     */
	case 0x0d00:	/* FileMan    */
	case 0x0e00:	/* Driver     */
		outputf ("\tpsect\t%s,$%04x,$%04x,%d,%d,L%06x",
			buffer, HeadOSK.type, HeadOSK.attr,
			HeadOSK.edition, 0, HeadOSK.exec);
		if (HeadOSK.excpt)
		outputf (",L%06x", HeadOSK.excpt);
		break;
	}
#else
	outputf ("%c=============================================" CR CR, cc);

	/* dis_header �̏o�� */
	{
	FILE* fp = open_header_file ();

	if (fp) {
		while (fgets (buffer, sizeof buffer, fp)) {
		char* p = strchr (buffer, '\n');
		if (p)
			*p = '\0';
		outputf ("%s" CR, buffer);
		}
		fclose (fp);
	}
	else {
		if (Doscall_mac_path)
		outputf ("%s%s" CR, op_include, Doscall_mac_path);
		if (Iocscall_mac_path)
		outputf ("%s%s" CR, op_include, Iocscall_mac_path);
		if (Fefunc_mac_path)
		outputf ("%s%s" CR, op_include, Fefunc_mac_path);
		if (Sxcall_mac_path)
		outputf ("%s%s" CR, op_include, Sxcall_mac_path);
	}
	outputf (CR);
	}
#endif	/* OSKDIS */

#ifndef	OSKDIS
	{	/* �O����`�V���{���̏o�� */
	char* colon = (SymbolColonNum == 0) ? ""
			: (SymbolColonNum == 1) ? ":" : "::";
	if (output_symbol_table (op_equ, op_xdef, colon))
		outputf (CR);
	}
#endif	/* !OSKDIS */

	/* .cpu �^�����߂̏o�� */
	outputf ("%s%s" CR, op_cpu, mputype_numstr (Current_Mputype));

}


private char*
mputype_numstr (mputypes m)
{
	if (m & M000) return "68000";
	if (m & M010) return "68010";
	if (m & M020) return "68020";
	if (m & M030) return "68030";
	if (m & M040) return "68040";
 /* if (m & M060) */
		  return "68060";
}


/*

  �I�y�R�[�h�̏o��

  input :
	ptr : buffer of opecode ( like "move.l",0 )

*/
private void
output_opecode (char* ptr)
{
	char buf[32];

	if (option_U)
	ptr = strupr (strcpy (buf, ptr));
	outputa (ptr);
}



/************************************************/
/*	    LIBC/XC �ȊO�Ŏg�p����֐�		*/
/************************************************/

#ifdef NEED_MAKE_CMDLINE
private char*
make_cmdline (char** argv)
{
	char* ptr = Malloc (1);

	for (*ptr = '\0'; *argv; argv++) {
	ptr = Realloc (ptr, strlen (ptr) + strlen (*argv) + 2);
	strcat (strcat (ptr, " "), *argv);
	}

	return ptr;
}
#endif


/************************************************/
/*		�ȉ��� OSKDIS �p�̊֐�		*/
/************************************************/

#ifdef	OSKDIS

private void
oskdis_gen (lblbuf* lptr)
{
	lblbuf* lptr = gen (CR, BeginBSS, next (BeginTEXT), XDEF_BSS, 0, -1);
	lblmode nmode = lptr->mode;
	address pc = lptr->label;

	output_opecode (CR "\tvsect" CR CR);
	while (pc <= BeginDATA) {
	lblmode  mode = nmode;
	address  nlabel;

	lptr   = Next (lptr);
	nlabel = lptr->label;
	nmode  = lptr->mode;

	if (pc == BeginDATA) break; /* �D�� */

	vlabelout (pc, mode);
	bssgen (pc, nlabel, mode & 0xff);
	pc = nlabel;
	}

	DEBUG_PRINT (("\nTEST=%08x, DATA=%08x, BSS=%08x, LAST=%08x\n",
			BeginTEXT, BeginDATA, BeginBSS, Last));

	lptr = next (BeginDATA);	/* vsect �̏������f�[�^�̈���|�C���g */
	switch (HeadOSK.type & 0x0f00) {
	case 0x0100:		/* Program  */
	case 0x0b00:		/* Trap     */
		{
		ULONG* idatsiz = Top - (ULONG) Head.base + (ULONG) HeadOSK.idata + 4;
		lptr = idatagen (BeginDATA + *idatsiz, lptr);
		}			/* vsect �̏������f�[�^���o�� */
		break;
	default:
		lptr = idatagen (Last, lptr);
		break;
	}
	pc = lptr->label;
	DEBUG_PRINT (("pc=0x%.8x, last=0x%.8x\n", pc, Last));

	output_opecode (CR "\tends" CR);	/* end vsect */

	if (pc < Last) {
	output_opecode (CR "\tvsect\tremote" CR CR);
	while (pc <= Last) {
		lblmode  mode = nmode;
		address  nlabel;

		lptr   = Next (lptr);
		nlabel = lptr->label;
		nmode  = lptr->mode;

		if (pc == Last) break;	/* �D�� */

		vlabelout (pc, mode);
		bssgen (pc, nlabel, mode & 0xff);
		pc = nlabel;
	}
	output_opecode (CR "\tends" CR);	/* end vsect remote */
	}

	output_opecode (CR "\tends" CR);	/* end psect */
	output_opecode (CR "\tend" CR);
}

/*

�@�������f�[�^���o�͂���(������)

*/
private address
idatagen_sub (address pc, address wkpc, address pcend, lblmode mode)
{

	while (pc < pcend) {
	ULONG byte = pcend - pc;

	switch (mode & 0xff) {
	case LONGSIZE:
		if (byte != 4)
		goto XX;
		if (mode & CODEPTR) {
		ULONG x = peekl (wkpc);
		char* s = (x == 0) ?		PSEUDO DC_L "_btext\t* $%08x" CR
			: (x == HeadOSK.name) ? PSEUDO DC_L "_bname\t* $%08x" CR
						  : PSEUDO DC_L "L%06x" CR;
		outputf (s, x);
		} else if (mode & DATAPTR)
		outputf (PSEUDO DC_L "L%06x" CR, peekl (wkpc) + BeginBSS);
		else
		outputf (PSEUDO DC_L "$%08x" CR, peekl (wkpc));
		break;

	case WORDSIZE:
		if (byte != 2)
		goto XX;
		outputf (PSEUDO DC_W "$%04x" CR, peekw (wkpc));
		break;

	default:
XX:	    byteout ((ULONG)wkpc - (ULONG)Top + (ULONG)Head.base, byte, FALSE);
	}
	pc += byte;
	wkpc += byte;
	}
	return wkpc;
}

/*

  �������f�[�^���o�͂���

  input:
	end     : block end address
	lptr    : lblbuf* ( contains block start address )

*/
private lblbuf*
idatagen (address end, lblbuf* lptr)
{
	address pc = lptr->label;
	address wkpc = Top - (ULONG) Head.base + (ULONG) HeadOSK.idata + 8;

	newline (pc);

	while (pc < end) {
	lblmode mode = lptr->mode;

	do {
		lptr = Next (lptr);
	} while (lptr->shift);

	label_line_out (pc, mode);

	DEBUG_PRINT (("PC=%08x, NEXT=%08x, MODE=%08x\n", pc, lptr->label, mode));

	wkpc = idatagen_sub (pc, wkpc, lptr->label, mode);
	pc = nlabel;
	}

	return lptr;
}


/*

  vsect ���x���o��

  input :
	adrs : label address
	mode : label mode

*/
private void
vlabelout (address adrs, lblmode mode)
{
	char  buff[128];
	int   opt_c_save = option_C;

	if (isHIDDEN (mode))
	return;

	if (opt_c_save == 0)
	option_C = 1;	/* �񏉊��� vsect �̓��x�������̋L�q�͏o���Ȃ� */
	label_line_out (adrs, mode);
	option_C = opt_c_save;
	outputa (buff);
}

/*

  ���[�h�e�[�u���̏o��

  input:
	pc : word table begin address
	pcend : word table end address

*/
private void
wgen (address pc, address pcend)
{
	char buf[128];
	address store;

	charout ('w');

	for (store = pc + Ofst; store + 2 <= pcend + Ofst; store += 2) {
	address label = (int)*(signed short*)store;

	if ((LONG)label < (LONG)BeginTEXT) {
		make_proper_symbol (buf, BeginTEXT);
		strcat (buf, "-");
		itox6d (buf + strlen (buf), (ULONG)(BeginTEXT - label));
	} else if ((LONG)label > (LONG)Last) {
		make_proper_symbol (work, Last);
		strcat (buf, "+");
		itox6d (buf + strlen (buf), (ULONG)(label - Last));
	} else
		make_proper_symbol (work, label);

	output_opecode (PSEUDO DC_W);
	outputa (buf);
	newline (store - Ofst);
	}

	if (store != pcend + Ofst)
	dataout (store - Ofst, pcend + Ofst - store, UNKNOWN);
}

#endif	/* OSKDIS */


/* EOF */
