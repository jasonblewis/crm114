//  crm_osb_bayes.c  - Controllable Regex Mutilator,  version v1.0
//  Copyright 2001-2007 William S. Yerazunis, all rights reserved.
//
//  This software is licensed to the public under the Free Software
//  Foundation's GNU GPL, version 2.  You may obtain a copy of the
//  GPL by visiting the Free Software Foundations web site at
//  www.fsf.org, and a copy is included in this distribution.
//
//  Other licenses may be negotiated; contact the
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



#if !CRM_WITHOUT_OSB_BAYES


static int collect_obj_bayes_statistics(const char *cssfile,
        FEATUREBUCKET_TYPE                         *hashes,
        int hfsize,                                                          /* unit: number of features! */
        CRM_DECODED_PORTA_HEADER_INFO              *header,
        int histbins,
        FILE                                       *report_out);


////////////////////////////////////////////////////////////////////
//
//     the hash coefficient table (hctable) should be full of relatively
//     prime numbers, and preferably superincreasing, though both of those
//     are not strict requirements.
//
static const int hctable[] =
{
    1, 7,
    3, 13,
    5, 29,
    11, 51,
    23, 101,
    47, 203,
    97, 407,
    197, 817,
    397, 1637,
    797, 3277
};



//          Where does the nominative data start?


//
//    How to learn Osb_Bayes style  - in this case, we'll include the single
//    word terms that may not strictly be necessary.
//

int crm_expr_alt_osb_bayes_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        VHT_CELL **vht,
        CSL_CELL *tdw,
        char *txtptr, int txtstart, int txtlen)
{
    //     learn the osb_bayes transform spectrum of this input window as
    //     belonging to a particular type.
    //     learn <flags> (classname) /word/
    //
    int i, j, k;
    int h;                       //  h is our counter in the hashpipe;
    char ltext[MAX_PATTERN];     //  the variable to learn
    int llen;
    char htext[MAX_PATTERN];     //  the hash name
    int hlen;
    int cflags, eflags;
    struct stat statbuf;            //  for statting the hash file
    int hfsize;                     //  size of the hash file
    FEATUREBUCKET_TYPE *hashes;     //  the text of the hash file
    crmhash_t *hashpipe;
    int hashcounts;
    //
    int sense;
    int microgroom;
    int how_many_terms;
    int fev;
    int made_new_file;

    CRM_DECODED_PORTA_HEADER_INFO hdr = { 0 };
    int histbins;

    CRM_PORTA_HEADER_INFO *info_block;     // contains both learncounts and featurecounts and then some - located in mmap()ped space.

    //          map of the features already seen (used for uniqueness tests)
    char *learnfilename;
    unsigned char *seen_features = NULL;

    if (user_trace)
        fprintf(stderr, "OSB Learn\n");
    if (internal_trace)
        fprintf(stderr, "executing a LEARN\n");

    //   Keep the gcc compiler from complaining about unused variables
    //  i = hctable[0];

    //           extract the hash file name
    hlen = crm_get_pgm_arg(htext, MAX_PATTERN, apb->p1start, apb->p1len);
    hlen = crm_nexpandvar(htext, hlen, MAX_PATTERN, vht, tdw);
    CRM_ASSERT(hlen < MAX_PATTERN);
    //
    //           extract the variable name (if present)
    llen = crm_get_pgm_arg(ltext, MAX_PATTERN, apb->b1start, apb->b1len);
    llen = crm_nexpandvar(ltext, llen, MAX_PATTERN, vht, tdw);


    //     get the "this is a word" regex
    //plen = crm_get_pgm_arg(ptext, MAX_PATTERN, apb->s1start, apb->s1len);
    //plen = crm_nexpandvar(ptext, plen, MAX_PATTERN, vht, tdw);

    //            set our cflags, if needed.  The defaults are
    //            "case" and "affirm", (both zero valued).
    //            and "microgroom" disabled.
    cflags = REG_EXTENDED;
    eflags = 0;
    sense = +1;
    if (apb->sflags & CRM_NOCASE)
    {
        cflags |= REG_ICASE;
        eflags = 1;
        if (user_trace)
            fprintf(stderr, "turning oncase-insensitive match\n");
    }
    if (apb->sflags & CRM_REFUTE)
    {
        sense = -sense;
        if (user_trace)
            fprintf(stderr, " refuting learning\n");
    }
    microgroom = 0;
    if (apb->sflags & CRM_MICROGROOM)
    {
        microgroom = 1;
        if (user_trace)
            fprintf(stderr, " enabling microgrooming.\n");
    }
    how_many_terms = OSB_BAYES_WINDOW_LEN;
    if (apb->sflags & CRM_UNIGRAM)
    {
        how_many_terms = 2;
        if (user_trace)
            fprintf(stderr, " using unigrams only.\n");
    }

    //
    //             grab the filename, and stat the file
    //      note that neither "stat", "fopen", nor "open" are
    //      fully 8-bit or wchar clean...
    if (!crm_nextword(htext, hlen, 0, &i, &j) || j == 0)
    {
        fev = nonfatalerror_ex(SRC_LOC(),
                               "\nYou didn't specify a valid filename: '%.*s'\n",
                               hlen,
                               htext);
        return fev;
    }
    j += i;
    CRM_ASSERT(i < hlen);
    CRM_ASSERT(j <= hlen);

    //             filename starts at i,  ends at j. null terminate it.
    htext[j] = 0;
    learnfilename = &htext[i];
    if (!learnfilename)
    {
        untrappableerror("Cannot allocate classifier memory", "Stick a fork in us; we're _done_.");
        return 1;
    }

    //             and stat it to get it's length
    k = stat(learnfilename, &statbuf);

    made_new_file = 0;

    //             quick check- does the file even exist?
    if (k != 0)
    {
        //      file didn't exist... create it
        FILE *f;
        if (user_trace)
        {
            fprintf(stderr, "\nHad to create new CSS file %s\n", learnfilename);
        }
        f = fopen(learnfilename, "wb");
        if (!f)
        {
            char dirbuf[DIRBUFSIZE_MAX];

            fev = fatalerror_ex(SRC_LOC(),
                                "\n Couldn't open your new CSS file '%s' for writing; (full path: '%s') errno=%d(%s)\n",
                                learnfilename,
                                mk_absolute_path(dirbuf, WIDTHOF(dirbuf), learnfilename),
                                errno,
                                errno_descr(errno));
            return fev;
        }
        //       did we get a value for sparse_spectrum_file_length?
        if (sparse_spectrum_file_length == 0)
        {
            sparse_spectrum_file_length =
                DEFAULT_OSB_BAYES_SPARSE_SPECTRUM_FILE_LENGTH;
        }

        {
            CRM_PORTA_HEADER_INFO classifier_info = { 0 };

            classifier_info.classifier_bits = CRM_OSB_BAYES;
            classifier_info.hash_version_in_use = selected_hashfunction;
            classifier_info.v.OSB_Bayes.sparse_spectrum_size = sparse_spectrum_file_length;

            if (0 != fwrite_crm_headerblock(f, &classifier_info, NULL))
            {
                fev = fatalerror_ex(SRC_LOC(),
                                    "\n Couldn't write header to file %s; errno=%d(%s)\n",
                                    learnfilename, errno, errno_descr(errno));
                fclose(f);
                return fev;
            }

            //       put in sparse_spectrum_file_length entries of NULL
            if (file_memset(f, 0,
                            sparse_spectrum_file_length * sizeof(FEATUREBUCKET_TYPE)))
            {
                fev = fatalerror_ex(SRC_LOC(),
                                    "\n Couldn't write to file %s; errno=%d(%s)\n",
                                    learnfilename, errno, errno_descr(errno));
                fclose(f);
                return fev;
            }

            made_new_file = 1;
            //
            fclose(f);
        }
        //    and reset the statbuf to be correct
        k = stat(learnfilename, &statbuf);
        CRM_ASSERT_EX(k == 0, "We just created/wrote to the file, stat shouldn't fail!");
    }
    //
    hfsize = statbuf.st_size;

    //
    //      map the .css file into memory
    //
    hashes = crm_mmap_file(learnfilename,
                           0,
                           hfsize,
                           PROT_READ | PROT_WRITE,
                           MAP_SHARED,
                           CRM_MADV_RANDOM,
                           &hfsize);
    if (hashes == MAP_FAILED)
    {
        fev = fatalerror("Couldn't get access to the statistics file named: ",
                         learnfilename);
        return fev;
    }

    if (user_trace)
    {
        fprintf(stderr, "Sparse spectra file %s has length %d bins\n",
                learnfilename, (int)(hfsize / sizeof(FEATUREBUCKET_TYPE)));
    }

    //          if this is a new file, set the proper version number.
    if (made_new_file)
    {
        hashes[0].hash  = 0;
        hashes[0].key   = 0;
        hashes[0].value = 1;
    }

    //        check the version of the file
    //
    //if (hashes[0].hash != 0 ||
    //  hashes[0].key  != 0 )
    //{
    //  fprintf(stderr, "Hash was: %d, key was %d\n", hashes[0].hash, hashes[0].key);
    //  fev = fatalerror ("The .css file is the wrong type!  We're expecting "
    //           "a Osb_Bayes-spectrum file.  The filename is: ",
    //           learnfilename);
    //
    //  return (fev);
    //}

#if defined(REDICULOUS_CODE)  /* [i_a] */
    //
    //         In this format, bucket 0.value contains the start of the spectra.
    //
    hashes[0].value = 1;
    spectra_start = hashes[0].value;
#endif


    //
    //   now set the hfsize to the number of entries, not the number
    //   of bytes total
    hfsize = hfsize / sizeof(FEATUREBUCKET_TYPE);

    crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 1, "Li", (unsigned long long int)apb->sflags, (int)hfsize);

    //
    //     if the 'unique' flag was specified, malloc an array to hold the
    //     bitmap of what features have been seen before.
    seen_features = NULL;
    if (apb->sflags & CRM_UNIQUE)
    {
        //     Note that we _calloc_, not malloc, to zero the memory first.
        seen_features = calloc(hfsize, sizeof(seen_features[0]));
        if (seen_features == NULL)
        {
            untrappableerror(" Couldn't allocate enough memory to keep track",
                             "of nonunique features.  This is deadly");
            return 1;
        }
    }

