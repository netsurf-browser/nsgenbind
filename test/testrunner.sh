#!/bin/sh

outline() {
echo >>${LOGFILE}
echo "-----------------------------------------------------------"  >>${LOGFILE}
echo >>${LOGFILE}
}

BUILDDIR=$1
TESTDIR=$2

# locations
LOGFILE=${BUILDDIR}/testlog

GENJSBIND=${BUILDDIR}/genjsbind

BINDINGDIR=${TESTDIR}/data/bindings
BINDINGTESTS=$(ls ${BINDINGDIR}/*.bnd)

IDLDIR=${TESTDIR}/data/idl

echo "$*" >${LOGFILE}

for TEST in ${BINDINGTESTS};do

  TESTNAME=$(basename ${TEST} .bnd)

  echo -n "    TEST: ${TESTNAME}......"
  outline

  echo  ${GENJSBIND} -d -v -I ${IDLDIR} -o ${BUILDDIR}/test_${TESTNAME}.c ${TEST} >>${LOGFILE} 2>&1   

  ${GENJSBIND} -d -v -I ${IDLDIR} -o ${BUILDDIR}/test_${TESTNAME}.c ${TEST} >>${LOGFILE} 2>&1

  if [ $? -eq 0 ]; then
    echo "PASS"
  else
    echo "FAIL"
  fi
  

done

