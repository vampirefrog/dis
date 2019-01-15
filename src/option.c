/* $Id: option.c,v 1.1 1996/11/07 08:03:54 ryo freeze $
 *
 *	Source Code Generator
 *	Option analyze , etc
 *	Copyright (C) 1989,1990 K.Abe, 1994 R.ShimiZu
 *	All rights reserved.
 *	Copyright (C) 1997-2010 Tachibana
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __LIBC__
#include <sys/xglob.h>
#else
#define _toslash(p) (p)
#endif

#include "analyze.h"		/* Arg_after_call */
#include "disasm.h"
#include "estruct.h"
#include "etc.h"		/* strupr */
#include "fpconv.h"		/* Inreal_flag */
#include "generate.h"
#include "getopt.h"
#include "global.h"
#include "hex.h"
#include "include.h"		/* xxx_mac */
#include "option.h"
#include "output.h"		/* Output_AddressCommentLine, Output_SplitByte */
#include "symbol.h"		/* Output_Symbol */


boolean option_a, option_c, option_d, option_e, option_g, option_h,
	option_i, option_k, option_l, option_p, option_q, option_r,
	option_v, option_x, option_y, option_z, option_B,
	option_D, option_E, option_I, option_M, option_N, /* option_Q, */
	option_S, option_T, option_U, option_Y, option_Z;

boolean option_overwrite;


/* main.c */
extern void	print_title (void);

extern short	Emulate_mode;	/* bit0=1:fpsp bit1=1:isp emulation命令を認識する */
extern int	String_length_min;
extern char	CommentChar;
extern address	Base, Exec;
extern int	Verbose;
extern int	Quiet_mode;
#ifndef	OSKDIS
extern char	FileType;
#endif

extern char*	Filename_in;
extern char*	Filename_out;
extern char*	Labelfilename_in;
extern char*	Labelfilename_out;
extern char*	Tablefilename;


/* static 関数プロトタイプ */
private void	option_switch (int, char**);



