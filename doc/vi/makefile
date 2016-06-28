all: viin.pdf viapp.pdf visummary.pdf
viin.pdf: vi.in
	tbl vi.in | troff -ms | dpost | ps2pdf - $@
viapp.pdf: vi.chars
	tbl vi.chars | troff -ms -n22 | dpost | ps2pdf - $@
visummary.pdf: vi.summary
	tbl vi.summary | troff -ms | dpost | ps2pdf - $@
clean:
	rm -f *.pdf
