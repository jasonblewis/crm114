#! /bin/sh

#
# filter the produced output and reference file: we don't care about the reported feature set
# as that may well differ quite a bit between GerH builds and vanilla CRM114
#
# All we care about are the 3 top copyright lines
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

sed -e 's/\(This is CRM114, version [^ ,]\+\).*\(([^)]\+([^)]\+))\).*/\1 XYZ \2/' < "${reffile}" \
| sed -e '/Copyright/s/200[6-8]/200X/g' \
| sed -e '/Classifiers included in this build/,$d' > "${targetname}.refout"
sed -e 's/\(This is CRM114, version [^ ,]\+\).*\(([^)]\+([^)]\+))\).*/\1 XYZ \2/' < "${testfile}" \
| sed -e '/Copyright/s/200[6-8]/200X/g' \
| sed -e '/Classifiers included in this build/,$d' > "${targetname}.tstout"


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




