CODE_FILE=stk.c

test: stk.o
	gcc -D__TEST__ -Wall -o DSH4test ${CODE_FILE}
release:
	gcc -Wall -o DSH ${CODE_FILE}
	'cp' -av DSH /var/www/icons/DSH2
debug:
	gcc -g -D__TEST__ -D__DEBUG__ -Wall -o DSH4test ${CODE_FILE} 
