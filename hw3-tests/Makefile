# $Id: Makefile,v 1.47 2023/10/20 02:03:45 leavens Exp leavens $
# Makefile for parser and static analysis in COP 3402

# Add .exe to the end of target to get that suffix in the rules
COMPILER = compiler
# Add .exe to the end of target to get that suffix in the rules
LEXER = lexer

# Tools used
CC = gcc
CFLAGS = -g -std=c17 -Wall
YACC = bison
YACCFLAGS = -Wall --locations -d -v
LEX = flex
LEXFLAGS =
# Unix command names
MV = mv
RM = rm -f
SUBMISSIONZIPFILE = submission.zip
ZIP = zip -9
PL0 = pl0
# Add the names of your own files with a .o suffix to link them in the program
# Feel free to edit the following definition of COMPILER_OBJECTS
COMPILER_OBJECTS = $(COMPILER)_main.o $(PL0)_lexer.o \
		parser.o unparser.o id_use.o \
		id_attrs.o ast.o $(PL0).tab.o file_location.o utilities.o \
		scope.o scope_check.o symtab.o  

# If you want to test the lexical analysis part separately,
# then you might want to build the lexer,
# and if so, then add the names of your own .o files for the lexer below
LEXER_OBJECTS = $(LEXER)_main.o $(LEXER).o $(PL0)_lexer.o \
		ast.o $(PL0).tab.o file_location.o utilities.o 

# different kinds of tests
ASTTESTS = hw3-asttest0.pl0 hw3-asttest1.pl0 hw3-asttest2.pl0 \
	hw3-asttest3.pl0 hw3-asttest4.pl0 hw3-asttest5.pl0 \
	hw3-asttest6.pl0 hw3-asttest7.pl0 hw3-asttest8.pl0 \
	hw3-asttest9.pl0 hw3-asttestA.pl0 hw3-asttestB.pl0 \
	hw3-asttestC.pl0 hw3-asttestD.pl0
REGULARTESTS = hw3-test0.pl0 hw3-test1.pl0 hw3-test2.pl0 hw3-test3.pl0 \
	hw3-test4.pl0 hw3-test5.pl0 hw3-test6.pl0 hw3-test7.pl0 \
	hw3-test8.pl0 hw3-test9.pl0 hw3-testA.pl0
ERRTESTS = hw3-errtest0.pl0 hw3-errtest1.pl0 hw3-errtest2.pl0 \
	hw3-errtest3.pl0 hw3-errtest4.pl0 hw3-errtest5.pl0
PARSEERRTESTS = hw3-parseerrtest0.pl0 hw3-parseerrtest1.pl0 hw3-parseerrtest2.pl0 \
	hw3-parseerrtest3.pl0 hw3-parseerrtest4.pl0 hw3-parseerrtest5.pl0 \
	hw3-parseerrtest6.pl0 hw3-parseerrtest7.pl0 hw3-parseerrtest8.pl0
NONDECLTESTS = $(ASTTESTS) $(REGULARTESTS) $(ERRTESTS) $(PARSEERRTESTS)
SCOPETESTS = hw3-scope-test0.pl0 hw3-scope-test1.pl0  hw3-scope-test2.pl0
DECLERRTESTS = hw3-declerrtest0.pl0 hw3-declerrtest1.pl0 hw3-declerrtest2.pl0 \
	hw3-declerrtest3.pl0 hw3-declerrtest4.pl0 hw3-declerrtest5.pl0 \
	hw3-declerrtest6.pl0 hw3-declerrtest7.pl0 hw3-declerrtest8.pl0 \
	hw3-declerrtest9.pl0 hw3-declerrtestA.pl0 hw3-declerrtestB.pl0 \
	hw3-declerrtestC.pl0
DECLTESTS = $(SCOPETESTS) $(DECLERRTESTS)
GOODTESTS = $(ASTTESTS) $(REGULARTESTS) $(SCOPETESTS)
BADTESTS = $(ERRTESTS) $(PARSEERRTESTS) $(DECLERRTESTS)
# ALLTESTS is all of the test files, if you add more tests you can add to this list
ALLTESTS = $(NONDECLTESTS) $(DECLTESTS)
EXPECTEDOUTPUTS = $(ALLTESTS:.pl0=.out)
# STUDENTESTOUTPUTS is all of the .myo files corresponding to the tests
# if you add more tests, you can add more to this list
STUDENTTESTOUTPUTS = $(ALLTESTS:.pl0=.myo)

