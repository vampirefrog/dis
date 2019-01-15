/* $Id: disasm.h,v 1.1 1996/10/24 04:27:42 ryo freeze $
 *
 *	�\�[�X�R�[�h�W�F�l���[�^
 *	�t�A�Z���u�����W���[���w�b�_
 *	Copyright (C) 1989,1990 K.Abe, 1994 R.ShimiZu
 *	All rights reserved.
 *	Copyright (C) 1997-2010 Tachibana
 *
 */

#ifndef	DISASM_H
#define	DISASM_H


#include "estruct.h"


#define MAX_MACRO_LEN	16
#define SXCALL_MAX	0x1000	/* SX Window use 0xa000 - 0xafff */


/* MPU_types */
#define M000	(1<<0)		/* 68000 */
#define M010	(1<<1)		/* 68010 */
#define M020	(1<<2)		/* 68020 */
#define M030	(1<<3)		/* 68030 */
#define M040	(1<<4)		/* 68040 */
#define M060	(1<<6)		/* 68060 */
#define MISP	(1<<7)		/* 060ISP(software emulation) */

typedef unsigned char mputypes;

/* FPCP_type */
#define F881	(1<<0)		/* 68881 */
#define F882	(1<<1)		/* 68882 */
#define F88x	(F881|F882)
#define F040	(1<<2)		/* 68040 */
#define F4SP	(1<<3)		/* 040FPSP(software emulation) */
#define F060	(1<<4)		/* 68060 */
#define F6SP	(1<<5)		/* 060FPSP(software emulation) */

/* MMU_type */
#define MMU851	(1<<0)		/* 68020 + 68851 */
#define MMU030	(1<<1)		/* 68030 internal MMU */
#define MMU040	(1<<2)		/* 68040 internal MMU */
#define MMU060	(1<<3)		/* 68060 internal MMU */


typedef enum {
    OTHER ,		/* ���ʂ̖��� */
    JMPOP ,		/* ���򖽗� */
    JSROP ,		/* �T�u���[�`���R�[������ */
    RTSOP ,		/* ���^�[������ */
    BCCOP ,		/* �������򖽗� */
    UNDEF = 15 ,	/* ����` */
} opetype;


typedef enum {
    DregD ,		/* �f�[�^���W�X�^���� */
    AregD ,		/* �A�h���X���W�X�^���� */
    AregID ,		/* �A�h���X���W�X�^�Ԑ� */
    AregIDPI ,		/* �|�X�g�C���N�������g�A�h���X���W�X�^�Ԑ� */
    AregIDPD ,		/* �v���f�N�������g�A�h���X���W�X�^�Ԑ� */
    AregDISP ,		/* �f�B�X�v���[�X�����g�t�A�h���X���W�X�^�Ԑ� */
    AregIDX ,		/* �C���f�b�N�X�t�A�h���X���W�X�^�Ԑ� */
    AbShort = 8 ,	/* ��΃V���[�g�A�h���X */
    AbLong ,		/* ��΃����O�A�h���X */
    PCDISP ,		/* �f�B�X�v���[�X�����g�t�v���O�����J�E���^���� */
    PCIDX ,		/* �C���f�b�N�X�t�v���O�����J�E���^���� */
    IMMED ,		/* �C�~�f�B�G�C�g�f�[�^ */
    SRCCR = 16 ,	/* CCR / SR �`�� */

    AregIDXB,		/* �C���f�b�N�X&�x�[�X�f�B�X�v���[�X�����g�t���A�h���X���W�X�^�Ԑ� */
    AregPOSTIDX,	/* �|�X�g�C���f�b�N�X�t���������Ԑ� */
    AregPREIDX,		/* �v���C���f�b�N�X�t���������Ԑ� */
    PCIDXB,		/* �C���f�b�N�X&�x�[�X�f�B�X�v���[�X�����g�t���v���O�����J�E���^�Ԑ� */
    PCPOSTIDX,		/* �|�X�g�C���f�b�N�X�t��PC�������Ԑ� */
    PCPREIDX,		/* �v���C���f�b�N�X�t��PC�������Ԑ� */

    CtrlReg,		/* ���䃌�W�X�^ */

    RegPairD,		/* ���W�X�^�y�A(����) dx:dy */
    RegPairID,		/* ���W�X�^�y�A(�Ԑ�) (rx):(ry) */
    BitField,		/* �r�b�g�t�B�[���h�� {offset:width} */

    MMUreg,		/* MMU���W�X�^ */

    FPregD,		/* FPn */
    FPCRSR,		/* FPSR,FPCR,FPIAR */
    FPPairD,		/* ���W�X�^�y�A(����) FPx:FPy */
    KFactor,		/* K-Factor {offset:width} */

} adrmode;


