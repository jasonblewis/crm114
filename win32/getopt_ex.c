/*!
 \file
 */

/*
 * Getopt for GNU.
 *
 * NOTE: getopt is now part of the C library, so if you don't know what
 * "Keep this file name-space clean" means, talk to drepper@gnu.org
 * before changing it!
 *
 * Copyright (C) 1987, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98
 *  Free Software Foundation, Inc.
 *
 * The GNU C Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The GNU C Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the GNU C Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * [i_a] if you get weird 'redefinition' errors and such, turn OFF
 *       using precompiled headers in MSVC6. No need for #include stdafx.h
 *       then.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "getopt_ex.h"



#if defined (SYSV_GETOPT_BASIC)

int optind  = 1;                /* index of which argument is next  */
char *optarg = NULL;            /* pointer to argument of current option */
int opterr  = 1;                /* allow error message  */

static char *letP = NULL;       /* remember next option char's location */
static char SW = '-';           /* The switch character, either '-' or '/' */

/*
 * Parse the command line options, System V style.
 *
 * Standard option syntax is:
 *
 *   option ::= SW [optLetter]* [argLetter space* argument]
 *
 * where
 *  - SW is either '/' or '-', according to the current setting
 *    of the MSDOS switchar (int 21h function 37h).
 *  - there is no space before any optLetter or argLetter.
 *  - opt/arg letters are alphabetic, not punctuation characters.
 *  - optLetters, if present, must be matched in optionS.
 *  - argLetters, if present, are found in optionS followed by ':'.
 *  - argument is any white-space delimited string.  Note that it
 *    can include the SW character.
 *  - upper and lower case letters are distinct.
 *
 * There may be multiple option clusters on a command line, each
 * beginning with a SW, but all must appear before any non-option
 * arguments (arguments not introduced by SW).  Opt/arg letters may
 * be repeated: it is up to the caller to decide if that is an error.
 *
 * The character SW appearing alone as the last argument is an error.
 * The lead-in sequence SWSW ("--" or "//") causes itself and all the
 * rest of the line to be ignored (allowing non-options which begin
 * with the switch char).
 *
 * The string *optionS allows valid opt/arg letters to be recognized.
 * argLetters are followed with ':'.  Getopt () returns the value of
 * the option character found, or EOF if no more options are in the
 * command line.  If option is an argLetter then the global optarg is
 * set to point to the argument string (having skipped any white-space).
 *
 * The global optind is initially 1 and is always left as the index
 * of the next argument of argv[] which getopt has not taken.  Note
 * that if "--" or "//" are used then optind is stepped to the next
 * argument before getopt() returns EOF.
 *
 * If an error occurs, that is an SW char precedes an unknown letter,
 * then getopt() will return a '?' character and normally prints an
 * error message via perror().  If the global variable opterr is set
 * to false (zero) before calling getopt() then the error message is
 * not printed.
 *
 * For example, if the MSDOS switch char is '/' (the MSDOS norm) and
 *
 *   optionS == "A:F:PuU:wXZ:"
 *
 * then 'P', 'u', 'w', and 'X' are option letters and 'F', 'U', 'Z'
 * are followed by arguments.  A valid command line may be:
 *
 *   aCommand  /uPFPi /X /A L someFile
 *
 * where:
 *  - 'u' and 'P' will be returned as isolated option letters.
 *  - 'F' will return with "Pi" as its argument string.
 *  - 'X' is an isolated option.
 *  - 'A' will return with "L" as its argument.
 *  - "someFile" is not an option, and terminates getOpt.  The
 *    caller may collect remaining arguments using argv pointers.
 */

int getopt(int argc, char *argv[], char *optionS)
{
    unsigned char ch;
    char *optP;

    if (argc > optind)
    {
        if (letP == NULL)
        {
            if ((letP = argv[optind]) == NULL
                || *(letP++) != SW)
            {
                goto gopEOF;
            }
            if (*letP == SW)
            {
                optind++;
                goto gopEOF;
            }
        }
        if (0 == (ch = *(letP++)))
        {
            optind++;
            goto gopEOF;
        }
        if (':' == ch  ||  (optP = strchr(optionS, ch)) == NULL)
        {
            goto gopError;
        }
        if (':' == *(++optP))
        {
            optind++;
            if (0 == *letP)
            {
                if (argc <= optind)
                    goto gopError;
                letP = argv[optind++];
            }
            optarg = letP;
            letP = NULL;
        }
        else
        {
            if (0 == *letP)
            {
                optind++;
                letP = NULL;
            }
            optarg = NULL;
        }
        return ch;
    }

gopEOF:
    optarg = letP = NULL;
    return EOF;

gopError:
    optarg = NULL;
    errno  = EINVAL;
    if (opterr)
        perror("get command line option");
    return '?';
}



#else /* SYSV_GETOPT_BASIC */



