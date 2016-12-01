
all:
		gcc -o client client.c -lsocket -lnsl -lresolv
		gcc -o aws aws.c -lsocket -lnsl -lresolv
		gcc -o serverA serverA.c -lsocket -lnsl -lresolv
		gcc -o serverB serverB.c -lsocket -lnsl -lresolv
		gcc -o serverC serverC.c -lsocket -lnsl -lresolv

$(phony serverA): 
		./serverA

serverB:
		./serverB

serverC:
		./serverC

aws:
		./aws
