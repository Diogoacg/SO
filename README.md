# SO

Abrir 2 terminais, no primeiro gcc -o servidor servidor.c e ./servidor
No segundo gcc -o cliente cliente.c e ./cliente execute "flag" "comandos" por exemplo ./cliente execute -u "ls -l | ping 1.1.1.1"

Se der erro inicia primeiro o cliente, qd tivermos o makefile nao vai dar problema

descobri que a pipeline são comandos que interagem entre si, para já simplesmente corre os programas

parte basica praticamnete feita, menos alguns erros como por vezes o servidor fechar do nada

acho que só falta as pipelines