/*
 *  GETOPT(3)           Linux Programmer's Manual           GETOPT(3)
 *
 *  NAME
 *         getopt - Parse command line options
 *
 *  SYNOPSIS
 *         #include <unistd.h>
 *
 *         int getopt(int argc, char * const argv[],
 *                    const char *optstring);
 *
 *         extern char *optarg;
 *         extern int optind, opterr, optopt;
 *
 *         #include <getopt.h>
 *
 *         int getopt_long(int argc, char * const argv[],
 *                    const char *optstring,
 *                    const struct option *longopts, int *longindex);
 *
 *         int getopt_long_only(int argc, char * const argv[],
 *                    const char *optstring,
 *                    const struct option *longopts, int *longindex);
 *
 *  DESCRIPTION
 *         The  getopt()  function parses the command line arguments.
 *         Its arguments argc and argv are  the  argument  count  and
 *         array  as passed to the main() function on program invoca-
 *         tion.  An element of argv that starts with `-' (and is not
 *         exactly "-" or "--") is an option element.  The characters
 *         of this element (aside from the initial  `-')  are  option
 *         characters.   If getopt() is called repeatedly, it returns
 *         successively each of the option characters  from  each  of
 *         the option elements.
 *
 *         If  getopt()  finds  another  option character, it returns
 *         that character, updating the external variable optind  and
 *         a  static  variable  nextchar  so  that  the  next call to
 *         getopt() can resume the scan  with  the  following  option
 *         character or argv-element.
 *
 *         If  there  are no more option characters, getopt() returns
 *         EOF.  Then optind is the index in argv of the first  argv-
 *         element that is not an option.
 *
 *         optstring  is  a  string  containing the legitimate option
 *         characters.  If such a character is followed by  a  colon,
 *         the  option  requires  an  argument,  so  getopt  places a
 *         pointer to the following text in the same argv-element, or
 *         the  text  of  the following argv-element, in optarg.  Two
 *         colons mean an option takes an optional arg; if  there  is
 *         text  in  the  current  argv-element,  it  is  returned in
 *         optarg, otherwise optarg is set to zero.  This  is  a  GNU
 *         extension.   If  optstring  contains W followed by a semi-
 *         colon, then -W foo is treated as the  long  option  --foo.
 *         (The  -W  option is reserved by POSIX.2 for implementation
 *         extensions.)  This  behaviour  is  a  GNU  extension,  not
 *         available with libraries before GNU libc 2.
 *
 *         By  default,  getopt() permutes the contents of argv as it
 *         scans, so that eventually all the non-options are  at  the
 *         end.   Two other modes are also implemented.  If the first
 *         character of optstring is `+' or the environment  variable
 *         POSIXLY_CORRECT  is  set,  then option processing stops as
 *         soon as a non-option  argument  is  encountered.   If  the
 *         first  character of optstring is `-', then each non-option
 *         argv-element is handled as if it were the argument  of  an
 *         option  with  character code 1.  (This is used by programs
 *         that were written to expect options  and  other  argv-ele-
 *         ments in any order and that care about the ordering of the
 *         two.)  The special argument `--' forces an end of  option-
 *         scanning regardless of the scanning mode.
 *
 *         If  getopt()  does  not  recognize an option character, it
 *         prints an error message to stderr, stores the character in
 *         optopt,  and returns `?'.  The calling program may prevent
 *         the error message by setting opterr to 0.
 *
 *         The getopt_long() function works like getopt() except that
 *         it  also  accepts long options, started out by two dashes.
 *         Long option names may be abbreviated if  the  abbreviation
 *         is unique or is an exact match for some defined option.  A
 *         long option may take a parameter, of the form  --arg=param
 *         or --arg param.
 *
 *         longopts  is a pointer to the first element of an array of
 *         struct option declared in <getopt.h> as
 *
 *            struct option {
 *                const char *name;
 *                int has_arg;
 *                int *flag;
 *                int val;
 *            }
 *
 *         The meanings of the different fields are:
 *
 *         name   is the name of the long option.
 *
 *         has_arg
 *                is: no_argument (or 0) if the option does not  take
 *                an argument, required_argument (or 1) if the option
 *                requires an argument, or optional_argument  (or  2)
 *                if the option takes an optional argument.
 *
 *         flag   specifies  how  results  are  returned  for  a long
 *                option.   If  flag  is  NULL,  then   getopt_long()
 *                returns val.  (For example, the calling program may
 *                set val to the equivalent short option  character.)
 *                Otherwise, getopt_long() returns 0, and flag points
 *                to a variable which is set to val if the option  is
 *                found,  but  left  unchanged  if  the option is not
 *                found.
 *
 *         val    is the value to return, or to load into  the  vari-
 *                able pointed to by flag.
 *
 *         The  last  element  of  the  array  has  to be filled with
 *         zeroes.
 *
 *         If longindex is not NULL, it points to a variable which is
 *         set  to the index of the long option relative to longopts.
 *
 *         getopt_long_only() is like getopt_long(), but `-' as  well
 *         as  `--'  can  indicate  a long option.  If an option that
 *         starts with `-' (not `--') doesn't match  a  long  option,
 *         but  does  match  a  short option, it is parsed as a short
 *         option instead.
 *
 *  RETURN VALUE
 *         The getopt() function returns the option character if  the
 *         option  was found successfully, `:' if there was a missing
 *         parameter for one of  the  options,  `?'  for  an  unknown
 *         option character, or EOF for the end of the option list.
 *
 *         getopt_long()   and  getopt_long_only()  also  return  the
 *         option character when a short option is recognized.  For a
 *         long option, they return val if flag is NULL, and 0 other-
 *         wise.  Error and EOF returns are the same as for getopt(),
 *         plus  `?'  for an ambiguous match or an extraneous parame-
 *         ter.
 *
 *  ENVIRONMENT VARIABLES
 *         POSIXLY_CORRECT
 *                If this is set, then  option  processing  stops  as
 *                soon as a non-option argument is encountered.
 *
 *         _<PID>_GNU_nonoption_argv_flags_
 *                This  variable  was used by bash 2.0 to communicate
 *                to GNU libc which  arguments  are  the  results  of
 *                wildcard  expansion and so should not be considered
 *                as options.  This behaviour  was  removed  in  bash
 *                version  2.01, but the support remains in GNU libc.
 *
 *  EXAMPLE
 *         The following  example  program,  from  the  source  code,
 *         illustrates the use of getopt_long() with most of its fea-
 *         tures.
 *
 *         #include <stdio.h>
 *
 *         int
 *         main (argc, argv)
 *              int argc;
 *              char **argv;
 *         {
 *           int c;
 *           int digit_optind = 0;
 *
 *           while (1)
 *             {
 *               int this_option_optind = optind ? optind : 1;
 *               int option_index = 0;
 *               static struct option long_options[] =
 *               {
 *                 {"add", 1, 0, 0},
 *                 {"append", 0, 0, 0},
 *                 {"delete", 1, 0, 0},
 *                 {"verbose", 0, 0, 0},
 *                 {"create", 1, 0, 'c'},
 *                 {"file", 1, 0, 0},
 *                 {0, 0, 0, 0}
 *               }
 *
 *               c = getopt_long (argc, argv, "abc:d:012",
 *                          long_options, &option_index);
 *               if (c == -1)
 *              break;
 *
 *               switch (c)
 *                 {
 *                 case 0:
 *                   printf ("option %s", long_options[option_index].name);
 *                   if (optarg)
 *                     printf (" with arg %s", optarg);
 *                   printf ("\n");
 *                   break;
 *
 *                 case '0':
 *                 case '1':
 *                 case '2':
 *                   if (digit_optind != 0 && digit_optind != this_option_optind)
 *                     printf ("digits occur in two different argv-elements.\n");
 *                   digit_optind = this_option_optind;
 *                   printf ("option %c\n", c);
 *                   break;
 *
 *                 case 'a':
 *                   printf ("option a\n");
 *                   break;
 *
 *                 case 'b':
 *                   printf ("option b\n");
 *                   break;
 *
 *                 case 'c':
 *                   printf ("option c with value `%s'\n", optarg);
 *                   break;
 *
 *                 case 'd':
 *                   printf ("option d with value `%s'\n", optarg);
 *                   break;
 *
 *                 case '?':
 *                   break;
 *
 *                 default:
 *                   printf ("?? getopt returned character code 0%o ??\n", c);
 *                 }
 *             }
 *
 *           if (optind < argc)
 *             {
 *               printf ("non-option ARGV-elements: ");
 *               while (optind < argc)
 *               printf ("%s ", argv[optind++]);
 *               printf ("\n");
 *             }
 *
 *           exit (0);
 *         }
 *
 *  BUGS
 *         This manpage is confusing.
 *
 *         The POSIX.2 specification  of  getopt()  has  a  technical
 *         error  described  in  POSIX.2 Interpretation 150.  The GNU
 *         implementation (and probably  all  other  implementations)
 *         implements  the  correct behaviour rather than that speci-
 *         fied.
 *
 *  CONFORMING TO
 *         getopt():
 *                POSIX.2,   provided   the   environment    variable
 *                POSIXLY_CORRECT is set.  Otherwise, the elements of
 *                argv aren't really const, because we permute  them.
 *                We  pretend  they're  const  in the prototype to be
 *                compatible with other systems.
 *
 *  GNU                         8 May 1998                          1
 */

