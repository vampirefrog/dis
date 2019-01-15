/* $Id: analyze.c,v 1.1 1996/11/07 08:04:58 ryo freeze $
 *
 *	�\�[�X�R�[�h�W�F�l���[�^
 *	������̓��W���[��
 *	Copyright (C) 1989,1990 K.Abe, 1994 R.ShimiZu
 *	All rights reserved.
 *	Copyright (C) 1997-2010 Tachibana
 *
 */

#include <stdio.h>
#include <string.h>

#ifdef	__HUMAN68K__
#ifdef	__LIBC__
#include <sys/xstart.h>
#else
extern void __volatile _stack_over (void);
extern void *_SSTA;
#endif
#endif

#include "analyze.h"
#include "disasm.h"
#include "estruct.h"
#include "etc.h"	/* charout, peek[wl] */
#include "global.h"
#include "label.h"	/* regist_label etc. */
#include "offset.h"	/* depend_address , nearadrs */


USEOPTION	option_i;

boolean		Reason_verbose;
boolean		Arg_after_call = FALSE;


/* private �֐��v���g�^�C�v */
private address	limitadrs (address);
private boolean	branch_job (address, address, address, address,
				analyze_mode, address);
private void	regist_data (disasm*, operand*);
private boolean	prolabel (address);
private void	pcix (address, address);
private void	unregist_data (operand*);


/*
	right_opecode()������.
	�A�h���X�Ɉˑ��\�ȃI�y�����h���ˑ����Ă����
	neardepend�����̈ˑ��A�h���X�ɐi�߂�.
	�ˑ��s�\���A�ˑ����Ă��Ȃ���΂��̂܂�.
*/
static INLINE address
skip_depend_ea (address neardepend, disasm* code, operand* op)
{
    switch (op->ea) {
	case AbLong:
	    if (neardepend == op->eaadrs)
		neardepend = nearadrs (neardepend + 1);
	    break;
	case IMMED:
	    if (neardepend == op->eaadrs && code->size == LONGSIZE)
		neardepend = nearadrs (neardepend + 1);
	    break;
	case AregIDXB:
	    if (neardepend == op->eaadrs && op->exbd == 4)
		neardepend = nearadrs (neardepend + 1);
	    break;
	case AregPOSTIDX:
	case AregPREIDX:
	    if (neardepend == op->eaadrs && op->exbd == 4)
		neardepend = nearadrs (neardepend + 1);
	case PCPOSTIDX:
	case PCPREIDX:
	    if (neardepend == op->eaadrs2 && op->exod == 4)
		neardepend = nearadrs (neardepend + 1);
	    break;
	default:
	    break;
    }
    return neardepend;
}

/*

  right opecode ?

*/
static INLINE boolean
right_opecode (address neardepend, disasm* code, address pc)
{
    if (pc <= neardepend)
	return TRUE;

    neardepend = skip_depend_ea (neardepend, code, &code->op1);
    neardepend = skip_depend_ea (neardepend, code, &code->op2);
    neardepend = skip_depend_ea (neardepend, code, &code->op3);
    neardepend = skip_depend_ea (neardepend, code, &code->op4);

#ifdef DEBUG
    if (pc > neardepend && (Debug & BREASON))
	printf ("right opecode failed\n");
#endif
    return (pc <= neardepend);
}


/*

  ���� PROLABEL �Ƃ��ēo�^�ς݂��ǂ������ׂ�

*/
static INLINE boolean
datlabel (address adrs)
{
    lblbuf* ptr = search_label (adrs);

    if (ptr && isDATLABEL (ptr->mode))
	return TRUE;
    return FALSE;
}


/*

  ��͂���

  input:
    start : analyze start address
    mode  : ANALYZE_IGNOREFAULT
	    �����ł̖���`���ߓ��𖳎�����
	    ANALYZE_NORMAL
	    �����Ŗ���`���ߓ������t������A�Ăяo�����̓f�[�^�̈�
  return:
    FALSE : pc ����̓v���O�����̈�ƔF�߂��Ȃ�����

*/
#define TRUEret		goto trueret
#define FALSEret	goto falseret
#define ROUTINE_END(pc)	ch_lblmod (pc, DATLABEL | UNKNOWN)

