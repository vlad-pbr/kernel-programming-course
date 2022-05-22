
NETSTAT_COMMAND=netstat -nt | grep 127.0.0.1:80 | grep ESTABLISHED | grep -v tcp6
ARGS ?= 127.0.0.1 80 500 5

run:
	rm -f a.out && gcc prog.c && ./a.out $(ARGS)

watch:
	watch -n 5 "echo -n 'Open connections: '; ${NETSTAT_COMMAND} | wc -l; ${NETSTAT_COMMAND}"

request:
	BEFORE_STAMP=$$(date +%s)\
	; curl -s http://127.0.0.1:80 > /dev/null \
	&& echo "\nRequest took $$(( $$(date +%s) - BEFORE_STAMP )) seconds"