#ifndef _
/*
 * This is for other GNU distributions with internationalized messages.
 * When compiling libc, the _ macro is predefined.
 */
#ifdef HAVE_LIBINTL_H

#include <libintl.h>
#define _(msgid)  _(msgid)

#else

#define _(msgid)  (msgid)

#endif
#endif  // _

/*
 * This version of `getopt' appears to the caller like standard Unix `getopt'
 * but it behaves differently for the user, since it allows the user
 * to intersperse the options with the other arguments.
 *
 * As `getopt' works, it permutes the elements of ARGV so that,
 * when it is done, all the options precede everything else.  Thus
 * all application programs are extended to handle flexible argument order.
 *
 * Setting the environment variable POSIXLY_CORRECT disables permutation.
 * Then the behavior is completely standard.
 *
 * GNU application programs can use a third alternative mode in which
 * they can distinguish the relative order of options and other arguments.
 */

// #include "getopt.h"

/*
 * For communication from `getopt' to the caller.
 * When `getopt' finds an option that takes an argument,
 * the argument value is returned here.
 * Also, when `ordering' is RETURN_IN_ORDER,
 * each non-option ARGV-element is returned here.
 */

char *optarg = NULL;

/*
 * Index in ARGV of the next element to be scanned.
 * This is used for communication to and from the caller
 * and for communication between successive calls to `getopt'.
 *
 * On entry to `getopt', zero means this is the first call; initialize.
 *
 * When `getopt' returns -1, this is the index of the first of the
 * non-option elements that the caller should itself scan.
 *
 * Otherwise, `optind' communicates from one call to the next
 * how much of ARGV has been scanned so far.
 */

/* 1003.2 says this must be 1 before any call.  */
int optind = 1;

/*
 * Formerly, initialization of getopt depended on optind==0, which
 * causes problems with re-calling getopt as programs generally don't
 * know that.
 */

int __getopt_initialized = 0;

/*
 * The next char to be scanned in the option-element
 * in which the last option character we returned was found.
 * This allows us to pick up the scan where we left off.
 *
 * If this is zero, or a null string, it means resume the scan
 * by advancing to the next ARGV-element.
 */

static char *nextchar;

/*
 * Callers store zero here to inhibit the error message
 * for unrecognized options.
 */

int opterr = 1;

/*
 * Set to an option character which was unrecognized.
 * This must be initialized on some systems to avoid linking in the
 * system's own getopt implementation.
 */

int optopt = '?';