#ifdef OSB_LEARNCOUNTS
    //       If LEARNCOUNTS is enabled, we normalize with documents-learned.
    //
    //       We use the reserved h2 == 0 setup for the learncount.
    //
    // first, it's useless to spend time on calculating these learncount hashes as they can be completely
    // precomputed. Even easier: take two precalculated random numbers, convert tem to an index
    // and you're good to go.
    //
    // Of course, with the new GerH versioning header, one should consider putting this kind of
    // info in the binary header section instead, just to make sure we do not clutter the hash table
    // with useless stuff.
#if defined(CRM_WITHOUT_VERSIONING_HEADER)
#error \
    "This code is not meant to compile without version header support. Absence of such support in legacy CSS databases is supported as best we can for backwards compatibility reasons ONLY"
#else
    {
        void *hdr_ptr = crm_get_header_for_mmap_file(hashes);
        int hdr_check;

        crmhash_t h1, h2;

        if (!hdr_ptr)
        {
            fev = nonfatalerror_ex(SRC_LOC(), "The CSS database file '%s' does not have the proper format: "
                                              "There's no versioning header available or the versioning header is "
                                              "a non-native format. Migrate this database to native versioning-headered format "
                                              "for optimum performance.",
                                   learnfilename);
            return fev;
        }
        hdr_check = crm_decode_header(hdr_ptr, CRM_OSB_BAYES, TRUE, &hdr);
        if (hdr_check)
        {
            fev = nonfatalerror_ex(SRC_LOC(), "The CSS database file '%s' does not have the required "
                                              "versioning header format for classifier %s: error code %d(%s)",
                                   learnfilename,
                                   "OSB Bayes",
                                   hdr_check,
                                   crm_decode_header_err2msg(hdr_check));
            return fev;
        }

        info_block = hdr.native_classifier_info_in_file;
        CRM_ASSERT(info_block != NULL);

        if (sense > 0)
        {
            info_block->v.OSB_Bayes.learncount += sense;
        }
        else
        {
            if (info_block->v.OSB_Bayes.learncount + sense > 0)
            {
                info_block->v.OSB_Bayes.learncount += sense;
            }
            else
            {
                info_block->v.OSB_Bayes.learncount = 0;
            }
        }
        if (user_trace)
        {
            fprintf(stderr, "This file has had %u documents learned!\n",
                    (int)info_block->v.OSB_Bayes.learncount);
        }

#if 10  // we need this to keep results fully BillY compatible. My sense says this is just '1' too many. :-S
        if (info_block->v.OSB_Bayes.features_learned == 0)
        {
            info_block->v.OSB_Bayes.features_learned = 1;
        }
#endif
    }
#endif
#endif


    //   Start by priming the pipe... we will shift to the left next.
    //     sliding, hashing, xoring, moduloing, and incrmenting the
    //     hashes till there are no more.



    //   init the hashpipe with 0xDEADBEEF
    hashpipe = calloc(BAYES_MAX_FEATURE_COUNT, sizeof(hashpipe[0]));
    if (!hashes)
    {
        untrappableerror("Cannot allocate classifier memory", "Stick a fork in us; we're _done_.");
    }
    hashcounts = 0;

    //   Use the flagged vector tokenizer
    crm_vector_tokenize_selector(apb, // the APB
                                 vht,
                                 tdw,
                                 txtptr + txtstart,       // intput string
                                 txtlen,                  // how many bytes
                                 0,                       // starting offset
                                 NULL,                    // tokenizer
                                 NULL,                    // coeff array
                                 hashpipe,                // where to put the hashed results
                                 BAYES_MAX_FEATURE_COUNT, //  max number of hashes
                                 NULL,
                                 NULL,
                                 &hashcounts // how many hashes we actually got
                                );
    CRM_ASSERT(hashcounts >= 0);
    CRM_ASSERT(hashcounts < BAYES_MAX_FEATURE_COUNT);
    CRM_ASSERT(hashcounts % 2 == 0);

    //    and the big loop...
    for (k = 0; k < hashcounts; k++)
    {
        crmhash_t hindex;
        crmhash_t h1, h2;
        int th = 0;                 // a counter used for TSS tokenizing
        unsigned int incrs;
        int j;
        //
        //     old Hash polynomial: h0 + 3h1 + 5h2 +11h3 +23h4
        //     (coefficients chosen by requiring superincreasing,
        //     as well as prime)
        //
        th = 0;
// #ifdef TGB
// #ifdef TGB2
// #ifdef TSS
// #ifdef SBPH
        hindex = hashpipe[k++];
        h1 = hindex;

        //   and what's our primary hash index?  Note that
        //   hindex = 0 is reserved for our version and
        //   usage flags, so we autobump those to hindex=1
        hindex %= hfsize;
        if (hindex == 0)
            hindex = 1;

        //   this is the secondary (crosscut) hash, used for
        //   confirmation of the key value.  Note that it shares
        //   no common coefficients with the previous hash.
        h2 = hashpipe[k];
        if (h2 == 0)
            h2 = 0xdeadbeef;

// #ifdef ARBITRARY_WINDOW_LENGTH
        crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE, k, "iii", (unsigned int)hindex, (unsigned int)h1, (unsigned int)h2);

        if (internal_trace)
        {
            fprintf(stderr, "Polynomial %d has hash: 0x%08lX  h1:0x%08lX  h2:0x%08lX\n",
                    k, (unsigned long int)hashpipe[k], (unsigned long int)h1, (unsigned long int)h2);
        }

        //
        //   we now look at both the primary (h1) and
        //   crosscut (h2) indexes to see if we've got
        //   the right bucket or if we need to look further
        //
        incrs = 0;
        while (                         //    part 1 - when to stop if sense is positive:
            !(sense > 0
              //          in positive mode, stop when we hit
              //          the correct slot, OR when we hit an
              //          zero-value (reusable) slot
             && (hashes[hindex].value == 0
                || (hashes[hindex].hash == h1
                   && hashes[hindex].key  == h2)))
              &&
            !(sense <= 0
              //          in negative/refute mode, stop when
              //          we hit the correct slot, or a truly
              //          unused (not just zero-valued reusable)
              //          slot.
             && ((hashes[hindex].hash == h1
                 && hashes[hindex].key == h2)
                || (hashes[hindex].value == 0
                   && hashes[hindex].hash == 0
                   && hashes[hindex].key == 0
                   ))))
        {
            //
            incrs++;
            //
            //       If microgrooming is enabled, and we've found a
            //       chain that's too long, we groom it down.
            //
            if (microgroom && (incrs > MICROGROOM_CHAIN_LENGTH))
            {
                int zeroedfeatures;

                crm_analysis_mark(&analysis_cfg, MARK_MICROGROOM, h1, "i", (int)incrs);

#ifdef STOCHASTIC_AMNESIA
                //     set the random number generator up...
                //     note that this is repeatable for a
                //     particular test set, yet dynamic.  That
                //     way, we don't always autogroom away the
                //     same feature; we depend on the previous
                //     feature's key.
                srand((unsigned int)h2);
#endif
                //
                //   and do the groom.
                zeroedfeatures = crm_microgroom(hashes,
                                                seen_features,
                                                hfsize,
                                                hindex);
                info_block->v.OSB_Bayes.features_learned -= zeroedfeatures;

                crm_analysis_mark(&analysis_cfg, MARK_MICROGROOM, h1, "ii", (int)incrs, (int)zeroedfeatures);

                //  since things may have moved after a
                //  microgroom, restart our search
                hindex = h1 % hfsize;
#if defined(REDICULOUS_CODE)  /* [i_a] */
                if (hindex < spectra_start)
                    hindex = spectra_start;
#else
                if (hindex < 1)
                    hindex = 1;
#endif
                incrs = 0;
            }
            //      check to see if we've incremented ourself all the
            //      way around the .css file.  If so, we're full, and
            //      can hold no more features (this is unrecoverable)
            if (incrs > hfsize - 3)
            {
                nonfatalerror("Your program is stuffing too many "
                              "features into this size .css file.  "
                              "Adding any more features is "
                              "impossible in this file.",
                              "You are advised to build a larger "
                              ".css file and merge your data into "
                              "it.");
                goto learn_end_regex_loop;
            }
            hindex++;
#if defined(REDICULOUS_CODE)  /* [i_a] */
            if (hindex >= hfsize)
                hindex = spectra_start;
#else
            if (hindex >= hfsize)
                hindex = 1;
#endif
        }

        crm_analysis_mark(&analysis_cfg, MARK_CHAIN_LENGTH, h1, "ii", (int)incrs, (int)hashes[hindex].value);

        if (internal_trace)
        {
            if (hashes[hindex].value == 0)
            {
                fprintf(stderr, "New feature at %d\n", (int)hindex);
            }
            else
            {
                fprintf(stderr, "Old feature at %d\n", (int)hindex);
            }
        }
        //      always rewrite hash and key, as they may be incorrect
        //    (on a reused bucket) or zero (on a fresh one)
        //
        //      watch out - sense may be both + or -, so check before
        //      adding it...
        //
        //   ( and do this only if we either aren't in UNIQUE mode, or
        //     if we haven't seen the feature before in this file)
        if (!seen_features || seen_features[hindex] < 1)
        {
            if (seen_features)
                seen_features[hindex]++;
            //
            if (seen_features && seen_features[hindex] > 1)
            {
                fprintf(stderr, "Hork up a hairball - seenfeatures %d\n",
                        seen_features[hindex]);
            }
            //     let the embedded feature counter sorta keep up...
            if (sense > 0)
            {
                info_block->v.OSB_Bayes.features_learned += sense;

                //     Right slot, set it up
                //
                hashes[hindex].hash = h1;
                hashes[hindex].key  = h2;
                if (hashes[hindex].value + sense
                    >= FEATUREBUCKET_VALUE_MAX - 1)
                {
                    hashes[hindex].value = FEATUREBUCKET_VALUE_MAX - 1;
                }
                else
                {
                    hashes[hindex].value += sense;
                }

                crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE_HIT, h1, "ii", (int)incrs, (int)hashes[hindex].value);

                if (incrs == 0)
                {
                    crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE_DIRECT_HIT, h1, "ii", (int)incrs, (int)hashes[hindex].value);
                }
            }
            else if (sense < 0)
            {
                if (hashes[hindex].value <= -sense)
                {
                    info_block->v.OSB_Bayes.features_learned -= hashes[hindex].value;
                    hashes[hindex].value = 0;
                }
                else
                {
                    info_block->v.OSB_Bayes.features_learned += sense;
                    hashes[hindex].value += sense;
                }

                crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE_HIT_REFUTE, h1, "ii", (int)incrs, (int)hashes[hindex].value);
            }
        }
    }     //   end the while k==0

