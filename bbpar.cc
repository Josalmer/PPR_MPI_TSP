/*
 ============================================================================
 Name        :	bbpar.cpp
 Author      :	Jose Saldaña Mercado
 Copyright   :	GNU Open Souce and Free license
 Description : 	Resuelve el problema del viajante de comercio TSP (traveling 
 				salesman problem) mediante un algoritmo Branch-And-Bound 
				Paralelo distribuido con MPI 
 ============================================================================
 */

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <mpi.h>
#include "libbb.h"

using namespace std;

unsigned int NCIUDADES;
bool DIFUSION;
int idproc, size, siguiente, anterior;
bool token_presente;
MPI_Comm comunicadorCota, comunicadorCarga;

main(int argc, char **argv) {
	switch (argc) {
	case 4:
		NCIUDADES = atoi(argv[1]);
		DIFUSION = atoi(argv[3]) == 1;
		break;
	default:
		cerr << "La sintaxis es: bbseq <tamaño> <archivo> <difusion_cota_superior>" << endl;
		exit(1);
		break;
	}

	// MPI init
	MPI::Init(argc, argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &idproc);
	siguiente = (idproc + 1) % size;
	anterior = (idproc - 1 + size) % size;

	// Inicialización de comunicadores
    MPI_Comm_split(MPI_COMM_WORLD, // a partir del comunicador global.
        0, // Todos van al mismo comunicador
        idproc, // indica el orden de asignacion de rango dentro de los nuevos comunicadores
        &comunicadorCarga); // Referencia al nuevo comunicador creado.
	MPI_Comm_split(MPI_COMM_WORLD, 1, idproc, &comunicadorCota);

	int **tsp0 = reservarMatrizCuadrada(NCIUDADES);
	tNodo nodo,	  // nodo a explorar
		lnodo,	  // hijo izquierdo
		rnodo,	  // hijo derecho
		solucion; // mejor solucion
	bool end,  // condicion de fin
		nueva_U;  // hay nuevo valor de c.s.
	int U;		  // valor de c.s.
	int iteraciones = 0;
	tPila pila; // pila de nodos a explorar

	U = INFINITO;	 // inicializa cota superior
	InicNodo(&nodo); // inicializa estructura nodo

	if (idproc == 0) { // Solo proceso 0
		LeerMatriz(argv[2], tsp0); // lee matriz de fichero
		token_presente = true;
	}
	MPI_Bcast(&tsp0[0][0], NCIUDADES * NCIUDADES, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);

	double t = MPI::Wtime();

	if (idproc != 0) {
		loadBalance(pila, end, solucion);
		if (!end) {
			pila.pop(nodo);
		}
	}
	end = Inconsistente(tsp0);

	while (!end) { // ciclo del Branch&Bound
		Ramifica(&nodo, &lnodo, &rnodo, tsp0);
		nueva_U = false;
		if (Solucion(&rnodo)) {
			if (rnodo.ci() < U) { // se ha encontrado una solucion mejor
				U = rnodo.ci();
				nueva_U = true;
				CopiaNodo(&rnodo, &solucion);
			}
		} else { //  no es un nodo solucion
			if (rnodo.ci() < U) { //  cota inferior menor que cota superior
				if (!pila.push(rnodo)) {
					printf("Error: pila agotada\n");
					liberarMatriz(tsp0);
					exit(1);
				}
			}
		}
		if (Solucion(&lnodo)) {
			if (lnodo.ci() < U) { // se ha encontrado una solucion mejor
				U = lnodo.ci();
				nueva_U = true;
				CopiaNodo(&lnodo, &solucion);
			}
		} else { // no es nodo solucion
			if (lnodo.ci() < U) { // cota inferior menor que cota superior
				if (!pila.push(lnodo)) {
					printf("Error: pila agotada\n");
					liberarMatriz(tsp0);
					exit(1);
				}
			}
		}

		// Difusion cota superior
		if (DIFUSION) {
			uBroadcast(U, nueva_U);
		}

		if (nueva_U) {
			pila.acotar(U);
		}

		loadBalance(pila, end, solucion);

		if (!end) {
			pila.pop(nodo);
		}

		iteraciones++;
	}
	MPI_Barrier(MPI_COMM_WORLD);
	t = MPI::Wtime() - t;
	MPI_Comm_free(&comunicadorCota);
	MPI_Comm_free(&comunicadorCarga);

	cout << "----- Proceso " << idproc << ", " << iteraciones << " iteraciones realizadas -----" << endl;
	int itTotal;
	MPI_Reduce(&iteraciones, &itTotal, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);

	if (idproc == 0) {
		printf("Solucion: \n");
		EscribeNodo(&solucion);
		cout << "Tiempo gastado= " << t << endl;
		cout << "Numero de iteraciones = " << itTotal << endl << endl;
	}


	liberarMatriz(tsp0);
	MPI::Finalize();
	return 0;
}
