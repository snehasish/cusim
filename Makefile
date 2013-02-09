SIM_HOME = .
BINDIR=$(SIM_HOME)/bin
OBJDIR=$(SIM_HOME)/obj
SRCDIR=$(SIM_HOME)/src
VPATH=$(SIM_HOME)
INCDIR= -I$(SIM_HOME)/gzstream

CC = g++
#CWARN = -W -Wall -Wshadow -Wimplicit -Wreturn-type -Wcomment -Wtrigraphs -Wformat -Wparentheses -Wpointer-arith -Wuninitialized -O
CWARD =  
CDBG = -g $(CWARN) -fno-inline 
CFLAGS = $(INCDIR) -std=gnu++0x -g -fno-inline 
DFLAGS = $(INCDIR) -g $(CWARN) -fno-inline 
LDFLAGS = -L$(SIM_HOME)/gzstream -lgzstream -lz -lpthread


CTAG = etags
CTAGFILE = filelist
# src, object and bin files
TGT = ideal
#Debug Target
DBGTGT=ideal-dbg


COMMONOBJS = $(OBJDIR)/memblock.o $(OBJDIR)/cacheblock.o $(OBJDIR)/evictionrecord.o $(OBJDIR)/datalogger.o $(OBJDIR)/datahub.o $(OBJDIR)/idealcache.o $(OBJDIR)/cachecontroller.o $(OBJDIR)/predictor.o 

OBJS = $(COMMONOBJS) $(OBJDIR)/idealsim.o


DBGOBJS = $(COMMONOBJS) $(OBJDIR)/idealsim.o


#-- Rules
all: gzstream-lib $(TGT) 
dbg: $(DBGTGT)

gzstream-lib: $(SIM_HOME)/gzstream/libgzstream.a

$(SIM_HOME)/gzstream/libgzstream.a: 
	make -C $(SIM_HOME)/gzstream

$(TGT): $(BINDIR)/$(TGT)
	@echo "$@ uptodate"

$(DBGTGT): $(BINDIR)/$(DBGTGT)
	@echo "$@ uptodate"

$(BINDIR)/$(DBGTGT): $(DBGOBJS)
	$(CC) $(DFLAGS) -o $@ $(DBGOBJS) $(LDFLAGS)

$(OBJDIR)/%.dbg.o: $(SRCDIR)/%.C
	$(CC) $(DFLAGS) -DDEBUG=1 -c -o $@ $?

$(BINDIR)/$(TGT): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)



# more complicated dependency computation, so all prereqs listed
# will also become command-less, prereq-less targets
#   sed:    strip the target (everything before colon)
#   sed:    remove any continuation backslashes
#   fmt -1: list words one per line
#   sed:    strip leading spaces
#   sed:    add trailing colons
# $< detects only dependencies.
# Otherwise it will try to include the header in the
# compilation leading to a linker error.

-include $(OBJS:.o=.d)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CC) $(CFLAGS) -c -o $@ $<
	gcc -MM $(CFLAGS) $? > $(OBJDIR)/$*.d
	mv -f $(OBJDIR)/$*.d $(OBJDIR)/$*.d.tmp
	@sed -e 's|.*:|obj/$*.o:|' < obj/$*.d.tmp > obj/$*.d
	sed -e 's/.*://' -e 's/\\$$//' < obj/$*.d.tmp | fmt -1 | \
	sed -e 's/^ *//' -e 's/$$/:/' >> obj/$*.d
	rm -f $(OBJDIR)/$*.d.tmp


$(OBJDIR)/%.dbg.o: $(SRCDIR)/%.cpp
	$(CC) $(DFLAGS) -c -o $@ $<
	gcc -MM $(CFLAGS) $? > $(OBJDIR)/$*.d
	mv -f $(OBJDIR)/$*.d $(OBJDIR)/$*.d.tmp
	@sed -e 's|.*:|obj/$*.o:|' < obj/$*.d.tmp > obj/$*.d
	sed -e 's/.*://' -e 's/\\$$//' < obj/$*.d.tmp | fmt -1 | \
	sed -e 's/^ *//' -e 's/$$/:/' >> obj/$*.d
	rm -f $(OBJDIR)/$*.d.tmp

.PHONY : clean depend fresh

tag :
	find . -name "*.C" -print -or -name "*.h" -print | xargs etags

clean :
	-rm -f $(OBJDIR)/*.o $(OBJDIR)/*.d $(PARSE_C) $(PARSE_H)
	-rm -f $(SRCDIR)/*.output $(LEX_C)
	-rm -f */*~ *~ core
	-rm -f $(BINDIR)/$(TGT) $(BINDIR)/$(DBGTGT) $(BINDIR)/*.o
	make -C gzstream

fresh : clean all