learn_end_regex_loop:
    //
    // This bit of code can cause a significant performance hit for a learn, so we run
    // it only when we need it.
    //
    if (user_trace || analysis_cfg.instruments[MARK_CSS_STATS_GROUP])
    {
        histbins = FEATUREBUCKET_VALUE_MAX;
        if (histbins > FEATUREBUCKET_HISTOGRAM_MAX)
            histbins = FEATUREBUCKET_HISTOGRAM_MAX;

        collect_obj_bayes_statistics(learnfilename,
                                     hashes,
                                     hfsize,                                     /* unit: number of features! */
                                     &hdr,
                                     histbins,
                                     stderr);
    }


    //  and remember to let go of the mmap and the pattern bufffer
    //  (we force the munmap, because otherwise we still have a link
    //  to the file which stays around until program exit)
    crm_force_munmap_addr(hashes);

    //
    //     If we had the seen_features array, we let go of it.
    if (seen_features)
        free(seen_features);
    seen_features = NULL;

    if (hashpipe)
        free(hashpipe);

#if 0  /* now touch-fixed inside the munmap call already! */
#if defined(HAVE_MMAP) || defined(HAVE_MUNMAP)
    //    Because mmap/munmap doesn't set atime, nor set the "modified"
    //    flag, some network filesystems will fail to mark the file as
    //    modified and so their cacheing will make a mistake.
    //
    //    The fix is to do a trivial read/write on the .css ile, to force
    //    the filesystem to repropagate it's caches.
    //
    crm_touch(learnfilename);
#endif
#endif

    return 0;
}


