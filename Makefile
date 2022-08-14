
override CPPFLAGS := -DNDEBUG -O3 -fPIE -Wall -Wextra -Wpedantic -Wconversion \
  -Wcast-align -Wformat=2 -Wstrict-overflow=5 -Wsign-promo $(CPPFLAGS)
override CXXFLAGS := --std=c++2a -Woverloaded-virtual $(CXXFLAGS)
override LDLIBS   := -lcrypto $(LDLIBS)
prefix            := /usr/local

.PHONY: default
default: unaesgcm-real

unaesgcm-real: aesgcm.cpp main.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@
	strip --strip-all $@

test:          aesgcm.cpp test.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -UNDEBUG $(LDFLAGS) $^ $(LDLIBS) -o $@

main.cpp:   aesgcm.hpp
test.cpp:   aesgcm.hpp hex.hpp fixcapvec.hpp
aesgcm.cpp: aesgcm.hpp fixcapvec.hpp
aesgcm.hpp: hex.hpp

override INSTALLDIR := $(DESTDIR)$(prefix)
.PHONY: install
install:
	mkdir -p \
		"$(INSTALLDIR)/libexec/unaesgcm" \
		"$(INSTALLDIR)/bin" \
		"$(INSTALLDIR)/share/applications"
	cp unaesgcm-real "$(INSTALLDIR)/libexec/unaesgcm/"
	cp unaesgcm aesgcm-open "$(INSTALLDIR)/bin/"
	ln -sf aesgcm-open "$(INSTALLDIR)/bin/aesgcm-open-gui"
	chmod 755 \
		"$(INSTALLDIR)/bin/unaesgcm" \
		"$(INSTALLDIR)/bin/aesgcm-open"
	printf "`cat unaesgcm.desktop.printf`" "$(prefix)" > \
		"$(INSTALLDIR)/share/applications/unaesgcm.desktop"
	update-desktop-database "$(INSTALLDIR)/share/applications"

.PHONY: uninstall
uninstall:
	rm -f \
		"$(INSTALLDIR)/share/applications/unaesgcm.desktop" \
		"$(INSTALLDIR)/bin/aesgcm-open-gui" \
		"$(INSTALLDIR)/bin/aesgcm-open" \
		"$(INSTALLDIR)/bin/unaesgcm" \
		"$(INSTALLDIR)/libexec/unaesgcm/unaesgcm-real"
	update-desktop-database "$(INSTALLDIR)/share/applications"
	-rmdir "$(INSTALLDIR)/libexec/unaesgcm"

README.html: README.md
	markdown $< > $@
LICENSE.html: LICENSE.md
	markdown $< > $@

.PHONY: clean
clean:
	rm -f unaesgcm-real test README.html LICENSE.html
