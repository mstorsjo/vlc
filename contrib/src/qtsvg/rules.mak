# Qt

QTSVG_VERSION_MAJOR := 5.12
QTSVG_VERSION := $(QTSVG_VERSION_MAJOR).0
QTSVG_URL := https://download.qt.io/official_releases/qt/$(QTSVG_VERSION_MAJOR)/$(QTSVG_VERSION)/submodules/qtsvg-everywhere-src-$(QTSVG_VERSION).tar.xz

DEPS_qtsvg += qt $(DEPS_qt)

ifdef HAVE_WIN32
PKGS += qtsvg
endif

ifeq ($(call need_pkg,"Qt5Svg"),)
PKGS_FOUND += qtsvg
endif

$(TARBALLS)/qtsvg-$(QTSVG_VERSION).tar.xz:
	$(call download,$(QTSVG_URL))

.sum-qtsvg: qtsvg-$(QTSVG_VERSION).tar.xz

qtsvg: qtsvg-$(QTSVG_VERSION).tar.xz .sum-qtsvg
	$(UNPACK)
	mv qtsvg-everywhere-src-$(QTSVG_VERSION) qtsvg-$(QTSVG_VERSION)
	$(APPLY) $(SRC)/qtsvg/0001-Force-the-usage-of-QtZlib-header.patch
	$(MOVE)

.qtsvg: qtsvg
	cd $< && $(PREFIX)/bin/qmake
	# Make && Install libraries
	cd $< && $(MAKE)
	cd $< && $(MAKE) -C src sub-plugins-install_subtargets sub-svg-install_subtargets
	mv $(PREFIX)/plugins/iconengines/libqsvgicon.a $(PREFIX)/lib/
	mv $(PREFIX)/plugins/imageformats/libqsvg.a $(PREFIX)/lib/
	cd $(PREFIX)/lib/pkgconfig; sed -i.orig \
		-e 's/d\.a/.a/g' \
		-e 's/-lQt\([^ ]*\)d/-lQt\1/g' \
		-e 's/-llibEGLd -llibGLESv2d/-llibEGL -llibGLESv2/g' \
		-e '/Libs:/  s/-lQt5Svg/-lqsvg -lqsvgicon -lQt5Svg/ ' \
		Qt5Svg.pc
	touch $@
