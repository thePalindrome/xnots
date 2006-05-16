all:	rsync xnots.1.txt

xnots.1.txt:	../xnots.1
	groff -mman -Tascii -P -cbu ../xnots.1 > xnots.1.txt

.PHONY: rsync
rsync:
	rsync -rlzvC --delete --delete-excluded --exclude=Makefile --exclude='*.swp' . gi1242@shell.sourceforge.net:/home/groups/x/xn/xnots/htdocs
