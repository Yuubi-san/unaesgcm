
override CPPFLAGS := -DNDEBUG -O3 -fPIE -Wall -Wextra -Wpedantic -Wconversion \
  -Wcast-align -Wformat=2 -Wstrict-overflow=5 -Wsign-promo $(CPPFLAGS)
override CXXFLAGS := --std=c++2a -Woverloaded-virtual $(CXXFLAGS)
override LDLIBS   := -lcrypto $(LDLIBS)

.PHONY: default
default: unaesgcm-real

unaesgcm-real: unaesgcm.cpp main.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@
	strip --strip-all $@

test:          unaesgcm.cpp test.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -UNDEBUG $(LDFLAGS) $^ $(LDLIBS) -o $@

main.cpp:     unaesgcm.hpp
test.cpp:     unaesgcm.hpp hex.hpp fixcapvec.hpp
unaesgcm.cpp: unaesgcm.hpp fixcapvec.hpp
unaesgcm.hpp: hex.hpp

.PHONY: install
install:
	mkdir -p \
		/usr/local/libexec/unaesgcm \
		/usr/local/bin
	cp unaesgcm-real /usr/local/libexec/unaesgcm/
	cp unaesgcm aesgcm-open /usr/local/bin/
	chmod +x \
		/usr/local/bin/unaesgcm \
		/usr/local/bin/aesgcm-open

.PHONY: uninstall
uninstall:
	rm -f \
		/usr/local/bin/aesgcm-open \
		/usr/local/bin/unaesgcm \
		/usr/local/libexec/unaesgcm/unaesgcm-real
	-rmdir /usr/local/libexec/unaesgcm

README.html: README.md
	markdown $< > $@
LICENSE.html: LICENSE.md
	markdown $< > $@

.PHONY: clean
clean:
	rm -f unaesgcm-real test README.html LICENSE.html