/*
 * Describe how to deal with options that follow non-option ARGV-elements.
 *
 * If the caller did not specify anything,
 * the default is REQUIRE_ORDER if the environment variable
 * POSIXLY_CORRECT is defined, PERMUTE otherwise.
 *
 * REQUIRE_ORDER means don't recognize them as options;
 * stop option processing when the first non-option is seen.
 * This is what Unix does.
 * This mode of operation is selected by either setting the environment
 * variable POSIXLY_CORRECT, or using `+' as the first character
 * of the list of option characters.
 *
 * PERMUTE is the default.  We permute the contents of ARGV as we scan,
 * so that eventually all the non-options are at the end.  This allows options
 * to be given in any order, even with programs that were not written to
 * expect this.
 *
 * RETURN_IN_ORDER is an option available to programs that were written
 * to expect options and other ARGV-elements in any order and that care about
 * the ordering of the two.  We describe each non-option ARGV-element
 * as if it were the argument of an option with character code 1.
 * Using `-' as the first character of the list of option characters
 * selects this mode of operation.
 *
 * The special argument `--' forces an end of option-scanning regardless
 * of the value of `ordering'.  In the case of RETURN_IN_ORDER, only
 * `--' can cause `getopt' to return -1 with `optind' != ARGC.
 */

static enum
{
    REQUIRE_ORDER, PERMUTE, RETURN_IN_ORDER
} ordering;

/* Value of POSIXLY_CORRECT environment variable.  */
static char *posixly_correct;


static char *my_index(const char *str, int chr)
{
    while (*str)
    {
        if (*str == chr)
            return (char *)str;

        str++;
    }
    return 0;
}

/* Handle permutation of arguments.  */

/*
 * Describe the part of ARGV that contains non-options that have
 * been skipped.  `first_nonopt' is the index in ARGV of the first of them;
 * `last_nonopt' is the index after the last of them.
 */

static int first_nonopt;
static int last_nonopt;

#if defined (BASH2_SUPPORT)
/*
 * Bash 2.0 gives us an environment variable containing flags
 * indicating ARGV elements that should not be considered arguments.
 */

char *__getopt_nonoption_flags;

static int nonoption_flags_max_len;
static int nonoption_flags_len;

static int original_argc;
static char *const *original_argv;

/*
 * Make sure the environment variable bash 2.0 puts in the environment
 * is valid for the getopt call we must make sure that the ARGV passed
 * to getopt is that one passed to the process.
 */

static void store_args_and_env(int argc, char *const *argv)
{
    /*
     * XXX This is no good solution.  We should rather copy the args so
     * that we can compare them later.  But we must not use malloc(3).
     */
    original_argc = argc;
    original_argv = argv;
}

#define SWAP_FLAGS(ch1, ch2)                                            \
    if (nonoption_flags_len > 0)                                        \
    {                                                                   \
        char __tmp = __getopt_nonoption_flags[ch1];                     \
        __getopt_nonoption_flags[ch1] = __getopt_nonoption_flags[ch2];  \
        __getopt_nonoption_flags[ch2] = __tmp;                          \
    }

#else   /* !BASH2_SUPPORT */

#define SWAP_FLAGS(ch1, ch2)        /* nothing */

#endif  /* BASH2_SUPPORT */

/*
 * Exchange two adjacent subsequences of ARGV.
 * One subsequence is elements [first_nonopt,last_nonopt)
 * which contains all the non-options that have been skipped so far.
 * The other is elements [last_nonopt,optind), which contains all
 * the options processed since those non-options were skipped.
 *
 * `first_nonopt' and `last_nonopt' are relocated so that they describe
 * the new indices of the non-options in ARGV after they are moved.
 */

static void exchange(char **argv)
{
    int bottom = first_nonopt;
    int middle = last_nonopt;
    int top = optind;
    char *tem;

    /*
     * Exchange the shorter segment with the far end of the longer segment.
     * That puts the shorter segment into the right place.
     * It leaves the longer segment in the right place overall,
     * but it consists of two parts that need to be swapped next.
     */

#if defined (BASH2_SUPPORT)
    /*
     * First make sure the handling of the `__getopt_nonoption_flags'
     * string can work normally.  Our top argument must be in the range
     * of the string.
     */
    if (nonoption_flags_len > 0 && top >= nonoption_flags_max_len)
    {
        /* We must extend the array.  The user plays games with us and
         * presents new arguments.  */
        char *new_str = calloc((top + 1), sizeof(new_str[0]));

        if (new_str == NULL)
        {
            nonoption_flags_len = nonoption_flags_max_len = 0;
        }
        else
        {
            memset(__mempcpy(new_str, __getopt_nonoption_flags, nonoption_flags_max_len),
                   0, top + 1 - nonoption_flags_max_len);
            nonoption_flags_max_len = top + 1;
            __getopt_nonoption_flags = new_str;
        }
    }
#endif

    while (top > middle && middle > bottom)
    {
        if (top - middle > middle - bottom)
        {
            /* Bottom segment is the short one.  */
            int len = middle - bottom;
            int i;

            /* Swap it with the top part of the top segment.  */
            for (i = 0; i < len; i++)
            {
                tem = argv[bottom + i];
                argv[bottom + i] = argv[top - (middle - bottom) + i];
                argv[top - (middle - bottom) + i] = tem;
                SWAP_FLAGS(bottom + i, top - (middle - bottom) + i);
            }
            /* Exclude the moved bottom segment from further swapping.  */
            top -= len;
        }
        else
        {
            /* Top segment is the short one.  */
            int len = top - middle;
            int i;

            /* Swap it with the bottom part of the bottom segment.  */
            for (i = 0; i < len; i++)
            {
                tem = argv[bottom + i];
                argv[bottom + i] = argv[middle + i];
                argv[middle + i] = tem;
                SWAP_FLAGS(bottom + i, middle + i);
            }
            /* Exclude the moved top segment from further swapping.  */
            bottom += len;
        }
    }

    /* Update records for the slots the non-options now occupy.  */

    first_nonopt += (optind - last_nonopt);
    last_nonopt = optind;
}

