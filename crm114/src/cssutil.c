//  cssutil.c - utility for munging css files, version X0.1
//  Copyright 2001-2007  William S. Yerazunis, all rights reserved.
//
//  This software is licensed to the public under the Free Software
//  Foundation's GNU GPL, version 2.0.  You may obtain a copy of the
//  GPL by visiting the Free Software Foundations web site at
//  www.fsf.org .  Other licenses may be negotiated; contact the
//  author for details.
//

//  include some standard files

#include "crm114_sysincludes.h"

//  include any local crm114 configuration file
#include "crm114_config.h"

//  include the crm114 data structures file
#include "crm114_structs.h"

//  and include the routine declarations file
#include "crm114.h"






//
//    Global variables



/* [i_a] no variable instantiation in a common header file */
int vht_size = 0;

int cstk_limit = 0;

int max_pgmlines = 0;

int max_pgmsize = 0;

int user_trace = 0;

int internal_trace = 0;

int debug_countdown = 0;

int cmdline_break = 0;

int cycle_counter = 0;

int ignore_environment_vars = 0;

int data_window_size = 0;

int sparse_spectrum_file_length = 0;

int microgroom_chain_length = 0;

int microgroom_stop_after = 0;

double min_pmax_pmin_ratio = 0.0;

int profile_execution = 0;

int prettyprint_listing = 0;  //  0= none, 1 = basic, 2 = expanded, 3 = parsecode

int engine_exit_base = 0;  //  All internal errors will use this number or higher;
//  the user programs can use lower numbers freely.


//        how should math be handled?
//        = 0 no extended (non-EVAL) math, use algebraic notation
//        = 1 no extended (non-EVAL) math, use RPN
//        = 2 extended (everywhere) math, use algebraic notation
//        = 3 extended (everywhere) math, use RPN
int q_expansion_mode = 0;

int selected_hashfunction = 0;  //  0 = default

int act_like_Bill = 0;





//   The VHT (Variable Hash Table)
VHT_CELL **vht = NULL;

//   The pointer to the global Current Stack Level (CSL) frame
CSL_CELL *csl = NULL;

//    the data window
CSL_CELL *cdw = NULL;

//    the temporarys data window (where argv, environ, newline etc. live)
CSL_CELL *tdw = NULL;

//    the pointer to a CSL that we use during matching.  This is flipped
//    to point to the right data window during matching.  It doesn't have
//    it's own data, unlike cdw and tdw.
CSL_CELL *mdw = NULL;

////    a pointer to the current statement argparse block.  This gets whacked
////    on every new statement.
//ARGPARSE_BLOCK *apb = NULL;




//    the app path/name
char *prog_argv0 = NULL;

//    the auxilliary input buffer (for WINDOW input)
char *newinputbuf = NULL;

//    the globals used when we need a big buffer  - allocated once, used
//    wherever needed.  These are sized to the same size as the data window.
char *inbuf = NULL;
char *outbuf = NULL;
char *tempbuf = NULL;


#if !defined(CRM_WITHOUT_BMP_ASSISTED_ANALYSIS)
CRM_ANALYSIS_PROFILE_CONFIG analysis_cfg = { 0 };
#endif /* CRM_WITHOUT_BMP_ASSISTED_ANALYSIS */







static char version[] = "1.2";


void helptext(void)
{
    /* GCC warning: warning: string length <xxx> is greater than the length <509> ISO C89 compilers are required to support */
    fprintf(stdout,
            "cssutil version %s - generic css file utility.\n"
            "Usage: cssutil [options]... css-file\n"
            "\n",
            VERSION);
    fprintf(stdout,
            "            -b   - brief; print only summary\n"
            "            -h   - print this help\n"
            "            -q   - quite mode; no warning messages\n"
            "            -r   - report then exit (no menu)\n"
            "            -s css-size  - if no css file found, create new\n"
            "                           one with this many buckets.\n"
            "            -S css-size  - same as -s, but round up to next\n"
            "                           2^n + 1 boundary.\n"
            "            -v   - print version and exit\n"
            "            -D   - dump css file to stdout in CSV format.\n"
            "            -R csv-file  - create and restore css from CSV\n"
           );
}