.DEFAULT: $(COMPILER)
$(COMPILER): $(COMPILER_OBJECTS)
	$(CC) $(CFLAGS) -o $(COMPILER) $(COMPILER_OBJECTS)

$(COMPILER)_main.o: $(COMPILER)_main.c
	$(CC) $(CFLAGS) -c $<

$(PL0).tab.o: $(PL0).tab.c $(PL0).tab.h
	$(CC) $(CFLAGS) -c $<

$(PL0).tab.c $(PL0).tab.h: $(PL0).y ast.h parser_types.h machine_types.h 
	$(YACC) $(YACCFLAGS) $(PL0).y

.PHONY: start-bison-file
start-bison-file:
	@if test -f $(PL0).y; \
        then echo "$(PL0).y already exists, not starting it!" >&2; \
              exit 2 ; \
        fi
	cp bison_$(PL0)_y_top.y $(PL0).y
	chmod u+w $(PL0).y

$(PL0)_lexer.l: $(PL0).tab.h

.PRECIOUS: $(PL0)_lexer.c
$(PL0)_lexer.c: $(PL0)_lexer.l $(PL0).tab.h
	$(LEX) $(LEXFLAGS) $<

$(PL0)_lexer.o: $(PL0)_lexer.c ast.h utilities.h file_location.h
	$(CC) $(CFLAGS) -Wno-unused-function -Wno-unused-but-set-variable -c $(PL0)_lexer.c