/* Initialize the internal data when the first call is made.  */

static const char *_getopt_initialize(int argc, char *const *argv, const char *optstring)
{
    /*
     * Start processing options with ARGV-element 1 (since ARGV-element 0
     * is the program name); the sequence of previously skipped
     * non-option ARGV-elements is empty.
     */

    first_nonopt = last_nonopt = optind;

    nextchar = NULL;

    posixly_correct = getenv("POSIXLY_CORRECT");

    /* Determine how to handle the ordering of options and nonoptions.  */

    if (optstring[0] == '-')
    {
        ordering = RETURN_IN_ORDER;
        ++optstring;
    }
    else if (optstring[0] == '+')
    {
        ordering = REQUIRE_ORDER;
        ++optstring;
    }
    else if (posixly_correct != NULL)
    {
        ordering = REQUIRE_ORDER;
    }
    else
    {
        ordering = PERMUTE;
    }

#if defined (BASH2_SUPPORT)
    if (posixly_correct == NULL
        && argc == original_argc && argv == original_argv)
    {
        if (nonoption_flags_max_len == 0)
        {
            if (__getopt_nonoption_flags == NULL
                || __getopt_nonoption_flags[0] == 0)
            {
                nonoption_flags_max_len = -1;
            }
            else
            {
                const char *orig_str = __getopt_nonoption_flags;
                int len = nonoption_flags_max_len = strlen(orig_str);
                if (nonoption_flags_max_len < argc)
                    nonoption_flags_max_len = argc;
                __getopt_nonoption_flags = (char *)calloc(nonoption_flags_max_len, sizeof(__getopt_nonoption_flags[0]));
                if (__getopt_nonoption_flags == NULL)
                    nonoption_flags_max_len = -1;
                else
                    memset(__mempcpy(__getopt_nonoption_flags, orig_str, len),
                           0, nonoption_flags_max_len - len);
            }
        }
        nonoption_flags_len = nonoption_flags_max_len;
    }
    else
    {
        nonoption_flags_len = 0;
    }
#endif

    return optstring;
}


/*
 * Scan elements of ARGV (whose length is ARGC) for option characters
 * given in OPTSTRING.
 *
 * If an element of ARGV starts with '-', and is not exactly "-" or "--",
 * then it is an option element.  The characters of this element
 * (aside from the initial '-') are option characters.  If `getopt'
 * is called repeatedly, it returns successively each of the option characters
 * from each of the option elements.
 *
 * If `getopt' finds another option character, it returns that character,
 * updating `optind' and `nextchar' so that the next call to `getopt' can
 * resume the scan with the following option character or ARGV-element.
 *
 * If there are no more option characters, `getopt' returns -1.
 * Then `optind' is the index in ARGV of the first ARGV-element
 * that is not an option.  (The ARGV-elements have been permuted
 * so that those that are not options now come last.)
 *
 * OPTSTRING is a string containing the legitimate option characters.
 * If an option character is seen that is not listed in OPTSTRING,
 * return '?' after printing an error message.  If you set `opterr' to
 * zero, the error message is suppressed but we still return '?'.
 *
 * If a char in OPTSTRING is followed by a colon, that means it wants an arg,
 * so the following text in the same ARGV-element, or the text of the following
 * ARGV-element, is returned in `optarg'.  Two colons mean an option that
 * wants an optional arg; if there is text in the current ARGV-element,
 * it is returned in `optarg', otherwise `optarg' is set to zero.
 *
 * If OPTSTRING starts with `-' or `+', it requests different methods of
 * handling the non-option ARGV-elements.
 * See the comments about RETURN_IN_ORDER and REQUIRE_ORDER, above.
 *
 * Long-named options begin with `--' instead of `-'.
 * Their names may be abbreviated as long as the abbreviation is unique
 * or is an exact match for some defined option.  If they have an
 * argument, it follows the option name in the same ARGV-element, separated
 * from the option name by a `=', or else the in next ARGV-element.
 * When `getopt' finds a long-named option, it returns 0 if that option's
 * `flag' field is nonzero, the value of the option's `val' field
 * if the `flag' field is zero.
 *
 * The elements of ARGV aren't really const, because we permute them.
 * But we pretend they're const in the prototype to be compatible
 * with other systems.
 *
 * LONGOPTS is a vector of `struct option' terminated by an
 * element containing a name which is zero.
 *
 * LONGIND returns the index in LONGOPT of the long-named option found.
 * It is only valid when a long-named option has been found by the most
 * recent call.
 *
 * If LONG_ONLY is nonzero, '-' as well as '--' can introduce
 * long-named options.
 */

