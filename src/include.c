/* $Id: include.c,v 1.1 1996/11/07 08:03:42 ryo freeze $
 *
 *	�\�[�X�R�[�h�W�F�l���[�^
 *	�C���N���[�h�t�@�C���ǂݍ��݃��W���[��
 *	Copyright (C) 1989,1990 K.Abe, 1994 R.ShimiZu
 *	All rights reserved.
 *	Copyright (C) 1997-2010 Tachibana
 *
 */

#include <stdio.h>
#include <stdlib.h>	/* getenv() */
#include <string.h>

#include "disasm.h"	/* OSlabel , FElabel , SXlabel */
#include "estruct.h"
#include "etc.h"
#include "generate.h"	/* IOCSlabel */
#include "global.h"
#include "include.h"


USEOPTION	option_Y;


/* External variables */
#ifdef	OSKDIS
const char*  OS9_mac_path;
const char* MATH_mac_path;
const char*  CIO_mac_path;
#else
const char*  Doscall_mac_path;
const char* Iocscall_mac_path;
const char*   Fefunc_mac_path;
const char*   Sxcall_mac_path;

const char*  doscall_mac = "doscall.mac";
const char* iocscall_mac = "iocscall.mac";
const char*   fefunc_mac = "fefunc.mac";
#endif	/* OSKDIS */


/* static �֐��v���g�^�C�v */
#ifndef	OSKDIS
private void	search_and_readfile (const char** bufptr, const char* filename,
			char*** label, int hbyte, char* callname, int max);
#endif	/* !OSKDIS */
private int	readfile (const char* filename, char*** label,
			int hbyte, char* callname, int max);


#ifdef	OSKDIS
/*

  os9defs.d mathdefs.d ciodefs.d ��
  ���ϐ� DISDEFS �ɂ��������ēǂݍ���.

*/

extern void
load_OS_sym (void)
{
    static const char os9defs_d[] = "os9defs.d";
    static const char mathdefs_d[] = "mathdefs.d";
    static const char ciodefs_d[] = "ciodefs.d";
    char* dis_inc;
    int plen;

    eprintf ("%s %s %s ��ǂݍ��݂܂�.\n", os9defs_d, mathdefs_d, ciodefs_d);

    if ((dis_inc = getenv ("DISDEFS")) == NULL) {
    /*  err ("���ϐ� DISDEFS ���ݒ肳��Ă��܂���.\n");  */
	eprintf ("���ϐ� DISDEFS ���ݒ肳��Ă��܂���.\n");
	return;
    }
    plen = strlen (dis_inc);

    OS9_mac_path = Malloc (plen + sizeof os9defs_d + 2);
    strcat (strcat (strcpy (OS9_mac_path, dis_inc), "/"), os9defs_d);
    if (!readfile (OS9_mac_path, OS9label, 0, "OS9")) {
	eprintf ("%s ���I�[�v���o���܂���.\n", os9defs_d);
	OS9_mac_path = NULL;
    }

    MATH_mac_path = Malloc (plen + sizeof mathdefs_d + 2);
    strcat (strcat (strcpy (MATH_mac_path, dis_inc), "/"), mathdefs_d);
    if (!readfile (MATH_mac_path, MATHlabel, 0, "T$Math")) {
	eprintf ("%s ���I�[�v���o���܂���.\n", mathdefs_d);
	MATH_mac_path = NULL;
    }

    CIO_mac_path = Malloc (plen + sizeof ciodefs_d + 2);
    strcat (strcat (strcpy (CIO_mac_path, dis_inc), "/"), ciodefs_d);
    if (!readfile (CIO_mac_path, CIOlabel, 0, "CIO$Trap")) {
	eprintf ("%s ���I�[�v���o���܂���.\n", ciodefs_d);
	CIO_mac_path = NULL;
    }
}

#else	/* !OSKDIS */
/*

  doscall.mac iocscall.mac fefunc.mac ���J�����g�f�B���N�g���Ⴕ����
  ���ϐ� (dis_)include �ɂ��������ēǂݍ���.

*/

