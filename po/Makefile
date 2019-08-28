include Makevars

XGETTEXT = xgettext
MSGFMT = msgfmt
MSGMERGE = msgmerge
MSGINIT = msginit

LOCALEDIR ?= usr/share/locale
APPLICATIONSDIR ?= usr/share/applications

XGETTEXTOPTIONSCOMMON = --add-comments --sort-output --msgid-bugs-address=$(BUGSADDR) --package-name=$(DOMAIN) --from-code=UTF-8

PO_REGEX = ^([a-z][a-z]|[a-z][a-z]_[A-Z][A-Z])\.po$$
LOCALE_REGEX = ^[a-z][a-z]_[A-Z][A-Z]\.utf8$$

check_defined = \
    $(strip $(foreach 1,$1, \
        $(call __check_defined,$1,$(strip $(value 2)))))
__check_defined = \
    $(if $(value $1),, \
      $(error Undefined $1$(if $2, ($2))))

$(call check_defined, DOMAIN)
$(call check_defined, BUGSADDR)


.SUFFIXES: .po .mo .pot

PO = $(wildcard *.po)
LANG = $(basename $(PO))
ifneq ($(SOURCES_SH)$(SOURCES_PY)$(SOURCES_JAVA)$(SOURCES_C)$(SOURCES_CPP),)
MO = $(addsuffix .mo,$(LANG))
endif
DESKTOPS = $(basename $(SOURCES_DESKTOP))

%.mo: %.po
	@$(MSGFMT) -c --verbose --statistics -o $@ $<

%.po: SHELL := /bin/bash
%.po:
	@if [[ ! "$@" =~ $(PO_REGEX) ]] ; then \
	 echo "Invalid po file name, must be either xx.po or xx_YY.po" ;\
	 exit 1; \
	fi
	@if ! test -f $@; then \
	 echo "File $@ does not exist." ; \
	 if [[ "$*.utf8" =~ $(LOCALE_REGEX) ]] ; then \
	   if LANG=POSIX locale -a | grep -qxF "$*.utf8" ; then : ; else  \
	    echo "Locale '$*.utf8' not found on your system" ; exit 1 ; \
	   fi; \
	 else \
	   if LANG=POSIX locale -a | grep -qE "^$*_" ; then : ; else \
	    echo "No locale matching '$*' found on your system (see 'locale -a')" ; exit 1; \
	  fi; \
	 fi; \
	 echo -e "You can create it with :\n"; \
	 for ln in $$(LANG=POSIX locale -a | grep -E "^($*_|$*).*\.utf8") ; do \
	  echo " # $(MSGINIT) -l $$ln -i $(DOMAIN).pot -o $@"; \
	 done; \
	 echo ; exit 1; \
	fi;

%.desktop: %.desktop.in $(PO)
	@echo Generating desktop file : $@
	@tmpfiles="$@-t $@-n-t"; \
	cp $< $@-t || exit 1; \
	for pofile in $(PO); do \
	   $(MSGFMT) --desktop --template $@-t -o $@-n-t \
	   --locale $${pofile%.po} $$pofile \
	   || { $(RM) $$tmpfiles; exit 1; }; \
	   mv $@-n-t $@-t || { $(RM) $$tmpfiles; exit 1; } \
	done; \
	mv $@-t $@ || { $(RM) $$tmpfiles; exit 1; } \

all: $(MO) $(DESKTOPS)

update-po: SHELL := /bin/bash
update-po: $(DOMAIN).pot
	@for po in $(PO); do \
	echo -n "Updating $$po "; \
	if [[ ! "$$po" =~ $(PO_REGEX) ]] ; then \
	 echo " : invalid po file name, must be either xx.po or xx_YY.po" ;\
	 exit 1; \
	fi; \
	$(MSGMERGE) --previous -U $$po $(DOMAIN).pot; \
	done;

$(DOMAIN).pot: Makevars $(SOURCES_SH) $(SOURCES_PY) $(SOURCES_JAVA) $(SOURCES_C) $(SOURCES_CPP) $(SOURCES_DESKTOP)
	@echo "Updating $@ .. "
	@$(shell :> $@)
ifdef SOURCES_SH
	@echo -n "  - SH .. "
	@$(XGETTEXT) $(XGETTEXTOPTIONSCOMMON) $(XGETTEXTOPTIONS_SH) -o $@ -j $(SOURCES_SH)
	@echo done
endif
ifdef SOURCES_PY
	@echo -n "  - PY .. "
	@$(XGETTEXT) $(XGETTEXTOPTIONSCOMMON) $(XGETTEXTOPTIONS_PY) -o $@ -j $(SOURCES_PY)
	@echo done
endif
ifdef SOURCES_JAVA
	@echo -n "  - JAVA .. "
	@$(XGETTEXT) $(XGETTEXTOPTIONSCOMMON) $(XGETTEXTOPTIONS_JAVA) -o $@ -j $(SOURCES_JAVA)
	@echo done
endif
ifdef SOURCES_C
	@echo -n "  - C .. "
	@$(XGETTEXT) $(XGETTEXTOPTIONSCOMMON) $(XGETTEXTOPTIONS_C) -o $@ -j $(SOURCES_C)
	@echo done
endif
ifdef SOURCES_CPP
	@echo -n "  - CPP .. "
	@$(XGETTEXT) $(XGETTEXTOPTIONSCOMMON) $(XGETTEXTOPTIONS_CPP) -o $@ -j $(SOURCES_CPP)
	@echo done
endif
ifdef SOURCES_DESKTOP
	@echo -n "  - DESKTOP .. "
	@$(XGETTEXT) $(XGETTEXTOPTIONSCOMMON) $(XGETTEXTOPTIONS_DESKTOP) -o $@ -j $(SOURCES_DESKTOP)
	@echo done
endif

install: all
	for i in $(MO) ; do \
	  t=$(DESTDIR)/$(LOCALEDIR)/`basename $$i .mo`/LC_MESSAGES ;\
	  install -d $$t ;\
	  install -m 644 $$i $$t/$(DOMAIN).mo ;\
	done
	for i in $(DESKTOPS) ; do \
	  t=$(DESTDIR)/$(APPLICATIONSDIR) ;\
	  install -d $$t ;\
	  install -m 644 $$i $$t/`basename $$i` ;\
	done

clean:
	$(RM) $(MO) *~ $(DESKTOPS)

distclean: clean

maintainer-clean: clean

check: SHELL := /bin/bash
check:
	@for po in $(PO); do \
	echo -n "Checking $$po "; \
	if [[ ! "$$po" =~ $(PO_REGEX) ]] ; then \
	 echo " : invalid po file name, must be either xx.po or xx_YY.po" ;\
	 exit 1; \
	else \
	 echo ".. done"; \
	fi; \
	done;

.PHONY: all update-po install clean distclean maintainer-clean check