$(LEXER): $(LEXER_OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

$(LEXER)_main.o: $(LEXER)_main.c
	$(CC) $(CFLAGS) -c $<

# rule for compiling individual .c files
%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	$(RM) *~ *.o '#'*
	$(RM) $(PL0)_lexer.c $(PL0)_lexer.h
	$(RM) $(PL0).tab.c $(PL0).tab.h $(PL0).output
	$(RM) $(COMPILER).exe $(COMPILER)
	$(RM) *.stackdump core
	$(RM) $(SUBMISSIONZIPFILE)

cleanall: clean
	$(RM) *.myo

.PRECIOUS: %.myo
%.myo: %.pl0 $(COMPILER)
	./$(COMPILER) $< > $@ 2>&1

.PHONY: check-outputs check-nondecl-outputs check-decl-outputs
check-outputs: check-nondecl-outputs check-decl-outputs
	@echo 'Be sure to look for two test summaries above (nondeclaration and declaration tests)'

check-nondecl-outputs: $(COMPILER) $(NONDECLTESTS)
	@DIFFS=0; \
	for f in `echo $(NONDECLTESTS) | sed -e 's/\\.pl0//g'`; \
	do \
		echo running "$$f.pl0"; \
		./$(COMPILER) "$$f.pl0" >"$$f.myo" 2>&1; \
		diff -w -B "$$f.out" "$$f.myo" && echo 'passed!' || DIFFS=1; \
	done; \
	if test 0 = $$DIFFS; \
	then \
		echo 'All nondeclaration tests passed!'; \
	else \
		echo 'Some nondeclaration test(s) failed!'; \
	fi

check-decl-outputs: $(COMPILER) $(DECLTESTS)
	@DIFFS=0; \
	for f in `echo $(DECLTESTS) | sed -e 's/\\.pl0//g'`; \
	do \
		echo running "$$f.pl0"; \
		./$(COMPILER) "$$f.pl0" >"$$f.myo" 2>&1; \
		diff -w -B "$$f.out" "$$f.myo" && echo 'passed!' || DIFFS=1; \
	done; \
	if test 0 = $$DIFFS; \
	then \
		echo 'All declaration checking tests passed!'; \
	else \
		echo 'Some declaration checking test(s) failed!'; \
	fi

check-good-outputs: $(COMPILER) $(GOODTESTS)
	DIFFS=0; \
	for f in `echo $(GOODTESTS) | sed -e 's/\\.pl0//g'`; \
	do \
		$(RM) "$$f.myo" ; \
		./$(COMPILER) $$f.pl0 >"$$f.myo" 2>&1; \
		diff -w -B "$$f.out" "$$f.myo" && echo 'passed!' || DIFFS=1; \
	done; \
	if test 0 = $$DIFFS; \
	then \
		echo 'All tests passed!'; \
	else \
		echo 'Test(s) failed!'; \
	fi

check-bad-outputs: $(COMPILER) $(BADTESTS)
	DIFFS=0; \
	for f in `echo $(BADTESTS) | sed -e 's/\\.pl0//g'`; \
	do \
		$(RM) "$$f.myo" ; \
		./$(COMPILER) $$f.pl0 >"$$f.myo" 2>&1; \
		diff -w -B "$$f.out" "$$f.myo" && echo 'passed!' || DIFFS=1; \
	done; \
	if test 0 = $$DIFFS; \
	then \
		echo 'All tests passed!'; \
	else \
		echo 'Test(s) failed!'; \
	fi

$(SUBMISSIONZIPFILE): *.c *.h $(STUDENTTESTOUTPUTS)
	$(ZIP) $(SUBMISSIONZIPFILE) $(PL0).y $(PL0)_lexer.l *.c *.h Makefile
	$(ZIP) $(SUBMISSIONZIPFILE) $(STUDENTTESTOUTPUTS) $(ALLTESTS) $(EXPECTEDOUTPUTS)

.PRECIOUS: $(SOURCESLIST)
	echo *.c > $(SOURCESLIST)

# developer's section below...

.PRECIOUS: %.out
%.out: %.pl0 $(COMPILER)
	./$(COMPILER) $< > $@ 2>&1

.PHONY: create-outputs
create-outputs: $(COMPILER) $(ALLTESTS)
	@if test '$(IMTHEINSTRUCTOR)' != true ; \
	then \
		echo 'Students should use the target check-outputs,' ; \
		echo 'as using this target (create-outputs) will invalidate the tests' ; \
		exit 1; \
	fi; \
	for f in `echo $(ALLTESTS) | sed -e 's/\\.pl0//g'`; \
	do \
		echo running compiler on "$$f.pl0"; \
		$(RM) "$$f.out"; \
		./$(COMPILER) "$$f.pl0" >"$$f.out" 2>&1; \
	done; \
	echo done creating test outputs!

.PHONY: digest
digest: digest.txt

digest.txt: create-outputs
	for f in `ls $(ALLTESTS) | sed -e 's/\\.pl0//g'`; \
        do cat $$f.pl0; echo " "; cat $$f.out; echo " "; echo " "; \
        done >digest.txt

# don't use develop-clean unless you want to regenerate the expected outputs
.PHONY: develop-clean
develop-clean: clean
	@if test '$(IMTHEINSTRUCTOR)' != true ; \
	then \
		echo 'Students should use the target clean,' ; \
		echo 'as using this target (develop-clean) will invalidate the tests'; \
		exit 1; \
	fi
	$(RM) *.out digest.txt

TESTSZIPFILE = ~/temp/hw3-tests.zip
PROVIDEDFILES = compiler_main.c utilities.[ch] lexer.[ch] unparser.[ch] \
		machine_types.h parser_types.h bison_pl0_y_top.y parser.[ch] \
		ast.[ch] file_location.[ch] id_attrs.[ch] id_use.[ch]

hw3-tests.zip: create-outputs $(TESTSZIPFILE)

$(TESTSZIPFILE): $(ALLTESTS) Makefile $(PROVIDEDFILES)
	$(RM) $(TESTSZIPFILE)
	chmod 444 $(ALLTESTS) $(EXPECTEDOUTPUTS) $(PROVIDEDFILES)
	chmod 744 Makefile
	$(ZIP) $(TESTSZIPFILE) $(ALLTESTS) $(EXPECTEDOUTPUTS) Makefile $(PROVIDEDFILES)

.PHONY: hw3-solution.zip
hw3-solution.zip ~/temp/hw3-solution.zip: Makefile $(PROVIDEDFILES) \
			lexer.[ch] reserved.[ch] parser*.[ch] scope*.[ch] \
			compiler_main.c $(ALLTESTS) sources.txt
	$(MAKE) clean create-outputs
	$(ZIP) ~/temp/hw3-solution.zip $^ $(EXPECTEDOUTPUTS)

.PHONY: check-separately
check-separately: $(COMPILER_OBJECTS)
