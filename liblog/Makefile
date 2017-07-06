
CROSS_PREFIX    ?=
CC  = $(CROSS_PREFIX)gcc
CXX = $(CROSS_PREFIX)g++
LD  = $(CROSS_PREFIX)ld
AR  = $(CROSS_PREFIX)ar


LIBNAME     = liblog
TGT_LIB_H   = $(LIBNAME).h
TGT_LIB_A   = $(LIBNAME).a
TGT_LIB_SO  = $(LIBNAME).so
TGT_UNIT_TEST   = test_$(LIBNAME)
TGT_LIB_SO_VER  = $(LIBNAME).so.1.0.0

OBJS_LIB    = $(LIBNAME).o
OBJS_UNIT_TEST  = test_$(LIBNAME).o

CFLAGS  := -g -Wall -Werror -fPIC 
SHARED  := -shared
LDFLAGS = -lpthread


TGT := $(TGT_LIB_A)
TGT += $(TGT_LIB_SO)
TGT += $(TGT_UNIT_TEST)

OBJS    := $(OBJS_LIB) $(OBJS_UNIT_TEST)

all: $(TGT)

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@ 

$(TGT_LIB_A): $(OBJS_LIB)
	$(AR) rcs $@ $^

$(TGT_LIB_SO): $(OBJS_LIB)
	$(CC) -o $@ $^ $(SHARED)
	@mv $(TGT_LIB_SO) $(TGT_LIB_SO_VER)
	@ln -sf $(TGT_LIB_SO_VER) $(TGT_LIB_SO)

$(TGT_UNIT_TEST): $(OBJS_UNIT_TEST) $(ANDROID_MAIN_OBJ)
	$(CC) -o $@ $^ $(TGT_LIB_A) $(LDFLAGS)

clean:                                                    
	$(RM) -f $(OBJS)           
	$(RM) -f $(TGT)
	$(RM) -f $(TGT_LIB_SO)*
	$(RM) -f *.log
