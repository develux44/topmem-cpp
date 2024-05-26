topmem:
	g++ topmem.cpp -O3 -o topmem
	
install:
ifneq ($(shell id -u), 0)
	@echo "You must be root to perform this action."
else
	mv topmem /usr/bin/topmem
	chmod +x /usr/bin/topmem
endif
	
clean:
	rm -f topmem