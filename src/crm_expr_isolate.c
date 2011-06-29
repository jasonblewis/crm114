//    crm_expr_isolate.c  - isolate a variable (includes mem management)
//  Copyright 2001-2007  William S. Yerazunis, all rights reserved.
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



//           Allow creation of a temporary isolated variable;
//         this lives in the same big buffer as the environ
//           args, the arg0 args, and the basic formatting args
//           (like :_nl:, :_ht:, :_bs:, etc).

int crm_expr_isolate(CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
    char temp_vars[MAX_VARNAME];
    int tvlen;
    int vn_start_here;
    int vstart;
    int vlen;
    int mc;
    int vallen;
    int iso_status;

    if (user_trace)
        fprintf(stderr, "executing an ISOLATE statement\n");


    //    get the list of variable names
    //
    CRM_ASSERT(apb != NULL);
    tvlen = crm_get_pgm_arg(temp_vars, MAX_VARNAME, apb->p1start, apb->p1len);
    tvlen = crm_nexpandvar(temp_vars, tvlen, MAX_VARNAME);
    if (!crm_nextword(temp_vars, tvlen, 0, &vstart, &vlen)
    || vlen < 3)
    {
        nonfatalerror("This statement is missing the variable to isolate"
                      " so I'll just ignore the whole statement.", "");
    }

    if (internal_trace)
		fprintf(stderr, "  creating isolated vars: (len: %d) ***%s***\n", tvlen, temp_vars);

    mc = 0;

    //  Now, find the vars (space-delimited, doncha know) and make them
    //  isolated.
    //
    vstart = 0;
    vlen = 0;
    vn_start_here = 0;
    for (;;)
    {
        if (!crm_nextword(temp_vars, tvlen, vn_start_here, &vstart, &vlen)
			 || vlen == 0)
        {
            break;
        }
        else  // not done yet.
        {
            //        must make a copy of the varname.
            char vname[MAX_VARNAME];
            int vmidx;

			        vn_start_here = vstart + vlen + 1;

		if (vlen >= MAX_VARNAME)
		{
			nonfatalerror_ex(SRC_LOC(), "ISOLATE statement comes with a variable name which is too long (len = %d) "
				"while the maximum allowed size is %d.",
					vlen,
MAX_VARNAME-1);
			vlen = MAX_VARNAME-1;
		}
            memmove(vname, &temp_vars[vstart], vlen);
            vname[vlen] = 0;
            if (vlen < 3)
            {
                nonfatalerror("The variable you're asking me to ISOLATE "
                              " has an utterly bogus name.  I'll ignore"
                              " the rest of the statement", " ");
                break;
            }
            if (strcmp(vname, ":_dw:") == 0)
            {
                nonfatalerror("You can't ISOLATE the :_dw: data window! ",
                    "We'll just ignore that for now");
				continue;
			}
            else  //  OK- isolate this variable
            {
                vmidx = crm_vht_lookup(vht, vname, vlen);
                //
                //     Now, check these cases in order:
                //
                //     not preexisting, no /value/ -  isolate, set to ""
                //     not preexisting, with /value/ -isolate, set /val/
                //     preexisting _dw, with /value/ - isolate, set to /val/
                //     preexisting _dw, no /value/ - isolate, set to dwval.
                //     if preexisting AND default turned on - do nothing!
                //     preexisting isolated, no /value/ - copy value.
                //     preexisting isolated, with /value/ - alter /value/
                //
                //    not preexisting.
                if (vht[vmidx] == NULL)
                {
                    //   not preexisting, no slash value
                    if (internal_trace)
                        fprintf(stderr, "Not a preexisting var.\n");

                    if (!apb->s1start)
                    {
                        // no slash value- set to ""
                        if (internal_trace)
						{
                            fprintf(stderr, "No initialization value given, using"
                                            " a zero-length string.\n");
						}
                        tempbuf[0] = 0;
                        vallen = 0;
                    }
                    else
                    {
                        //  not preexisting, has a /value/, use it.
                        if (internal_trace)
                            fprintf(stderr, "using the slash-value given.\n");
                        vallen = crm_get_pgm_arg(tempbuf, data_window_size, apb->s1start, apb->s1len);
                        vallen = crm_nexpandvar(tempbuf, vallen, data_window_size - tdw->nchars);
                    }
                }
                else                //      it IS preexisting
                {
                    //   It is preexisting, maybe in isolation, maybe
                    //   not but we're isolating again.  So we need to
                    //   copy again.
                    //
                    if (internal_trace)
                        fprintf(stderr, "Preexisting var.\n");
                    if (apb->sflags & CRM_DEFAULT)
                    {
                        if (user_trace)
						{
                            fprintf(stderr, " var exists, default flag on, "
                                            "so no action taken.\n");
						}
                        continue;
                    }

                    if (apb->s1start)
                    {
                        //  yes, statement has a /value/
                        //    get the /value/
                        if (internal_trace)
                            fprintf(stderr, "Using the provided slash-val.\n");
                        vallen = crm_get_pgm_arg(tempbuf, data_window_size, apb->s1start, apb->s1len);
                        vallen = crm_nexpandvar(tempbuf, vallen, data_window_size - tdw->nchars);
                    }
                    else
                    {
                        //     no /value/, so we need to use the old value.
                        //
                        if (internal_trace)
                            fprintf(stderr, "No slash-value, using old value.\n");
						if (data_window_size < vlen + 2)
						{
							                fatalerror("The variable you're asking me to ISOLATE "
                              " has a name that way too long.  I'll try to ignore"
                              " this statement", " ");
								break;
						}

                        strcpy(tempbuf, ":*");
                        memcpy(tempbuf + 2, vname, vlen);
                        vallen = crm_nexpandvar(tempbuf, vlen + 2, data_window_size - tdw->nchars);
                    }
                }
                //
                //     Now we have the name of the variable in vname/vlen,
                //    and the value string in tempbuf/vallen.  We can then
                //    isolate it with crm_isolate_this, which always does
                //    the right thing (pretty much).  :-)
                //
                iso_status = crm_isolate_this(&vmidx,
                    vname, 0, vlen,
                    tempbuf, 0, vallen);
                if (iso_status != 0)
                    return iso_status;
            }
        }
    }
    return 0;
}


