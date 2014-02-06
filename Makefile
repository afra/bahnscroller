
#
# Defines the part type that this project uses.
#
PART=LM4F120H5QR

#
# Set the processor variant.
#
VARIANT=cm4f

#
# The base directory for StellarisWare.
#
ROOT=../stellaris

#
# Include the common make definitions.
#
include ${ROOT}/makedefs

#
# Where to find source files that do not live in this directory.
#
VPATH=
VPATH+=../stellaris/utils

#
# Where to find header files that do not live in the source directory.
#
IPATH=../stellaris

all: ${COMPILER}
all: ${COMPILER}/main.axf

#
# The rule to clean out all the build products.
#
clean:
	@rm -rf ${COMPILER} ${wildcard *~}

#
# The rule to create the target directory.
#
${COMPILER}:
	@mkdir -p ${COMPILER}

${COMPILER}/main.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/main.axf: ${COMPILER}/uartstdio.o
${COMPILER}/main.axf: ${COMPILER}/main.o
${COMPILER}/main.axf: ${COMPILER}/ustdlib.o
${COMPILER}/main.axf: ${ROOT}/driverlib/${COMPILER}-cm4f/libdriver-cm4f.a
${COMPILER}/main.axf: main.ld
SCATTERgcc_main=main.ld
ENTRY_main=ResetISR
CFLAGSgcc=-DTARGET_IS_BLIZZARD_RA1 -DUART_BUFFERED

#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