//      How to do a Osb_Bayes CLASSIFY some text.
//
int crm_expr_alt_osb_bayes_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        VHT_CELL **vht,
        CSL_CELL *tdw,
        char *txtptr, int txtstart, int txtlen)
{
    //      classify the sparse spectrum of this input window
    //      as belonging to a particular type.
    //
    //       This code should look very familiar- it's cribbed from
    //       the code for LEARN
    //
    int i, j, k;
    char ostext[MAX_PATTERN];   //  optional pR offset
    int oslen;
    double pR_offset;
    //  the hash file names
    char htext[MAX_PATTERN + MAX_CLASSIFIERS * MAX_FILE_NAME_LEN];
    int htext_maxlen = MAX_PATTERN + MAX_CLASSIFIERS * MAX_FILE_NAME_LEN;
    int hlen;
    //  the match statistics variable
    char stext[MAX_PATTERN + MAX_CLASSIFIERS * (MAX_FILE_NAME_LEN + 100)];
    char *stext_ptr = stext;
    int stext_maxlen = MAX_PATTERN + MAX_CLASSIFIERS * (MAX_FILE_NAME_LEN + 100);
    int slen;
    char svrbl[MAX_PATTERN];     //  the match statistics text buffer
    int svlen;
    int fnameoffset;
    char fname[MAX_FILE_NAME_LEN];
    int eflags;
    int cflags;
    //  int vhtindex;
    int not_microgroom = 1;
    int use_chisquared = 0;
    int how_many_terms;

    //
    //            use embedded feature index counters, rather than full scans
    CRM_PORTA_HEADER_INFO *info_block[MAX_CLASSIFIERS];     // contains both learncounts and featurecounts and then some - located in mmap()ped space.
    unsigned int total_learns;
    unsigned int total_features;

    //       map of features already seen (used for uniqueness tests)
    int use_unique = 0;
    unsigned char *seen_features[MAX_CLASSIFIERS];

    struct stat statbuf;     //  for statting the hash file
    crmhash_t *hashpipe;

    unsigned int fcounts[MAX_CLASSIFIERS];     // total counts for feature normalize
    unsigned int totalcount = 0;

    double cpcorr[MAX_CLASSIFIERS];        // corpus correction factors

#if defined(GER) || 10
    hitcount_t hits[MAX_CLASSIFIERS];          // actual hits per feature per classifier
    hitcount_t totalhits[MAX_CLASSIFIERS];     // actual total hits per classifier
    double chi2[MAX_CLASSIFIERS];              // chi-squared values (such as they are)
    hitcount_t expected;                       // expected hits for chi2.
    int unk_features;                          //  total unknown features in the document
    hitcount_t htf;                            // hits this feature got.
#else
    double hits[MAX_CLASSIFIERS];       // actual hits per feature per classifier
    int totalhits[MAX_CLASSIFIERS];     // actual total hits per classifier
    double chi2[MAX_CLASSIFIERS];       // chi-squared values (such as they are)
    int expected;                       // expected hits for chi2.
    int unk_features;                   //  total unknown features in the document
    double htf;                         // hits this feature got.
#endif
    double tprob;                                       //  total probability in the "success" domain.
    double min_success = 0.5;                           // minimum probability to be considered success

    double ptc[MAX_CLASSIFIERS];     // current running probability of this class
    double renorm = 0.0;
    //double pltc[MAX_CLASSIFIERS];     // current local probability of this class

    //  int hfds[MAX_CLASSIFIERS];
    FEATUREBUCKET_TYPE *hashes[MAX_CLASSIFIERS];
    int hashlens[MAX_CLASSIFIERS];
    char *hashname[MAX_CLASSIFIERS];
    int succhash;
    int vbar_seen;     // did we see '|' in classify's args?
    int maxhash;
    int fnstart, fnlen;
    int fn_start_here;
    int bestseen;
    int thistotal;
    int ifile;
    uint32_t *feature_weight;
    uint32_t *order_no;
    int hashcounts;

    if (internal_trace)
        fprintf(stderr, "executing a CLASSIFY\n");


    //           extract the hash file names
    hlen = crm_get_pgm_arg(htext, htext_maxlen, apb->p1start, apb->p1len);
    hlen = crm_nexpandvar(htext, hlen, htext_maxlen, vht, tdw);
    CRM_ASSERT(hlen < MAX_PATTERN);


    //       extract the optional pR offset value
    //
    oslen = crm_get_pgm_arg(ostext, MAX_PATTERN, apb->s2start, apb->s2len);
    pR_offset = 0;
    min_success = 0.5;
    if (oslen > 0)
    {
        oslen = crm_nexpandvar(ostext, oslen, MAX_PATTERN, vht, tdw);
        CRM_ASSERT(oslen < MAX_PATTERN);
        ostext[oslen] = 0;
        pR_offset = strtod(ostext, NULL);
        min_success = 1.0 - 1.0 / (1 + pow(10, pR_offset));
    }

    //            extract the optional "match statistics" variable
    //
    svlen = crm_get_pgm_arg(svrbl, MAX_PATTERN, apb->p2start, apb->p2len);
    svlen = crm_nexpandvar(svrbl, svlen, MAX_PATTERN, vht, tdw);
    CRM_ASSERT(svlen < MAX_PATTERN);
    {
        int vstart, vlen;

        if (crm_nextword(svrbl, svlen, 0, &vstart, &vlen))
        {
            crm_memmove(svrbl, &svrbl[vstart], vlen);
            svlen = vlen;
            svrbl[vlen] = 0;
        }
        else
        {
            svlen = 0;
            svrbl[0] = 0;
        }
    }
    if (user_trace)
    {
        fprintf(stderr, "Status out var %s (len %d)\n",
                svrbl, svlen);
    }

    //     status variable's text (used for output stats)
    //
    stext[0] = 0;
    slen = 0;

    //            set our flags, if needed.  The defaults are
    //            "case"
    cflags = REG_EXTENDED;
    eflags = 0;

    if (apb->sflags & CRM_NOCASE)
    {
        if (user_trace)
            fprintf(stderr, " setting NOCASE for tokenization\n");
        cflags |= REG_ICASE;
        eflags = 1;
    }

    not_microgroom = 1;
    if (apb->sflags & CRM_MICROGROOM)
    {
        not_microgroom = 0;
        if (user_trace)
            fprintf(stderr, " disabling fast-skip optimization.\n");
    }

    use_unique = 0;
    if (apb->sflags & CRM_UNIQUE)
    {
        use_unique = 1;
        if (user_trace)
            fprintf(stderr, " unique engaged -repeated features are ignored \n");
    }

    how_many_terms = OSB_BAYES_WINDOW_LEN;
    if (apb->sflags & CRM_UNIGRAM)
    {
        how_many_terms = 2;
        if (user_trace)
            fprintf(stderr, " using unigram features only \n");
    }

    use_chisquared = 0;
    if (apb->sflags & CRM_CHI2)
    {
        use_chisquared = 1;
        if (user_trace)
            fprintf(stderr, " using chi^2 chaining rule \n");
    }



    //       Now, the loop to open the files.
    bestseen = 0;
    thistotal = 0;
    //  goodcount = evilcount = 1;   // prevents a divide-by-zero error.
    //cpgood = cpevil = 0.0;
    //ghits = ehits = 0.0 ;
    //psucc = 0.5;
    //pfail = (1.0 - psucc);
    //pic = 0.5;
    //pnic = 0.5;


    //      initialize our arrays for N .css files
    for (i = 0; i < MAX_CLASSIFIERS; i++)
    {
        fcounts[i] = 0;           // check later to prevent a divide-by-zero
                                  // error on empty .css file
        cpcorr[i] = 0.0;          // corpus correction factors
        hits[i] = 0;              // absolute hit counts
        totalhits[i] = 0;         // absolute hit counts
        ptc[i] = 0.5;             // priori probability
        //pltc[i] = 0.5;            // local probability
    }


    //        --  probabilistic evaluator ---
    //     S = success; A = a testable attribute of success
    //     ns = not success, na = not attribute
    //     the chain rule we use is:
    //
    //                   P(A|S) P(S)
    //  P (S|A) =   -------------------------
    //             P(A|S) P(S) + P(A|NS) P(NS)
    //
    //     and we apply it repeatedly to evaluate the final prob.  For
    //     the initial a-priori probability, we use 0.5.  The output
    //     value (here, P(S|A) ) becomes the new a-priori for the next
    //     iteration.
    //
    //     Extension - we generalize the above to I classes as and feature
    //      F as follows:
    //
    //                         P(F|Ci) P(Ci)
    //    P(Ci|F) = ----------------------------------------
    //              Sum over all classes Ci of P(F|Ci) P(Ci)
    //
    //     We also correct for the unequal corpus sizes by multiplying
    //     the probabilities by a renormalization factor.  if Tg is the
    //     total number of good features, and Te is the total number of
    //     evil features, and Rg and Re are the raw relative scores,
    //     then the corrected relative scores Cg aqnd Ce are
    //
    //     Cg = (Rg / Tg)
    //     Ce = (Re / Te)
    //
    //     or  Ci = (Ri / Ti)
    //
    //     Cg and Ce can now be used as "corrected" relative counts
    //     to calculate the naive Bayesian probabilities.
    //
    //     Lastly, the issue of "over-certainty" rears it's ugly head.
    //     This is what happens when there's a zero raw count of either
    //     good or evil features at a particular place in the file; the
    //     strict but naive mathematical interpretation of this is that
    //     "feature A never/always occurs when in good/evil, hence this
    //     is conclusive evidence of good/evil and the probabilities go
    //     to 1.0 or 0.0, and are stuck there forevermore.  We use the
    //     somewhat ad-hoc interpretation that it is unreasonable to
    //     assume that any finite number of samples can appropriately
    //     represent an infinite continuum of spewage, so we can bound
    //     the certainty of any measure to be in the range:
    //
    //        limit: [ 1/featurecount+2 , 1 - 1/featurecount+2].
    //
    //     The prior bound is strictly made-up-on-the-spot and has NO
    //     strong theoretical basis.  It does have the nice behavior
    //     that for feature counts of 0 the probability is clipped to
    //     [0.5, 0.5], for feature counts of 1 to [0.333, 0.666]
    //     for feature counts of 2 to [0.25, 0.75], for 3 to
    //     [0.2, 0.8], for 4 to [0.166, 0.833] and so on.
    //

    vbar_seen = 0;
    maxhash = 0;
    succhash = 0;
    fnameoffset = 0;

    //    now, get the file names and mmap each file
    //     get the file name (grody and non-8-bit-safe, but doesn't matter
    //     because the result is used for open() and nothing else.
    //   GROT GROT GROT  this isn't NULL-clean on filenames.  But then
    //    again, stdio.h itself isn't NULL-clean on filenames.
    if (user_trace)
        fprintf(stderr, "Classify list: -%s-\n", htext);
    fn_start_here = 0;
    fnlen = 1;

    while (fnlen > 0 && ((maxhash < MAX_CLASSIFIERS - 1)))
    {
        if (crm_nextword(htext, hlen, fn_start_here, &fnstart, &fnlen)
           && fnlen > 0)
        {
            strncpy(fname, &htext[fnstart], fnlen);
            fname[fnlen] = 0;
            //      fprintf(stderr, "fname is '%s' len %d\n", fname, fnlen);
            fn_start_here = fnstart + fnlen + 1;
            if (user_trace)
            {
                fprintf(stderr, "Classifying with file -%s- "
                                "succhash=%d, maxhash=%d\n",
                        fname, succhash, maxhash);
            }
            if (fname[0] == '|' && fname[1] == 0)
            {
                if (vbar_seen)
                {
                    nonfatalerror("Only one '|' allowed in a CLASSIFY.\n",
                                  "We'll ignore it for now.");
                }
                else
                {
                    succhash = maxhash;
                }
                vbar_seen++;
            }
            else
            {
                //  be sure the file exists
                //             stat the file to get it's length
                k = stat(fname, &statbuf);
                //             quick check- does the file even exist?
                if (k != 0)
                {
                    nonfatalerror("Nonexistent Classify table named: ", fname);
                }
                else
                {
                    // [i_a] check hashes[] range BEFORE adding another one!
                    if (maxhash >= MAX_CLASSIFIERS)
                    {
                        nonfatalerror("Too many classifier files.",
                                      "Some may have been disregarded");
                    }
                    else
                    {
                        //  file exists - do the mmap
                        //
                        hashlens[maxhash] = statbuf.st_size;
                        //  mmap the hash file into memory so we can bitwhack it

                        hashes[maxhash] = crm_mmap_file(fname,
                                                        0,
                                                        hashlens[maxhash],
                                                        PROT_READ,
                                                        MAP_SHARED,
                                                        CRM_MADV_RANDOM,
                                                        &hashlens[maxhash]);

                        if (hashes[maxhash] == MAP_FAILED)
                        {
                            nonfatalerror("Couldn't memory-map the table file", fname);
                        }
                        else
                        {
#ifdef CSS_VERSION_CHECK
                            //
                            //     Check to see if this file is the right version
                            //
                            int fev;
                            if (hashes[maxhash][0].hash != 0
                               || hashes[maxhash][0].key  != 0)
                            {
                                fev = fatalerror("The .css file is the wrong version!  Filename is: ",
                                                 fname);
                                return fev;
                            }
#endif

#if defined(REDICULOUS_CODE)  /* [i_a] */
                              //     grab the start of the actual spectrum data.
                              //
                            spectra_start = hashes[maxhash][0].value;
#endif

                            //  set this hashlens to the length in features instead
                            //  of the length in bytes.
                            hashlens[maxhash] /= sizeof(FEATUREBUCKET_TYPE);

                            crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 2, "Lii", (unsigned long long int)apb->sflags, (int)hashlens[maxhash],
                                              (int)maxhash);

                            hashname[maxhash] = (char *)calloc((fnlen + 10), sizeof(hashname[maxhash][0]));
                            if (!hashname[maxhash])
                            {
                                untrappableerror("Couldn't alloc hashname[maxhash]\n",
                                                 "We need that part later, so we're stuck.  Sorry.");
                            }
                            strncpy(hashname[maxhash], fname, fnlen);
                            hashname[maxhash][fnlen] = 0;
                            maxhash++;
                        }
                    }
                }
            }
        }
    }

    //
    //    If there is no '|', then all files are "success" files.
    if (succhash == 0)
        succhash = maxhash;

    //    a CLASSIFY with no arguments is always a "success".
    if (maxhash == 0)
        return 0;

    if (user_trace)
    {
        fprintf(stderr, "Running with %d files for success out of %d files\n",
                succhash, maxhash);
    }

    // sanity checks...  Uncomment for super-strict CLASSIFY.
    //
    //    do we have at least 1 valid .css files?
    if (maxhash == 0)
    {
        return nonfatalerror("Couldn't open at least 1 .css file for classify().", "");
    }

#if 0
    //    do we have at least 1 valid .css file at both sides of '|'?
    if (!vbar_seen || succhash <= 0 || (maxhash <= succhash))
    {
        return nonfatalerror("Couldn't open at least 1 .css file per SUCC | FAIL category "
                             "for classify().\n", "Hope you know what are you doing.");
    }
