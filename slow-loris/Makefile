
run:
	rm -f a.out && gcc prog.c && ./a.out $(args)

watch:
	watch -n 5 "netstat -nt | grep 127.0.0.1:80 | grep ESTABLISHED | wc -l"