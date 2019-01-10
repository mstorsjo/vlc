# generate Direct3D11 temporary include

ifdef HAVE_CROSS_COMPILE
IDL_INCLUDES = -I/usr/include/wine/windows/ -I/usr/include/wine/wine/windows/
else
#ugly way to get the default location of standard idl files
IDL_INCLUDES = -I/`echo $(MSYSTEM) | tr A-Z a-z`/$(BUILD)/include
endif

D3D11_COMMIT_ID := a0cd5afeb60be3be0860e9a203314c10485bb9b8
D3D11_X_COMMIT_ID := cb090affef8ebcd8c13ffc271b4500dea3b52f33
DXGI12_COMMIT_ID := 790a6544347b53c314b9c6f1ea757a2d5504c67e
DXGITYPE_COMMIT_ID := f4aba520d014ecfe3563e33860de001caf2804e2
D3D11_IDL_URL := http://sourceforge.net/p/mingw-w64/mingw-w64/ci/$(D3D11_X_COMMIT_ID)/tree/mingw-w64-headers/direct-x/include/d3d11.idl?format=raw
D3D11_1_H_URL := http://sourceforge.net/p/mingw-w64/mingw-w64/ci/$(D3D11_X_COMMIT_ID)/tree/mingw-w64-headers/direct-x/include/d3d11_1.h?format=raw
D3D11_2_H_URL := http://sourceforge.net/p/mingw-w64/mingw-w64/ci/$(D3D11_X_COMMIT_ID)/tree/mingw-w64-headers/direct-x/include/d3d11_2.h?format=raw
D3D11_3_H_URL := http://sourceforge.net/p/mingw-w64/mingw-w64/ci/$(D3D11_X_COMMIT_ID)/tree/mingw-w64-headers/direct-x/include/d3d11_3.h?format=raw

DXGI12_IDL_URL := http://sourceforge.net/p/mingw-w64/mingw-w64/ci/$(D3D11_X_COMMIT_ID)/tree/mingw-w64-headers/direct-x/include/dxgi1_2.idl?format=raw
DXGITYPE_IDL_URL := http://sourceforge.net/p/mingw-w64/mingw-w64/ci/$(D3D11_X_COMMIT_ID)/tree/mingw-w64-headers/direct-x/include/dxgitype.idl?format=raw
DXGIFORMAT_IDL_URL := http://sourceforge.net/p/mingw-w64/mingw-w64/ci/$(D3D11_X_COMMIT_ID)/tree/mingw-w64-headers/direct-x/include/dxgiformat.idl?format=raw
DXGICOMMON_IDL_URL := http://sourceforge.net/p/mingw-w64/mingw-w64/ci/$(D3D11_X_COMMIT_ID)/tree/mingw-w64-headers/direct-x/include/dxgicommon.idl?format=raw
DXGI_IDL_URL := http://sourceforge.net/p/mingw-w64/mingw-w64/ci/$(D3D11_X_COMMIT_ID)/tree/mingw-w64-headers/direct-x/include/dxgi.idl?format=raw
DXGI12_IDL_URL = http://sourceforge.net/p/mingw-w64/mingw-w64/ci/$(D3D11_X_COMMIT_ID)/tree/mingw-w64-headers/direct-x/include/dxgi1_2.idl?format=raw
DXGI13_IDL_URL = http://sourceforge.net/p/mingw-w64/mingw-w64/ci/$(D3D11_X_COMMIT_ID)/tree/mingw-w64-headers/direct-x/include/dxgi1_3.idl?format=raw
DXGI14_IDL_URL = http://sourceforge.net/p/mingw-w64/mingw-w64/ci/$(D3D11_X_COMMIT_ID)/tree/mingw-w64-headers/direct-x/include/dxgi1_4.idl?format=raw
DXGI15_IDL_URL = http://sourceforge.net/p/mingw-w64/mingw-w64/ci/$(D3D11_X_COMMIT_ID)/tree/mingw-w64-headers/direct-x/include/dxgi1_5.idl?format=raw
DXGI16_IDL_URL = http://sourceforge.net/p/mingw-w64/mingw-w64/ci/$(D3D11_X_COMMIT_ID)/tree/mingw-w64-headers/direct-x/include/dxgi1_6.idl?format=raw

