MD=oxford-deep
TEX=temp

.PHONY: pdf tex clean

pdf: tex
	xelatex $(TEX).tex
	mv $(TEX).pdf $(MD).pdf

tex: $(MD).md
	pandoc --latex-engine=xelatex --template=pandoc-template.tex \
		   -s -f markdown -t latex -o $(TEX).tex $^

clean:
	rm -f $(TEX).*