extern boolean
analyze (address start, analyze_mode mode)
{
    address	pc = start, store = start + Ofst;
    address	limit;
    address	neardepend;
    disasm	code;
    int		orib;			/* ori.b #??,d0 = $0000???? */

#ifdef DEBUG
    if (Debug & (BDEBUG | BREASON))
	printf ("an %x(%d)\n", start, mode);
#endif
    charout ('>');

    if (Available_text_end <= pc) {
	if (Reason_verbose)
	    eprintf ("\n%06x : The PC is out of a valid section\t", pc);
	FALSEret;
    } else if (depend_address (pc) || depend_address (pc - 2)) {
	if (Reason_verbose)
	    eprintf ("\n%06x : PC is dependent on address\t", pc);
	FALSEret;
    } else if ((ULONG)pc & 1) {
	if (Reason_verbose)
	    eprintf ("\n%06x : If PC is odd\t", pc);
	FALSEret;
    } else if (((ULONG)pc & 1) != 0 || *(LONG*)store == 0) {
	if (Reason_verbose)
	    eprintf ("\n%06x : The PC starts with ori.b #0,d0\t", pc);
	FALSEret;
    }

    /* the follow 4 lines are omitable, maybe */
#if 0
    if (prolabel (start)) {
	regist_label (start, PROLABEL);
	TRUEret;
    }
#endif

    if (!regist_label (start, PROLABEL)) {	/* ����� ch_lblmod �ł͑ʖ� */
#ifdef DEBUG
	if (Debug & BDEBUG)
	    printf ("%x cannot be prolabel\n", pc);
#endif
	TRUEret;
    }

    /* �X�^�b�N�`�F�b�N */
#ifdef __HUMAN68K__
    {
	register void* sp __asm ("sp");

	if (sp < _SSTA) {
	    eputc ('\n');
	    _stack_over ();
	}
    }
#endif

    PCEND = Available_text_end;
    orib = 0;
    limit = limitadrs (pc);
    neardepend = nearadrs (pc);

    while (1) {
	address previous_opval, previous_pc;

	if (neardepend == pc) {				/* �A�h���X�ˑ��`�F�b�N */
	    not_program (start, (address) min((ULONG) pc, (ULONG) limit));
	    if (Reason_verbose)
		eprintf ("\n%06x : PC is dependent on address\t", pc);
	    FALSEret;
	}

	if (*(UWORD*)store == 0) {			/* ori.b #??,d0 �`�F�b�N */
	    if (orib >= 2 || *(UWORD*)(store + 2) == 0) {
		not_program (start, (address) min ((ULONG) pc, (ULONG) limit));
		if (Reason_verbose)
		    eprintf ((orib >= 2) ? "\n%06x : ori.b #??,d0 are consecutive\t"
					 : "\n%06x : I found ori.b #0,d0\t", pc);
		FALSEret;
	    }
	    orib++;
	} else
	    orib = 0;

	if (limit == pc && prolabel (limit)) {	/* ���ɉ�͍ς݂̗̈�ƂԂ������H */
#ifdef DEBUG
	    if (Debug & BDEBUG)
		printf ("Already\n");
#endif
	    ROUTINE_END (pc);
	    TRUEret;
	}

	if ((pc > limit) ||	/* �ŏ��� code.op.opval �͏���������Ȃ����Ǒ��v */
	    (pc <= code.op1.opval && code.op1.opval < limit) ||
	    (pc <= code.op2.opval && code.op2.opval < limit) ||
	    (pc <= code.op3.opval && code.op3.opval < limit) ||
	    (pc <= code.op4.opval && code.op4.opval < limit))
	    limit = limitadrs (pc);

	previous_opval = code.op1.ea == PCIDX ? code.op1.opval : (address) -1;
	previous_pc = pc;
	store += dis (store, &code, &pc);

#ifdef DEBUG
	if (Debug & BTRACE) {
	    static const char size[10][2] = {
		".b", ".w", ".l", ".q", ".s", ".s", ".d", ".x", ".p", ""
	    };

	    printf ("%05x:%05x\t%s.2%s",
			previous_pc, limit, code.opecode, size[code.size]);
	    if (code.op1.operand[0])
		printf ("\t%s", code.op1.operand);
	    if (code.op2.operand[0])
		printf (",%s", code.op2.operand);
	    if (code.op3.operand[0])
		printf (",%s", code.op3.operand);
	    if (code.op4.operand[0])
		printf (",%s", code.op4.operand);
	    printf ("\n");
	}
#endif
	/* add '90 Sep 24 */
	if (Available_text_end < pc) {
	    if (Reason_verbose)
		eprintf ("\n%06x : PC ���L���ȃZ�N�V�������O�ꂽ\t", pc);
	    not_program (start, (address) min ((ULONG) previous_pc, (ULONG) limit));
	    FALSEret;
	}

	if (neardepend < pc) {
#ifdef DEBUG
	    if (Debug & BDEBUG) {
		printf ("over neardepend:%x\n", pc);
		printf ("size %d ea1 %d ea2 %d ea3 %d ea4 %d"
			"op1val %x op2val %x op3val %x op4val %x"
			"ea1adrs %x ea2adrs %x ea3adrs %x ea4adrs %s"
			"\n",
			code.size, code.op1.ea, code.op2.ea, code.op3.ea, code.op4.ea,
			code.op1.opval, code.op2.opval, code.op3.opval, code.op4.opval,
			code.op1.eaadrs, code.op2.eaadrs, code.op3.eaadrs, code.op4.eaadrs
			);
	    }
#endif
	    if (right_opecode (neardepend, &code, pc))
		neardepend = nearadrs (pc);
	    else {
		if (Reason_verbose)
		    eprintf ("\n%06x : �A�h���X�ˑ��̃I�y�����h�������łȂ�\t", previous_pc);
		not_program (start, (address) min ((ULONG) previous_pc, (ULONG) limit));
		FALSEret;
	    }
	}

	if (code.flag == OTHER
	 || ((code.flag == JMPOP || code.flag == JSROP)
		&& AregPOSTIDX <= code.jmpea && code.jmpea != PCIDXB)) {
	    regist_data (&code, &code.op1);
	    regist_data (&code, &code.op2);
	    regist_data (&code, &code.op3);
	    regist_data (&code, &code.op4);
	}

	switch (code.flag) {
	case UNDEF:
	    if (Reason_verbose)
		eprintf ("\n%06x : ����`����(%04x %04x)\t", previous_pc,
			peekw (previous_pc + Ofst), peekw (previous_pc + Ofst + 2));
	    not_program (start, (address) min((ULONG) previous_pc, (ULONG) limit));
	    FALSEret;
	case OTHER:
	    break;
	case JMPOP:
	    if (!(code.jmp == (address) -1 ||
		  (code.op1.ea == AbLong && !INPROG (code.jmp, code.op1.eaadrs)) ||
		   code.op1.ea == AbShort ||
		   AregIDXB <= code.op1.ea)) {
		if (code.op1.ea == PCIDX)
		    pcix (code.jmp, previous_opval);
		else
		    if (!branch_job (code.jmp, start, pc, previous_pc, mode, limit))
			FALSEret;
	    }
	case RTSOP:
	    ROUTINE_END (pc);
	    TRUEret;
	case JSROP:
	    if (!(code.jmp == (address) -1 ||
		  (code.op1.ea == AbLong && !INPROG (code.jmp, code.op1.eaadrs)) ||
		   code.op1.ea == AbShort ||
		   (AregIDXB < code.op1.ea && code.op1.ea != PCIDXB))) {
		if (code.op1.ea == PCIDX)
		    pcix (code.jmp, previous_opval);
		else
		    if (!branch_job (code.jmp, start, pc, previous_pc, mode, limit))
			FALSEret;
	    }

	    /* -G : �T�u���[�`���R�[���̒���Ɉ�����u�����Ƃ�F�߂� */
	    if (Arg_after_call && datlabel (pc))
		TRUEret;

	    break;
	case BCCOP:
	    if (!branch_job (code.jmp, start, pc, previous_pc, mode, limit))
		FALSEret;
	    break;
	}
    }

trueret:
#ifdef DEBUG
    if (Debug & BDEBUG)
	printf ("ret\n");
#endif
    charout ('<');
    return TRUE;

falseret:
    charout ('?');
    ch_lblmod (start, DATLABEL | UNKNOWN | FORCE);
    return FALSE;
}