DST_D3D11_H = $(PREFIX)/include/d3d11.h
DST_D3D11_H = $(PREFIX)/include/d3d11.h
DST_DXGIDEBUG_H = $(PREFIX)/include/dxgidebug.h
DST_DXGITYPE_H = $(PREFIX)/include/dxgitype.h
DST_DXGIFORMAT_H = $(PREFIX)/include/dxgiformat.h
DST_DXGICOMMON_H = $(PREFIX)/include/dxgicommon.h
DST_DXGI_IDL = $(PREFIX)/include/dxgi.idl
DST_DXGI_H = $(PREFIX)/include/dxgi.h
DST_DXGI12_H = $(PREFIX)/include/dxgi1_2.h
DST_DXGI13_H = $(PREFIX)/include/dxgi1_3.h
DST_DXGI14_H = $(PREFIX)/include/dxgi1_4.h
DST_DXGI15_H = $(PREFIX)/include/dxgi1_5.h
DST_DXGI16_H = $(PREFIX)/include/dxgi1_6.h
DST_D3D11_1_H = $(PREFIX)/include/d3d11_1.h
DST_D3D11_2_H = $(PREFIX)/include/d3d11_2.h
DST_D3D11_3_H = $(PREFIX)/include/d3d11_3.h

ifdef HAVE_WIN32
PKGS += d3d11
endif

$(TARBALLS)/d3d11.idl:
	$(call download_pkg,$(D3D11_IDL_URL),d3d11)

$(TARBALLS)/d3d11_1.h:
	$(call download_pkg,$(D3D11_1_H_URL),d3d11)

$(TARBALLS)/d3d11_2.h:
	$(call download_pkg,$(D3D11_2_H_URL),d3d11)

$(TARBALLS)/d3d11_3.h:
	$(call download_pkg,$(D3D11_3_H_URL),d3d11)

$(TARBALLS)/dxgidebug.idl:
	(cd $(TARBALLS) && patch -fp1) < $(SRC)/d3d11/dxgidebug.patch

$(TARBALLS)/dxgitype.idl:
	$(call download_pkg,$(DXGITYPE_IDL_URL),d3d11)

$(TARBALLS)/dxgiformat.idl:
	$(call download_pkg,$(DXGIFORMAT_IDL_URL),d3d11)

$(TARBALLS)/dxgicommon.idl:
	$(call download_pkg,$(DXGICOMMON_IDL_URL),d3d11)

$(TARBALLS)/dxgi.idl:
	$(call download_pkg,$(DXGI_IDL_URL),d3d11)

$(TARBALLS)/dxgi1_2.idl:
	$(call download_pkg,$(DXGI12_IDL_URL),d3d11)

$(TARBALLS)/dxgi1_3.idl:
	$(call download_pkg,$(DXGI13_IDL_URL),d3d11)

$(TARBALLS)/dxgi1_4.idl:
	$(call download_pkg,$(DXGI14_IDL_URL),d3d11)

$(TARBALLS)/dxgi1_5.idl:
	$(call download_pkg,$(DXGI15_IDL_URL),d3d11)

$(TARBALLS)/dxgi1_6.idl:
	$(call download_pkg,$(DXGI16_IDL_URL),d3d11)


.sum-d3d11: $(TARBALLS)/d3d11.idl  $(TARBALLS)/d3d11_1.h $(TARBALLS)/d3d11_2.h $(TARBALLS)/d3d11_3.h $(TARBALLS)/dxgidebug.idl $(TARBALLS)/dxgi1_2.idl $(TARBALLS)/dxgitype.idl $(TARBALLS)/dxgiformat.idl $(TARBALLS)/dxgi.idl  $(TARBALLS)/dxgicommon.idl

d3d11: .sum-d3d11
	mkdir -p $@
	cp $(TARBALLS)/d3d11.idl $@ && cd $@ && patch -fp1 < ../$(SRC)/d3d11/processor_format.patch

dxgi12: .sum-d3d11
	mkdir -p $@
	cp $(TARBALLS)/dxgi1_2.idl $@ && cd $@ && patch -fp1 < ../$(SRC)/d3d11/dxgi12.patch

$(DST_D3D11_H): d3d11
	mkdir -p -- "$(PREFIX)/include/"
	$(WIDL) -DBOOL=WINBOOL -I$(PREFIX)/include $(IDL_INCLUDES) -h -o $@ $</d3d11.idl

$(DST_D3D11_1_H): d3d11
	mkdir -p -- "$(PREFIX)/include/"
	cp $(TARBALLS)/d3d11_1.h $@