/*

  オプションを解析する

*/
extern void
analyze_args (int argc, char* argv[])
{
    int fileind;
    char* envptr;

    static const char optionlist[] =
	"a::b:cde::fg::hijklm:n:o:pq::rs::u::vw:xyz:"
	"ABC::DEFGIK:L:MNP:QR:S::T::UV:W:XYZ::#:-:";

#ifdef	READ_FEFUNC_H
    fefunc_mac = "fefunc.h";
#endif

    /* process environment variable `dis_opt' */
    if (/* option_Q == FALSE && */ (envptr = getenv (DIS_ENVNAME))) {
	int c;
	int count, cnt;
	char *envs, *envp;	/* envs=固定, envp=作業用 */
	char **args, **argp;	/* args=固定, argp=作業用 */

	/* 引数をスペースで分割する */
	while (*envptr == ' ')
	    envptr++;
	envp = envs = Malloc (strlen (envptr) + 1);
	for (count = 1; *envptr; count++) {
	    while (*envptr && (*envptr != ' '))
		*envp++ = *envptr++;
	    *envp++ = '\0';
	    while (*envptr == ' ')
		envptr++;
	}

	/* 各引数へのポインタ配列を作る */
	argp = args = Malloc ((count + 1) * sizeof (char*));
	envp = envs;
	*argp++ = argv[0];		/* my name */
	for (cnt = count; --cnt;) {
	    *argp++ = envp;
	    while (*envp++)
		;
	}
	*argp = NULL;

	/* オプション解析 */
	/* optind = -1; */
	while ((c = dis_getopt (count, args, optionlist)) != EOF)
	    option_switch (c, args);

	/* 忘れずに解放 */
	Mfree (args);
	Mfree (envs);
    }

    /* process commandline variable */
    {
	int c;
	optind = -1;
	while ((c = dis_getopt (argc, argv, optionlist)) != EOF)
	    option_switch (c, argv);
	fileind = optind;
    }


    while (fileind < argc) {
	if (Filename_in == NULL)
	    Filename_in = argv[fileind++];
	else if (Filename_out == NULL)
	    Filename_out = argv[fileind++];
	else
	    err ("Too many file names.\n");
    }
    if ((Filename_in  == NULL) || (*Filename_in  == (char)'\0'))
	usage (1);
    if ((Filename_out == NULL) || (*Filename_out == (char)'\0'))
	Filename_out = "-";


    if ((MMU_type & MMU851) && !(MPU_types & M020))
	err ("-m68851 can only be used with -m68020.\n");

    if ((FPCP_type & F88x) && !(MPU_types & (M020|M030)))
	err ("-m6888x can only be used with -m68020/68030.\n");

    if ((Emulate_mode & 2) && (MPU_types & M060))
	MPU_types |= MISP;

    if (Emulate_mode & 1) {
	short tmp = FPCP_type;
	if (tmp & F040) tmp |= F4SP;
	if (tmp & F060) tmp |= F6SP;
	FPCP_type = tmp;
    }


    /* if invoked with -T , labelfile must be read */
    if (option_T)
	option_g = TRUE;


    /* ラベルファイル/テーブルファイル名を作成する */
    {
	char* file = strcmp ("-", Filename_out) ? Filename_out : "aout";
	char* buf = Malloc (strlen (file) + 1);
	size_t len;

	_toslash (strcpy (buf, file));

	{
	    char* p = strrchr (buf, '/');
	    p = p ? (p + 1) : buf;
	    p = strrchr ((*p == (char)'.') ? (p + 1) : p, '.');
	    if (p)
		*p = '\0';		/* 拡張子を削除する */
	}
	len = strlen (buf) + 4 + 1;

	if (option_g && Labelfilename_in == NULL) {
	    Labelfilename_in = Malloc (len);
	    strcat (strcpy (Labelfilename_in, buf), ".lab");
	}
	if (option_e && Labelfilename_out == NULL) {
	    Labelfilename_out = Malloc (len);
	    strcat (strcpy (Labelfilename_out, buf), ".lab");
	}
	if (option_T && Tablefilename == NULL) {
	    Tablefilename = Malloc (len);
	    strcat (strcpy (Tablefilename, buf), ".tab");
	}

	Mfree (buf);
    }
}


private int
ck_atoi (char* str)
{
    unsigned char c;
    unsigned char* ptr = (unsigned char*) str;

    while ((c = *ptr++) != '\0') {
	if ((c < '0') || ('9' < c))
	    err ("Specification of numerical value is abnormal.\n");
    }

    return atoi (str);
}


private void
init_fpuid_table (void)
{
    memset (FPUID_table, 0, sizeof FPUID_table);
}


private const char**
include_option (char* p)
{
    char eq;

    if (strncmp (p, "exclude-", 8) != 0 &&
	strncmp (p, "include-", 8) != 0)
	return NULL;

    eq = (*p == (char)'e') ? '\0' : '=';
    p += 8;

    if (strncmp (p, "doscall-mac", 11) == 0 && p[11] == eq)
	return &doscall_mac;
    if (strncmp (p,"iocscall-mac", 12) == 0 && p[12] == eq)
	return &iocscall_mac;
    if (strncmp (p,  "fefunc-mac", 10) == 0 && p[10] == eq)
	return &fefunc_mac;

    return NULL;
}

private const char*
include_filename (char* p)
{
    size_t len;

    if (*p == (char)'e')
	return NULL;

    while (*p++ != '=')
	;

    len = strlen (p);
    return len ? _toslash (strcpy (Malloc (len + 1), p))
	       : NULL;
}