//
//      General-purpose routine to "do the right thing" to isolate a
//      string value.  This routine takes care of all of the possible
//      extenuating circumstances, like is/is not already isolated,
//      is/is not requiring a reclaim, etc.  (how it knows?  If the
//      value is in the TDW, it may need to be reclaimed, otherwise it
//      doesn't reclaim.)
//
//      Note - this routine will calloc and then free a spare buffer if
//      it gets handed data that's already in the TDW.  Best not to do
//      that if you can avoid it; that is an efficiency speedup
//
int crm_isolate_this(int *vptr,
                     char *nametext, int namestart, int namelen,
                     char *valuetext, int valuestart, int valuelen)

{
    int is_old;
    int is_old_isolated;
    int vmidx;
    int oldvstart = 0;
    int oldvlen = 0;

    int neededlen;

    if (internal_trace)
    {
        fprintf(stderr, "using crm_isolate_this, vptr = %p\n", (void *)vptr);
    }


    //    keep track of the amount of storage needed for this variable
    //    to be inserted into the isolated space.

    neededlen = 10;
    //
    //        gather information
    //         In particular - does the name already exits?  Is it
    //         already isolated?
    if (vptr)
    {
        vmidx = *vptr;
    }
    else
    {
        vmidx = crm_vht_lookup(vht, &nametext[namestart], namelen);
    }

    //     check the vht - if it's not here, we need to add it.
    //

    if (vht[vmidx] == NULL)
    {
        //  optimization - if it's not old, we don't need to run a reclaim
        //  phase later on.
        is_old = 0;

        //  must allow space for the name.
        neededlen += namelen;
    }
    else
    {
        is_old = 1;
    }

    if (is_old && vht[vmidx]->valtxt == tdw->filetext)
    {
        is_old_isolated = 1;
        //           and save old start and length for the incremental reclaimer
        oldvstart = vht[vmidx]->vstart;
        oldvlen = vht[vmidx]->vlen;

        //  how much space will the new value take up?
        neededlen += valuelen - oldvlen;
    }
    else
    {
        is_old_isolated = 0;
        //  and how much space will we need?
        neededlen += valuelen;
    }

    //     Do a check - is there enough space in the tdw to hold
    //     the new variable (both name and value)?
    if (tdw->nchars + neededlen > data_window_size)
    {
        char vname[129];
        strncpy(vname, &nametext[namestart],
            ((128 < namelen) ? 128 : namelen));
        vname[128] = 0; /* [i_a] boundary bug */
        fatalerror("You have blown the memory-storage gaskets while trying"
                   "to store the ISOLATEd variable ", vname);
        return 1;
    }
    //   If we get to here, there's more than enough space; so we're good to
    //   go with no further checks.


    //     If there wasn't a vht slot existing, make one.
    //
    if (!is_old)
    {
        //  nope, never seen this var before, add it into the VHT
        //       namestart is where we are now.
        int nstart, vstart;
        if (internal_trace)
            fprintf(stderr, "... this is a new isolated var\n");
        //
        //      Put the name into the tdw memory area, add a & after it.
        //
        //       do the name first.  Start on a newline.
        tdw->filetext[tdw->nchars] = '\n';
        tdw->nchars++;
        nstart = tdw->nchars;
        memmove(&tdw->filetext[nstart], &nametext[namestart], namelen);
        tdw->nchars = tdw->nchars + namelen;
        tdw->filetext[tdw->nchars] = '=';
        tdw->nchars++;

        //      and put in the value in now
        vstart = tdw->nchars;
        memmove(&tdw->filetext[vstart], &valuetext[valuestart], valuelen);
        tdw->nchars = tdw->nchars + valuelen;
        tdw->filetext[tdw->nchars] = '\n';
        tdw->nchars++;

        //      now, we whack the actual VHT.
        crm_setvar(NULL, 0,
            tdw->filetext, nstart, namelen,
            tdw->filetext, vstart, valuelen,
            csl->cstmt, 0);
        //     that's it.    It's now in the TDW and in the VHT
        return 0;
    }

    //  No, it's a preexisting variable.  We need to do the shuffle.
    //
    //    Note that this code is almost but not quite a mirror
    //    of the code that lives in crm_set_temp_nvar.
    //
    //
    if (internal_trace)
        fprintf(stderr, "Resetting valtxt to point at tdw.\n");
    vht[vmidx]->valtxt = tdw->filetext;
    if (internal_trace)
        fprintf(stderr, "Fresh start: offset %d length %d.\n",
            tdw->nchars, valuelen);
    //
    //
    //   If we have a zero-length string, followed by a
    //   non-zero-length string, next to each other, with
    //   no intervening allocations, both strings will
    //   have the _same_ start point.  This messes things
    //   up badly on subsequent alters.  Thus, we
    //   _must_ put a spacer in.
    //
    //   This code must also be echoed in crm_set_temp_nvar
    //
    if (valuelen == 0)
    {
        tdw->filetext[tdw->nchars] = '\n';
        tdw->nchars++;
    }
    //            (end of danger zone)
    //
    vht[vmidx]->vstart = tdw->nchars;
    vht[vmidx]->vlen   = valuelen;
    if (internal_trace)
        fprintf(stderr, "Memmoving the value in.\n");
    memmove(&(tdw->filetext[tdw->nchars]),
        tempbuf,
        valuelen);
    tdw->nchars = tdw->nchars + valuelen;
    //
    // trailing separator
    tdw->filetext[tdw->nchars] = '\n';
    tdw->nchars++;
    //
    //      and reset 'previous match' start and length
    if (internal_trace)
        fprintf(stderr, "reset the previous-match to start.\n");
    vht[vmidx]->mstart = vht[vmidx]->vstart;
    vht[vmidx]->mlen   = 0;


    //     Step 2 - if this was isolated, reclaim the
    //     old storage if nobody else is using it.
    //
    if (is_old_isolated)
    {
        if (internal_trace)
            fprintf(stderr, "This was already an isolated var, so "
                            "do a reclamation on the old space.\n");
        //  vstart==0 means "ignore this value" to reclamation
        //
        crm_compress_tdw_section(vht[vmidx]->valtxt,
            oldvstart,
            oldvstart + oldvlen);
    }

    return 0;
}