int _getopt_internal(int argc, char *const *argv, const char *optstring, const struct option *longopts, int *longind, int long_only,
                    getopt_print_error_f *print_error, void *propagate)
{
    optarg = NULL;

    if (optind == 0 || !__getopt_initialized)
    {
        if (optind == 0)
            optind = 1;/* Don't scan ARGV[0], the program name.  */
        optstring = _getopt_initialize(argc, argv, optstring);
        __getopt_initialized = 1;
    }

    /*
     * Test whether ARGV[optind] points to a non-option argument.
     * Either it does not have option syntax, or there is an environment flag
     * from the shell indicating it is not an option.  The later information
     * is only used when the used in the GNU libc.
     */
#if defined (BASH2_SUPPORT)

#define NONOPTION_P                                             \
    (argv[optind][0] != '-' || argv[optind][1] == 0             \
     || (optind < nonoption_flags_len                           \
         && __getopt_nonoption_flags[optind] == '1'))

#else

#define NONOPTION_P                                             \
    (argv[optind][0] != '-' || argv[optind][1] == 0)

#endif

    if (nextchar == NULL || *nextchar == 0)
    {
        /* Advance to the next ARGV-element.  */

        /*
         * Give FIRST_NONOPT & LAST_NONOPT rational values if OPTIND has been
         * moved back by the user (who may also have changed the arguments).
         */
        if (last_nonopt > optind)
            last_nonopt = optind;
        if (first_nonopt > optind)
            first_nonopt = optind;

        if (ordering == PERMUTE)
        {
            /*
             * If we have just processed some options following some non-options,
             * exchange them so that the options come first.
             */

            if (first_nonopt != last_nonopt && last_nonopt != optind)
                exchange((char **)argv);
            else if (last_nonopt != optind)
                first_nonopt = optind;

            /* Skip any additional non-options
             * and extend the range of non-options previously skipped.  */

            while (optind < argc && NONOPTION_P)
                optind++;
            last_nonopt = optind;
        }

        /* The special ARGV-element `--' means premature end of options.
         * Skip it like a null option,
         * then exchange with previous non-options as if it were an option,
         * then skip everything else like a non-option.  */

        if (optind != argc && !strcmp(argv[optind], "--"))
        {
            optind++;

            if (first_nonopt != last_nonopt && last_nonopt != optind)
                exchange((char **)argv);
            else if (first_nonopt == last_nonopt)
                first_nonopt = optind;
            last_nonopt = argc;

            optind = argc;
        }

        /* If we have done all the ARGV-elements, stop the scan
         * and back over any non-options that we skipped and permuted.  */

        if (optind == argc)
        {
            /* Set the next-arg-index to point at the non-options
             * that we previously skipped, so the caller will digest them.  */
            if (first_nonopt != last_nonopt)
                optind = first_nonopt;
            return -1;
        }

        /* If we have come to a non-option and did not permute it,
         * either stop the scan or describe it to the caller and pass it by.  */

        if (NONOPTION_P)
        {
            if (ordering == REQUIRE_ORDER)
                return -1;

            optarg = argv[optind++];
            return 1;
        }

        /* We have found another option-ARGV-element.
         * Skip the initial punctuation.  */

        nextchar = (argv[optind] + 1
                    + (longopts != NULL && argv[optind][1] == '-'));
    }

    /* Decode the current option-ARGV-element.  */

    /*
     * Check whether the ARGV-element is a long option.
     *
     * If long_only and the ARGV-element has the form "-f", where f is
     * a valid short option, don't consider it an abbreviated form of
     * a long option that starts with f.  Otherwise there would be no
     * way to give the -f short option.
     *
     * On the other hand, if there's a long option "fubar" and
     * the ARGV-element is "-fu", do consider that an abbreviation of
     * the long option, just like "--fu", and not "-f" with arg "u".
     *
     * This distinction seems to be the most useful approach.
     */

    if (longopts != NULL
        && (argv[optind][1] == '-'
            || (long_only && (argv[optind][2] || !my_index(optstring, argv[optind][1])))))
    {
        char *nameend;
        const struct option *p;
        const struct option *pfound = NULL;
        int exact = 0;
        int ambig = 0;
        int indfound = -1;
        int option_index;

        for (nameend = nextchar; *nameend && *nameend != '='; nameend++)
            /* Do nothing.  */;

        /* Test all long options for either exact match
         * or abbreviated matches.  */
        for (p = longopts, option_index = 0; p->name; p++, option_index++)
            if (!strncmp(p->name, nextchar, nameend - nextchar))
            {
                if ((unsigned int)(nameend - nextchar) == (unsigned int)strlen(p->name))
                {
                    /* Exact match found.  */
                    pfound = p;
                    indfound = option_index;
                    exact = 1;
                    break;
                }
                else if (pfound == NULL)
                {
                    /* First nonexact match found.  */
                    pfound = p;
                    indfound = option_index;
                }
                else
                {
                    /* Second or later nonexact match found.  */
                    ambig = 1;
                }
            }

        if (ambig && !exact)
        {
            if (opterr)
            {
                print_error(propagate, _("%s: option `%s' is ambiguous\n"),
                            argv[0], argv[optind]);
            }
            nextchar += strlen(nextchar);
            optind++;
            optopt = 0;
            return '?';
        }

        if (pfound != NULL)
        {
            option_index = indfound;
            optind++;
            if (*nameend)
            {
                /* Don't test has_arg with >, because some C compilers don't
                 * allow it to be used on enums.  */
                if (pfound->has_arg != no_argument)
                {
                    optarg = nameend + 1;
                }
                else
                {
                    if (opterr)
                    {
                        if (argv[optind - 1][1] == '-')
                            /* --option */
                            print_error(propagate,
                                        _("%s: option `--%s' doesn't allow an argument\n"),
                                        argv[0], pfound->name);
                        else
                            /* +option or -option */
                            print_error(propagate,
                                        _("%s: option `%c%s' doesn't allow an argument\n"),
                                        argv[0], argv[optind - 1][0], pfound->name);
                    }

                    nextchar += strlen(nextchar);

                    optopt = pfound->val;
                    return '?';
                }
            }
            else if (pfound->has_arg == required_argument)
            {
                if (optind < argc)
                {
                    optarg = argv[optind++];
                }
                else
                {
                    if (opterr)
                        print_error(propagate,
                                    _("%s: option `%s' requires an argument\n"),
                                    argv[0], argv[optind - 1]);
                    nextchar += strlen(nextchar);
                    optopt = pfound->val;
                    return optstring[0] == ':' ? ':' : '?';
                }
            }
            nextchar += strlen(nextchar);
            if (longind != NULL)
                *longind = option_index;
            if (pfound->flag)
            {
                *(pfound->flag) = pfound->val;
                return 0;
            }
            return pfound->val;
        }

        /*
         * Can't find it as a long option.  If this is not getopt_long_only,
         * or the option starts with '--' or is not a valid short
         * option, then it's an error.
         * Otherwise interpret it as a short option.
         */
        if (!long_only || argv[optind][1] == '-'
            || my_index(optstring, *nextchar) == NULL)
        {
            if (opterr)
            {
                if (argv[optind][1] == '-')
                {
                    /* --option */
                    print_error(propagate, _("%s: unrecognized option `--%s'\n"),
                                argv[0], nextchar);
                }
                else
                {
                    /* +option or -option */
                    print_error(propagate, _("%s: unrecognized option `%c%s'\n"),
                                argv[0], argv[optind][0], nextchar);
                }
            }
            nextchar = (char *)"";
            optind++;
            optopt = 0;
            return '?';
        }
    }

    /* Look at and handle the next short option-character.  */

    {
        char c = *nextchar++;
        char *temp = my_index(optstring, c);

        /* Increment `optind' when we start to process its last character.  */
        if (*nextchar == 0)
            ++optind;

        if (temp == NULL || c == ':')
        {
            if (opterr)
            {
                if (posixly_correct)
                    /* 1003.2 specifies the format of this message.  */
                    print_error(propagate, _("%s: illegal option -- %c\n"),
                                argv[0], c);
                else
                    print_error(propagate, _("%s: invalid option -- %c\n"),
                                argv[0], c);
            }
            optopt = c;
            return '?';
        }
        /* Convenience. Treat POSIX -W foo same as long option --foo */
        if (temp[0] == 'W' && temp[1] == ';')
        {
            char *nameend;
            const struct option *p;
            const struct option *pfound = NULL;
            int exact = 0;
            int ambig = 0;
            int indfound = 0;
            int option_index;

            /* This is an option that requires an argument.  */
            if (*nextchar != 0)
            {
                optarg = nextchar;
                /* If we end this ARGV-element by taking the rest as an arg,
                 * we must advance to the next element now.  */
                optind++;
            }
            else if (optind == argc)
            {
                if (opterr)
                {
                    /* 1003.2 specifies the format of this message.  */
                    print_error(propagate, _("%s: option requires an argument -- %c\n"),
                                argv[0], c);
                }
                optopt = c;
                if (optstring[0] == ':')
                    c = ':';
                else
                    c = '?';
                return c;
            }
            else
            {
                /* We already incremented `optind' once;
                 * increment it again when taking next ARGV-elt as argument.  */
                optarg = argv[optind++];
            }

            /* optarg is now the argument, see if it's in the
             * table of longopts.  */

            for (nextchar = nameend = optarg; *nameend && *nameend != '='; nameend++)
                /* Do nothing.  */;

            /* Test all long options for either exact match
             * or abbreviated matches.  */
            for (p = longopts, option_index = 0; p->name; p++, option_index++)
            {
                if (!strncmp(p->name, nextchar, nameend - nextchar))
                {
                    if ((unsigned int)(nameend - nextchar) == (unsigned int)strlen(p->name))
                    {
                        /* Exact match found.  */
                        pfound = p;
                        indfound = option_index;
                        exact = 1;
                        break;
                    }
                    else if (pfound == NULL)
                    {
                        /* First nonexact match found.  */
                        pfound = p;
                        indfound = option_index;
                    }
                    else
                    {
                        /* Second or later nonexact match found.  */
                        ambig = 1;
                    }
                }
            }
            if (ambig && !exact)
            {
                if (opterr)
                {
                    print_error(propagate, _("%s: option `-W %s' is ambiguous\n"),
                                argv[0], argv[optind]);
                }
                nextchar += strlen(nextchar);
                optind++;
                return '?';
            }
            if (pfound != NULL)
            {
                option_index = indfound;
                if (*nameend)
                {
                    /* Don't test has_arg with >, because some C compilers don't
                     * allow it to be used on enums.  */
                    if (pfound->has_arg != no_argument)
                    {
                        optarg = nameend + 1;
                    }
                    else
                    {
                        if (opterr)
                        {
                            print_error(propagate, _("%s: option `-W %s' doesn't allow an argument\n"),
                                        argv[0], pfound->name);
                        }

                        nextchar += strlen(nextchar);
                        return '?';
                    }
                }
                else if (pfound->has_arg == required_argument)
                {
                    if (optind < argc)
                    {
                        optarg = argv[optind++];
                    }
                    else
                    {
                        if (opterr)
                        {
                            print_error(propagate,
                                        _("%s: option `%s' requires an argument\n"),
                                        argv[0], argv[optind - 1]);
                        }
                        nextchar += strlen(nextchar);
                        return optstring[0] == ':' ? ':' : '?';
                    }
                }
                nextchar += strlen(nextchar);
                if (longind != NULL)
                    *longind = option_index;
                if (pfound->flag)
                {
                    *(pfound->flag) = pfound->val;
                    return 0;
                }
                return pfound->val;
            }
            nextchar = NULL;
            return 'W'; /* Let the application handle it.   */
        }
        if (temp[1] == ':')
        {
            if (temp[2] == ':')
            {
                /* This is an option that accepts an argument optionally.  */
                if (*nextchar != 0)
                {
                    optarg = nextchar;
                    optind++;
                }
                else
                {
                    optarg = NULL;
                }
                nextchar = NULL;
            }
            else
            {
                /* This is an option that requires an argument.  */
                if (*nextchar != 0)
                {
                    optarg = nextchar;
                    /* If we end this ARGV-element by taking the rest as an arg,
                     * we must advance to the next element now.  */
                    optind++;
                }
                else if (optind == argc)
                {
                    if (opterr)
                    {
                        /* 1003.2 specifies the format of this message.  */
                        print_error(propagate,
                                    _("%s: option requires an argument -- %c\n"),
                                    argv[0], c);
                    }
                    optopt = c;
                    if (optstring[0] == ':')
                        c = ':';
                    else
                        c = '?';
                }
                else
                {
                    /* We already incremented `optind' once;
                     * increment it again when taking next ARGV-elt as argument.  */
                    optarg = argv[optind++];
                }
                nextchar = NULL;
            }
        }
        return c;
    }
}

