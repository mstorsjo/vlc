# libmicrodns

LIBMICRODNS_VERSION := 0.0.8
LIBMICRODNS_URL := https://github.com/videolabs/libmicrodns/releases/download/$(LIBMICRODNS_VERSION)/microdns-$(LIBMICRODNS_VERSION).tar.gz

ifndef HAVE_MACOSX
ifdef BUILD_NETWORK
PKGS += microdns
endif
endif
ifeq ($(call need_pkg,"microdns >= 0.0.1"),)
PKGS_FOUND += microdns
endif

$(TARBALLS)/microdns-$(LIBMICRODNS_VERSION).tar.gz:
	$(call download_pkg,$(LIBMICRODNS_URL),microdns)

.sum-microdns: $(TARBALLS)/microdns-$(LIBMICRODNS_VERSION).tar.gz

microdns: microdns-$(LIBMICRODNS_VERSION).tar.gz .sum-microdns
	$(UNPACK)
	$(APPLY) $(SRC)/microdns/configure-win32-version.patch
	$(MOVE)

.microdns: microdns
	cd $< && $(HOSTVARS) ./configure $(HOSTCONF)
	cd $< && $(MAKE) install
	touch $@
