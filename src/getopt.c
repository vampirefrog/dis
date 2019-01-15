/*
 * PROJECT C Library, X68000 PROGRAMMING INTERFACE DEFINITION
 * --------------------------------------------------------------------
 * This file is written by the Project C Library Group,	 and completely
 * in public domain. You can freely use, copy, modify, and redistribute
 * the whole contents, without this notice.
 * --------------------------------------------------------------------
 * $Id: getopt.c,v 1.1 1996/11/07 08:03:38 ryo freeze $
 */

/* System headers */
#include <stdio.h>
#include <stdlib.h>

#include "getopt.h"

char *optarg;					/* �������w���|�C���^ */
int optind = -1;				/* ARGV �̌��݂̃C���f�b�N�X */
static int opterr = 1;				/* �G���[�\���t���O */
static int _getopt_no_ordering;			/* ORDER �t���O */

/* File scope functions */
static void rotate_right (char *vector[], int size)
{
    char *temp;

    /* �Ō����ۑ� */
    temp = vector[size - 1];

    /* size �����[�e�[�g */
    while (size--)
	vector[size] = vector[size - 1];

    /* �擪�ɕۑ����Ă������̂�... */
    vector[0] = temp;
}

/* Functions */
int dis_getopt (int argc, char *argv[], const char *options)
{
    int index;					/* ���݂̃C���f�b�N�X */
    int next;					/* ���̃C���f�b�N�X */
    int optchar;				/* �I�v�V�������� */
    char *string;				/* �I�v�V���������� */
    const char *ptr;				/* options �T���p */

    static int init;				/* ���񏉊�������K�v���� */
    static int savepoint;			/* ���ꊷ���p�̋L���|�C���g */
    static int rotated;				/* �����t���O */
    static int comidx;				/* �����C���f�b�N�X */

#ifdef __LIBC__
#define ERR(fmt,arg1,arg2)	if (opterr) eprintf(fmt,arg1,arg2)
#else
#define ERR(fmt,arg1,arg2)	if (opterr) fprintf(stderr,fmt,arg1,arg2)
#endif

    /* �������̕K�v������Ώ��������� */
    if (init || optind < 0) {
	optind = 1;
	optarg = 0;
	rotated = 0;
	init = 0;
	comidx = 1;
	savepoint = 0;
    }

    /* �{���J�n�ʒu��ݒ� */
    index = optind;

  again:

    /* ���������o�� */
    string = argv[index];
    next = index + 1;

    /* ���łɏI�肩�H */
    if (string == 0) {
	if (savepoint > 0)
	    optind = savepoint;
	init = 1;
	return EOF;
    }

    /* '-' �Ŏn�܂邩�H */
    if (*string == '-') {

	/* �t���O���|�C���^��i�߂� */
	string += comidx;

	/* ���m�� "-" ���H�Ȃ�Ε��ʂ̈��� */
	if ((optchar = *string++) == '\0')
	    goto normal_arg;

	/* ORDERING �̕K�v������Έ�����𕔕��I�Ƀ��[�e�[�g */
	if (savepoint > 0) {
	    rotate_right (&argv[savepoint], index - savepoint + 1);
	    rotated = 1;
	    index = savepoint;
	    savepoint++;
	}

	/* ���m�� "--" �Ȃ�΋����I�ɑ{�����I��ɂ��� */
	if (optchar == '-' && *string == '\0' && comidx == 1){
	    init = 1;
	    optchar = EOF;
	    goto goback;
	}

	/* �I�v�V���������Q�̒�����Y��������̂����邩���ׂ� */
	for (ptr = options; *ptr; ptr++)
	    if (*ptr != ':' && *ptr == optchar)
		break;

	/* �����𔺂��ꍇ�Ȃ�΃C���f�b�N�X�͏����� */
	if (*string == '\0' || ptr[1] == ':')
	    comidx = 1;

	/* �����Ȃ���Ε����I�v�V�����C���f�b�N�X�����Z */
	else {
	    comidx++;
	    index--;
	}

	/* ���ǌ�����Ȃ������Ȃ�... */
	if (*ptr == '\0') {
	    ERR ("%s: unrecognized option '-%c'\n", argv[0], optchar);
	    optchar = '?';
	}

	/* �����������I�v�V�����w��� ':' ������Ȃ�... */
	else if (ptr[1] == ':') {

	    /* ���� argv ���Ɉ��������邩 */
	    if (*string)
		optarg = string;

	    /* options�w�肪 ?:: �Ȃ�΁A���� argv �����炵�����Ȃ� */
	    else if (ptr[2] == ':')
		optarg = NULL;

	    /* ���̈����ɂ��邩 */
	    else if (argv[next]) {

		/* ORDERING �̕K�v������Ε����I�ɓ��ꊷ���� */
		if (rotated) {
		    rotate_right (&argv[savepoint], next - savepoint + 1);
		    index = savepoint;
		}

		/* �Ȃ����... */
		else
		    index++;

		/* ���̈�����Ԃ� */
		optarg = argv[index];

	    }

	    /* �Ȃ����... */
	    else {
		ERR ("%s: option '-%c' requires an argument\n", argv[0], optchar);
		optchar = '?';
	    }

	}

      goback:

	/* �l��ݒ肵�Ė߂� */
	rotated = 0;
	savepoint = 0;
	optind = index + 1;
	return optchar;

    }

    /* ���ʂ̈��� */
    else {

      normal_arg:

	/* ORDERING ����K�v�����邩�H */
	if (_getopt_no_ordering) {
	    init = 1;
	    optind = index;
	    return EOF;
	}

	/* �����̈ʒu���L�����A���̃I�v�V�����𒲂ׂ� */
	else {
	    if (savepoint == 0)
		savepoint = index;
	    index++;
	    goto again;
	}

    }
}

/* EOF */
