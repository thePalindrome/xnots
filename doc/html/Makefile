all:	xnots.1.txt rsync 

xnots.1.txt:	../xnots.1
	groff -mman -Tascii -P -cbu ../xnots.1 > xnots.1.txt

.PHONY: rsync
rsync:
	rsync -rLzvC --delete --delete-excluded --exclude=Makefile --exclude='*.swp' --exclude='.svn/' ./ gi1242,xnots@shell.sourceforge.net:/home/project-web/xnots/htdocs/
