// ///////////////////////////////////////////////////////////////////////////////////////////////////////

// [INF1316] LAB 05 - Everton Pereira Militão 2320462, Lúcio Campos 2320955

// ///////////////////////////////////////////////////////////////////////////////////////////////////////

// QUESTÃO 1 - Execute o programa Corrida de Sapo algumas vezes e analise os resultados sobre a ordem de ]
// chegada dos sapos. Obs: compile com a opção –lpthread.

// - CÓDIGO: --------------------------------------------------------------------------------------------

// #include <pthread.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>

// #define NUM_THREADS 5
// #define PULO_MAXIMO 100
// #define DESCANSO_MAXIMO 1
// #define DISTANCIA_PARA_CORRER 100

// static int classificacao = 1;
// static char *resp[200];
// static int cont = 0;

// void *Correr(void *sapo);

// int main()
// {
//     classificacao = 1;

//     pthread_t threads[NUM_THREADS];
//     int t;

//     printf("Corrida iniciada ... \n");

//     for (t = 0; t < NUM_THREADS; t++)
//         pthread_create(&threads[t], NULL, Correr, (void *)(long)t);

//     for (t = 0; t < NUM_THREADS; t++)
//         pthread_join(threads[t], NULL);   

//     printf("\nAcabou!!\n");

//     pthread_exit(NULL);
// }

// void *Correr(void *sapo)
// {
//     int pulos = 0;
//     int distanciaJaCorrida = 0;

//     while (distanciaJaCorrida <= DISTANCIA_PARA_CORRER)
//     {
//         int pulo = rand() % PULO_MAXIMO;
//         distanciaJaCorrida += pulo;
//         pulos++;

//         printf("Sapo %ld pulou\n", (long)sapo);

//         int descanso = rand() % DESCANSO_MAXIMO;
//         sleep(descanso);
//     }

//     printf("Sapo %ld chegou na posicao %d com %d pulos\n", (long)sapo, classificacao, pulos);

//     cont++;
//     classificacao++;

//     pthread_exit(NULL);
// } 

// - LINHA DE COMPILAÇÃO: --------------------------------------------------------------------------------

// gcc Ex1.c -o Ex1 -lpthread

// - LINHA DE EXECUÇÃO: ----------------------------------------------------------------------------------

// ./Ex1

// - SAÍDA: ----------------------------------------------------------------------------------------------

// Corrida iniciada ... 
// Sapo 0 pulou
// Sapo 1 pulou
// Sapo 3 pulou
// Sapo 4 pulou
// Sapo 3 pulou
// Sapo 3 chegou na posicao 1 com 2 pulos
// Sapo 4 pulou
// Sapo 2 pulou
// Sapo 0 pulou
// Sapo 1 pulou
// Sapo 2 pulou
// Sapo 4 pulou
// Sapo 4 chegou na posicao 2 com 3 pulos
// Sapo 2 pulou
// Sapo 2 chegou na posicao 3 com 3 pulos
// Sapo 0 chegou na posicao 3 com 2 pulos
// Sapo 1 chegou na posicao 3 com 2 pulos

// Acabou!!

// - RESPOSTA: -------------------------------------------------------------------------------------------

// O programa demonstra que múltiplas threads podem acessar e modificar variáveis globais ao 
// mesmo tempo, causando resultados diferentes a cada execução. No código, as variáveis classificacao 
// e cont são incrementadas por várias threads sem qualquer mecanismo de sincronização. Isso pode 
// fazer com que duas threads utilizem o mesmo valor antes da atualização, gerando posições incorretas. 
// Esse comportamento caracteriza uma condição de corrida causada pela ausência de exclusão mútua.

// ///////////////////////////////////////////////////////////////////////////////////////////////////////

// QUESTÃO 2 - Usando mutex, modifique o programa Corrida de Sampo para que o problema identificado 
// anteriormente não ocorra. 

// - CÓDIGO: --------------------------------------------------------------------------------------------

// #include <pthread.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>

// #define NUM_THREADS 5
// #define PULO_MAXIMO 100
// #define DESCANSO_MAXIMO 1
// #define DISTANCIA_PARA_CORRER 100

// static int classificacao = 1;
// static char *resp[200];
// static int cont = 0;

// //mutex global
// static pthread_mutex_t lock;

// void *Correr(void *sapo);

// int main()
// {
//     classificacao = 1;

//     pthread_t threads[NUM_THREADS];
//     int t;

//     //inicializa o mutex
//     pthread_mutex_init(&lock, NULL);

//     printf("Corrida iniciada ... \n");