private void
option_switch (int opt, char** argv)
{
    switch (opt) {
    case 'a':
	option_a = TRUE;
	Output_AddressCommentLine = (optarg ? ck_atoi (optarg) : 5);
	break;
    case 'b':
	Generate_SizeMode = ck_atoi (optarg);
	break;
    case 'c':
	option_c = TRUE;
	break;
    case 'd':
	option_d = TRUE;
	break;
    case 'e':
	option_e = TRUE;
	if (optarg)
	    Labelfilename_out = optarg;
	break;
    case 'f':
	Disasm_Exact = FALSE;
	break;
    case 'g':
	option_g = TRUE;
	if (optarg)
	    Labelfilename_in = optarg;
	break;
    case 'h':
	option_h = TRUE;
	break;
    case 'i':
	option_i = TRUE;
	break;
    case 'j':
	Disasm_AddressErrorUndefined = FALSE;
	break;
    case 'k':
	option_k = TRUE;
	break;
    case 'l':
	option_l = TRUE;
	break;
    case 'm':
	{
	    int init = 0;
	    char* next;
#ifndef	__LIBC__
	    char* new_optarg = (char*) Malloc (strlen (optarg) + 1);
	    optarg = strcpy (new_optarg, optarg);
#endif
	    do {
		int m;

		next = strchr (optarg, ',');
		if (next)
		    *next++ = '\0';

#ifdef	COLDFIRE
		if (strcasecmp (optarg, "cf") == 0
		 || strcasecmp (optarg, "coldfire") == 0) {
		    Disasm_ColdFire = TRUE;
		    continue;
		}
#endif
		if (strcasecmp (optarg, "cpu32") == 0) {
		    Disasm_CPU32 = TRUE;
		    continue;
		}
		if (strcasecmp (optarg, "680x0") == 0)
		    m = -1;
		else {
		    m = ck_atoi (optarg);
		    if (m == 68851) {
			MMU_type |= MMU851;
			continue;
		    } else if (m == 68881 || m == 68882) {
			int id = next ? ck_atoi (next) : 1;

			FPCP_type |= (m == 68882) ? F882 : F881;
			if (id & ~7)
			    err ("The value of FPU ID is abnormal (-m).\n");
			FPUID_table[id*2] = 1;
			break;
		    }
		}

		/* 680x0, 68000, 68010, ... */
		if (init == 0) {
		    init = 1;
		    MPU_types = 0;
		    FPCP_type = MMU_type = 0;
		    init_fpuid_table ();
		}
		switch (m) {
		case 68000:
		case 68008:
		    MPU_types |= M000;
		    break;
		case 68010:
		    MPU_types |= M010;
		    break;
		case 68020:
		    MPU_types |= M020;
		    break;
		case 68030:
		    MPU_types |= M030;
		    MMU_type |= MMU030;
		    break;
		case 68040:
		    MPU_types |= M040;
		    FPCP_type |= F040;
		    MMU_type |= MMU040;
		    FPUID_table[1*2] = 1;
		    break;
		case 68060:
		    MPU_types |= M060;
		    FPCP_type |= F060;
		    MMU_type |= MMU060;
		    FPUID_table[1*2] = 1;
		    break;
		case -1:	/* -m680x0 */
		    MPU_types |= (M000|M010|M020|M030|M040|M060);
		    FPCP_type |= (F040|F060);
		    MMU_type |= (MMU030|MMU040|MMU060);
		    FPUID_table[1*2] = 1;
		    break;
		default:
		    err ("The specified MPU and FPU are not supported (-m).\n");
		    /* NOT REACHED */
		}
	    } while ((optarg = next) != NULL);
#ifndef	__LIBC__
	    Mfree (new_optarg);
#endif
	}
	break;
    case 'n':
	String_length_min = ck_atoi (optarg);
	break;
    case 'o':
	String_width = ck_atoi (optarg);
	if( String_width < 1 || 80 < String_width )
	    err ("The value is an invalid range (-o).\n");
	break;
    case 'p':
	option_p = TRUE;
	break;
    case 'q':
	option_q = TRUE;
	if( optarg ) Quiet_mode = ck_atoi (optarg);
	if( Quiet_mode < 0 || 1 < Quiet_mode )
	    err ("Value is invalid range (-q).\n");
	break;
    case 'r':
	option_r = TRUE;
	break;
    case 's':
	Output_Symbol = optarg ? ck_atoi (optarg) : 0;
	if ((unsigned short)Output_Symbol > 2)
	    err ("Value is invalid range (-s).\n");
	break;
    case 'u':
	Disasm_UnusedTrapUndefined = FALSE;
	if (optarg && (ck_atoi (optarg) == 1))
	    Disasm_SX_Window = TRUE;
	break;
    case 'v':
	option_v = TRUE;
	break;
    case 'w':
	Data_width = ck_atoi (optarg);
	if( Data_width < 1 || 32 < Data_width )
	    err ("Value is invalid range (-w).\n");
	break;
    case 'x':
	option_x = TRUE;
	break;
    case 'y':
	option_y = TRUE;
	break;
    case 'z':
	option_z = TRUE;
	Absolute = ABSOLUTE_ZOPT;
	FileType = 'r';
	{
	    char* p;
#ifndef	__LIBC__
	    char* new_optarg = (char*) Malloc (strlen (optarg) + 1);
	    optarg = strcpy (new_optarg, optarg);
#endif
	    p = strchr (optarg, ',');
	    if (p) {
		*p++ = '\0';
		Base = (address) atox (optarg);
		Exec = (address) atox (p);
	    } else
		Exec = Base = (address) atox (optarg);
#ifndef	__LIBC__
	    Mfree (new_optarg);
#endif
	}
	if (Base > Exec) {
	    err ("The value is invalid range (-z).\n");
	}
	break;

    case 'A':
	Disasm_MnemonicAbbreviation = TRUE;
	break;
    case 'B':
	option_B = TRUE;
	break;
    case 'C':
	SymbolColonNum = optarg ? ck_atoi (optarg) : 0;
	if( SymbolColonNum < 0 || 3 < SymbolColonNum )
	    err ("Value is invalid range (-C).\n");
	break;
    case 'D':
	option_D = TRUE;
	break;
    case 'E':
	option_E = TRUE;
	break;
    case 'F':
	Disasm_Dbra = FALSE;
	break;
    case 'G':
	Arg_after_call = TRUE;
	break;
    case 'I':
	option_I = TRUE;
	break;
    case 'K':
	if (!optarg[0] || optarg[1])
	    err ("Only one character can be specified for the comment character (-K).\n");
	CommentChar = *optarg;
	break;
    case 'L':
	if (!optarg[0] || optarg[1])
	    err ("Only the first letter of the label can be specified (-L).\n");
	if ((char)'0' <= *optarg && *optarg <= (char)'9')
	    err ("Numbers can not be used as the first character of the label (-L).\n");
	Label_first_char = *optarg;
	break;
    case 'M':
	option_M = TRUE;
	break;
    case 'N':
	option_N = TRUE;
	break;
    case 'P':
	Emulate_mode = ck_atoi (optarg);
	if (Emulate_mode & ~0x03)
	    err ("Value is invalid range (-P).\n");
	break;
    case 'Q':
    /*  option_Q = TRUE;  */
	break;
    case 'R':
	UndefRegLevel = ck_atoi (optarg);
	if (UndefRegLevel & ~0x0f)
	    err ("Value is invalid range (-R).\n");
	break;
    case 'S':
	option_S = TRUE;
	Output_SplitByte = ( optarg ? ck_atoi (optarg) : 64 ) * 1024;
	break;
    case 'T':
	option_T = TRUE;
	if (optarg)
	    Tablefilename = optarg;
	break;
    case 'U':
	option_U = TRUE;
	strupr (opsize);
	/* fall through */
    case 'X':
	strupr (Hex);
	break;
    case 'V':
	Verbose = ck_atoi (optarg);
	if( Verbose < 0 || 2 < Verbose )
	    err ("Value is invalid range (-V).\n");
	break;
    case 'W':
	Compress_len = ck_atoi (optarg);
	break;
    case 'Y':
	option_Y = TRUE;
	break;
    case 'Z':
	option_Z = TRUE;
	Zerosupress_mode = optarg ? ck_atoi(optarg) : 0;
	if( Zerosupress_mode < 0 || 1 < Zerosupress_mode )
	    err ("Value is invalid range (-Z).\n");
	break;
    case '#':
	Debug = ck_atoi (optarg);
	break;
    case '-':
	{
#ifndef	OSKDIS
	    const char** p;
	    char c = optarg[0];
	    if (!optarg[1] && (c == (char)'x' || c == (char)'r' || c == (char)'z')) {
		FileType = c;
		break;
	    }
#endif

#define isLONGOPT(str) (strcmp (optarg, str) == 0)
	         if (isLONGOPT (   "fpsp"))	Emulate_mode |=  1;
	    else if (isLONGOPT ("no-fpsp"))	Emulate_mode &= ~1;
	    else if (isLONGOPT (   "isp"))	Emulate_mode |=  2;
	    else if (isLONGOPT ("no-isp"))	Emulate_mode &= ~2;
	    else if (isLONGOPT ("no-fpu"))	{ FPCP_type = 0; init_fpuid_table (); }
	    else if (isLONGOPT ("no-mmu"))	MMU_type = 0;

	    else if (isLONGOPT ("sp"))		sp_a7_flag = TRUE;
	    else if (isLONGOPT ("a7"))		sp_a7_flag = FALSE;
	    else if (isLONGOPT ("old-syntax"))	Old_syntax = TRUE;
	    else if (isLONGOPT ("new-syntax"))	Old_syntax = FALSE;
	    else if (isLONGOPT ("inreal"))	Inreal_flag = TRUE;
	    else if (isLONGOPT ("real"))	Inreal_flag = FALSE;

	    else if (isLONGOPT ("overwrite"))	option_overwrite = TRUE;
	    else if (isLONGOPT ("help"))	usage (0);
	    else if (isLONGOPT ("version"))	{ printf ("dis version %s\n", Version);
						  exit (0); }
#ifndef	OSKDIS
	    else if (strncmp (optarg, "header=", 7) == 0)
		Header_filename = optarg + 7;
	    else if ((p = include_option (optarg)) != NULL)
		*p = include_filename (optarg);
#endif
	    else
		err ("%s: unrecognized option '--%s'\n", argv[0], optarg);
	}
	break;
/*  case '?':  */
    default:
	exit (EXIT_FAILURE);
	break;
    }
    return;
}


