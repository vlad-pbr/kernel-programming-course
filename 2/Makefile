
run:
	gcc prog.c && sudo ./a.out

command:
	# start http://localhost/tmp/file.txt out.txt

send:
	# generating file
	echo ${RANDOM} | md5sum | head -c 32 | tee /tmp/file.txt ; echo ; echo

	# downloading file
	curl --http0.9 --http1.0 -H 'User-Agent:' -H 'Accept:' http://localhost/tmp/file.txt ; echo
	