//     for (t = 0; t < NUM_THREADS; t++)
//         pthread_create(&threads[t], NULL, Correr, (void *)(long)t);

//     for (t = 0; t < NUM_THREADS; t++)
//         pthread_join(threads[t], NULL);

//     printf("\nAcabou!!\n");

//     //destroi o mutex
//     pthread_mutex_destroy(&lock);

//     pthread_exit(NULL);
// }

// void *Correr(void *sapo)
// {
//     int pulos = 0;
//     int distanciaJaCorrida = 0;

//     while (distanciaJaCorrida <= DISTANCIA_PARA_CORRER)
//     {
//         int pulo = rand() % PULO_MAXIMO;
//         distanciaJaCorrida += pulo;
//         pulos++;

//         printf("Sapo %ld pulou\n", (long)sapo);

//         int descanso = rand() % DESCANSO_MAXIMO;
//         sleep(descanso);
//     }


//     pthread_mutex_lock(&lock);

//     printf("Sapo %ld chegou na posicao %d com %d pulos\n", (long)sapo, classificacao, pulos);

//     cont++;
//     classificacao++;

//     pthread_mutex_unlock(&lock);

//     pthread_exit(NULL);
// }

// - LINHA DE COMPILAÇÃO: --------------------------------------------------------------------------------

// gcc Ex2.c -o Ex2 -lpthread

// - LINHA DE EXECUÇÃO: ----------------------------------------------------------------------------------

// ./Ex2

// - SAÍDA: ----------------------------------------------------------------------------------------------

// Corrida iniciada ... 
// Sapo 0 pulou
// Sapo 1 pulou
// Sapo 3 pulou
// Sapo 2 pulou
// Sapo 1 pulou
// Sapo 0 pulou
// Sapo 4 pulou
// Sapo 3 pulou
// Sapo 1 chegou na posicao 1 com 2 pulos
// Sapo 4 pulou
// Sapo 0 chegou na posicao 2 com 2 pulos
// Sapo 2 pulou
// Sapo 3 chegou na posicao 3 com 2 pulos
// Sapo 2 pulou
// Sapo 2 chegou na posicao 4 com 3 pulos
// Sapo 4 pulou
// Sapo 4 pulou
// Sapo 4 chegou na posicao 5 com 4 pulos

// Acabou!!

// - RESPOSTA: -------------------------------------------------------------------------------------------

// Com o uso do mutex, garantimos exclusão mútua no acesso às variáveis globais classificacao e cont. 
// Dessa forma, apenas uma thread por vez pode atualizar essas variáveis, eliminando a condição de corrida 
// observada anteriormente. Agora, a classificação dos sapos é consistente e o resultado é determinado 
// corretamente, mesmo com a execução concorrente das threads.

// ///////////////////////////////////////////////////////////////////////////////////////////////////////

// QUESTÃO 3 - Usando threads, escreva um programa C que implemente o problema do produtor/consumidor. O 
// produtor deve produzir dados (números inteiros pseudo-aleatórios) a cada 1 segundo colocando-os em uma 
// fila (buffer circular). O consumidor deve retirar dados da fila a cada 2 segundos. O tamanho máximo da 
// fila deve ser de 8 elementos (MAXFILA) e tanto o produtor como o consumidor devem produzir/consumir 64 
// elementos, evitando condições de corrida.

// - CÓDIGO: --------------------------------------------------------------------------------------------

// #include <pthread.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>

// #define MAXFILA 8
// #define NUM_ELEMENTOS 64

// int buffer[MAXFILA];
// int inicio = 0;
// int fim = 0;
// int count = 0;

// pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
// pthread_cond_t produzir = PTHREAD_COND_INITIALIZER;
// pthread_cond_t consumir = PTHREAD_COND_INITIALIZER;

// void *produtor(void *arg)
// {
//     for (int i = 1; i <= NUM_ELEMENTOS; i++)
//     {
//         sleep(1);
//         pthread_mutex_lock(&mutex);

//         while (count == MAXFILA)
//         {
//             pthread_cond_wait(&produzir, &mutex);
//         }

//         buffer[fim] = i;
//         fim = (fim + 1) % MAXFILA;
//         count++;

//         printf("Produtor produziu: %d (itens no buffer: %d)\n", i, count);

//         pthread_cond_signal(&consumir);
//         pthread_mutex_unlock(&mutex);
//     }

//     pthread_exit(NULL);
// }

// void *consumidor(void *arg)
// {
//     for (int i = 1; i <= NUM_ELEMENTOS; i++)
//     {
//         sleep(2);
//         pthread_mutex_lock(&mutex);

