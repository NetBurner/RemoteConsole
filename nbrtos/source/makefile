# Revision: 3.4.0

# Copyright 1998-2024 NetBurner, Inc.  ALL RIGHTS RESERVED

#    Permission is hereby granted to purchasers of NetBurner Hardware to use or
#    modify this computer program for any use as long as the resultant program
#    is only executed on NetBurner provided hardware.

#    No other rights to use this program or its derivatives in part or in
#    whole are granted.

#    It may be possible to license this or other NetBurner software for use on
#    non-NetBurner Hardware. Contact sales@Netburner.com for more information.

#    NetBurner makes no representation or warranties with respect to the
#    performance of this computer program, and specifically disclaims any
#    responsibility for any damages, special or consequential, connected with
#    the use of this program.

# NetBurner
# 16855 W Bernardo Dr
# San Diego, CA 92127
# www.netburner.com


NAME	= nbrtos
TARGET_TYPE = nblib


# Object Files
release_tagdata.cpp: $(NNDK_ROOT)/release_tag
	compfile $(NNDK_ROOT)/release_tag rtag_array rtag_len release_tagdata.cpp

UI_html.cpp: UI.html
	compfile UI.html UI_html_data UI_html_size UI_html.cpp

ROOT_html.cpp: ROOT.html
	compfile ROOT.html ROOT_html_data ROOT_html_size ROOT_html.cpp

RAW_html.cpp: RAW.html
	compfile RAW.html RAW_html_data RAW_html_size RAW_html.cpp

LOGO_gif.cpp: LOGO.gif
	compfile LOGO.gif LOGO_gif_data LOGO_gif_size LOGO_gif.cpp

CONSOLE_html.cpp: console.html
	compfile console.html console_html_data console_html_size console_html.cpp


# Source files
CPP_SRC		+= release_tagdata.cpp UI_html.cpp ROOT_html.cpp RAW_html.cpp LOGO_gif.cpp CONSOLE_html.cpp $(wildcard *.cpp)
C_SRC		+= $(wildcard *.c)
AS_SRC		+= $(wildcard *.s)

# TODO: Support subdirs
# SUBDIRS = $(patsubst %/NBLIBRARY,%,$(wildcard */NBLIBRARY))

CPP_SRC		+= $(wildcard ipv6/*.cpp)


#
# uCos routines require specific optimization
#
#ucos.o : ucos.cpp
#	$(CC) -c $(CXXFLAGS) -O0 $< -o $(@F)



#
# Include options
#
# -include $(NNDK_ROOT)/system/snmp/snmp2.mak
# -include $(NNDK_ROOT)/system/cryptolib/cryptolib2.mak
# -include $(NNDK_ROOT)/system/ipv6/ipv6_2.mak

include $(NNDK_ROOT)/make/boilerplate.mk
