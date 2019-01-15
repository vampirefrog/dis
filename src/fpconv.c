/*
 *
 *	�\�[�X�R�[�h�W�F�l���[�^
 *	���������_�����l������ϊ����W���[��
 *	Copyright (C) 1997-2010 Tachibana
 *
 */

/* System headers */
#include <stdio.h>	/* sprintf() */
#include <string.h>	/* strcpy() */

/* Headers */
#include "estruct.h"	/* enum boolean */
#include "fpconv.h"
#include "hex.h"	/* itox8_without_0supress() */

/* External functions */
#ifdef	HAVE_STRTOX
extern	long double strtox (const char* ptr);
#endif


/* External variables */
short	Inreal_flag = FALSE;


/* Functions */
extern void
fpconv_s (char* buf, float* valp)
{
	unsigned short e = *(unsigned short*)valp & 0x7f80;
	unsigned long m = *(unsigned long*)valp & 0x007fffff;

	/* �}0.0���ɏ��� */
	if (!Inreal_flag && e == 0 && m == 0) {
	strcpy (buf, (*(char*)valp < 0) ? "0f-0" : "0f0");
	return;
	}

	if (Inreal_flag || e == 0 || e == 0x7f80) {		/* �񐳋K����, ������A�� */
	*buf++ = '!';					/* ����Ȓl�Ȃ�����\���ŏo�� */
	itox8_without_0supress (buf, *(long*)valp);
	} else {
	sprintf (buf, "0f%.30g", *valp);
	}
}


/* Functions */
extern void
fpconv_d (char* buf, double* valp)
{
	unsigned short e = *(unsigned short*)valp & 0x7ff0;
	unsigned long mh = *(unsigned long*)valp & 0x000fffff;
	unsigned long ml = *(unsigned long*)valp;

	/* �}0.0���ɏ��� */
	if (!Inreal_flag && e == 0 && (mh | ml) == 0) {
	strcpy (buf, (*(char*)valp < 0) ? "0f-0" : "0f0");
	return;
	}

	if (Inreal_flag || e == 0 || e == 0x7ff0) {		/* �񐳋K����, ������A�� */
	long* p = (long*)valp;				/* ����Ȓl�Ȃ�����\���ŏo�� */

	*buf++ = '!';
	buf = itox8_without_0supress (buf, *p++);
	*buf++ = '_';
	(void)itox8_without_0supress (buf, *p);
	} else {
	sprintf (buf, "0f%.30g", *valp);
	}
}


/* File scope functions */
static void
fpconv_x_inreal (char* buf, long double* valp)
{
	unsigned long* p = (unsigned long*)valp;

	*buf++ = '!';
	buf = itox8_without_0supress (buf, *p++);
	*buf++ = '_';
	buf = itox8_without_0supress (buf, *p++);
	*buf++ = '_';
	(void)itox8_without_0supress (buf, *p++);
}

/* Functions */
extern void
fpconv_x (char* buf, long double* valp)
#ifdef NO_PRINTF_LDBL
{
	/* ��ɓ����\���ŏo�� */
	fpconv_x_inreal (buf, valp);
}
#else /* !NO_PRINTF_LDBL */
{
	unsigned short e = *(unsigned short*)valp & 0x7fff;
	unsigned long mh = ((unsigned long*)valp)[0];
	unsigned long ml = ((unsigned long*)valp)[1];

	/* �}0.0���ɏ��� */
	if (!Inreal_flag && e == 0 && (mh | ml) == 0) {
	strcpy (buf, (*(char*)valp < 0) ? "0f-0" : "0f0");
	return;
	}

	if (Inreal_flag || e == 0 || e == 0x7fff		/* �񐳋K����, ������A�� */
	 || ((unsigned short*)valp)[1]			/* ���g�p�r�b�g���Z�b�g */
	 || ((char*)valp)[4] >= 0				/* �����r�b�g��0 */
#ifndef	HAVE_STRTOX
	 || e <= 64						/* ���C�u�����Ɉˑ� */
#endif
	) {
	fpconv_x_inreal (buf, valp);			/* ����Ȓl�Ȃ�����\���ŏo�� */
	} else {
	sprintf (buf, "0f%.30Lg", *valp);

#ifdef	HAVE_STRTOX
	/* �w�������Ȃ瑽���������ϊ��ł��Ă��锤 */
	if (e >= 0x3fff)
		return;

	/* �������ϊ��ł��Ȃ�������A�����\���ŏo�͂��Ȃ��� */
	if (strtox (buf + 2) != *valp)
		fpconv_x_inreal (buf, valp);
#endif
	}
}
#endif	/* !NO_PRINTF_LDBL */