//         while (count == 0)
//         {
//             pthread_cond_wait(&consumir, &mutex);
//         }

//         int item = buffer[inicio];
//         inicio = (inicio + 1) % MAXFILA;
//         count--;

//         printf("Consumidor consumiu: %d (itens no buffer: %d)\n", item, count);

//         pthread_cond_signal(&produzir);
//         pthread_mutex_unlock(&mutex);
//     }

//     pthread_exit(NULL);
// }

// int main()
// {
//     pthread_t t_produtor, t_consumidor;

//     printf("Iniciando produtor e consumidor...\n");

//     pthread_create(&t_produtor, NULL, produtor, NULL);
//     pthread_create(&t_consumidor, NULL, consumidor, NULL);

//     pthread_join(t_produtor, NULL);
//     pthread_join(t_consumidor, NULL);

//     printf("Execução finalizada.\n");

//     pthread_exit(NULL);
// }

// - LINHA DE COMPILAÇÃO: --------------------------------------------------------------------------------

// gcc Ex3.c -o Ex3 -lpthread

// - LINHA DE EXECUÇÃO: ----------------------------------------------------------------------------------

// ./Ex3

// - SAÍDA: ----------------------------------------------------------------------------------------------

// Iniciando produtor e consumidor...
// Produtor produziu: 1 (itens no buffer: 1)
// Consumidor consumiu: 1 (itens no buffer: 0)
// Produtor produziu: 2 (itens no buffer: 1)
// Produtor produziu: 3 (itens no buffer: 2)
// Consumidor consumiu: 2 (itens no buffer: 1)
// Produtor produziu: 4 (itens no buffer: 2)
// Produtor produziu: 5 (itens no buffer: 3)
// Consumidor consumiu: 3 (itens no buffer: 2)
// Produtor produziu: 6 (itens no buffer: 3)
// Produtor produziu: 7 (itens no buffer: 4)
// Consumidor consumiu: 4 (itens no buffer: 3)
// Produtor produziu: 8 (itens no buffer: 4)
// Produtor produziu: 9 (itens no buffer: 5)
// ...
// Produtor produziu: 63 (itens no buffer: 8)
// Consumidor consumiu: 56 (itens no buffer: 7)
// Produtor produziu: 64 (itens no buffer: 8)
// Consumidor consumiu: 57 (itens no buffer: 7)
// Consumidor consumiu: 58 (itens no buffer: 6)
// Consumidor consumiu: 59 (itens no buffer: 5)
// Consumidor consumiu: 60 (itens no buffer: 4)
// Consumidor consumiu: 61 (itens no buffer: 3)
// Consumidor consumiu: 62 (itens no buffer: 2)
// Consumidor consumiu: 63 (itens no buffer: 1)
// Consumidor consumiu: 64 (itens no buffer: 0)


// - RESPOSTA: -------------------------------------------------------------------------------------------

// O programa demonstra o uso correto de threads, mutex e variáveis de condição para resolver o problema do 
// produtor/consumidor utilizando um buffer circular. A exclusão mútua impede que produtor e consumidor 
// acessem o buffer simultaneamente, enquanto as variáveis de condição fazem com que o produtor espere 
// quando o buffer está cheio e o consumidor espere quando está vazio. Dessa forma, o código evita condições 
// de corrida e garante o funcionamento sincronizado entre as threads, respeitando o limite máximo do buffer.

// ///////////////////////////////////////////////////////////////////////////////////////////////////////

// QUESTÃO 4 -Modifique o programa anterior, para que haja 2 (ou mais) threads consumidor e 2 (ou mais) 
// threads produtor. O que muda em relação ao uso do mutex e da variável de condição?

// - CÓDIGO: --------------------------------------------------------------------------------------------

// #include <pthread.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>

// #define MAXFILA 8
// #define NUM_ELEMENTOS 64
// #define NUM_PRODUTORES 2
// #define NUM_CONSUMIDORES 2

// int buffer[MAXFILA];
// int inicio = 0;
// int fim = 0;
// int count = 0;

// pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
// pthread_cond_t produzir = PTHREAD_COND_INITIALIZER;
// pthread_cond_t consumir = PTHREAD_COND_INITIALIZER;

// void *produtor(void *arg)
// {
//     int id = *(int *)arg;
//     for (int i = 1; i <= NUM_ELEMENTOS / NUM_PRODUTORES; i++)
//     {
//         sleep(1);
//         pthread_mutex_lock(&mutex);

//         while (count == MAXFILA)
//         {
//             pthread_cond_wait(&produzir, &mutex);
//         }

//         int item = rand() % 100 + 1;
//         buffer[fim] = item;
//         fim = (fim + 1) % MAXFILA;
//         count++;

