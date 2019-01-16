/* $Id: offset.c,v 1.1 1996/11/07 08:03:52 ryo freeze $
 *
 *	�\�[�X�R�[�h�W�F�l���[�^
 *	�Ĕz�u�e�[�u���Ǘ����W���[��
 *	Copyright (C) 1989,1990 K.Abe
 *	All rights reserved.
 *	Copyright (C) 1997-2010 Tachibana
 *
 */

#include <stdio.h>

#include "estruct.h"
#include "etc.h"
#include "global.h"
#include "offset.h"


/* #define DEBUG */


static address* Reltable = NULL;	/* �Ĕz�u�e�[�u���̓� */
static int Relnum;			/* �Ĕz�u�e�[�u���̗v�f�� */


#ifdef DEBUG
private void
disp_reltable (void)
{
	int i;

	for (i = 0; i < Relnum; i++)
	printf ("%5x\n", Reltable[i]);
}
#endif

/*

  �Ĕz�u�e�[�u������Ĕz�u���ׂ��A�h���X�̃e�[�u�������

*/
extern void
make_relocate_table (void)
{
	address destadrs = BeginTEXT;
	UBYTE* ptr;		/* �I�t�Z�b�g�\�ւ̃|�C���^ ( not always even !) */

	if ((Reltable = (address *) Malloc (Head.offset * sizeof(address))) == NULL)
	err ("We could not secure the expansion buffer for relocate information\n");

	Relnum = 0;
	ptr = (UBYTE*) (Top + Head.text + Head.data);

	while (ptr < (UBYTE*) (Top + Head.text + Head.data + Head.offset)) {
	if ((*ptr << 8) + *(ptr + 1) == 0x0001) {
		/* 0x0001 0x???? 0x???? : �����O���[�h���� */
		ptr += 2;
		destadrs += (ULONG) ( (*(ptr + 0) << 24)
				+ (*(ptr + 1) << 16)
				+ (*(ptr + 2) << 8)
				+  *(ptr + 3));
		ptr += 4;
	}
	else {
		/* 0x???? ( != 0x0001 ) : ���[�h���� */
		destadrs += (ULONG) (((*ptr << 8) + *(ptr + 1)) & 0xfffe);
		ptr += 2;
	}
	Reltable[Relnum++] = destadrs;
	}

#ifdef	DEBUG
	disp_reltable ();
#endif
}


/*

  ��n��

*/
extern void
free_relocate_buffer (void)
{
	/* if (Reltable) */
	Mfree (Reltable);
}


/*

  adrs ���v���O�����̃��[�h���ꂽ�A�h���X�Ɉˑ����Ă���Ȃ� TRUE
  label1    pea.l   label2
  ���̂Ƃ� depend_address( label1 + 2 ) == TRUE	 ( label2���ˑ����Ă��� )

*/
extern boolean
depend_address (address adrs)
{
	address* ptr = Reltable;
	int step = Relnum >> 1;

	while (step > 4) {
	if (*(ptr + step) <= adrs)	/* binary search */
		ptr += step;
	step >>= 1;
	}
	for (; ptr < Reltable + Relnum; ptr++) {
	if (*ptr == adrs) {
#ifdef	DEBUG
		printf ("%6x is address-dependent\n", adrs);
#endif
		return TRUE;
	} else
		if (adrs < *ptr)
		return FALSE;
	}
	return FALSE;
}


/*

  adrs ���������ŁA�ł��߂��A�h���X�ˑ��̃A�h���X��Ԃ�
  ���̂悤�ȗ̈悪������� 0xffffffff = -1 ��Ԃ�

*/
extern address
nearadrs (address adrs)
{
	address rc = (address)-1;
	int step = Relnum >> 1;
	address* ptr = Reltable;

	while (step > 4) {
	if (*(ptr + step) < adrs)	/* binary search */
		ptr += step;
	step >>= 1;
	}

	for (; ptr < Reltable + Relnum; ptr++) {
	if (adrs <= *ptr) {
		rc = *ptr;
		break;
	}
	}

#ifdef	DEBUG
	printf ("nearadrs (%x) = %x\n", adrs, rc);
#endif
	return rc;
}


/* EOF */
