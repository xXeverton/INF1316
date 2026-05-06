```T1/
├── src/                 
│   ├── main.c           
│   ├── kernel.c         
│   ├── controller.c     
│   ├── app.c            
│   └── common.h         
├── bin/                 
├── docs/                
│   └── relatorio.pdf    
└── Makefile
```

## O que vai em cada lugar:
src/ (Código-fonte): Onde toda a sua programação vai ficar. Separar as responsabilidades em arquivos diferentes facilita muito a manutenção.

main.c: Será o ponto de partida. Ele vai configurar o Pipe e fazer o primeiro fork() para iniciar o Kernel e o Controlador.

kernel.c: Conterá a lógica do KernelSim (receber interrupções, gerenciar filas e mandar sinais).

controller.c: Conterá o loop do InterController Sim (gerando os sorteios e enviando os IRQs).

app.c: O código padrão que os processos de aplicação (A1 a A5) vão executar usando exec().

common.h: Um arquivo de cabeçalho super importante para guardar as constantes (como o MAX), as estruturas de dados (como o contexto dos processos) e as assinaturas das funções para que todos os arquivos conversem entre si.

bin/ (Binários): Uma pasta vazia onde os executáveis gerados pela compilação serão salvos, mantendo a raiz do projeto limpa.

docs/ (Documentação): Onde você vai guardar o relatório final exigido no enunciado explicando a arquitetura do seu trabalho.

Makefile: O script que vai automatizar a compilação de todos esses arquivos do src/ e jogar o resultado no bin/ com um único comando.


## Como Rodar?

no terminal digite:

```bash
make
```

Depois digite:

```bash
./bin/simulador
```


Para limpar tudo (caso queira começar do zero):

```bash
make clean
```