/* File scope functions */
static boolean
is_normalized_p (packed_decimal* valp)
{

	if ((valp->ul.hi<<1 | valp->ul.mi | valp->ul.lo) == 0)
	return TRUE;					/* �}0.0 */

	if ( ((valp->ul.hi & 0x7fff0000) == 0x7fff0000)	/* ��,������ */
	  || ((valp->ul.hi & 0x3000fff0) != 0)		/* ���g�p�r�b�g���Z�b�g */
#if 0
	  || ((valp->ul.hi & 0x4fff0000) == 0x4fff0000)	/* �񐳋K���� */
#endif
	  || ((valp->ul.hi & 0x0000000f) > 9) )		/* BCD(������)�̒l���ُ� */
	return FALSE;

	{
	int i;
	unsigned char *ptr = &valp->uc[4];
	for (i = 0; i < 8; i++) {
		unsigned char c = *ptr++;
		if ((c > (unsigned char)0x99) || ((c & 0x0f) > (unsigned char)0x09))
		return FALSE;				/* BCD(������)�̒l���ُ� */
	}
	}

   return TRUE;
}


/* Functions */
extern void
fpconv_p (char* buf, packed_decimal* valp)
{

	if (Inreal_flag || !is_normalized_p (valp)) {
	long* p = (long*)valp;				/* ����Ȓl�Ȃ�����\���ŏo�� */

	*buf++ = '!';
	buf = itox8_without_0supress (buf, *p++);
	*buf++ = '_';
	buf = itox8_without_0supress (buf, *p++);
	*buf++ = '_';
	(void)itox8_without_0supress (buf, *p);

	} else {
	*buf++ = '0';
	*buf++ = 'f';
	if (valp->uc[0] & 0x80)
		*buf++ = '-';

	/* �����̐����� */
	*buf++ = '0' + (valp->ul.hi & 0x0000000f);
	*buf++ = '.';

	/* �����̏����� */
	{
		int i;
		unsigned char *ptr = &valp->uc[4];

		for (i = 0; i < 8; i++) {
		unsigned char c = *ptr++;
		*buf++ = '0' + (c >> 4);
		*buf++ = '0' + (c & 0x0f);
		}
	}

	/* ������'0'���폜 */
	while (*--buf == (char)'0')
		;
	if (*buf != (char)'.')
		buf++;

	/* �w�� */
	{
		int expo = (valp->uc[0] & 0x0f)*100
			 + (valp->uc[1] >> 4)*10
			 + (valp->uc[1] & 0x0f);

		if ((expo == 0) && !(valp->uc[0] & 0x40)) {
		/* "e+0"�͏ȗ����� */
		*buf = '\0';
		} else {
		*buf++ = 'e';
		*buf++ = (valp->uc[0] & 0x40) ? '-' : '+';
		buf += (expo >= 100) + (expo >= 10) + 1;
		*buf = '\0';
		do {
			*--buf = '0' + (expo % 10);
			expo /= 10;
		} while (expo);
		}
	}
	}
}


/* Functions */
extern void
fpconv_q (char* buf, quadword* valp)
{
	*buf++ = '!';
	buf = itox8_without_0supress (buf, valp->ul.hi);
	*buf++ = '_';
	(void)itox8_without_0supress (buf, valp->ul.lo);
}


/* EOF */
