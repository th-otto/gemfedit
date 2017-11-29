all: fontdisp

fontdisp: fontdisp.c
	gcc -O2 -Wall fontdisp.c -lX11 -s -o $@

clean::
	rm -f fontdisp