extern void
load_OS_sym (void)
{

    search_and_readfile (&Doscall_mac_path, doscall_mac, &OSlabel,  0xff,  OSCallName, 256);
    search_and_readfile (&Iocscall_mac_path,iocscall_mac,&IOCSlabel,0x00,IOCSCallName, 256);
    search_and_readfile (&Fefunc_mac_path,  fefunc_mac,  &FElabel,  0xfe,  FECallName, 256);

    if (Disasm_SX_Window
	&& (Sxcall_mac_path = getenv ("dis_sxmac")) != NULL
	&& *Sxcall_mac_path) {
	if (!readfile (Sxcall_mac_path, &SXlabel, 0x0a, SXCallName, SXCALL_MAX))
	    err ("%s ���I�[�v���o���܂���.\n" , Sxcall_mac_path);
    }
}
#endif	/* OSKDIS */


#ifndef	OSKDIS
/*

  1) ./ (-Y �I�v�V�����w�莞�̂�)
  2) $dis_include/
  3) $include/
  �̏��� readfile() ������.

*/
private void
search_and_readfile (const char** bufptr, const char* filename,
			char*** label, int hbyte, char* callname, int max)
{
    char* dis_inc = NULL;
    char* include = NULL;
    int path_flag;

    /* --exclude-***call �w�莞�͓ǂݍ��܂Ȃ� */
    if (filename == NULL)
	return;

    path_flag = strchr (filename, ':') || strchr (filename, '/');

    /* -Y �I�v�V�����w�莞�̓J�����g�f�B���N�g�����猟������. */
    /* �t�@�C�����Ƀp�X�f���~�^���܂܂��ꍇ�����̂܂܌���. */
    if (path_flag || option_Y) {
	if (readfile (filename, label, hbyte, callname, max)) {
	    *bufptr = filename;
	    return;
	}
    }

    /* �p�X�f���~�^���܂܂�Ȃ���Ί��ϐ����Q�Ƃ���. */
    if (!path_flag) { 
	int flen = strlen (filename) + 2;
	char* buf;

	/* ���ϐ� dis_include �̃p�X���猟������. */
	if ((dis_inc = getenv ("dis_include")) != NULL) {
	    buf = (char*) Malloc (strlen (dis_inc) + flen);
	    strcat (strcat (strcpy (buf, dis_inc), "/"), filename);
	    if (readfile (buf, label, hbyte, callname, max)) {
		*bufptr = buf;
		return;
	    }
	    Mfree (buf);
	}

	/* ���ϐ� include �̃p�X���猟������. */
	if ((include = getenv ("include")) != NULL) {
	    buf = (char*) Malloc (strlen (include) + flen);
	    strcat (strcat (strcpy (buf, include), "/"), filename);
	    if (readfile (buf, label, hbyte, callname, max)) {
		*bufptr = buf;
		return;
	    }
	    Mfree (buf);
	}
    }

    if (path_flag || dis_inc || include)
	err ("%s ���I�[�v���o���܂���.\n", filename);
    else
	err ("���ϐ� (dis_)include ���ݒ肳��Ă��܂���.\n");
}
#endif	/* !OSKDIS */


/*
	include�t�@�C������ǂݍ��񂾕������
	�V���{�����A�^�����ߖ��A����(�܂��͉�������)
	�ɕ������A�e������ւ̃|�C���^��ݒ肵�ĕԂ�.

	������̓��e���������Ȃ������ꍇ��0�A���������1��Ԃ�.
*/

#define IS_EOL(p) ((*(p) < 0x20) || \
	(*(p) == (char)'*') || (*(p) == (char)';') || (*(p) == (char)'#'))

