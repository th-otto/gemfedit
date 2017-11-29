all: fontdisp.exe

fontdisp.exe: fontdisp.c
	i686-pc-mingw32-gcc -mwin32 -O2 -Wall $< -lgdi32 -lcomdlg32 -s -o $@

clean::
	rm -f fontdisp.exe