//         printf("[Produtor %d] produziu: %d (itens no buffer: %d)\n", id, item, count);

//         pthread_cond_signal(&consumir);
//         pthread_mutex_unlock(&mutex);
//     }

//     pthread_exit(NULL);
// }

// void *consumidor(void *arg)
// {
//     int id = *(int *)arg;
//     for (int i = 1; i <= NUM_ELEMENTOS / NUM_CONSUMIDORES; i++)
//     {
//         sleep(2);
//         pthread_mutex_lock(&mutex);

//         while (count == 0)
//         {
//             pthread_cond_wait(&consumir, &mutex);
//         }

//         int item = buffer[inicio];
//         inicio = (inicio + 1) % MAXFILA;
//         count--;

//         printf("   [Consumidor %d] consumiu: %d (itens no buffer: %d)\n", id, item, count);

//         pthread_cond_signal(&produzir);
//         pthread_mutex_unlock(&mutex);
//     }

//     pthread_exit(NULL);
// }

// int main()
// {
//     srand(time(NULL));
//     pthread_t produtores[NUM_PRODUTORES];
//     pthread_t consumidores[NUM_CONSUMIDORES];
//     int ids_prod[NUM_PRODUTORES], ids_cons[NUM_CONSUMIDORES];

//     printf("Iniciando produtores e consumidores...\n");

//     for (int i = 0; i < NUM_PRODUTORES; i++)
//     {
//         ids_prod[i] = i + 1;
//         pthread_create(&produtores[i], NULL, produtor, &ids_prod[i]);
//     }

//     for (int i = 0; i < NUM_CONSUMIDORES; i++)
//     {
//         ids_cons[i] = i + 1;
//         pthread_create(&consumidores[i], NULL, consumidor, &ids_cons[i]);
//     }

//     for (int i = 0; i < NUM_PRODUTORES; i++)
//         pthread_join(produtores[i], NULL);

//     for (int i = 0; i < NUM_CONSUMIDORES; i++)
//         pthread_join(consumidores[i], NULL);

//     printf("Execução finalizada.\n");

//     pthread_exit(NULL);
// }

// - LINHA DE COMPILAÇÃO: --------------------------------------------------------------------------------

// gcc Ex4.c -o Ex4 -lpthread

// - LINHA DE EXECUÇÃO: ----------------------------------------------------------------------------------

// ./Ex4

// - SAÍDA: ----------------------------------------------------------------------------------------------

// Iniciando produtores e consumidores...
// [Produtor 1] produziu: 37 (itens no buffer: 1)
// [Produtor 2] produziu: 18 (itens no buffer: 2)
// [Produtor 1] produziu: 2 (itens no buffer: 3)
// [Produtor 2] produziu: 20 (itens no buffer: 4)
//    [Consumidor 1] consumiu: 37 (itens no buffer: 3)
//    [Consumidor 2] consumiu: 18 (itens no buffer: 2)
// [Produtor 1] produziu: 62 (itens no buffer: 3)
// [Produtor 2] produziu: 31 (itens no buffer: 4)
// [Produtor 1] produziu: 4 (itens no buffer: 5)
// ...
// [Produtor 1] produziu: 91 (itens no buffer: 8)
//    [Consumidor 1] consumiu: 72 (itens no buffer: 7)
//    [Consumidor 2] consumiu: 45 (itens no buffer: 6)
//    [Consumidor 1] consumiu: 47 (itens no buffer: 5)
//    [Consumidor 2] consumiu: 59 (itens no buffer: 4)
//    [Consumidor 2] consumiu: 9 (itens no buffer: 3)
//    [Consumidor 1] consumiu: 15 (itens no buffer: 2)
//    [Consumidor 2] consumiu: 35 (itens no buffer: 1)
//    [Consumidor 1] consumiu: 91 (itens no buffer: 0)
// Execução finalizada.


// - RESPOSTA: -------------------------------------------------------------------------------------------

// O programa mantém o mesmo controle de sincronização com mutex e variáveis de condição do exercício anterior, 
// mas agora permite múltiplos produtores e consumidores atuando simultaneamente. O mutex continua garantindo 
// que apenas uma thread acesse o buffer por vez, enquanto as variáveis de condição coordenam o momento 
// certo para inserir ou remover elementos. A principal diferença é que agora várias threads podem ficar 
// bloqueadas aguardando nas variáveis de condição quando o buffer estiver cheio ou vazio. Mesmo com várias 
// threads, o funcionamento permanece consistente, demonstrando a importância da sincronização correta em 
// sistemas concorrentes.



