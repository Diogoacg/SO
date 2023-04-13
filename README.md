# SO

Abrir 2 terminais, no primeiro gcc -o servidor servidor.c e ./servidor
No segundo gcc -o cliente cliente.c e ./cliente execute "flag" "comandos" por exemplo ./cliente execute -u "ls -l | ping 1.1.1.1"

Se der erro inicia primeiro o cliente, qd tivermos o makefile nao vai dar problema

Outro erro que ainda nao corrigi é ao chamar o status ainda pede mais 2 agrumentos, é so por algo ao calhas que funciona

descobri que a pipeline são comandos que interagem entre si, para já simplesmente corre os programas

parte basica praticamnete feita, menos alguns erros
