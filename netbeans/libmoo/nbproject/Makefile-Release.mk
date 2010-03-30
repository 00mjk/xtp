#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_CONF=Release
CND_DISTDIR=dist

# Include project Makefile
include Makefile_nb

# Object Directory
OBJECTDIR=build/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo/basis_set.o \
	${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo/fock.o \
	${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo/orbitals.o \
	${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo/charges.o \
	${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo/units.o \
	${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo/crgunittype.o \
	${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo/crgunit.o \
	${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo/mol_and_orb.o \
	${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo/jcalc.o

# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	${MAKE}  -f nbproject/Makefile-Release.mk ../../src/libmoo/libmoo.a

../../src/libmoo/libmoo.a: ${OBJECTFILES}
	${MKDIR} -p ../../src/libmoo
	${RM} ../../src/libmoo/libmoo.a
	${AR} rv ../../src/libmoo/libmoo.a ${OBJECTFILES} 
	$(RANLIB) ../../src/libmoo/libmoo.a

${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo/basis_set.o: nbproject/Makefile-${CND_CONF}.mk ../../src/libmoo/basis_set.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo
	${RM} $@.d
	$(COMPILE.cc) -O3 -I../../include -I../../../include -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo/basis_set.o ../../src/libmoo/basis_set.cpp

${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo/fock.o: nbproject/Makefile-${CND_CONF}.mk ../../src/libmoo/fock.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo
	${RM} $@.d
	$(COMPILE.cc) -O3 -I../../include -I../../../include -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo/fock.o ../../src/libmoo/fock.cc

${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo/orbitals.o: nbproject/Makefile-${CND_CONF}.mk ../../src/libmoo/orbitals.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo
	${RM} $@.d
	$(COMPILE.cc) -O3 -I../../include -I../../../include -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo/orbitals.o ../../src/libmoo/orbitals.cpp

${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo/charges.o: nbproject/Makefile-${CND_CONF}.mk ../../src/libmoo/charges.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo
	${RM} $@.d
	$(COMPILE.cc) -O3 -I../../include -I../../../include -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo/charges.o ../../src/libmoo/charges.cpp

${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo/units.o: nbproject/Makefile-${CND_CONF}.mk ../../src/libmoo/units.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo
	${RM} $@.d
	$(COMPILE.cc) -O3 -I../../include -I../../../include -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo/units.o ../../src/libmoo/units.cc

${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo/crgunittype.o: nbproject/Makefile-${CND_CONF}.mk ../../src/libmoo/crgunittype.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo
	${RM} $@.d
	$(COMPILE.cc) -O3 -I../../include -I../../../include -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo/crgunittype.o ../../src/libmoo/crgunittype.cc

${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo/crgunit.o: nbproject/Makefile-${CND_CONF}.mk ../../src/libmoo/crgunit.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo
	${RM} $@.d
	$(COMPILE.cc) -O3 -I../../include -I../../../include -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo/crgunit.o ../../src/libmoo/crgunit.cc

${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo/mol_and_orb.o: nbproject/Makefile-${CND_CONF}.mk ../../src/libmoo/mol_and_orb.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo
	${RM} $@.d
	$(COMPILE.cc) -O3 -I../../include -I../../../include -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo/mol_and_orb.o ../../src/libmoo/mol_and_orb.cpp

${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo/jcalc.o: nbproject/Makefile-${CND_CONF}.mk ../../src/libmoo/jcalc.cc 
	${MKDIR} -p ${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo
	${RM} $@.d
	$(COMPILE.cc) -O3 -I../../include -I../../../include -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/_DOTDOT/_DOTDOT/src/libmoo/jcalc.o ../../src/libmoo/jcalc.cc

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf:
	${RM} -r build/Release
	${RM} ../../src/libmoo/libmoo.a

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