$(DST_D3D11_2_H): d3d11
	mkdir -p -- "$(PREFIX)/include/"
	cp $(TARBALLS)/d3d11_2.h $@

$(DST_D3D11_3_H): d3d11
	mkdir -p -- "$(PREFIX)/include/"
	cp $(TARBALLS)/d3d11_3.h $@

$(DST_DXGIDEBUG_H): $(TARBALLS)/dxgidebug.idl
	mkdir -p -- "$(PREFIX)/include/"
	$(WIDL) -DBOOL=WINBOOL -I$(PREFIX)/include $(IDL_INCLUDES) -h -o $@ $<

$(DST_DXGITYPE_H): $(TARBALLS)/dxgitype.idl
	mkdir -p -- "$(PREFIX)/include/"
	$(WIDL) -DBOOL=WINBOOL -I$(PREFIX)/include $(IDL_INCLUDES) -h -o $@ $<

$(DST_DXGIFORMAT_H): $(TARBALLS)/dxgiformat.idl
	mkdir -p -- "$(PREFIX)/include/"
	$(WIDL) -DBOOL=WINBOOL -I$(PREFIX)/include $(IDL_INCLUDES) -h -o $@ $<

$(DST_DXGICOMMON_H): $(TARBALLS)/dxgicommon.idl
	mkdir -p -- "$(PREFIX)/include/"
	$(WIDL) -DBOOL=WINBOOL -I$(PREFIX)/include $(IDL_INCLUDES) -h -o $@ $<

$(DST_DXGI_H): $(TARBALLS)/dxgi.idl
	mkdir -p -- "$(PREFIX)/include/"
	$(WIDL) -DBOOL=WINBOOL -I$(PREFIX)/include $(IDL_INCLUDES) -h -o $@ $<

$(DST_DXGI12_H): $(TARBALLS)/dxgi1_2.idl
	mkdir -p -- "$(PREFIX)/include/"
	$(WIDL) -DBOOL=WINBOOL -I$(PREFIX)/include -I$(IDL_INCLUDES) -h -o $@ $<

$(DST_DXGI13_H): $(TARBALLS)/dxgi1_3.idl $(DST_DXGI12_H)
	mkdir -p -- "$(PREFIX)/include/"
	$(WIDL) -DBOOL=WINBOOL -Idxgi12 -I$(PREFIX)/include $(IDL_INCLUDES) -h -o $@ $<

$(DST_DXGI14_H): $(TARBALLS)/dxgi1_4.idl $(DST_DXGI13_H)
	mkdir -p -- "$(PREFIX)/include/"
	$(WIDL) -DBOOL=WINBOOL -Idxgi12 -I$(PREFIX)/include $(IDL_INCLUDES) -h -o $@ $<

$(DST_DXGI15_H): $(TARBALLS)/dxgi1_5.idl $(DST_DXGI14_H)
	mkdir -p -- "$(PREFIX)/include/"
	$(WIDL) -DBOOL=WINBOOL -Idxgi12 -I$(PREFIX)/include $(IDL_INCLUDES) -h -o $@ $<

$(DST_DXGI16_H): $(TARBALLS)/dxgi1_6.idl $(DST_DXGI15_H)
	mkdir -p -- "$(PREFIX)/include/"
	$(WIDL) -DBOOL=WINBOOL -Idxgi12 -I$(PREFIX)/include $(IDL_INCLUDES) -h -o $@ $<

.dxgitype: $(DST_DXGITYPE_H) $(DST_DXGIFORMAT_H) $(DST_DXGICOMMON_H) $(DST_DXGI_H)
	touch $@


.dxgi12: .dxgitype $(DST_DXGI12_H)
	touch $@

.dxgi13: .dxgi12 $(DST_DXGI13_H)
	touch $@

.dxgi14: .dxgi13 $(DST_DXGI14_H)
	touch $@

.dxgi15: .dxgi14 $(DST_DXGI15_H)
	touch $@

.dxgi16: .dxgi15 $(DST_DXGI16_H)
	touch $@

.d3d11_1: $(DST_D3D11_H) $(DST_D3D11_1_H)
	touch $@

.d3d11_2: .d3d11_1 $(DST_D3D11_2_H)
	touch $@

.d3d11_3: .d3d11_2 $(DST_D3D11_3_H)
	touch $@

.d3d11: .d3d11_3 $(DST_DXGIDEBUG_H) .dxgi16
	touch $@
