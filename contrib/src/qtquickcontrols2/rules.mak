# QtQuickControls 2

QTQC2_VERSION_MAJOR := 5.12
QTQC2_VERSION := $(QTQC2_VERSION_MAJOR).0
QTQC2_URL := http://download.qt.io/official_releases/qt/$(QTQC2_VERSION_MAJOR)/$(QTQC2_VERSION)/submodules/qtquickcontrols2-everywhere-src-$(QTQC2_VERSION).tar.xz

ifdef HAVE_WIN32
ifeq ($(findstring $(ARCH), arm aarch64),)
# There is no opengl available on windows on these architectures.
PKGS += qtquickcontrols2
endif
endif

ifeq ($(call need_pkg,"Qt5QuickControls2"),)
PKGS_FOUND += qtquickcontrols2
endif
# QtQuickControl(1) doesn't provide a .pc

DEPS_qtquickcontrols2 = qtdeclarative $(DEPS_qtdeclarative)

$(TARBALLS)/qtquickcontrols2-$(QTQC2_VERSION).tar.xz:
	$(call download,$(QTQC2_URL))

.sum-qtquickcontrols2: qtquickcontrols2-$(QTQC2_VERSION).tar.xz

qtquickcontrols2: qtquickcontrols2-$(QTQC2_VERSION).tar.xz .sum-qtquickcontrols2
	$(UNPACK)
	mv qtquickcontrols2-everywhere-src-$(QTQC2_VERSION) qtquickcontrols2-$(QTQC2_VERSION)
	$(MOVE)

.qtquickcontrols2: qtquickcontrols2
	cd $< && $(PREFIX)/bin/qmake
	# Make && Install libraries
	cd $< && $(MAKE)
	cd $< && $(MAKE) -C src sub-quickcontrols2-install_subtargets sub-imports-install_subtargets
	cp $(PREFIX)/qml/QtQuick/Controls.2/libqtquickcontrols2plugin.a $(PREFIX)/lib/
	cp $(PREFIX)/qml/QtQuick/Templates.2/libqtquicktemplates2plugin.a $(PREFIX)/lib/
	rm -rf $(PREFIX)/qml
	cd $(PREFIX)/lib/pkgconfig; sed -i.orig \
		-e 's/d\.a/.a/g' \
		-e 's/-lQt\([^ ]*\)d/-lQt\1/g' \
		-e 's/ -lQt5QuickControls2/ -lqtquickcontrols2plugin -lqtquicktemplates2plugin -lQt5QuickControls2/' \
		-e 's/-llibEGLd -llibGLESv2d/-llibEGL -llibGLESv2/g' \
		Qt5QuickControls2.pc
	touch $@
