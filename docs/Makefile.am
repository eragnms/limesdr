if HAVE_DOXYGEN
directory = $(top_srcdir)/docs

$(directory): doxyfile.stamp

#cd latex && $(MAKE), removed
doxyfile.stamp: Doxyfile Doxyfile.in overview.dox
	$(DOXYGEN) $^
	echo Timestamp > $@

all-local: doxyfile.stamp

clean-local:
	rm -rf $(top_srcdir)/libcsc/doc/apidocs
	rm -rf $(top_srcdir)/libcsc/doc/latex
	rm doxyfile.stamp

endif