private int
separate_line (char* ptr, char** symptr, char** psdptr, char** parptr)
{
    char* p = ptr;
    unsigned char c;

    /* �s���̋󔒂��΂� */
    while ((*p == (char)' ') || (*p == (char)'\t'))
	p++;

    /* ��s�܂��͒��ߍs�Ȃ�G���[ */
    if (IS_EOL (p))
	return 0;

    /* �V���{�����̎��� */
    *symptr = p;

    /* �V���{�����̖�����NUL�ɂ��� */
    p = strpbrk (p, ":. \t");
    if (p == NULL)
	return 0;
    c = *p;
    *p++ = '\0';

    /* �V���{����`��':'�A�󔒁A�^�����߂�'.'���΂� */
    while (c == (char)':')
	c = *p++;
    while ((c == (char)' ') || (c == (char)'\t'))
	c = *p++;
    if (c == (char)'.')
	c = *p++;
    p--;

    /* �^�����߂��L�q����Ă��Ȃ���΃G���[ */
    if (IS_EOL (p))
	return 0;

    /* �^�����߂ւ̃|�C���^��ݒ� */
    *psdptr = p;

    /* �^�����ߖ��̖�����NUL�ɂ��� */
    p = strpbrk (p, " \t");
    if (p == NULL)
	return 0;
    *p++ = '\0';

    /* �p�����[�^�܂ł̋󔒂��΂� */
    while ((*p == (char)' ') || (*p == (char)'\t'))
	p++;

    /* �p�����[�^���L�q����Ă��Ȃ���΃G���[ */
    if (IS_EOL (p))
	return 0;

    /* �p�����[�^�ւ̃|�C���^��ݒ� */
    *parptr = p;

    /* �p�����[�^�̖�����NUL�ɂ��� */
    while (*(unsigned char*)p++ > 0x20)
	;
    *--p = '\0';

    return 1;
}


/*

	�A�Z���u����equ/macro�^�����߂̍s��ǂݎ��.
	�V���{����`��ǂݎ������1��Ԃ��A����ȊO��0��Ԃ�.
	(��x�ڂ̒�`�𖳎���������Amacro��`�Ȃǂ�0)

*/

private int
getlabel (char* linebuf, char** label, int hbyte, char* callname)
{
    char* symptr;
    char* psdptr;
    char* parptr;

    if (separate_line (linebuf, &symptr, &psdptr, &parptr)) {

	/* �R�[���� equ �R�[���ԍ� */
	if (strcasecmp (psdptr, "equ") == 0) {
	    int val;
	    if (*parptr == (char)'$')
		parptr++;
	    else if (strncasecmp (parptr, "0x", 2) == 0)
		parptr += 2;
	    else
		return 0;
	    val = atox (parptr);

	    if (hbyte == 0xa) {		/* SXCALL */
		if (((val >> 12) == hbyte) && !label[val & 0xfff]) {
		    label[val & 0xfff] = strcpy (Malloc (strlen (symptr) + 1), symptr);
		    return 1;
		}
	    }
	    else {			/* DOS, IOCS, FPACK */
		if (((val >> 8) == hbyte) && !label[val & 0xff]) {
		    label[val & 0xff] = strcpy (Malloc (strlen (symptr) + 1), symptr);
		    return 1;
		}
	    }
	}

	/* �}�N����: .macro �������� */
	else if (strcasecmp (psdptr, "macro") == 0) {
	    if (strlen (symptr) < MAX_MACRO_LEN)
		strcpy (callname, symptr);
#if 0
	    else
		eprintf ("Warning : �}�N�������������܂�(%s)\n", symptr);
#endif
	}
    }
    return 0;
}


/*
	�t�@�C������V���{����`��ǂݍ���.
	�t�@�C���̃I�[�v���Ɏ��s�����ꍇ��0��Ԃ��A����ȊO��1��Ԃ�.
	�V���{���������`����Ă��Ȃ��Ă��x����\�����邾����
	�A�{�[�g�͂��Ȃ�.
*/
private int
readfile (const char* filename, char*** label, int hbyte, char* callname, int max)
{
    int label_num;
    FILE* fp;

    if ((fp = fopen (filename, "rt")) == NULL)
	return 0;

    /* �t�@���N�V�������ւ̔z����m�� */
    {
	char** p;
	int i;

	p = *label = Malloc (sizeof (char*) * max);
	for (i = 0; i < max; i++)
	    *p++ = NULL;
    }

    eprintf ("Reading %s...\n", filename);
    {
	char linebuf[256];

	label_num = 0;
	while (fgets (linebuf, sizeof linebuf, fp))
	    label_num += getlabel (linebuf, *label, hbyte, callname);
    }
    fclose (fp);

    if (label_num == 0)
#if 0
	err ("Error : %s �ɂ̓V���{�����P����`����Ă��܂���.\n", filename);
#else
	eprintf ("Warning : %s �ɂ̓V���{�����P����`����Ă��܂���.\n", filename);
#endif
    return 1;
}


/* EOF */