int main(int argc, char **argv)
{
    int i, k;                  //  some random counters, when we need a loop
    int v;
    int sparse_spectrum_file_length = DEFAULT_SPARSE_SPECTRUM_FILE_LENGTH;
    int user_set_css_length = 0;
    int hfsize;
    int64_t sum;              // sum of the hits... can be _big_.
    // int hfd;
    int brief = 0, quiet = 0, dump = 0, restore = 0;
    int opt, fields;
    int report_only = 0;

    int *bcounts;
    int maxchain;
    int curchain;
    int totchain;
    int fbuckets;
    int nchains;
    int zvbins;
    int ofbins;

    int histbins; // how many bins for the histogram

    char cmdstr[255];
    char cssfile[255];
    char csvfile[255];
    unsigned char cmdchr[2];
    char crapchr[2];
    double cmdval;
    int val;
    int zloop, cmdloop;
    int learns_index, features_index;
    int docs_learned = -1;
    int features_learned = -1;

    init_stdin_out_err_as_os_handles();

    //   copy app path/name into global static...
    prog_argv0 = argv[0];

    user_trace = DEFAULT_USER_TRACE_LEVEL;
    internal_trace = DEFAULT_INTERNAL_TRACE_LEVEL;

    histbins = FEATUREBUCKET_VALUE_MAX;
    if (histbins > FEATUREBUCKET_HISTOGRAM_MAX)
        histbins = FEATUREBUCKET_HISTOGRAM_MAX;
    bcounts = calloc((histbins + 2), sizeof(bcounts[0]));

    {
        struct stat statbuf;        //  filestat buffer
        FEATUREBUCKET_TYPE *hashes; //  the text of the hash file

        // parse cmdline options
        while ((opt = getopt(argc, argv, "bDhR:rqs:S:vtT")) != -1)
        {
            switch (opt)
            {
            case 'b':
                brief = 1;      // brief, no 'bin value ...' lines
                break;

            case 'D':
                dump = 1;       // dump css file, no cmd menu
                break;

            case 'q':
                quiet = 1;      // quiet mode, no warning messages
                break;

            case 'R':
                {
                    FILE *f;
                    unsigned int key, hash, value;

                    // count lines to determine number of buckets and check CSV format
                    if ((f = fopen(optarg, "rb")) != NULL)
                    {
                        sparse_spectrum_file_length = 0;
                        while (!feof(f))
                            if (fscanf(f, "%u;%u;%u\n", &key, &hash, &value) == 3)
                                sparse_spectrum_file_length++;
                            else
                            {
                                fprintf(stderr,
                                        "\n %s is not in the right CSV format.\n",
                                        optarg);
                                exit(EXIT_FAILURE);
                            }
                        fclose(f);
                        strcpy(csvfile, optarg);
                    }
                    else
                    {
                        fprintf(stderr,
                                "\n Couldn't open csv file %s; errno=%d.\n",
                                optarg, errno);
                        exit(EXIT_FAILURE);
                    }
                }
                restore = 1;    // restore css file, no cmd menu
                break;

            case 'r':
                report_only = 1; // print stats only, no cmd menu.
                break;

            case 's':        // set css size to option value
            case 'S':        // same as above but round up to next 2^n+1
                if (sscanf(optarg, "%d", &sparse_spectrum_file_length))
                {
                    if (!quiet)
                        fprintf(stderr,
                                "\nOverride css creation length to %d\n",
                                sparse_spectrum_file_length);
                    user_set_css_length = 1;
                }
                else
                {
                    fprintf(stderr,
                            "On -%c flag: Missing or incomprehensible number of buckets.\n",
                            opt);
                    exit(EXIT_FAILURE);
                }
                if (opt == 'S') // round up to next 2^n+1
                {
                    int k;

                    k = (int)floor(log2(sparse_spectrum_file_length - 1));
                    while ((2 << k) + 1 < sparse_spectrum_file_length)
                        k++;
                    sparse_spectrum_file_length = (2 << k) + 1;
                    user_set_css_length = 1;
                }
                break;

            case 't':
                if (user_trace == 0)
                {
                    user_trace = 1;
                    fprintf(stderr, "User tracing on");
                }
                else
                {
                    user_trace = 0;
                    fprintf(stderr, "User tracing off");
                }
                break;

            case 'T':
                if (internal_trace == 0)
                {
                    internal_trace = 1;
                    fprintf(stderr, "Internal tracing on");
                }
                else
                {
                    internal_trace = 0;
                    fprintf(stderr, "Internal tracing off");
                }
                break;

            case 'v':
                fprintf(stderr, " This is cssutil, version %s\n", version);
                fprintf(stderr, " Copyright 2001-2007 W.S.Yerazunis.\n");
                fprintf(stderr,
                        " This software is licensed under the GPL with ABSOLUTELY NO WARRANTY\n");
                exit(EXIT_SUCCESS);

            default:
                helptext();
                exit(EXIT_SUCCESS);
                break;
            }
        }

        if (optind < argc)
        {
            strncpy(cssfile, argv[optind], WIDTHOF(cssfile));
            cssfile[WIDTHOF(cssfile) - 1] = 0;
            /* [i_a] strncpy will NOT add a NUL sentinel when the boundary was reached! */
        }
        else
        {
            helptext();
            exit(EXIT_SUCCESS);
        }

        //       and stat it to get it's length
        k = stat(cssfile, &statbuf);
        //       quick check- does the file even exist?
        if (k == 0)
        {
            if (restore)
            {
                fprintf(stderr,
                        "\n.CSS file %s exists! Restore operation aborted.\n",
                        cssfile);
                exit(EXIT_FAILURE);
            }
            if (!quiet && user_set_css_length)
            {
                fprintf(stderr,
                        "\n.CSS file %s exists; -s, -S options ignored.\n",
                        cssfile);
            }
        }
        else
        {
            //      file didn't exist... create it
            if (!quiet && !restore)
                fprintf(stdout, "\nHad to create .CSS file %s\n", cssfile);
            if (crm_create_cssfile(cssfile, sparse_spectrum_file_length, 0, 0, 0) != EXIT_SUCCESS)
            {
                exit(EXIT_FAILURE);
            }
            k = stat(cssfile, &statbuf);
            CRM_ASSERT_EX(k == 0, "We just created/wrote to the file, stat shouldn't fail!");
        }
        hfsize = statbuf.st_size;
        //
        //   mmap the hash file into memory so we can bitwhack it
        hashes = crm_mmap_file(cssfile,
                               0,
                               hfsize,
                               PROT_READ | PROT_WRITE,
                               MAP_SHARED,
                               CRM_MADV_RANDOM,
                               &hfsize);
        if (hashes == MAP_FAILED)
        {
            fprintf(stderr,
                    "\n Couldn't open RW file %s; errno=%d(%s).\n", cssfile, errno, errno_descr(errno));
            exit(EXIT_FAILURE);
        }

        //   from now on, hfsize is buckets, not bytes.
        hfsize = hfsize / sizeof(FEATUREBUCKET_STRUCT);

#ifdef OSB_LEARNCOUNTS
        //       If LEARNCOUNTS is enabled, we normalize with documents-learned.
        //
        //       We use the reserved h2 == 0 setup for the learncount.
        //
        {
            const char *litf = "Learnings in this file";
            const char *fitf = "Features in this file";
            crmhash_t hcode, h1, h2;
            //
            hcode = strnhash(litf, strlen(litf));
            h1 = hcode % hfsize;
            h2 = 0;
            if (hashes[h1].hash != hcode)
            {
                // initialize the file?
                if (hashes[h1].hash == 0 && hashes[h1].key == 0)
                {
                    hashes[h1].hash = hcode;
                    hashes[h1].key = 0;
                    hashes[h1].value = 1;
                    learns_index = h1;
                }
                else
                {
                    //fatalerror (" This file should have learncounts, but doesn't!",
                    //  " The slot is busy, too.  It's hosed.  Time to die.");
                    //goto regcomp_failed;
                    fprintf(
                        stderr
                           ,
                        "\n Minor Caution - this file has the learncount slot in use.\n This is not a problem for Markovian classification, but it will have some\n issues with an OSB classfier.\n");
                }
            }
            //      fprintf(stderr, "This file has had %d documents learned!\n",
            //               hashes[h1].value);
            docs_learned = hashes[h1].value;
            hcode = strnhash(fitf, strlen(fitf));
            h1 = hcode % hfsize;
            h2 = 0;
            if (hashes[h1].hash != hcode)
            {
                // initialize the file?
                if (hashes[h1].hash == 0 && hashes[h1].key == 0)
                {
                    hashes[h1].hash = hcode;
                    hashes[h1].key = 0;
                    hashes[h1].value = 1;
                    features_index = h1;
                }
                else
                {
                    //fatalerror (" This file should have learncounts, but doesn't!",
                    //  " The slot is busy, too.  It's hosed.  Time to die.");
                    //goto regcomp_failed ;
                    fprintf(stderr,
                            "\n"
                            "Minor Caution - this file has the featurecount slot in use.\n"
                            "This is not a problem for Markovian classification, but it will have some\n"
                            "issues with an OSB classfier.\n");
                }
            }
            //fprintf(stderr, "This file has had %d features learned!\n",
            //               hashes[h1].value);
            features_learned = hashes[h1].value;
        }

#endif

        if (dump)
        {
            // dump the css file
            for (i = 0; i < hfsize; i++)
            {
                fprintf(stdout, "%lu;%lu;%lu\n",
                        (unsigned long int)hashes[i].key, (unsigned long int)hashes[i].hash, (unsigned long int)hashes[i].value);
            }
        }

        if (restore)
        {
            FILE *f;

            // restore the css file  - note that if we DIDN'T create
            //  it already, then this will fail.
            //
            if ((f = fopen(csvfile, "rb")) == NULL)
            {
                fprintf(stderr, "\n Couldn't open csv file %s; errno=%d(%s).\n",
                        csvfile, errno, errno_descr(errno));
                exit(EXIT_FAILURE);
            }
            else
            {
                for (i = 0; i < hfsize; i++)
                {
                    unsigned int k;
                    unsigned int h;
                    unsigned int v;
                    if (3 != fscanf(f, "%u;%u;%u\n", &k, &h, &v))
                    {
                        fprintf(stderr, "\n Couldn't parse csv file %s at line %d\n",
                                csvfile, i);
                        exit(EXIT_FAILURE);
                    }
                    hashes[i].key = k;
                    hashes[i].hash = h;
                    hashes[i].value = v;
                }
                fclose(f);
            }
        }

        zloop = 1;
        while (zloop == 1 && !restore && !dump)
        {
            zloop = 0;
            //crm_packcss (hashes, hfsize, 1, hfsize-1);
            sum = 0;
            maxchain = 0;
            curchain = 0;
            totchain = 0;
            fbuckets = 0;
            nchains = 0;
            zvbins = 0;
            ofbins = 0;
            //   calculate maximum overflow chain length
            for (i = 1; i < hfsize; i++)
            {
                if (hashes[i].key != 0)
                {
                    //  only count the non-special buckets for feature count
                    sum = sum + hashes[i].value;
                    //
                    fbuckets++;
                    curchain++;
                    if (hashes[i].value == 0)
                        zvbins++;
                    if (hashes[i].value >= FEATUREBUCKET_VALUE_MAX)
                        ofbins++;
                }
                else
                {
                    if (curchain > 0)
                    {
                        nchains++;
                        totchain += curchain;
                        if (curchain > maxchain)
                            maxchain = curchain;
                        curchain = 0;
                    }
                }
            }

            fprintf(stdout, "\n Sparse spectra file %s statistics: \n", cssfile);
            fprintf(stdout, "\n Total available buckets          : %12d ",
                    hfsize);
            fprintf(stdout, "\n Total buckets in use             : %12d  ",
                    fbuckets);
            fprintf(stdout, "\n Total in-use zero-count buckets  : %12d  ",
                    zvbins);
            fprintf(stdout, "\n Total buckets with value >= max  : %12d  ",
                    ofbins);
            fprintf(stdout, "\n Total hashed datums in file      : %12lld", (long long int)sum);
            fprintf(stdout, "\n Documents learned                : %12d  ",
                    docs_learned);
            fprintf(stdout, "\n Features learned                 : %12d  ",
                    features_learned);
            fprintf(stdout, "\n Average datums per bucket        : %12.2f",
                    (fbuckets > 0) ? (sum * 1.0) / (fbuckets * 1.0) : 0);
            fprintf(stdout, "\n Maximum length of overflow chain : %12d  ",
                    maxchain);
            fprintf(stdout, "\n Average length of overflow chain : %12.2f ",
                    (nchains > 0) ? (totchain * 1.0) / (nchains * 1.0) : 0);
            fprintf(stdout, "\n Average packing density          : %12.2f\n",
                    (fbuckets * 1.0) / (hfsize * 1.0));

            // set up histograms
            for (i = 0; i < histbins; i++)
                bcounts[i] = 0;
            for (v = 1; v < hfsize; v++)
            {
                if (hashes[v].value < histbins)
                {
                    bcounts[hashes[v].value]++;
                }
                else
                {
                    bcounts[histbins]++; // note that bcounts is len(histbins+2)
                }
            }

            if (!brief)
            {
                for (i = 0; i < histbins; i++)
                {
                    if (bcounts[i] > 0)
                    {
                        if (i < histbins)
                        {
                            fprintf(stdout, "\n bin value %8d found %9d times",
                                    i, bcounts[i]);
                        }
                        else
                        {
                            fprintf(stdout, "\n bin value %8d or more found %9d times",
                                    i, bcounts[i]);
                        }
                    }
                }
            }

            fprintf(stdout, "\n");
            cmdloop = 1;
            while (!report_only && cmdloop)
            {
                // clear command buffer
                cmdchr[0] = 0;
                fprintf(stdout, "Options:\n");
                fprintf(stdout, "   Z n - zero bins at or below a value\n");
                fprintf(stdout, "   S n - subtract a constant from all bins\n");
                fprintf(stdout, "   D n - divide all bins by a constant\n");
                fprintf(stdout, "   R - rescan\n");
                fprintf(stdout, "   P - pack\n");
                fprintf(stdout, "   Q - quit\n");
                fprintf(stdout, ">>> ");
                clearerr(stdin);
                fscanf(stdin, "%[^\n]", cmdstr);
                fscanf(stdin, "%c", crapchr);
                fields = sscanf(cmdstr, "%s %lf", cmdchr, &cmdval);
                if (strlen((char *)cmdchr) != 1)
                {
                    fprintf(stdout, "Unknown command: %s\n", cmdchr);
                    continue;
                }
                switch (tolower((int)cmdchr[0]))
                {
                case 'z':
                    if (fields != 2)
                    {
                        fprintf(stdout,
                                "Z command requires a numeric argument!\n");
                    }
                    else
                    {
                        val = (int)cmdval;
                        fprintf(stdout, "Working...");
                        for (i = 1; i < hfsize; i++)
                        {
                            if (hashes[i].value <= val)
                            {
                                hashes[i].value = 0;
                            }
                        }
                        fprintf(stdout, "done.\n");
                    }
                    break;

                case 's':
                    if (fields != 2)
                    {
                        fprintf(stdout,
                                "S command requires a numeric argument!\n");
                    }
                    else
                    {
                        val = (int)cmdval;
                        fprintf(stdout, "Working...");
                        for (i = 1; i < hfsize; i++)
                        {
                            if (hashes[i].value > val)
                            {
                                hashes[i].value = hashes[i].value - val;
                            }
                            else
                            {
                                hashes[i].value = 0;
                            }
                        }
                        fprintf(stdout, "done.\n");
                    }
                    break;

                case 'd':
                    val = (int)cmdval;
                    if (fields != 2)
                    {
                        fprintf(stdout,
                                "D command requires a numeric argument!\n");
                    }
                    else if (val == 0)
                    {
                        fprintf(stdout, "You can't divide by zero, nimrod!\n");
                    }
                    else
                    {
                        fprintf(stdout, "Working...");
                        for (i = 1; i < hfsize; i++)
                        {
                            hashes[i].value = hashes[i].value / val;
                        }
                        fprintf(stdout, "done.\n");
                    }
                    break;

                case 'r':
                    zloop = 1;
                    cmdloop = 0;
                    break;

                case 'p':
                    fprintf(stdout, "Working...");
                    crm_packcss(hashes, NULL, hfsize, 1, hfsize - 1);
                    zloop = 1;
                    cmdloop = 0;
                    break;

                case 'q':
                    fprintf(stdout, "Bye!\n");
                    cmdloop = 0;
                    break;

                default:
                    fprintf(stdout, "Unknown command: %c\n", cmdchr[0]);
                    break;
                }
            }
        }

        crm_munmap_file((void *)hashes);
    }
    return 0;
}




// bogus code to make link phase happy while we are in limbo between obsoleting this tool and
// getting cssXXXX script commands working in crm114 itself.
void free_stack_item(CSL_CELL *csl)
{}