/*

  adrs ����̃f�[�^���x���̃A�h���X��Ԃ�
  ���̂悤�ȃ��x����������� Available_text_end ��Ԃ�

*/
private address
limitadrs (address adrs)
{
    lblbuf* lptr = next (adrs + 1);

#if 0
    if (lptr->label == adrs && isDATLABEL (lptr->mode)) {
	ch_lblmod (adrs, PROLABEL);
	return limitadrs (adrs + 1);		/* ���� +1 �͉��̂���̂�? */
    }
#endif

#ifdef DEBUG
    if (Debug & BDEBUG)
	printf ("lmt(%x)=%x\n", adrs, lptr->label);
#endif

    return (address) min ((ULONG)lptr->label, (ULONG)Available_text_end);
}


/*

  ���򖽗ߓ��̏���

*/
static void
bra_to_odd (address pre_pc, address opval)
{
    if (Reason_verbose)
	eprintf ("\n%06x : ��A�h���X(%06x)�֕���\t", pre_pc, opval);
}

private boolean
branch_job (address opval, address start, address pc, address pre_pc,
		analyze_mode mode, address limit)
{
    if (opval == (address)-1)
	return TRUE;

    /* ��A�h���X�ւ̕��򂪂���΃v���O�����̈�ł͂Ȃ� */
    if ((LONG)opval & 1) {
	if (Disasm_AddressErrorUndefined == FALSE) {
	    /* -j: ��A�h���X�ւ̕���𖢒�`���߂Ɓu���Ȃ��v */

	    regist_label (opval, DATLABEL | UNKNOWN);
	    bra_to_odd (pre_pc, opval);		/* �x���͏�ɏo�� */
	    return TRUE;
	}
	ch_lblmod (start, DATLABEL | UNKNOWN | FORCE);
	bra_to_odd (pre_pc, opval);
	not_program (start, (address)min ((ULONG)pre_pc, (ULONG)limit));
	return FALSE;
    }

    /* ����悪���Ƀv���O�����̈�Ɣ������Ă��� */
    if (prolabel (opval)) {
	regist_label (opval, PROLABEL);		/* to increment label count */
	return TRUE;
    }

    /* �����̗̈����͂��� */
    if (!analyze (opval, mode) && mode != ANALYZE_IGNOREFAULT && !option_i) {
	/* ����悪�v���O�����̈�łȂ���΁A�Ăяo���������� */
#ifdef DEBUG
	if(Debug & BREASON)
	    printf ("falseret : %x falseret\n", pc);
#endif
	ch_lblmod (start, DATLABEL | UNKNOWN | FORCE);
	not_program (start, (address)min ((ULONG)pc, (ULONG)limit));
	return FALSE;
    }

    /* �����̓v���O�����̈悾���� */
    return TRUE;
}


