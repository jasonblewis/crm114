#! /bin/sh

#
# filter the produced output and reference file: 
# handle slight alterations to error reports in GerH versus BillY vanilla:
#

# cmdline: filter <expected> <testresult>

reffile="$1"
testfile="$2"
difffile="$3"
targetname="$4"

BUILDDIR=.
SRCDIR=.
REFDIR="${SRCDIR}/ref"

rm -f "${difffile}"

rm -f "${targetname}.refout" 
rm -f "${targetname}.tstout" 

if [ ! -f "${reffile}" ] || [ ! -f "${testfile}" ]; then
  echo "One of the files to compare is missing: '${reffile}' -- '${testfile}'"
  exit 66
fi



#
# vanilla BillY sometimes prints 'function' instead of 'in routine:', hence the double check there on the REF
# input
#

sed -n -e 's/This happened at line \([0-9]\+\) of file \([^ :]\+\):\?/ERR 1: This happened at line \1 of file \2:/' \
    -e 's/.*crm114: \*\([A-Z]\+\)\*/ERR 3: crm114: *\1*/' \
    -e '/ERR 1/{' \
    -e ':a' \
    -e 'p;n;' \
    -e '/runtime system location:/!d' \
    -e '/runtime system location:/!ba' \
    -e '}' \
    -e 's/(runtime system location: \([^ (]\+\)(\([0-9]\+\)) in routine: \([^ )]\+\)).*/ERR 2: (runtime system location: \1(LINENO) in routine: \3)/' \
    -e 's/(runtime system location: \([^ (]\+\)(\([0-9]\+\)) function \([^ )]\+\)).*/ERR 2: (runtime system location: \1(LINENO) in routine: \3)/' \
    -e 'p' \
    < "${reffile}" \
| sed -e 's/For some reason, I was unable to \([a-z]\+\)-open the file named '"'"'\?\([^'"'"']\+\)'"'"'\?.*/For some reason, I was unable to \1-open the file named "\2"/' \
    > "${targetname}.refout"
sed -n -e 's/This happened at line \([0-9]\+\) of file \([^ :]\+\):\?/ERR 1: This happened at line \1 of file \2:/'  \
    -e 's/.*crm114: \*\([A-Z]\+\)\*/ERR 3: crm114: *\1*/' \
    -e '/ERR 1/{' \
    -e ':a' \
    -e 'p;n;' \
    -e '/runtime system location:/!d' \
    -e '/runtime system location:/!ba' \
    -e 'x;}' \
    -e 's/(runtime system location: \([^ (]\+\)(\([0-9]\+\)) in routine: \([^ )]\+\)).*/ERR 2: (runtime system location: \1(LINENO) in routine: \3)/' \
    -e 'p' \
    < "${testfile}" \
| sed -e 's/For some reason, I was unable to \([a-z]\+\)-open the file named '"'"'\?\([^'"'"']\+\)'"'"'\?.*/For some reason, I was unable to \1-open the file named "\2"/' \
    > "${targetname}.tstout"


if [ ! -z "${CRM114_MAKE_SCRIPTS_DEBUG}" ]; then
  echo "------------------------------------------------------"
  cat "${targetname}.refout"
  echo "------------------------------------------------------"
  cat "${targetname}.tstout"
  echo "------------------------------------------------------"
fi

/usr/bin/diff -u -EbwBd --strip-trailing-cr "${targetname}.refout" "${targetname}.tstout" > "${difffile}"
retcode=$?

exit ${retcode}




