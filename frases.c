/*-----------------------------------------------------------------------------
PONTIFÍCIA UNIVERSIDADE CATÓLICA DO RIO GRANDE DO SUL
PROGRAMAÇÃO PARALELA E DISTRIBUIDA - ENGENHARIA DE COMPUTAÇÃO
Gabriel Fasoli Susin - gabriel.susin@acad.pucrs.br
Augusto Bergamin - augusto.bergamin@acad.pucrs.br
------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mpi.h"
// aonde por os tempos, qual  combinação para teste

#define SEND 1
//#define PORCENTAGEM 50
//#define TAMANHO 10000

int *interleaving(int vetor[], int tam)
{
	int *vetor_auxiliar;
	int i1, i2, i_aux;

	vetor_auxiliar = (int *)malloc(sizeof(int) * tam);

	i1 = 0;
	i2 = tam / 2;

	for (i_aux = 0; i_aux < tam; i_aux++) {
		if (((vetor[i1] <= vetor[i2]) && (i1 < (tam / 2)))
		    || (i2 == tam))
			vetor_auxiliar[i_aux] = vetor[i1++];
		else
			vetor_auxiliar[i_aux] = vetor[i2++];
	}

	return vetor_auxiliar;
}

void bs(int n, int * vetor)
{
    int c=0, d, troca, trocou =1;

    while (c < (n-1) & trocou )
        {
        trocou = 0;
        for (d = 0 ; d < n - c - 1; d++)
            if (vetor[d] > vetor[d+1])
                {
                troca      = vetor[d];
                vetor[d]   = vetor[d+1];
                vetor[d+1] = troca;
                trocou = 1;
                }
        c++;
        }
}

void printfv(int *vetor, int tam_vetor){
    int i;
    for(i = 0; i<tam_vetor;i++){
        printf("%d: %d\n",i,vetor[i]);
    }

}

// Inicializa de forma com que o menor rank fique com os maiores valores
void Inicializa( int *vetor,int tam_vetor,int my_rank, int tamanho){
    int i;
    int proc_n;
    proc_n = tamanho/tam_vetor;
    for(i=0;i<tam_vetor;i++){
        vetor[i]=((proc_n-my_rank-1)*tam_vetor)+(tam_vetor-i);
    }
}

int main(int argc, char** argv){
    int my_rank, proc_n, dado_esquerda, tam_vetor;
    int quantidade, tamanho, porcentagem;
    int *vetor;
    int *vetor_aux;
    int bcast;
    double t1,t2;
    MPI_Status status;
    MPI_Init(&argc , &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &proc_n);
    int bcast_vector[proc_n];

    int i;
	for(i=0; i<argc; i++)
	{
		if(!strcmp(argv[i],"-percent"))
		{
            porcentagem = atoi(argv[i+1]);
            //printf("Percent: %d\n",porcentagem);
			i++;
		}
		if(!strcmp(argv[i],"-size"))
		{
            tamanho = atoi(argv[i+1]);
            //printf("tamanho: %d\n",tamanho);
			i++;
		}
        if(!strcmp(argv[i],"-h")){
            printf("Usage: mpirun -np <NUMBER OF NODES> frases -size <VECTOR SIZE> -percent <VECTOR EXCHANGE PERCENT>\n");
            exit(1);}
	}


    tam_vetor = (tamanho/proc_n);
    // acerta a quantidade do vetor que sera trocado a cada iteração
    quantidade = (int)((porcentagem*tam_vetor)/100);
    printf("RANK:%d  quantidade: %d  tam_vetor:%d \n",my_rank, quantidade, tam_vetor);
    if(my_rank<proc_n-1){
        vetor_aux = malloc (quantidade*2*sizeof(int));}
    vetor = malloc (tam_vetor*sizeof(int));
    //inicializa cada vetor decrescentemente, sendo que o maior rank pega os menores numeros e assim por diante
    Inicializa (vetor,tam_vetor,my_rank,tamanho); 

    int cont =0;
    t1 = MPI_Wtime();
    while (1)
    {
        //ordena o vetor
        bs(tam_vetor,vetor);
        
        // manda o maior numero para o nodo a direita
        if(my_rank<proc_n-1){
            MPI_Send (&vetor[tam_vetor-1], sizeof(int), MPI_INT, my_rank+1, SEND, MPI_COMM_WORLD );}
        // recebe o maior nodo do numero a esquerda 
        if(my_rank>0){
            MPI_Recv (&dado_esquerda, sizeof(int), MPI_INT,my_rank-1,MPI_ANY_TAG,MPI_COMM_WORLD, &status);}
        
        // realiza broadcast do estado
        int breakk;
        for(i=0;i<proc_n;i++){  
            // se for sua vez de transmitir, decide se deve continuar comparando o numero recebido com o seu menor    
            if(i == my_rank){
                if(vetor[0]>dado_esquerda) bcast = 0;     
                else bcast = 1;}  
            //se for sua ver transmite, caso contrario recebe o estado dos outros             
            MPI_Bcast(&bcast,sizeof(int),MPI_INT,i,MPI_COMM_WORLD);
            bcast_vector[i] = bcast;
        }
        // verifica se alguem precisa continuar
        for(i=0;i<proc_n;i++){  
            if (bcast_vector[i] == 1) {breakk = 1;break;}
            else breakk = 0;
        }
        //se ninguem precisa, finaliza o loop
        if(breakk == 0)break;
        //  todos os nodos, menos o mais a esquerda, mandam sua parte do vetor para a esquerda
        if(my_rank>0){
            MPI_Send (&vetor[0], quantidade, MPI_INT, my_rank-1, SEND, MPI_COMM_WORLD );
        }
        //todos os nodos, menos o da direita, recebem a parte do veror da direita e colocam em um vetor auxiliar
        if(my_rank<proc_n-1){
            MPI_Recv (&vetor_aux[quantidade], quantidade, MPI_INT,my_rank+1,MPI_ANY_TAG,MPI_COMM_WORLD, &status);
            // junto do que receberam colocam a maior parte do seu vetor
            memcpy(&vetor_aux[0],&vetor[tam_vetor-quantidade],quantidade*sizeof(int));
            interleaving(vetor_aux,quantidade*2);
            // ordenam o vetor auxiliar
            bs(2*quantidade,vetor_aux);
            // colocam a menor parte no seu vetor
            memcpy(&vetor[tam_vetor-quantidade],&vetor_aux[0],quantidade*sizeof(int));
            // e mandam o resto para a direita
            MPI_Send (&vetor_aux[quantidade], quantidade, MPI_INT, my_rank+1, SEND, MPI_COMM_WORLD );
        }
        // todos os nodos menos o zero recebem da esquerda e guardam no começo do seu vetor
        if(my_rank>0)
            MPI_Recv (&vetor[0], quantidade, MPI_INT,my_rank-1,MPI_ANY_TAG,MPI_COMM_WORLD, &status);   
        
        cont++;
        printf("Contador:%d\n",cont); 
    }
t2 = MPI_Wtime();
if(my_rank == 0){
    printfv(vetor,tam_vetor);
    
}
//free(vetor);
//free(vetor_aux);

MPI_Finalize();
printf("Run time: %lf\n", t2-t1);
return 0;
}
