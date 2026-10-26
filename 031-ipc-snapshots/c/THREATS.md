# Modelo de ameaças e fronteiras de confiança

## Escopo e participantes

O alvo e o observador são programas cooperativos executados pelo mesmo usuário
em Linux. O ativo protegido pelo protocolo é a coerência da sessão: somente
um observador autenticado recebe a região, e somente os comandos previstos
podem alterar o contador pela conexão. O protótipo não fornece isolamento
entre programas hostis da mesma identidade.

O diretório privado 0700 limita a travessia por outros usuários; o socket 0600
limita a conexão. `SO_PEERCRED` consulta o UID associado pelo kernel à conexão,
em vez de confiar em um UID declarado no pedido. Um token novo de 256 bits,
gerado por `getrandom`, associa a autenticação ao segredo entregue ao filho
pelo pipe. A implementação não publica esse segredo em argumentos, ambiente
ou logs. A confidencialidade do pipe e das memórias dos dois processos faz
parte dessa suposição de confiança.

## Capacidade antes e depois de HELLO

Antes da autenticação, o cliente pode enviar bytes para o socket, mas não
recebe descritor nem comanda a região. Erro de credencial ou token fecha a
tentativa, preserva o contador e permite que o filho tente autenticar novamente.
Depois da autenticação, o alvo concede uma referência ao `memfd` por SCM_RIGHTS.
O número do descritor é local ao receptor; o recurso transferido é a
capacidade de acessar a região, não um identificador global adivinhável.

O observador precisa de um mapeamento gravável para operar o mutex. Portanto,
a capacidade concedida é maior que a autorização lógica de SNAPSHOT: inclui
alterar diretamente dados, mutex e tamanho do objeto. Um observador malicioso
pode falsificar invariantes, travar a sessão, truncar a região e provocar
falhas; os comandos restritos não impedem essas ações. UID e token não isolam
código hostil que já dispõe dessa capacidade ou pode inspecionar processos
da mesma identidade. Administrador e kernel também ficam fora da proteção.

## Consistência e invalidação

Todo acesso publicado aos dados exige ownership do mesmo mutex robusto entre
processos. A versão é conferida sob mutex antes das demais leituras. Contador
e redundância mudam junto com a geração; overflow é recusado antes de escrever.
Uma resposta contém uma cópia privada. A observação posterior também é uma
cópia sob mutex: geração maior é permitida; geração igual exige dados iguais.
Essas verificações detectam várias incoerências, mas não autenticam o conteúdo
contra um escritor malicioso que controle todos os campos.

`EOWNERDEAD` invalida a sessão. O processo que recebe ownership libera o mutex
sem declarar consistência; não há recuperação automática nem continuação dos
comandos. Erros fatais, perda do peer e timeout encerram a sessão. COMMAND e
VALUE são recusas recuperáveis: consomem a sequência esperada e não alteram os
dados. Não há wrap de sequência nem de geração.

## Disponibilidade e recursos

Linhas limitadas e deadlines monotônicos restringem o trabalho e as esperas
de cada operação. Leituras parciais e EINTR não renovam o prazo. O observador
fecha descritores recebidos indevidamente, inclusive quando o controle está
truncado; o alvo não repete SCM_RIGHTS em envios parciais. Esses controles
reduzem erros de protocolo e vazamentos acidentais. Não impedem negação de
serviço por um processo autorizado que abra tentativas sucessivas, detenha o
mutex ou interrompa os próprios usuários.

O alvo encerra a worker e aguarda o filho antes de liberar memória e mutex.
Se o filho não termina no prazo de saída, o alvo envia SIGKILL e faz reap. Um mutex
ocupado não é destruído; um irrecuperável sem usuários é descartado com a
região. Sockets e diretórios são removidos no cleanup normal e nos caminhos de
erro tratados. Encerramento forçado do próprio alvo, falha do kernel ou energia
interrompida podem impedir esse cleanup. A implementação não promete apagar
segredos de todas as cópias de memória nem executar child reaping de descendentes arbitrários
criados por um observador hostil.
