
NETSTAT_COMMAND=netstat -nt | grep 127.0.0.1:80 | grep ESTABLISHED

run:
	rm -f a.out && gcc prog.c && ./a.out $(args)

watch:
	watch -n 5 "netstat -nt | grep 127.0.0.1:80 | grep ESTABLISHED"

count:
	watch -n 5 "echo -n 'Open connections: '; ${NETSTAT_COMMAND} | wc -l; ${NETSTAT_COMMAND}"

request:
	curl http://127.0.0.1:80