/*

  �����e�B�u�I�t�Z�b�g�e�[�u�������j��
  move.w	Label(pc,d0),d0
  jsr		Label(pc,d0)

  Label dc.w	Label1-Label, Label2-Label ,...

  BUG: �����e�B�u�I�t�Z�b�g�e�[�u���Ɣ��f�����ꍇ�A
  �����Ƀe�[�u�����̃G���g����o�^���Ă��܂��̂ŁA���Ƃ���
  not_program �Ƃ����Ă��c���Ă��܂��B (�ő��ɂ��邱�Ƃł͂Ȃ���)

*/
private void
pcix (address table, address popval)
{
#ifdef DEBUG
    if (Debug & BDEBUG)
	printf ("pcix : %x %x\n", table, popval);
#endif

    if (table == popval)
	relative_table (table);
    else
	regist_label (table, DATLABEL | UNKNOWN);
}


/*

  table ���烊���e�B�u�I�t�Z�b�g�e�[�u���Ƃ��ēo�^

*/
extern void
relative_table (address table)
{
    address ptr = table;
    address tableend = next (table + 1)->label;

    regist_label (table, DATLABEL | RELTABLE);

    while (ptr < tableend) {
	address label = table + (WORD) peekw (ptr + Ofst);

	if (label < (address) BeginTEXT || (address) Last < label
	 || (table <= label && label < ptr + 2)) {
	    regist_label (ptr, DATLABEL | UNKNOWN | FORCE);	/* not rel table */
	    break;
	}
#ifdef DEBUG
	if (Debug & BDEBUG)
	    printf ("lbl %x\n", label);
#endif
	regist_label (label, DATLABEL | UNKNOWN);
	ptr += 2;
	tableend = next (ptr)->label;
    }
}


/*

  table ���烍���O���[�h�ȃ����e�B�u�I�t�Z�b�g�e�[�u���Ƃ��ēo�^

*/
extern void
relative_longtable (address table)
{
    address ptr = table;
    address tableend = next (table + 1)->label;

    regist_label (table, DATLABEL | RELLONGTABLE);

    while (ptr < tableend) {
	address label = table + (LONG) peekl (ptr + Ofst);

	if (label < (address) BeginTEXT || (address) Last < label
	 || (table <= label && label < ptr + 4)) {
	    regist_label (ptr, DATLABEL | UNKNOWN | FORCE);	/* not rel table */
	    break;
	}
	regist_label (label, DATLABEL | UNKNOWN);
	ptr += 4;
	tableend = next (ptr)->label;
    }
}