typedef struct {
    char	    operand[ 64 ];	/* �I�y�����h������ */
    adrmode	    ea;			/* �����A�h���X���[�h */
    address	    opval;		/* �I�y�����h�̒l */
    address	    opval2;		/* �I�y�����h�̒l(od�p) */
    address	    eaadrs;		/* �I�y�����h�̑��݃A�h���X */
    address	    eaadrs2;		/* �I�y�����h�̑��݃A�h���X(od�p) */
    unsigned char   labelchange1;	/* ���x�����\ -1�Ȃ�()�Ȃ�(bsr�p) */
    unsigned char   labelchange2;	/* ���x�����\ */
    unsigned char   exbd;		/* bd �̃T�C�Y(0,2,4)  0�Ȃ�T�v���X */
    unsigned char   exod;		/* od �̃T�C�Y(0,2,4)  0�Ȃ�T�v���X */
} operand;


typedef struct {
    char    opecode[ 32 ];  /* ���� */
    opesize size;	    /* �T�C�Y ( lea , pea �� long ) ( 0 = .b .w .l .s nothing ) */
    opesize size2;	    /* �T�C�Y ( lea, pea, moveq, bset, ... �� UNKNOWN ) */
    opesize default_size;   /* ���̖��߂̃f�t�H���g�̃T�C�Y */
    int     bytes;	    /* ���߂̃o�C�g�� */
    opetype flag;	    /* ���߂̎�� ( 0 = other jmp jsr rts bcc undef ) */
    mputypes mputypes;	    /* ���̖��߂����s�\��MPU�̎��(M000|M010|...) */
    char    fpuid;	    /* ���������_���߂̃R�v���Z�b�TID(0-7,-1�Ȃ�ʏ햽��) */
    char    opflags;	    /* FLAGS_xxx */
    char    reserved;	    /* �\�� */
    address jmp;	    /* �W�����v��A�h���X ( ���򖽗߂Ȃ� ) */
    adrmode jmpea;	    /* �����A�h���X���[�h ( ���򖽗߂Ȃ� ) */
    operand op1;
    operand op2;
    operand op3;
    operand op4;
} disasm;

#define FLAG_CANNOT_UPPER	0x01
#define FLAG_NEED_COMMENT	0x02
#define FLAG_NEED_NULSTR	0x04

#ifndef OSKDIS
extern char**	OSlabel;
extern char**	FElabel;
extern char**	SXlabel;
extern char	OSCallName[MAX_MACRO_LEN];
extern char	FECallName[MAX_MACRO_LEN];
extern char	SXCallName[MAX_MACRO_LEN];
#endif
extern char	FPUID_table[16];
extern boolean	Disasm_Exact;
extern boolean	Disasm_String;
extern boolean	Disasm_UnusedTrapUndefined;
extern boolean	Disasm_AddressErrorUndefined;
extern boolean	Disasm_Dbra;
extern boolean	Disasm_MnemonicAbbreviation;
extern boolean	Disasm_SX_Window;
extern boolean	Disasm_CPU32;
extern boolean	Disasm_ColdFire;
extern mputypes	MPU_types;
extern short	MMU_type;
extern short	FPCP_type;
extern int	UndefRegLevel;
extern address	PCEND;

extern int	dis (address, disasm*, address*);


#endif	/* DISASM_H */

/* EOF */
