all: exrm.pdf exsummary.pdf exchanges.pdf edittut.pdf editvindex.pdf
exrm.pdf:
	troff -ms ex.rm | dpost | ps2pdf - $@
exsummary.pdf:
	tbl ex.summary | troff -ms | dpost | ps2pdf - $@
exchanges.pdf:
	troff -ms ex.changes | dpost | ps2pdf - $@
edittut.pdf:
	troff -ms edit.tut | dpost | ps2pdf - $@
# index for versatec is different from the one in edit.tut
# because the fonts are different and entries reference page
# rather than section numbers.  if you have a typesetter
# you should just use the index in edit.tut, and ignore editvindex.
editvindex.pdf:
	troff -ms -n22 edit.vindex | dpost | ps2pdf - $@