#endif

    {
        int k;

        k = 1;
        //      count up the total first
        for (ifile = 0; ifile < maxhash; ifile++)
        {
            fcounts[ifile] = 0;
            //      for (k = 1; k < hashlens[ifile]; k++)
            // fcounts [ifile] = fcounts[ifile] + hashes[ifile][k].value;
            if (fcounts[ifile] == 0)
                fcounts[ifile] = 1;
            totalcount += fcounts[ifile];

#ifdef OSB_LEARNCOUNTS
            //       If LEARNCOUNTS is enabled, we normalize with
            //       documents-learned.
            //
            //       We use the reserved h2 == 0 setup for the learncount.
            //


            // first, it's useless to spend time on calculating these learncount hashes as they can be completely
            // precomputed. Even easier: take two precalculated random numbers, convert tem to an index
            // and you're good to go.
            //
            // Of course, with the new GerH versioning header, one should consider putting this kind of
            // info in the binary header section instead, just to make sure we do not clutter the hash table
            // with useless stuff.
#if defined(CRM_WITHOUT_VERSIONING_HEADER)
#error \
            "This code is not meant to compile without version header support. Absence of such support in legacy CSS databases is supported as best we can for backwards compatibility reasons ONLY"
#else
            {
                void *hdr_ptr;
                int hdr_check;
                CRM_DECODED_PORTA_HEADER_INFO hdr = { 0 };

                crmhash_t h1, h2;

                hdr_ptr = crm_get_header_for_mmap_file(hashes[ifile]);
                if (!hdr_ptr)
                {
                    nonfatalerror_ex(SRC_LOC(), "CSS database file #%d does not have the proper format: "
                                                "There's no versioning header available or the versioning header is "
                                                "a non-native format. Migrate this database to native versioning-headered format "
                                                "for optimum performance.",
                                     ifile);
                    return 1;
                }
                hdr_check = crm_decode_header(hdr_ptr, CRM_OSB_BAYES, TRUE, &hdr);
                if (hdr_check)
                {
                    nonfatalerror_ex(SRC_LOC(), "CSS database file #%d does not have the required "
                                                "versioning header format for classifier %s: error code %d(%s)",
                                     ifile,
                                     "OSB Bayes",
                                     hdr_check,
                                     crm_decode_header_err2msg(hdr_check));
                    return 1;
                }

                info_block[ifile] = hdr.native_classifier_info_in_file;
                CRM_ASSERT(info_block[ifile] != NULL);
            }

#endif
#endif
        }
    }
    //
    //     calculate cpcorr (count compensation correction)
    //
    total_learns = 0;
    total_features = 0;
    for (ifile = 0; ifile < maxhash; ifile++)
    {
        total_learns += info_block[ifile]->v.OSB_Bayes.learncount;
        total_features += info_block[ifile]->v.OSB_Bayes.features_learned;

        crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER_DB_TOTALS, ifile, "ii", (int)info_block[ifile]->v.OSB_Bayes.learncount,
                          (int)info_block[ifile]->v.OSB_Bayes.features_learned);
    }

    for (ifile = 0; ifile < maxhash; ifile++)
    {
        //   disable cpcorr for now... unclear that it's useful.
        // cpcorr[ifile] = 1.0;
        //
        //  new cpcorr - from Fidelis' work on evaluators.  Note that
        //   we renormalize _all_ terms, not just the min term.
        cpcorr[ifile] = total_learns / ((double)maxhash * (double)info_block[ifile]->v.OSB_Bayes.learncount);

        crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER_PARAMS, 1, "idi", (int)ifile, (double)cpcorr[ifile], (int)total_learns);

        if (use_chisquared)
            cpcorr[ifile] = 1.0;
    }

    if (internal_trace)
    {
        fprintf(stderr, " files  %d  learns ",
                maxhash);
        for (ifile = 0; ifile < maxhash; ifile++)
        {
            fprintf(stderr, "#%d %u  ",
                    ifile,
                    info_block[ifile]->v.OSB_Bayes.learncount
                   );
        }
        fprintf(stderr, "total %u",
                total_learns);
        for (ifile = 0; ifile < maxhash; ifile++)
        {
            fprintf(stderr, " cp%d %f",
                    ifile,
                    cpcorr[ifile]);
        }
        fprintf(stderr, "\n");
    }



    for (ifile = 0; ifile < maxhash; ifile++)
    {
        if (use_unique != 0)
        {
            //     Note that we _calloc_, not malloc, to zero the memory first.
            seen_features[ifile] = calloc(hashlens[ifile] + 1, sizeof(seen_features[ifile][0]));
            if (seen_features[ifile] == NULL)
            {
                untrappableerror(" Couldn't allocate enough memory to keep track",
                                 "of nonunique features.  This is deadly");
            }
        }
        else
        {
            seen_features[ifile] = NULL;
        }
    }
    //
    //   now all of the files are mmapped into memory,
    //   and we can do the polynomials and add up points.
    i = 0;
    j = 0;
    thistotal = 0;
    unk_features = 0;


    //   init the hashpipe with 0xDEADBEEF
    hashpipe = calloc(BAYES_MAX_FEATURE_COUNT, sizeof(hashpipe[0]));
    if (!hashpipe)
    {
        untrappableerror("Cannot allocate classifier memory", "Stick a fork in us; we're _done_.");
    }
    feature_weight = calloc(BAYES_MAX_FEATURE_COUNT, sizeof(feature_weight[0]));
    if (!feature_weight)
    {
        untrappableerror("Cannot allocate classifier memory", "Stick a fork in us; we're _done_.");
    }
    order_no = calloc(BAYES_MAX_FEATURE_COUNT, sizeof(order_no[0]));
    if (!order_no)
    {
        untrappableerror("Cannot allocate classifier memory", "Stick a fork in us; we're _done_.");
    }
    hashcounts = 0;

    //   Use the flagged vector tokenizer
    crm_vector_tokenize_selector(apb, // the APB
                                 vht,
                                 tdw,
                                 txtptr + txtstart,       // intput string
                                 txtlen,                  // how many bytes
                                 0,                       // starting offset
                                 NULL,                    // tokenizer
                                 NULL,                    // coeff array
                                 hashpipe,                // where to put the hashed results
                                 BAYES_MAX_FEATURE_COUNT, //  max number of hashes
                                 feature_weight,
                                 order_no,
                                 &hashcounts // how many hashes we actually got
                                );
    CRM_ASSERT(hashcounts >= 0);
    CRM_ASSERT(hashcounts < BAYES_MAX_FEATURE_COUNT);
    CRM_ASSERT(hashcounts % 2 == 0);


    //  stop when we no longer get any regex matches
    //   possible edge effect here- last character must be matchable, yet
    //    it's also the "end of buffer".
    {
        int i;

        for (i = 0; i < hashcounts; i++)
        {
            int j;
            int l;
            uint32_t fw;
            unsigned int th = 0;      //  a counter used only in TSS hashing
            crmhash_t hindex;
            crmhash_t h1, h2;
            int skip_this_feature = 0;
            //
            th = 0;

            j = order_no[i];
            fw = feature_weight[i];

// #ifdef FOO
// #ifdef TGB
// #ifdef TGB2
// #ifdef TSS
// #ifdef SBPH
            hindex = hashpipe[i++];
            h1 = hindex;

            //   this is the secondary (crosscut) hash, used for
            //   confirmation of the key value.  Note that it shares
            //   no common coefficients with the previous hash.
            h2 = hashpipe[i];
            if (h2 == 0)
                h2 = 0xdeadbeef;

// #ifdef ARBITRARY_WINDOW_LENGTH
            if (internal_trace)
            {
                fprintf(stderr, "Polynomial %d has hash: 0x%08lX  h1:0x%08lX  h2:0x%08lX\n",
                        i, (unsigned long int)hashpipe[i], (unsigned long int)h1, (unsigned long int)h2);
            }
            //
            //    Note - a strict interpretation of Bayesian
            //    chain probabilities should use 0 as the initial
            //    state.  However, because we rapidly run out of
            //    significant digits, we use a much less strong
            //    initial state.   Note also that any nonzero
            //    positive value prevents divide-by-zero
            //
            //     Fidelis Assis suggests that 0, 24, 14, 7, 4, 0...
            //     may be optimal.  We keep the 0th term only
            //     because it allows Arne's Optimization (that being
            //     not bothering to evaluate the higher-order terms if
            //     the first-order has a zero count)
            //     NOT because it improves accuracy.
            {
#if 0
                static const int fw_arr[10] =
                {
                    0, 24, 14, 7, 4, 2, 1, 0
                };
                // cubic weights seems to work well for chi^2...- Fidelis
                static const int chi_feature_weight[] =
                {
                    0, 125, 64, 27, 8, 1, 0
                };
                CRM_ASSERT(OSB_BAYES_WINDOW_LEN <= WIDTHOF(chi_feature_weight) - 2);
                CRM_ASSERT(OSB_BAYES_WINDOW_LEN <= WIDTHOF(fw_arr) - 2);
                fw = fw_arr[j];
#endif
                if (use_chisquared)
                {
#if 0
                    fw = chi_feature_weight[j];
#else
                    //  turn off weighting?
                    fw = 1;
#endif
                }
            }

            //       tally another feature
            unk_features++;
            //
            //       Zero out "Hits This Feature"
            htf = 0;

            //
            //    calculate the precursors to the local probabilities;
            //    these are the hits[k] array, and the htf total.
            //
            skip_this_feature = 0;
            {
                int k;

                for (k = 0; k < maxhash; k++)
                {
                    int lh, lh0;
                    lh = hindex % hashlens[k];
#if defined(REDICULOUS_CODE)  /* [i_a] */
                    if (lh < spectra_start)
                        lh = spectra_start;
#else
                    if (lh < 1)
                        lh = 1;
#endif
                    lh0 = lh;

                    crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE, j, "iii", (unsigned int)lh, (unsigned int)h1, (unsigned int)h2);

                    hits[k] = 0;
                    while (hashes[k][lh].key != 0
                          && (hashes[k][lh].hash != h1
                             || hashes[k][lh].key != h2))
                    {
                        lh++;
#if defined(REDICULOUS_CODE)  /* [i_a] */
                        if (lh >= hashlens[k])
                            lh = spectra_start;
#else
                        if (lh >= hashlens[k])
                            lh = 1;
#endif
                        if (lh == lh0)
                            break; // wraparound
                    }
                    if (hashes[k][lh].hash == h1 && hashes[k][lh].key == h2)
                    {
                        double l;
                        //
                        //    Note- if we've seen this feature before, we
                        //    will ignore it
                        l = hashes[k][lh].value * fw;
                        l *= cpcorr[k];                         // Correct with cpcorr
                        // remember totalhits
                        if (use_chisquared)
                        {
                            totalhits[k]++;
                        }
                        else
                        {
                            totalhits[k] += l;                             // remember totalhits  /* [i_a] compare this code with elsewhere; here totalhits is counted different; should it be double type??? */
                        }
                        hits[k] = l;
                        htf += hits[k];                         // and hits-this-feature

                        if (use_unique)
                        {
                            if (seen_features[k][lh] > 0)
                                skip_this_feature = 1;
                            if (seen_features[k][lh] < 250)
                                seen_features[k][lh]++;
                        }

                        crm_analysis_mark(&analysis_cfg, MARK_CHAIN_LENGTH, k, "ii", (int)(lh - lh0), (int)hashes[k][lh].value);

                        crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE_HIT, k, "iii", (int)(lh - lh0), (int)fw, (int)totalhits[k]);

                        if (lh == lh0)
                        {
                            crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE_DIRECT_HIT, k, "ii", (int)(lh - lh0), (int)fw);
                        }
                    }
                    else
                    {
                        crm_analysis_mark(&analysis_cfg, MARK_HASHPROBE_MISS, k, "i", (unsigned int)(lh - lh0));
                    }
                }
            }

            //      now update the probabilities.
            //
            //     NOTA BENE: there are a bunch of different ways to
            //      calculate local probabilities.  The text below
            //      refers to an experiment that may or may not make it
            //      as the "best way".
            //
            //      The hard part is this - what is the local in-class
            //      versus local out-of-class probability given the finding
            //      of a particular feature?
            //
            //      I'm guessing this- the validity is the differntial
            //      seen on the feature (that is, fgood - fevil )
            //      times the entropy of that feature as seen in the
            //      corpus (that is,
            //
            //              Pfeature*log2(Pfeature)
            //
            //      =
            //        totalcount_this_feature
            //            ---------------    * log2 (totalcount_this_feature)
            //         totalcount_all_features
            //
            //    (note, yes, the last term seems like it should be
            //    relative to totalcount_all_features, but a bit of algebra
            //    will show that if you view fgood and fevil as two different
            //    signals, then you end up with + and - totalcount inside
            //    the logarithm parenthesis, and they cancel out.
            //    (the 0.30102 converts "log10" to "log2" - it's not
            //    a magic number, it's just that log2 isn't in glibc)
            //

            //  HACK ALERT- this code here is still under development
            //  and should be viewed with the greatest possible
            //  suspicion.  :=)

            //    Now, some funky magic.  Our formula above is
            //    mathematically correct (if features were
            //    independent- something we conveniently ignore.),
            //    but because of the limited word length in a real
            //    computer, we can quickly run out of dynamic range
            //    early in a CLASSIFY (P(S) indistinguishable from
            //    1.00) and then there is no way to recover.  To
            //    remedy this, we use two alternate forms of the
            //    formula (in Psucc and Pfail) and use whichever
            //    form that yields the smaller probability to
            //    recalculate the value of the larger.
            //
            //    The net result of this subterfuge is a nonuniform
            //    representation of the probability, with a huge dynamic
            //    range in two places - near 0.0, and near 1.0 , which
            //    are the two places where we actually care.
            //
            //    Note upon note - we don't do this any more - instead we
            //    do a full renormalize and unstick at each local prob.
            //
            //   calculate renormalizer (the Bayesian formula's denomenator)

            crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER_PARAMS, 2, "iii", (int)htf, (int)skip_this_feature, (int)fw);

            if (!skip_this_feature)
            {
                if (use_chisquared)
                {
                    //  Actually, for chisquared with ONE feature
                    //  category (that being the number of weighted
                    //  hits) we end up with not having to do
                    //  anything here at all.  Instead, we look at
                    //  total hits expected in a document of this
                    //  length.
                    //
                    //  This actually makes sense, since the reality
                    //  is that most texts have an expected value of
                    //  far less than 1.0 for almost all features.
                    //  and thus common chi-squared assumptions
                    //  break down (like "need at least 5 in each
                    //  category"!)

                    // float renorm;
                    //double expected;
                    //for ( k = 0; k < maxhash; k++)
                    //        {
                    //   This is the first half of a BROKEN
                    //   chi-squared formula -
                    //
                    //  MeritProd =
                    //       Product (expected-observed)^2 / expected
                    //
                    //   Second half- when done with features, take the
                    //     featurecounth root of MeritProd.
                    //
                    //   Note that here the _lowest_ Merit is best fit.
                    //if (htf > 0 )
                    //  ptc[k] = ptc[k] *
                    //    (1.0 + ((htf/maxhash) - hits[k])
                    //     * (1.0 +(htf/maxhash) - hits[k]))
                    //    / (2.0 + htf/maxhash);
                    //
                    //  Renormalize to avoid really small
                    //  underflow...  this is unnecessary with
                    //  above better additive form
                    //
                    //renorm = 1.0;
                    //for (k = 0; k < maxhash; k++)
                    //renorm = renorm * ptc[k];
                    //for (k = 0; k < maxhash; k++)
                    //{
                    //  ptc[k] = ptc[k] / renorm;
                    //  fprintf(stderr, "K= %d, rn=%f, ptc[k] = %f\n",
                    //  //   k, renorm,  ptc[k]);
                    //}

                    //    Nota BENE: the above is not standard chi2
                    //    here's a better one.
                    //    Again, lowest Merit is best fit.
                    //if (htf > 0 )
                    //  {
                    //    expected = (htf + 0.000001) / (maxhash + 1.0);
                    //  ptc[k] = ptc[k] +
                    //((expected - hits[k])
                    // * (expected - hits[k]))
                    // / expected;
                    //}
                    //}
                }
                else                         //  if not chi-squared, use Bayesian
                {
                    int k;
                    //   calculate local probabilities from hits
                    //

                    if (htf > 0)
                    {
                        renorm = 0.0;
                        for (k = 0; k < maxhash; k++)
                        {
                            //pltc[k] = 0.5 +
                            //          ((hits[k] - (htf - hits[k]))
                            //           / (LOCAL_PROB_DENOM * (htf + 1.0)));
                            double pltc = ((2 * hits[k] - htf) / (LOCAL_PROB_DENOM * htf));

                            //   Calculate the per-ptc renormalization numerators
                            ptc[k] *= 0.5 + pltc;
                            //
                            //      part 2) renormalize to sum probabilities to 1.0
                            //
                            renorm += ptc[k];
                        }

                        //   if we have underflow (any probability == 0.0 ) then
                        //   bump the probability back up to 10^-308, or
                        //   whatever a small multiple of the minimum double
                        //   precision value is on the current platform.
                        //
                        //for (k = 0; k < maxhash; k++)
                        //{
                        //    if (ptc[k] < 1000 * DBL_MIN)
                        //        ptc[k] = 1000 * DBL_MIN;
                        //}
                        if (renorm < 10 * DBL_MIN)
                        {
                            renorm = 10 * DBL_MIN;
                        }

                        for (k = 0; k < maxhash; k++)
                        {
                            crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER_PARAMS, 3, "idd", (int)k, ptc[k], htf);
                            crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER_PARAMS, 4, "ddd", renorm, (double)hits[k], (double)htf);

                            ptc[k] /= renorm;
                        }
                    }
                }
            }
            if (internal_trace)
            {
                for (k = 0; k < maxhash; k++)
                {
                    fprintf(stderr,
                            " poly: %d  filenum: %d, HTF: %7ld, hits: %7ld, Pc: %6.4e\n",
                            j, k, (long int)htf, (long int)hits[k], ptc[k]);
                }
            }
            //
            //    avoid the fencepost error for window=1
            if (OSB_BAYES_WINDOW_LEN == 1)
            {
                j = 99999;
            }
        }
    }

    expected = 1;
    //     Do the chi-squared computation.  This is just
    //           (expected-observed)^2  / expected.
    //     Less means a closer match.
    //
    if (use_chisquared)
    {
        double features_here, learns_here;
        double avg_features_per_doc, this_doc_relative_len;
#if defined(GER) || 10
        double /* hitcount_t */ actual;  // must be floating point type to ensure proper chi2 calculation
#else
        double actual;
#endif

        //    The next statement appears stupid, but we don't have a
        //    good way to estimate the fraction of features that
        //    will be "out of corpus".  A very *rough* guess is that
        //    about 2/3 of the learned document features will be
        //    hapaxes - that is, features not seen before, so we'll
        //    start with the 1/3 that we expect to see in the corpus
        //    as not-hapaxes.
        expected = unk_features / 1.5;
        for (k = 0; k < maxhash; k++)
        {
            if (totalhits[k] > expected)
            {
                expected = totalhits[k] + 1;
            }
            if (internal_trace)
            {
                fprintf(stderr,
                        "expected[%d]: %d, totalhist[%d] = %d\n",
                        k,
                        (int)expected,
                        k,
                        (int)totalhits[k]);
            }
        }

        for (k = 0; k < maxhash; k++)
        {
            features_here = info_block[k]->v.OSB_Bayes.features_learned;
            learns_here = info_block[k]->v.OSB_Bayes.learncount;

            avg_features_per_doc = 1.0 + features_here / (learns_here + 1.0);
            this_doc_relative_len = unk_features / avg_features_per_doc;
            // expected = 1 + this_doc_relative_len * avg_features_per_doc / 3.0;
            // expected = 1 + this_doc_relative_len * avg_features_per_doc;
            actual = totalhits[k];
            chi2[k] = (expected - actual) * (expected - actual) / expected;
            //     There's a real (not closed form) expression to
            //     convert from chi2 values to probability, but it's
            //     lame.  We'll approximate it as 2^-chi2.  Close enough
            //     for government work.
            ptc[k] = 1.0 / pow(chi2[k], 2);

            crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER_PARAMS, 5, "idd", (int)k, ptc[k], chi2[k]);
            crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER_PARAMS, 6, "did", (double)expected, (int)unk_features, actual);

            if (user_trace)
            {
                fprintf(stderr,
                        "CHI2: k: %d, feats: %f, learns: %f, avg fea/doc: %f, rel_len: %f, exp: %d, act: %f, chi2: %f, p: %f\n",
                        k,
                        (double)features_here,
                        (double)learns_here,
                        avg_features_per_doc,
                        (double)this_doc_relative_len,
                        (int)expected,
                        actual,
                        chi2[k],
                        ptc[k]);
            }
        }
    }

    {
        int k;

        //  One last chance to force probabilities into the non-stuck zone
        //  and one last renormalize for both bayes and chisquared
        renorm = 0.0;
        for (k = 0; k < maxhash; k++)
        {
            renorm += ptc[k];
        }
        if (renorm < 10 * DBL_MIN)
        {
            renorm = 10 * DBL_MIN;
        }
        for (k = 0; k < maxhash; k++)
        {
            ptc[k] /= renorm;
        }

        if (user_trace)
        {
            for (k = 0; k < maxhash; k++)
            {
                fprintf(stderr, "Probability of match for file %d: %f\n", k, ptc[k]);
            }
        }
        //
        tprob = 0.0;
        for (k = 0; k < succhash; k++)
        {
            tprob += ptc[k];
        }
        if (tprob < 10 * DBL_MIN)
            tprob = 10 * DBL_MIN;

#if !defined(CRM_WITHOUT_BMP_ASSISTED_ANALYSIS)
        for (k = 0; k < maxhash; k++)
        {
            crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER_PARAMS, 7, "idd", (int)k, tprob, ptc[k]);
        }
