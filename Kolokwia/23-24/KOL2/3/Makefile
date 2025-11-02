all: cli srv cli_udp srv_udp

cli: cli.c
	gcc cli.c -o cli
cli_dup: cli_udp.c
	gcc cli_udp.c -o cli_udp
	

srv: srv.c
	gcc srv.c -o srv
	
srv_udp: srv_udp.c
	gcc srv_udp.c -o srv_udp

clean:
	rm -rf cli srv cli_udp srv_udp