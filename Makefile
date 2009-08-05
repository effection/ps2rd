all:
	make -C iop
	bin2o -n iop/dev9/bin/ps2dev9.irx ee/loader/ps2dev9_irx.o _ps2dev9_irx
	bin2o -n iop/smap/ps2smap.irx ee/loader/ps2smap_irx.o _ps2smap_irx
	bin2o -n $(PS2SDK)/iop/irx/ps2ip.irx ee/loader/ps2ip_irx.o _ps2ip_irx
	make -C ee

clean:
	make -C ee clean
	rm -f ee/loader/*_irx.o
	make -C iop clean

rebuild: clean all
