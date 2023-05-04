# SO

Ir para a pasta onde está o makefile, fazer make

abrir mais 2 terminais, desta vez na pasta bin, num executa o servidor, por exemplo ./monitor logs noutro(s) executar os pedidos dos clientes, ./tracer execute -u "comando", ./tracer execute -p "ls | grep txt" (em pipeline), ./tracer status (podem por a correr tipo um sleep e verificar depois), ./tracer stats-uniq [lista de pids], ./tracer stats-time [lista de pids] ./tracer stats-command comando [lista de pids]

nestes ultimos casos por exemplo ./tracer stats-uniq 2343 2345 1232

Penso estar tudo
