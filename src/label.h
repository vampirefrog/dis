/* $Id: label.h,v 1.1 1996/10/24 04:27:48 ryo freeze $
 *
 *	�\�[�X�R�[�h�W�F�l���[�^
 *	���x���Ǘ����W���[���w�b�_
 *	Copyright (C) 1989,1990 K.Abe
 *	All rights reserved.
 *	Copyright (C) 1997-2010 Tachibana
 *
 */

#ifndef	LABEL_H
#define	LABEL_H


typedef enum {
	PROLABEL =	0,			/* �v���O���� */
	DATLABEL =	0x010000,		/* �f�[�^ */
	FORCE    =	0x020000,		/* �����t���O */
	HIDDEN   =	0x040000,		/* 1 �Ȃ炻�̃��x���̓\�[�X���Ɍ���Ȃ� */
	TABLE    =	0x080000,		/* 1 �Ńe�[�u���J�n */
	ENDTABLE =	0x100000,		/* 1 �Ńe�[�u���I�� */
	SYMLABEL =	0x200000		/* 1 �ŃV���{����񂠂� */
#ifdef	OSKDIS
	CODEPTR  =	0x400000,		/* 1 �ŃR�[�h�|�C���^ */
	DATAPTR  =	0x800000,		/* 1 �Ńf�[�^�|�C���^ */
#endif	/* OSKDIS */
} lblmode;


struct _avl_node;
typedef struct {
	address		label;		/* �A�h���X */
	struct _avl_node	*avl;		/* AVL-tree-library side node */
	lblmode		mode;		/* ���� */
	short		shift;		/* ���� */
	unsigned short	count;		/* �o�^�� */
} lblbuf;


/*

  lblmode	���ʂW�r�b�g	�I�y���[�V�����T�C�Y(�f�[�^���x���̎��݈̂Ӗ�������)
		��16�r�b�g	0...PROLABEL	1...DATLABEL
		��17�r�b�g	0...����	1...����
  shift		���x���A�h���X�Ƃ̂���.�ʏ�O.
		���߂̃I�y�����h�Ƀ��x�����������肷���...

*/


extern void	init_labelbuf (void);
extern void	free_labelbuf (void);
extern boolean	regist_label (address adrs, lblmode mode);
extern void	unregist_label (address adrs);
extern lblbuf*	search_label (address adrs);
extern lblbuf*	next(address adrs);
extern lblbuf*	Next (lblbuf*);
extern lblbuf*	next_prolabel (address adrs);
extern lblbuf*	next_datlabel (address adrs);
extern int	get_Labelnum (void);

#define isPROLABEL(a)	(!isDATLABEL(a))
#define isDATLABEL(a)	((a) & DATLABEL)
#define isHIDDEN(a)	((a) & HIDDEN)
#define isTABLE(a)	((a) & TABLE)
#define isENDTABLE(a)	((a) & ENDTABLE)


#endif	/* LABEL_H */

/* EOF */