int getopt_ex(int argc, char *const *argv, const char *optstring, getopt_print_error_f *print_error, void *propagate)
{
    return _getopt_internal(argc, argv, optstring,
                            (const struct option *)0,
                            (int *)0,
                            0,
                            print_error, propagate);
}



#ifdef GETOPT_TEST1

/* Compile with -DTEST to make an executable for use in testing
 * the above definition of `getopt'.  */

int main(int argc, char **argv)
{
    int c;
    int digit_optind = 0;

    while (1)
    {
        int this_option_optind = optind ? optind : 1;

        c = getopt_ex(argc, argv, "abc:d:0123456789", fprintf, stdout);
        if (c == -1)
            break;

        switch (c)
        {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if (digit_optind != 0 && digit_optind != this_option_optind)
                printf("digits occur in two different argv-elements.\n");
            digit_optind = this_option_optind;
            printf("option %c\n", c);
            break;

        case 'a':
            printf("option a\n");
            break;

        case 'b':
            printf("option b\n");
            break;

        case 'c':
            printf("option c with value `%s'\n", optarg);
            break;

        case '?':
            break;

        default:
            printf("?? getopt returned character code 0%o ??\n", c);
            break;
        }
    }

    if (optind < argc)
    {
        printf("non-option ARGV-elements: ");
        while (optind < argc)
            printf("%s ", argv[optind++]);
        printf("\n");
    }

    return 0;
}

