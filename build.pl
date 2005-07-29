#!/usr/bin/env perl -w

#####
# @author Nick Kidd
# build.pl
#   - writes a Makefile
#   - forks("make clean")
#   - forks("make -j2")
#####
use strict;
use warnings;
use diagnostics;
use Carp();
local $SIG{__WARN__} = \&Carp::cluck;
use File::Basename;
use Cwd;

# Set PATH here b/c we fork make later
$ENV{'PATH'}        = '/bin:/usr/bin:/usr/local/bin';

# Check for WALi_HOME || WALI_HOME
my $TOPDIR;
$TOPDIR             = &Cwd::cwd() unless( $TOPDIR = $ENV{'WALi_HOME'} );

# Vars that depend on WALi_HOME
my $EDIR            = "$TOPDIR/Examples";
my $TDIR            = "$TOPDIR/Tests";
my $OBJDIR          = "$TOPDIR/objects";
my $LIBDIR          = "$TOPDIR/lib";
my $SRCDIR          = "$TOPDIR/Source";
my $WALiDIR         = "$SRCDIR/wali";
my $WFADIR          = "$WALiDIR/wfa";
my $WPDSDIR         = "$WALiDIR/wpds";
my $EWPDSDIR        = "$WPDSDIR/ewpds";
my $WITNESSDIR      = "$WALiDIR/witness";
my $INCS            = "-I$SRCDIR";
my $LIBS            = "";

# Vars not dependent on WALi_HOME
my $CXX             = "g++";
my $CC              = "gcc";
my $OBJSFX          = ".lo";
my $CXXSFX          = ".cpp";
my $CFLAGS          = "-g -Wall -Wformat=2 -W";
my $CPPFLAGS        = "";
my $LDFLAGS         = "";
my $LIBWALi         = "libwali.la";

# Check for GCC version to be used
if( my $val = $ENV{'GCC_HOME'} ) {
    $CXX = "$val/bin/g++";
    $CC  = "$val/bin/gcc";
}
else {
    print STDERR "ENV{'GCC_HOME'} not defined. Defauling to \"$CXX\" & \"$CC\"\n";
}

# Preprend libtool
$CXX = "libtool " . $CXX;
$CC  = "libtool " . $CC;

my @WALi_FILES = (
# namespace wali
    "$WALiDIR/Common$CXXSFX"
    , "$WALiDIR/Markable$CXXSFX"
    , "$WALiDIR/Printable$CXXSFX"
    , "$WALiDIR/SemElem$CXXSFX"
    , "$WALiDIR/SemElemPair$CXXSFX"
    , "$WALiDIR/Worklist$CXXSFX"
    , "$WALiDIR/Key$CXXSFX"
    , "$WALiDIR/KeySpace$CXXSFX"
    , "$WALiDIR/IntSource$CXXSFX"
    , "$WALiDIR/StringSource$CXXSFX"
    , "$WALiDIR/KeyPairSource$CXXSFX"
#namespace wali::wfa
    , "$WFADIR/WFA$CXXSFX"
    , "$WFADIR/State$CXXSFX"
    , "$WFADIR/Trans$CXXSFX"
    , "$WFADIR/TransFunctor$CXXSFX"
    , "$WFADIR/WeightMaker$CXXSFX"
#namespace wali::wpds
    , "$WPDSDIR/Config$CXXSFX"
    , "$WPDSDIR/Rule$CXXSFX"
    , "$WPDSDIR/RuleFunctor$CXXSFX"
    , "$WPDSDIR/LinkedTrans$CXXSFX"
    , "$WPDSDIR/WPDS$CXXSFX"
    , "$WPDSDIR/WitnessWPDS$CXXSFX"
#namespace wali::wpds::ewpds
    , "$EWPDSDIR/EWPDS$CXXSFX"
    , "$EWPDSDIR/ERule$CXXSFX"
    , "$EWPDSDIR/MergeFunction$CXXSFX"
#namespace wali::witness
    , "$WITNESSDIR/Witness$CXXSFX"
    , "$WITNESSDIR/WitnessExtend$CXXSFX"
    , "$WITNESSDIR/WitnessCombine$CXXSFX"
    , "$WITNESSDIR/WitnessRule$CXXSFX"
    , "$WITNESSDIR/WitnessTrans$CXXSFX"
    , "$WITNESSDIR/Visitor$CXXSFX"
    , "$WITNESSDIR/VisitorDot$CXXSFX"
);

sub print_obj_files()
{
    foreach my $tmp (@WALi_FILES) {
        my ($name,$path,$suffix) = fileparse($tmp,$CXXSFX);
        print MAKEFILE " $OBJDIR/$name$OBJSFX";
    }
}

#
# Open Makefile
#
open(MAKEFILE,'>', "./Makefile") or die "Can't open Makefile : $!";
print MAKEFILE "# -- DO NOT EDIT --\n";
print MAKEFILE "# -- Generated by $0\n";

#
# Generate preprocessing
#
print MAKEFILE "CC=$CXX\n";
print MAKEFILE "CXX=$CXX\n";
print MAKEFILE "CFLAGS=$CFLAGS\n";
print MAKEFILE "CPPFLAGS=$CPPFLAGS\n";
print MAKEFILE "LDFLAGS=$LDFLAGS\n";
print MAKEFILE "LIBS=$LIBS\n";
print MAKEFILE "\n.SUFFIXES: $CXXSFX $OBJSFX\n";

#
# Generate make commands
#
print MAKEFILE "\n.SILENT:\n\n";

#
# Generate all target defs
#
print MAKEFILE "all: $LIBDIR/$LIBWALi\n\n";

#
# Generate clean
#
print MAKEFILE "\n.PHONY: clean\n";
print MAKEFILE "clean:\n\t rm -rf $LIBDIR/$LIBWALi";
print_obj_files();
print MAKEFILE "\n\t\$(MAKE) -C $TDIR clean\n\n";

#
# Generate test cases
#
print MAKEFILE "\n.PHONY: t\n";
print MAKEFILE "t:\n\t\$(MAKE) all ; \$(MAKE) -C $TDIR clean all\n\n";

#
# Generate C++ source to object file defs
#
print MAKEFILE "\n";
foreach my $tmp (@WALi_FILES) {
    my ($name,$path,$suffix) = fileparse($tmp,$CXXSFX);
    print MAKEFILE "$OBJDIR/$name$OBJSFX: $tmp\n";
    print MAKEFILE "\t\@echo \"Compiling $name$suffix ...\"\n";
    print MAKEFILE "\t$CXX $CFLAGS $CPPFLAGS $INCS -o \$\@ -c \$^\n\n";
}

#
# Generate libwpds++.la def
#
print MAKEFILE "$LIBDIR/$LIBWALi:";
print_obj_files();
print MAKEFILE "\n";
print MAKEFILE "\t\@echo \"Creating $LIBDIR/$LIBWALi...\"\n";
print MAKEFILE "\t$CXX -avoid-version -rpath $LIBDIR -o \$\@ \$\^ $LDFLAGS $LIBS";
print MAKEFILE "\n\n";

#
# make clean ; make -j2 all
#
system("/usr/bin/env make -f $SRCDIR/Makefile clean") && die "system failed: $!";
system("/usr/bin/env make -f $SRCDIR/Makefile all") && die "system failed: $!";
exit(0);
