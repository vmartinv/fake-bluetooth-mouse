CXX ?= g++
CXXFLAGS ?= -O2 -Wall -Wextra -std=c++20

forward: forward.cpp mouse_frame.h
	$(CXX) $(CXXFLAGS) -o $@ $<

test/test_mouse_frame: test/test_mouse_frame.cpp mouse_frame.h test/doctest.h
	$(CXX) $(CXXFLAGS) -o $@ $<

test: test/test_mouse_frame
	./test/test_mouse_frame

lint:
	clang-format --dry-run --Werror forward.cpp mouse_frame.h test/test_mouse_frame.cpp
	clang-tidy forward.cpp test/test_mouse_frame.cpp -- $(CXXFLAGS)

clean:
	rm -f forward test/test_mouse_frame

.PHONY: test lint clean