#endif /* GETOPT_TEST1 */


/* getopt_long and getopt_long_only entry points for GNU getopt.
 * Copyright (C) 1987,88,89,90,91,92,93,94,96,97,98
 *   Free Software Foundation, Inc.
 * This file is part of the GNU C Library.
 *
 * The GNU C Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The GNU C Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the GNU C Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.  */

int getopt_long_ex(int argc, char *const *argv, const char *options, const struct option *long_options, int *opt_index,
               getopt_print_error_f *print_error, void *propagate)
{
    return _getopt_internal(argc, argv, options, long_options, opt_index, 0, print_error, propagate);
}

/* Like getopt_long, but '-' as well as '--' can indicate a long option.
 * If an option that starts with '-' (not '--') doesn't match a long option,
 * but does match a short option, it is parsed as a short option
 * instead.  */
int getopt_long_only_ex(int argc, char *const *argv, const char *options, const struct option *long_options, int *opt_index,
                    getopt_print_error_f *print_error, void *propagate)
{
    return _getopt_internal(argc, argv, options, long_options, opt_index, 1, print_error, propagate);
}

#ifdef GETOPT_TEST2

int main(int argc, char **argv)
{
    int c;
    int digit_optind = 0;

    while (1)
    {
        int this_option_optind = optind ? optind : 1;
        int option_index = 0;
        static struct option long_options[] =
        {
            { "add", 1, 0, 0 },
            { "append", 0, 0, 0 },
            { "delete", 1, 0, 0 },
            { "verbose", 0, 0, 0 },
            { "create", 0, 0, 0 },
            { "file", 1, 0, 0 },
            { 0, 0, 0, 0 }
        };

        c = getopt_long_ex(argc, argv, "abc:d:0123456789",
                           long_options, &option_index, fprintf, stdout);
        if (c == -1)
            break;

        switch (c)
        {
        case 0:
            printf("option %s", long_options[option_index].name);
            if (optarg)
                printf(" with arg %s", optarg);
            printf("\n");
            break;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if (digit_optind != 0 && digit_optind != this_option_optind)
                printf("digits occur in two different argv-elements.\n");
            digit_optind = this_option_optind;
            printf("option %c\n", c);
            break;

        case 'a':
            printf("option a\n");
            break;

        case 'b':
            printf("option b\n");
            break;

        case 'c':
            printf("option c with value `%s'\n", optarg);
            break;

        case 'd':
            printf("option d with value `%s'\n", optarg);
            break;

        case '?':
            break;

        default:
            printf("?? getopt returned character code 0%o ??\n", c);
            break;
        }
    }

    if (optind < argc)
    {
        printf("non-option ARGV-elements: ");
        while (optind < argc)
            printf("%s ", argv[optind++]);
        printf("\n");
    }

    return 0;
}

#endif /* GETOPT_TEST2 */

#endif

