topmem:
	g++ topmem.cpp -o topmem
	sudo mv topmem /usr/bin/topmem
	sudo chmod +x /usr/bin/topmem

clean:
	rm -f topmem