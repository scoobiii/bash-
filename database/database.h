/**
 * database.h - Definições para o componente de Banco de Dados da Rinha de Backend
 * 
 * Versão: 1.0.1
 * Data: 28/04/2025
 * 
 * Responsabilidades:
 * - Definir estruturas de dados para o banco em memória
 * - Declarar funções para operações CRUD
 * - Configurar constantes e parâmetros de persistência
 * 
 * Equipe:
 * - Zeh Sobrinho (Product Owner)
 * - Manus (Full Stack AGI DevOp)
 */

#ifndef DATABASE_H
#define DATABASE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>

// Constantes
#define BUFFER_SIZE 8192
#define MAX_PESSOAS 100000
#define SNAPSHOT_INTERVAL 60  // segundos
#define MAX_SEARCH_RESULTS 50 // Definido aqui
#define HASH_TABLE_SIZE 150001  // Primo próximo a 1.5x MAX_PESSOAS

// Estrutura para armazenar uma pessoa
typedef struct {
    char id[37];  // UUID
    char nome[101];
    char apelido[33];
    char nascimento[11];  // YYYY-MM-DD
    char stack[1024];  // Lista de tecnologias
    int usado;  // Flag para indicar se o slot está em uso
    int next;   // Próximo índice na lista encadeada (para hash table)
} Pessoa;

// Estrutura para o banco de dados
typedef struct {
    Pessoa *pessoas;
    int contador_pessoas;
    int *hash_table;  // Hash table para busca rápida por ID
    int *apelido_hash;  // Hash table para busca rápida por apelido
    pthread_mutex_t mutex;
    char data_dir[256];
} Database;

// Funções para operações CRUD
int create_pessoa(Database *db, const Pessoa *pessoa);
int get_pessoa_by_id(Database *db, const char *id, Pessoa *pessoa);
int search_pessoas(Database *db, const char *termo, Pessoa *resultado, int max_results);
int count_pessoas(Database *db);

// Funções para persistência
void save_snapshot(Database *db);
void load_snapshot(Database *db);
void *snapshot_thread(void *arg);

// Funções para processamento de comandos
void process_command(int client_socket, Database *db);
void parse_command(const char *buffer, char *operation, char *data);

// Funções de hash
unsigned int hash_string(const char *str);

// Funções de utilidade
void cleanup(int sig);
void log_message(const char *format, ...);

#endif // DATABASE_H
