#include <iostream>
#include <chrono>
#include <fstream>
#include <thread>
using namespace std;

int int_max = 2147483647;

int **A = nullptr;      //matricea de adiacenta initiala
int n;                  //numarul de virfuri

double density;         //densitatea grafului nr.de v/nr.posibil de v

void show_matrix(int **tmp); //afisam matricea
void mem_alloc_matrix(); //alocam memorie pentru matrici
void random_graph(int v, double a, double b, int max_weight); //generare matrice de adiacenta
void mem_free();     //eliberam toata memoria alocata dinamic

void djikstra(int start);
int find_min_node(int *cost, int *visited, int n);
void print_cost(int *cost, int start);
void print_path(int *path, int start);
void find_path(int *parents, int current_node, int start);

void djikstra_thread(int num_thread);
void djikstra_range(int a, int b);

int main()
{

	/*random_graph(10, 0.3, 0.4, 10);
	show_matrix(A);

	djikstra(0);
	show_matrix(A);

	mem_free(); */

	ofstream File("minim.txt");
	int k = 0;

	for (int i = 20; i < 3000; i *= 1.2)
	{
		random_graph(i, 0.1, 0.4, 10);

		auto begin = chrono::high_resolution_clock::now();

		djikstra_thread(8);

		auto end = chrono::high_resolution_clock::now();
		auto dur = end - begin;
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
		cout << "Nr." << k << "   Nr.virfuri = " << i << "   Djikstra: " << ms << endl;
		File << k << "\t\t\t" << i << "\t\t\t" << ms << endl;

		k++;
		mem_free();
	}
	File.close();

	return 0;
}

void djikstra_range(int a, int b)
{
	for (int i = a; i < b; i++)
	{
		djikstra(i);
	}
}

void djikstra_thread(int num_thread)
{
	int vertex_th = n / num_thread;
	int last_vertex = -1;
	
	thread th[8];

	for (int i = 0; i < num_thread; i++)
	{
	    if (i == num_thread - 1)
        {
			cout << "    Thread nr. " << i << "......... range: " << last_vertex + 1 << " - " << n - 1 << endl;
            th[i] = thread(djikstra_range, last_vertex + 1, n - 1);
        }
        else
        {
			cout << "    Thread nr. " << i << " ......... range: " << last_vertex + 1 << " - " << last_vertex + vertex_th << endl;
            th[i] = thread(djikstra_range, last_vertex + 1, last_vertex + vertex_th);
            last_vertex += vertex_th;
        }

	}

	for (int i = 0; i < num_thread; i++)
	{
		cout << "    th" << i;
		th[i].join();
		cout << " - ok   ";
	}
	cout << "\n\n\n";
}

void show_matrix(int **tmp)
{
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			if (tmp[i][j] == int_max)
				cout << 0 << " ";
			else
				cout << tmp[i][j] << " ";
		}
		cout << endl;
	}
	cout << "\n Densitatea = " << density << endl;
}

void mem_alloc_matrix()
{
	A = new int *[n];
	for (int i = 0; i < n; i++)
	{
		A[i] = new int[n];
		for (int j = 0; j < n; j++)
		{
			A[i][j] = int_max;
		}
	}
}

void random_graph(int v, double a, double b, int max_weight)
{
	n = v;                      //setam numarul de virfuri
	double tmp_density;                  //densitatea local
	int edge = 0;               //numarul de muchii adaugate
	int max_edge = (n - 1) * (n - 1);
	mem_alloc_matrix();                 //alocam memorie si initializam cu 0

	int mode = 0;         //modul 0 initializeaza prima data graful cu n - 1 muchii

	int step = n;
	int step_in_for = 0;

	do
	{
		if (mode == 0) //generam minimul necesar de muchii
		{
			for (int i = 0; i < n; i++)
			{
				for (int j = 0; j < n; j++)
				{
					if (i == j && i > 0)
					{
						A[i][j - 1] = rand() % max_weight;
						while (A[i][j - 1] == 0) //in caz ca s-a generat 0
						{
							A[i][j - 1] = rand() % max_weight;
						}
						A[i - 1][j] = A[i][j - 1];
						edge++;
					}
				}
			}
			mode = 1;
			if (n == 2) break;   //cazul in care graful nu poate fi considerat rar mediu sau dens
			if (n == 3) break;
		}
		else
		{    //vom adauga treptat muchii pina cind nu satisfacem conditia de densitate
			for (int i = 0; i < n; i++)
			{
				for (int j = 0; j < n; j++)
				{
					if (step_in_for >= step && i != j)
					{
						A[i][j] = rand() % max_weight;
						while (A[i][j] == 0)  //in caz ca s-a generat 0
						{
							A[i][j] = rand() % max_weight;
						}
						A[j][i] = A[i][j];
						edge++;
						step_in_for = 0;
					}
					step_in_for++;
				}
			}
			step--;
		}
		tmp_density = (double)edge / max_edge;
	} while (tmp_density < a || tmp_density > b);

	density = tmp_density;
}

void mem_free()
{
	if (A != nullptr)
	{
		for (int i = 0; i < n; i++)
		{
			delete[] A[i];
		}
		delete[] A;
		A = nullptr;
	}
}

void djikstra(int start)
{
	int *cost = new int[n];
	int *path = new int[n];
	int *visited = new int[n] {0};
	int i, j, current_node;

	visited[start] = 1;             //marcam nodul de start ca vizitat

	for (j = 0; j < n; j++)
	{
		cost[j] = A[start][j];
		if (A[start][j] != int_max)
			path[j] = start;
	}

	for (i = 1; i < n - 1; i++) //n - 1 ultima iteratie nu are sens toate sunt vizitate
	{
		current_node = find_min_node(cost, visited, n);
		visited[current_node] = 1;

		for (j = 0; j < n; j++)
		{
			if (visited[j] == 0 &&      //daca nu a fost vizitat
				A[current_node][j] != int_max &&    //daca exista asa muchie
				cost[current_node] + A[current_node][j] < cost[j]) //daca ponderea muchiei este mai mica decit costul deja existent
			{
				cost[j] = cost[current_node] + A[current_node][j];
				path[j] = current_node;
			}
		}
	}

	//print_cost(cost, start);
	//print_path(path, start);

	delete[] visited;
	delete[] cost;
	delete[] path;
}

int find_min_node(int *cost, int *visited, int n)
{
	int min_index, min_cost;

	min_cost = int_max;

	for (int i = 0; i < n; i++)
	{
		if (cost[i] < min_cost && visited[i] == false)
		{
			min_cost = cost[i];
			min_index = i;
		}
	}

	return min_index;
}

void print_cost(int *cost, int start)
{
	cout << "\nCost:\n";

	for (int i = 0; i < n; i++)
		if (i != start)
			cout << "(" << start << ", " << i << ") = " << cost[i] << endl;
	cout << endl;
}

void print_path(int *path, int start)
{
	for (int i = 0; i < n; i++)
	{
		if (i != start)
		{
			cout << start << " -> " << i << ": ";
			find_path(path, i, start);
			cout << endl;
		}
	}
}

void find_path(int *parents, int current_node, int start)
{
	if (current_node == start)
	{
		cout << current_node << " ";
		return;
	}
	find_path(parents, parents[current_node], start);
	cout << current_node << " ";
}


