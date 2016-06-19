all: viin.pdf viapp.pdf visummary.pdf
viin.pdf:
	tbl vi.in | troff -ms | dpost | ps2pdf - $@
viapp.pdf:
	tbl vi.chars | troff -ms -n22 | dpost | ps2pdf - $@
visummary.pdf:
	tbl vi.summary | troff -ms | dpost | ps2pdf - $@
clean:
	rm -f *.pdf