#endif
    }

    //
    //      Do the calculations and format some output, which we may or may
    //      not use... but we need the calculated result anyway.
    //


    if (1 /* svlen > 0 */)
    {
        // char buf[1024];
        double accumulator;
        double remainder;
        double overall_pR;
        int m;

        // buf[0] = 0;
        accumulator = tprob;

        CRM_ASSERT(bestseen == 0);
        CRM_ASSERT(succhash >= 1);

        remainder = 0.0;
        for (m = succhash; m < maxhash; m++)
        {
            remainder += ptc[m];
        }
        if (remainder < 10 * DBL_MIN)
            remainder = 10 * DBL_MIN;
        overall_pR = log10(accumulator) - log10(remainder);

        //   note also that strcat _accumulates_ in stext.
        //  There would be a possible buffer overflow except that _we_ control
        //   what gets written here.  So it's no biggie.

        if (tprob > min_success)
        {
            // if a pR offset was given, print it together with the real pR
            if (oslen > 0)
            {
                snprintf(stext_ptr, stext_maxlen,
                         "CLASSIFY succeeds; (alt.osb) success probability: "
                         "%6.4f  pR: %6.4f / %6.4f\n",
                         tprob, overall_pR, pR_offset);
            }
            else
            {
                snprintf(stext_ptr, stext_maxlen,
                         "CLASSIFY succeeds; (alt.osb) success probability: "
                         "%6.4f  pR: %6.4f\n", tprob, overall_pR);
            }
        }
        else
        {
            // if a pR offset was given, print it together with the real pR
            if (oslen > 0)
            {
                snprintf(stext_ptr, stext_maxlen,
                         "CLASSIFY fails; (alt.osb) success probability: "
                         "%6.4f  pR: %6.4f / %6.4f\n",
                         tprob, overall_pR, pR_offset);
            }
            else
            {
                snprintf(stext_ptr, stext_maxlen,
                         "CLASSIFY fails; (alt.osb) success probability: "
                         "%6.4f  pR: %6.4f\n", tprob, overall_pR);
            }
        }
        stext_ptr[stext_maxlen - 1] = 0;
        stext_maxlen -= (int)strlen(stext_ptr);
        stext_ptr += strlen(stext_ptr);

        crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER_PARAMS, 8, "dd", tprob, overall_pR);

        bestseen = 0;
        for (m = 0; m < maxhash; m++)
        {
            if (ptc[m] > ptc[bestseen])
            {
                bestseen = m;
            }
        }

        remainder = 0.0;
        for (m = 0; m < maxhash; m++)
        {
            if (bestseen != m)
            {
                remainder += ptc[m];
            }
        }
        if (remainder < 10 * DBL_MIN)
            remainder = 10 * DBL_MIN;
        snprintf(stext_ptr, stext_maxlen, "Best match to file #%d (%s) "
                                          "prob: %6.4f  pR: %6.4f\n",
                 bestseen,
                 hashname[bestseen],
                 ptc[bestseen],
                 (log10(ptc[bestseen]) - log10(remainder)));
        stext_ptr[stext_maxlen - 1] = 0;
        stext_maxlen -= (int)strlen(stext_ptr);
        stext_ptr += strlen(stext_ptr);

        snprintf(stext_ptr, stext_maxlen, "Total features in input file: %d\n", unk_features);
        stext_ptr[stext_maxlen - 1] = 0;
        stext_maxlen -= (int)strlen(stext_ptr);
        stext_ptr += strlen(stext_ptr);

        for (m = 0; m < maxhash; m++)
        {
            int k;
            remainder = 0.0; // 10 * DBL_MIN;
            for (k = 0; k < maxhash; k++)
            {
                if (k != m)
                {
                    remainder += ptc[k];
                }
            }
            if (remainder < 10 * DBL_MIN)
                remainder = 10 * DBL_MIN;

            if (use_chisquared)
            {
                snprintf(stext_ptr, stext_maxlen,
                         "#%d (%s):"
                         " features: %d, hits: %d,"                // exp: %d,"
                         " chi2: %3.2e, pR: %6.2f\n",
                         m,
                         hashname[m],
                         (int)info_block[m]->v.OSB_Bayes.features_learned,
                         (int)totalhits[m],
                         chi2[m],
                         (log10(ptc[m]) - log10(remainder)));
                stext_ptr[stext_maxlen - 1] = 0;
                stext_maxlen -= (int)strlen(stext_ptr);
                stext_ptr += strlen(stext_ptr);
            }
            else
            {
                snprintf(stext_ptr, stext_maxlen,
                         "#%d (%s):"
                         " features: %d, hits: %d, prob: %3.2e, pR: %6.2f\n",
                         m,
                         hashname[m],
                         (int)info_block[m]->v.OSB_Bayes.features_learned,
                         (int)totalhits[m],
                         ptc[m],
                         (log10(ptc[m]) - log10(remainder)));
                stext_ptr[stext_maxlen - 1] = 0;
                stext_maxlen -= (int)strlen(stext_ptr);
                stext_ptr += strlen(stext_ptr);
            }
        }
    }

    // check here if we got enough room in stext to stuff everything
    // perhaps we'd better rise a nonfatalerror, instead of just
    // whining on stderr
    if (stext_maxlen <= 1)
    {
        nonfatalerror("WARNING: not enough room in the buffer to create "
                      "the statistics text.  Perhaps you could try bigger "
                      "values for MAX_CLASSIFIERS or MAX_FILE_NAME_LEN?",
                      " ");
    }
    if (svlen > 0)
    {
        crm_destructive_alter_nvariable(svrbl, svlen, stext, (int)strlen(stext), csl->calldepth);
    }



    //  cleanup time!
    //  remember to let go of the fd's and mmaps
    {
        int k;

        for (k = 0; k < maxhash; k++)
        {
            //      close (hfds [k]);
            if (hashes[k])
            {
                crm_munmap_file(hashes[k]);
            }
            //   and let go of the seen_features array
            if (seen_features[k])
                free(seen_features[k]);
            seen_features[k] = NULL;

            //
            //  Free the hashnames, to avoid a memory leak.
            //
            if (hashname[k])
            {
                free(hashname[k]);
            }
        }
    }

    if (hashpipe)
    {
        free(hashpipe);
    }
    if (feature_weight)
    {
        free(feature_weight);
    }
    if (order_no)
    {
        free(order_no);
    }


    if (tprob <= min_success)
    {
        if (user_trace)
        {
            fprintf(stderr, "CLASSIFY was a FAIL, skipping forward.\n");
        }
        //    and do what we do for a FAIL here
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
        csl->next_stmt_due_to_fail = csl->mct[csl->cstmt]->fail_index;
#else
        csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
#endif
        if (internal_trace)
        {
            fprintf(stderr, "CLASSIFY.OSB.BAYES.ALT is jumping to statement line: %d/%d\n", csl->mct[csl->cstmt]->fail_index, csl->nstmts);
        }
        CRM_ASSERT(csl->cstmt >= 0);
        CRM_ASSERT(csl->cstmt <= csl->nstmts);
        csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;
        return 0;
    }

    //
    //   all done... if we got here, we should just continue execution
    if (user_trace)
        fprintf(stderr, "CLASSIFY was a SUCCESS, continuing execution.\n");
    return 0;
}