extern void
usage (int exitcode)
{
    static const char message[] =
	"usage: dis [option] Execution file name [Output file name]\n"
	"option:\n"

	/* Lower case option */
	"	-a[num]		num Comment out addresses for each line (every 5 lines if num is omitted)\n"
	"	-b num		Relative branch instruction size output (0: Auto 1: Always omitted 2: Always attached)\n"
	"	-c		Do not perform label check\n"
	"	-d		Specified at the time of device driver\n"
	"	-e[file]	Output of label file\n"
	"	-f		Do not check the indefinite byte of byte manipulation instruction ($ 00 or $ff ?)\n"
	"	-g[file]	Read label file\n"
	"	-h		Pay attention to the address following $4e75(rts) in the data area\n"
	"	-i		Even if there is an undefined instruction at the branch destination, it is not regarded as a data area\n"
	"	-j		Do not regard instructions that address errors will occur as undefined instructions\n"
	"	-k		We assume that there is no label pointing in the instruction\n"
	"	-l		I will not search severely until I can not find the program area\n"
	"	-m 680x0[,...]	Specify MPU to be disassembled (68000-68060, 680x0)\n"
	"	-m 68851	68851 Enable instruction (valid only when -m68020 is specified)\n"
	"	-m 6888x[,ID]	Specify valid FPCP and its ID (68881/68882 ID=0-7, default 1)\n"
	"	-n num		Minimum length to evaluate as a string. If not 0(Initial value = 3)\n"
	"	-o num		Number of digits in character string area (1 ≦ num ≦ 80 initial value = 60)\n"
	"	-p		The program area in the data area is not discriminated\n"
	"	-q[num]		Do not output message ([0]: normal 1: information about table is not output)\n"
	"	-r		Add a hexadecimal comment to a character string\n"
	"	-s[num]		Output of symbol table ([0]:not 1:[normal] 2:all)\n"
/* -t */
	"	-u[num]		Unused A, F line trap is not regarded as an undefined instruction (num = 1 SX-Window compatible)\n"
	"	-v		Output a simple disassemble list\n"
	"	-w num		Number of horizontal bytes in the data area (1 ≦ num ≦ 32 initial value = 8)\n"
	"	-x		Append the actual opcode with a hexadecimal comment\n"
	"	-y		Do not check whether all data areas are program areas\n"
	"	-z base,exec	We consider the executable file as a binary file from base and parse it from exec\n"

	/* Uppercase option */
	"\n"
	"	-A		Change cmpi, movea etc. to cmp, move etc.\n"
	"	-B		Breaking a line even after bra\n"
	"	-C[num]		Colon after the label (0: not attached 1: 1 for every [2]: normal 3:2 for all)\n"
	"	-D		You can also accept programs in the data section\n"
	"	-E		Do not check rewriting of indefinite bytes of byte manipulation instruction\n"
	"	-F		Output dbra, fdbra as dbf, fdbf\n"
	"	-G		Analyze the program that puts the argument immediately after the subroutine call\n"
/* -H */
	"	-I		Displays the address of the label to be inserted in the command\n"
/* -J */
	"	-K char		Use char as a comment character\n"
	"	-L char		Use char as the first character of label name\n"
	"	-M		cmpi, move, addi.b, subi.b Add comments to #imm and pack, unpk\n"
	"	-N		If size is default, it does not\n"
/* -O */
	"	-P num		Enable software emulation instruction (bit specification, initial value = 3)\n"
	"		+1	Enable unimplemented floating point instructions\n"
	"		+2	Enable unimplemented integer instructions\n"
/*	"	-Q		Do not reference environment variable dis_opt\n"	*/
	"	-R num		Specification of check item of unused field (bit designation, initial value = 15)\n"
	"		+1	Check unused register fields in mulu.l, muls.l, ftst.x\n"
	"		+2	Check for suppressed register fields in extended addressing\n"
	"		+4	Check scaling against suppressed index register\n"
	"		+8	Check size specification (.l) for suppressed index register\n"
	"	-S[num]		Divide the output file every num KB (if num is omitted, 64 KB)\n"
	"	-T[file]	Read table description file\n"
	"	-U		Output mnemonic in capital letters\n"
	"	-V num		Display of cause of backtrack (0:Disable [1]:Program area 2:all areas)\n"
	"	-W num		The minimum number of bytes to output the same data in. Dcb. If 0, it is not compressed (initial value = 64)\n"
	"	-X		Output hexadecimal numbers in upper case\n"
	"	-Y		Search the include file from the current directory\n"
	"	-Z[num]		Zero suppression of hexadecimal number ([0]:normal 1:omitted optional '$' omitted)\n"

	/* Word name option */
	"\n"

#ifndef	OSKDIS
	"	--include-XXX-mac=file	Specifying the include file (XXX = doscall, iocscall, fefunc)\n"
	"	--exclude-XXX-mac	Do not read include files\n"
#endif
	"	--header=file	Specification of header file (Overrides environment variable dis_header)\n"
	"	--(no-)fpsp	Make the unimplemented floating point instruction [Enable] (invalid)\n"
	"	--(no-)isp	Make unimplemented integer instruction [Enable] (invalid)\n"
	"	--no-fpu	Invalidate the internal FPU instruction (specified after -m 68040 to 68060)\n"
	"	--no-mmu	Invalidate the built-in MMU instruction (specified after -m 68030 to 68060)\n"
	"	--sp		Write a7 register as 'sp' (- a7 by default)\n"
	"	--old-syntax	Output addressing as old notation (--new-syntax by default)\n"
	"	--(in)real	Output floating point in [real notation] (internal expression)\n"
	"	--overwrite	Overwrite file unconditionally\n"
	"	--version	Display version\n"
	"	--help		Display usage\n"

#if 0	/* Hidden option */
	"\n"
#ifndef	OSKDIS
	"	--x|r|z		Consider an executable file as X | R | Z format\n"
#endif
	"	-# num		Debug mode\n"
#endif
	; /* end of message */

    print_title ();
    printf (message);
    exit (exitcode);
}

/* EOF */