#ifdef	OSKDIS
/*

  table ���烏�[�h�e�[�u���Ƃ��ēo�^

*/
extern void
w_table (address table)
{
    address ptr = table;
    address tableend = next (table + 1)->label;

    regist_label (table, DATLABEL | WTABLE);

    while (ptr < tableend) {
	address label = (WORD) peekw (ptr + Ofst);

#ifdef DEBUG
	if (Debug & BDEBUG)
	    printf ("wlbl %x\n", label);
#endif
	regist_label (label, DATLABEL | UNKNOWN);
	ptr += 2;
	tableend = next (ptr)->label;
    }
}

#endif	/* OSKDIS */

/*

  table ���� �����O���[�h�e�[�u���Ƃ��ēo�^

*/
extern void
z_table (address table)
{
    address ptr = table;
    address tableend = next (table + 1)->label;

    regist_label (table, DATLABEL | ZTABLE);

    while (ptr + 4 <= tableend) {
	address label = (address) peekl (ptr + Ofst);

	regist_label (label, DATLABEL | UNKNOWN);
#ifdef DEBUG
	if (Debug & BDEBUG)
	    printf ("zlbl %x\n", label);
#endif
	ptr += 4;
	tableend = next (ptr)->label;
    }
}


/*

  from ���� to �܂ł̗̈���v���O�����Ŗ����������ɂ���

*/
extern void
not_program (address from, address to)
{
    address	store;
    address	pcend_save = PCEND;
    address	pc = from;
    disasm	code;

#ifdef DEBUG
    if (Debug & BDEBUG)
	printf ("\nnot_prog %x - %x\n", from, to);
#endif
    ch_lblmod (from, DATLABEL | UNKNOWN | FORCE);
    store = pc + Ofst;
    PCEND = to;
    while (pc < to) {
	store += dis (store, &code, &pc);
	unregist_data (&code.op1);
	unregist_data (&code.op2);
	unregist_data (&code.op3);
	unregist_data (&code.op4);
    }
    PCEND = pcend_save;

#ifdef DEBUG
    if (Debug & BDEBUG)
	printf ("end of not_prog\n");
#endif
}


/*

  ���� PROLABEL �Ƃ��ēo�^�ς݂��ǂ������ׂ�

*/
private boolean
prolabel (address adrs)
{
    lblbuf* ptr = search_label (adrs);

    if (ptr && isPROLABEL (ptr->mode))
	return TRUE;
    return FALSE;
}


/*

  �f�[�^�Ƃ��ēo�^����

*/
private void
regist_data (disasm* code, operand* op)
{

    if (op->opval != (address)-1) {
	if (op->ea == IMMED && code->size2 == LONGSIZE && INPROG (op->opval, op->eaadrs))
	    regist_label (op->opval, DATLABEL | UNKNOWN);

	else if (op->ea == PCDISP || op->ea == PCIDX || op->ea == PCIDXB
	 || (op->ea == AbLong && INPROG (op->opval, op->eaadrs))
	 || (op->ea == AregIDXB && INPROG (op->opval, op->eaadrs) && op->exbd == 4))
	    regist_label (op->opval, DATLABEL | code->size2);

	else if (op->ea == PCPOSTIDX || op->ea == PCPREIDX) {
	    if (op->exod == 4 && INPROG (op->opval2, op->eaadrs2))
		regist_label (op->opval2, DATLABEL | code->size2);
	    regist_label (op->opval, DATLABEL | (op->exbd ? LONGSIZE : code->size2));
	}

	else if (op->ea == AregPOSTIDX || op->ea == AregPREIDX) {
	    if (op->exbd == 4 && INPROG (op->opval, op->eaadrs))
		regist_label (op->opval, DATLABEL | LONGSIZE);
	    if (op->exod == 4 && INPROG (op->opval2, op->eaadrs2))
		regist_label (op->opval2, DATLABEL | code->size2);
	}
    }
}


/*

  �f�[�^���x���Ƃ��Ă̓o�^��������

*/
private void
unregist_data (operand* op)
{

    if (op->opval != (address)-1 &&
	(op->labelchange1
	 || ((op->ea == AbLong || op->ea == IMMED) && INPROG (op->opval, op->eaadrs))
	)
    )
	unregist_label (op->opval);

    if (op->labelchange2 && INPROG (op->opval2, op->eaadrs2))
	unregist_label (op->opval2);
}


/*

  �o�^�ς݃��x���̃��[�h��ς���

*/
extern boolean
ch_lblmod (address pc, lblmode mode)
{
    lblbuf* ptr = search_label (pc);

    if (regist_label (pc, mode)) {
	if (ptr)
	    unregist_label (pc);
	return TRUE;
    }

    return FALSE;
}


/* EOF */