#else /* CRM_WITHOUT_OSB_BAYES */

int crm_expr_alt_osb_bayes_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
                            "ERROR: the %s classifier has not been incorporated in this CRM114 build.\n"
                            "You may want to run 'crm -v' to see which classifiers are available.\n",
                            "OSB-Bayes");
}


int crm_expr_alt_osb_bayes_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
                            "ERROR: the %s classifier has not been incorporated in this CRM114 build.\n"
                            "You may want to run 'crm -v' to see which classifiers are available.\n",
                            "OSB-Bayes");
}

#endif /* CRM_WITHOUT_OSB_BAYES */




int crm_expr_alt_osb_bayes_css_merge(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
                            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
                            "You may want to run 'crm -v' to see which classifiers are available.\n",
                            "OSB-Bayes");
}


int crm_expr_alt_osb_bayes_css_diff(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
                            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
                            "You may want to run 'crm -v' to see which classifiers are available.\n",
                            "OSB-Bayes");
}


int crm_expr_alt_osb_bayes_css_backup(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
                            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
                            "You may want to run 'crm -v' to see which classifiers are available.\n",
                            "OSB-Bayes");
}


int crm_expr_alt_osb_bayes_css_restore(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
                            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
                            "You may want to run 'crm -v' to see which classifiers are available.\n",
                            "OSB-Bayes");
}


