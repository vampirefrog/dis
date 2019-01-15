/* $Id: estruct.h,v 1.1 1996/10/24 04:27:44 ryo freeze $
 *
 *	�\�[�X�R�[�h�W�F�l���[�^
 *	�\���̒�`�t�@�C��
 *	Copyright (C) 1989,1990 K.Abe, 1994 R.ShimiZu
 *	All rights reserved.
 *	Copyright (C) 1997-2010 Tachibana
 *
 */

#ifndef	ESTRUCT_H
#define	ESTRUCT_H

#ifdef __HUMAN68K__
#define __attribute__(x) /* NOTHING */
#endif

/* #include <class.h> */
typedef unsigned char	UBYTE;
typedef short		WORD;
typedef unsigned short	UWORD;
typedef long		LONG;
typedef unsigned long	ULONG;

typedef unsigned long UINTPTR;
#define PRI_UINTPTR "lx"

#ifdef	TRUE
#undef	TRUE
#endif
#ifdef	FALSE
#undef	FALSE
#endif
typedef enum { FALSE, TRUE } boolean;

typedef enum {
    BYTESIZE,		/* �o�C�g */
    WORDSIZE,		/* ���[�h */
    LONGSIZE,		/* �����O���[�h */
    QUADSIZE,		/* �N���b�h���[�h */
    SHORTSIZE,		/* �V���[�g( ���Ε��򖽗� ) */

    SINGLESIZE,		/* 32bit�����^ */
    DOUBLESIZE,		/* 64bit�����^ */
    EXTENDSIZE,		/* 96bit�����^ */
    PACKEDSIZE,		/* 96bitBCD�����^ */
    NOTHING,		/* ���� */

    STRING,		/* ������ */
    RELTABLE,		/* �����P�[�^�u���I�t�Z�b�g�e�[�u�� */
    RELLONGTABLE,	/* �����O���[�h�ȃ����P�[�^�u���I�t�Z�b�g�e�[�u�� */
    ZTABLE,		/* ��ΔԒn�e�[�u�� */
#ifdef	OSKDIS
    WTABLE,		/* ���[�h�e�[�u�� */
#endif	/* OSKDIS */

    EVENID,
    CRID,
    WORDID,
    LONGID,
    BYTEID,
    ASCIIID,
    ASCIIZID,
    LASCIIID,
    BREAKID,
    ENDTABLEID,
    UNKNOWN = 128	/* �s�� */
} opesize;


typedef UBYTE*	address;


typedef struct {	/* .x �t�@�C���̃w�b�_ */
    UWORD   head;
    char    reserve2;
    char    mode;
    address base;
    address exec;
    ULONG   text;
    ULONG   data;
    ULONG   bss;
    ULONG   offset;
    ULONG   symbol;
    char    reserve[ 0x1c ];
    ULONG   bindinfo;
} __attribute__ ((packed)) xheader;

typedef struct {    /* .x �t�@�C���̃w�b�_ */
    unsigned short   head;
    char    reserve2;
    char    mode;
    unsigned int  base;
    unsigned int  exec;
    unsigned int   text;
    unsigned int   data;
    unsigned int   bss;
    unsigned int   offset;
    unsigned int   symbol;
    char    reserve[ 0x1c ];
    unsigned int   bindinfo;
} __attribute__ ((packed)) xfileheader;

typedef struct {	/* .z �t�@�C���̃w�b�_ */
    UWORD   header;
    ULONG   text;
    ULONG   data;
    ULONG   bss;
    char    reserve[8];
    address base;
    UWORD   pudding;
} __attribute__ ((packed)) zheader;

#ifdef	OSKDIS
typedef struct {	/* OS-9/680x0 ���W���[���̃w�b�_    */
    UWORD   head;	/* +00 $4AFC			    */
    UWORD   sysrev;	/* +02 ���r�W�����h�c		    */
    ULONG   size;	/* +04 ���W���[���T�C�Y		    */
    ULONG   owner;	/* +08 �I�[�i�h�c		    */
    address name;	/* +0C ���W���[�����I�t�Z�b�g	    */
    UWORD   accs;	/* +10 �A�N�Z�X����		    */
    UWORD   type;	/* +12 ���W���[���^�C�v�^����	    */
    UWORD   attr;	/* +14 �����^���r�W����		    */
    UWORD   edition;	/* +16 �G�f�B�V����		    */
    address usage;	/* +18 �g�����R�����g�̃I�t�Z�b�g   */
    address symbol;	/* +1C �V���{���e�[�u��		    */
    char    resv[14];	/* +20 �\��ς�			    */
    UWORD   parity;	/* +2E �w�b�_�p���e�B		    */
    address exec;	/* +30 ���s�I�t�Z�b�g		    */
    address excpt;	/* +34 ���[�U�g���b�v�G���g��	    */
    ULONG   mem;	/* +38 �������T�C�Y		    */
    ULONG   stack;	/* +3C �X�^�b�N�T�C�Y		    */
    address idata;	/* +40 �������f�[�^�I�t�Z�b�g	    */
    address irefs;	/* +44 �������Q�ƃI�t�Z�b�g	    */
    address init;	/* +48 ���������s�G���g��(TRAPLIB)  */
    address term;	/* +4C �I�����s�G���g��(TRAPLIB)    */
} __attribute__ ((packed)) os9header;
#endif	/* OSKDIS */


#endif	/* ESTRUCT_H */

/* EOF */
