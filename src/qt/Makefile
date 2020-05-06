.PHONY: FORCE
all: FORCE
	$(MAKE) -C .. verium_qt test_bitcoin_qt
clean: FORCE
	$(MAKE) -C .. verium_qt_clean test_bitcoin_qt_clean
check: FORCE
	$(MAKE) -C .. test_bitcoin_qt_check
verium-qt verium-qt.exe: FORCE
	 $(MAKE) -C .. verium_qt