int crm_expr_alt_osb_bayes_css_info(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
                            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
                            "You may want to run 'crm -v' to see which classifiers are available.\n",
                            "OSB-Bayes");
}


int crm_expr_alt_osb_bayes_css_analyze(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
                            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
                            "You may want to run 'crm -v' to see which classifiers are available.\n",
                            "OSB-Bayes");
}


int crm_expr_alt_osb_bayes_css_create(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
                            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
                            "You may want to run 'crm -v' to see which classifiers are available.\n",
                            "OSB-Bayes");
}


int crm_expr_alt_osb_bayes_css_migrate(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        char *txtptr, int txtstart, int txtlen)
{
    return nonfatalerror_ex(SRC_LOC(),
                            "ERROR: the %s classifier tools have not been incorporated in this CRM114 build.\n"
                            "You may want to run 'crm -v' to see which classifiers are available.\n",
                            "OSB-Bayes");
}



static int collect_obj_bayes_statistics(const char *cssfile,
        FEATUREBUCKET_TYPE                         *hashes,
        int hfsize,                                                          /* unit: number of features! */
        CRM_DECODED_PORTA_HEADER_INFO              *header,
        int histbins,
        FILE                                       *report_out)
{
    int i;
    int *bcounts;
    int docs_learned;
    int features_learned;
    double avg_datums_per_bucket;
    double avg_ovchain_length;
    double avg_pack_density;

    int64_t sum = 0;            // summed stored weights
    int maxchain = 0;           // absolute maximum chain length
    int curchain = 0;
    int totchain = 0;       // summed chain lengths for average calc
    int fbuckets = 0;       // number of occupied feature buckets
    int nchains = 0;        // number of chains
    int zvbins = 0;         // number of zeroed, yet occupied feature bins
    int ofbins = 0;         // number of feature bins which have an overflowing weight
    int specials = 1;       // number of 'special markers' in the hash table; we count the header as one of them.
    int specials_in_chains = 0;
    int brief = 0;

    if (!header || !hashes || !cssfile || !report_out || !hfsize)
        return -1;

    docs_learned = header->binary_section.classifier_info.v.OSB_Bayes.learncount;
    features_learned = header->binary_section.classifier_info.v.OSB_Bayes.features_learned;

    if (histbins == 0)
    {
        brief = 1;
        histbins = 3;
    }

    if (histbins <= 2)
        return -1;

    // set up histogram
    bcounts = calloc(histbins, sizeof(bcounts[0]));
    histbins--;

    //   calculate maximum overflow chain length; skip header feature at index 0
    for (i = 1; i < hfsize; i++)
    {
        if (hashes[i].hash != 0)
        {
            if (hashes[i].key != 0)
            {
                //  only count the non-special buckets for feature count
                sum += hashes[i].value;

                if (hashes[i].value < histbins)
                {
                    bcounts[hashes[i].value]++;
                }
                else
                {
                    bcounts[histbins]++;                     // note that bcounts is len(histbins+1)
                }

                fbuckets++;
                curchain++;
                if (hashes[i].value == 0)
                {
                    zvbins++;
                }
                if (hashes[i].value >= FEATUREBUCKET_VALUE_MAX)
                {
                    ofbins++;
                }
            }
            else
            {
                // hash != 0, key == 0: special!
                specials++;
                if (curchain > 0)
                {
                    specials_in_chains++;
                    curchain++;
                }
            }
        }
        else
        {
            if (curchain > 0)
            {
                nchains++;
                totchain += curchain;
                if (curchain > maxchain)
                {
                    maxchain = curchain;
                }
                curchain = 0;
            }
        }
    }
    histbins++;     // restore real bcounts[] size now.

    avg_datums_per_bucket = ((fbuckets > 0) ? sum / (fbuckets * 1.0) : 0);
    avg_ovchain_length = ((nchains > 0) ? totchain / (nchains * 1.0) : 0);
    avg_pack_density = (fbuckets + specials) / (hfsize * 1.0);

    if (user_trace)
    {
        fprintf(report_out, "\n"
                            "Sparse spectra file %s statistics:\n",
                cssfile);
        fprintf(report_out, "Total available buckets          : %12d\n",
                hfsize);
        fprintf(report_out, "Total buckets in use             : %12d\n",
                fbuckets);
        fprintf(report_out, "Total in-use zero-count buckets  : %12d\n",
                zvbins);
        fprintf(report_out, "Total buckets with value >= max  : %12d\n",
                ofbins);
        fprintf(report_out, "Total hashed datums in file      : %12lld\n",
                (long long int)sum);
        fprintf(report_out, "Documents learned                : %12d\n",
                docs_learned);
        fprintf(report_out, "Features learned                 : %12d\n",
                features_learned);
        fprintf(report_out, "Average datums per bucket        : %12.2f\n",
                avg_datums_per_bucket);
        fprintf(report_out, "Maximum length of overflow chain : %12d\n",
                maxchain);
        fprintf(report_out, "Average length of overflow chain : %12.2f\n",
                avg_ovchain_length);
        fprintf(report_out, "Average packing density          : %12.2f\n",
                avg_pack_density);
        fprintf(report_out, "Number of special slots          : %12d\n",
                specials);
        fprintf(report_out, "# Special slots in a chain       : %12d\n",
                specials_in_chains);

        if (!brief)
        {
            fprintf(report_out, "\n"
                                "Histogram:\n");
            for (i = 0; i < histbins; i++)
            {
                if (bcounts[i] > 0)
                {
                    if (i < histbins)
                    {
                        fprintf(report_out, "bin value %8d found %9d times\n",
                                i, bcounts[i]);
                    }
                    else
                    {
                        fprintf(report_out, "bin value %8d or more found %9d times\n",
                                i, bcounts[i]);
                    }
                }
            }
        }
        fprintf(report_out, "--- end ---\n");
    }

    crm_analysis_mark(&analysis_cfg, MARK_CSS_STATS_GROUP, 1, "iii", hfsize, fbuckets, zvbins);
    crm_analysis_mark(&analysis_cfg, MARK_CSS_STATS_GROUP, 2, "iii", ofbins, docs_learned, features_learned);
    crm_analysis_mark(&analysis_cfg, MARK_CSS_STATS_GROUP, 3, "Ldd", (long long int)sum, avg_datums_per_bucket, avg_pack_density);
    crm_analysis_mark(&analysis_cfg, MARK_CSS_STATS_GROUP, 4, "idi", maxchain, avg_ovchain_length, specials);
    crm_analysis_mark(&analysis_cfg, MARK_CSS_STATS_GROUP, 5, "i", specials_in_chains);

    // write histogram data: pack it in the markers: as we know the 'extra' value can carry 48 bits and
    // histograms are shorter than 16-bit (65536) items or less anyhow, we can pack 3 indices in the 'extra'
    // and their values in the args...
    {
        uint64_t ev = 0;
        int val[3];
        int vc = 0;
        for (i = 0; i < histbins; i++)
        {
            if (bcounts[i] > 0)
            {
                val[vc++] = bcounts[i];
                if (vc == 1)
                {
                    ev = i;
                }
                else
                {
                    ev <<= 16;
                    ev |= (i & 0xFFFFU);
                }
                if (vc == 3)
                {
                    crm_analysis_mark(&analysis_cfg, MARK_CSS_STATS_HISTOGRAM, ev, "iii", val[0], val[1], val[2]);
                    vc = 0;
                }
            }
        }

        if (vc > 0)
        {
            crm_analysis_mark(&analysis_cfg, MARK_CSS_STATS_HISTOGRAM, ev, (vc == 2 ? "ii" : "i"), val[0], val[1]);
        }
    }
    return 0;
}

